from __future__ import annotations
from dataclasses import dataclass, field, asdict
from pathlib import Path
from collections import Counter
import argparse, csv, json, math, random, statistics

ACTION_ORDER = ["HOLD","RECOVER","DEFEND","CAPTURE_RESOURCE","ATTACK","SEIZE","SIEGE"]
RANKS = [
    ("Legendary",1500,100,2),("Elite",700,75,1),("Veteran",300,50,1),
    ("Seasoned",100,25,0),("Recruit",0,0,0),
]
UNIT_LINES = {
    "fighter_a": (4,12,30,60),"fighter_b": (3,9,45,90),
    "fighter_c": (3,8,65,125),"fighter_d": (2,6,85,160),
    "ranged": (3,8,60,110),"support": (2,6,65,100),"beast": (1,3,150,280),
}

@dataclass
class Params:
    ap:int=3; move_cost:int=8; start_gold:int=240; base_income:int=18
    resource_income:int=9; start_strength:int=1050; target_strength:int=1900
    respawn_strength:int=220; respawn_days:int=3; hero_power_per_level:int=55
    respawn_supply:int=700; respawn_readiness:int=550; respawn_fatigue:int=150
    respawn_preserve_veterancy:bool=False; respawn_recruit_same_day:bool=False
    repair_mode:str="current_free"; repair_cost_fraction:float=.45; repair_days:int=4
    siege_floor:int=200; siege_loss:int=70; siege_threshold:int=450
    battle_variance:int=100; comeback_floor:float=0.0; construction_slots:int=999

@dataclass
class Memory:
    kind:str; opponent:str=""; place:str=""; turn:int=1; intensity:int=800

@dataclass
class Logistics:
    supply:int=1000; readiness:int=1000; fatigue:int=0

@dataclass
class Building:
    building_id:str; integrity:int=1000; condition:str="Intact"
    repair_days_remaining:int=0

    def operational(self, threshold:int=500)->bool:
        return self.condition in ("Intact","Damaged") and self.integrity >= threshold

@dataclass
class Pool:
    unit_id:str; available:int; weekly_growth:int; capacity:int
    cost:int; strength:int; building_id:str

@dataclass
class Hero:
    hero_id:str; xp:int=0; memories:list[Memory]=field(default_factory=list)

    @property
    def level(self)->int:
        level=1
        while self.xp >= xp_for_level(level+1):
            level += 1
        return level

@dataclass
class Army:
    army_id:str; faction_id:str; commander_id:str; region_id:str; strength:int
    logistics:Logistics=field(default_factory=Logistics)
    regiment_xp:int=0; respawn_day:int=0

@dataclass
class Faction:
    faction_id:str; capital:str; gold:int; army:Army; hero:Hero
    buildings:dict[str,Building]; pools:dict[str,Pool]
    owned:set[str]=field(default_factory=set)
    actions:Counter=field(default_factory=Counter)
    battles_won:int=0; battles_lost:int=0
    snapshot14_strength:int=0; snapshot14_gold:int=0
    shock_day:int=0; recovery70_day:int=0

@dataclass
class Region:
    region_id:str; owner:str=""; neighbors:list[str]=field(default_factory=list)
    resource:bool=False; capital:bool=False; roads:set[str]=field(default_factory=set)

@dataclass
class Campaign:
    day:int; regions:dict[str,Region]; factions:dict[str,Faction]
    action_log:list[dict]=field(default_factory=list)
    battle_log:list[dict]=field(default_factory=list)

def xp_for_level(level:int)->int:
    level=max(1,level)
    return (level-1)*level*125

def rank_stats(xp:int)->tuple[str,int,int]:
    for name,threshold,combat,morale in RANKS:
        if xp >= threshold:
            return name,combat,morale
    return "Recruit",0,0

def _decay(intensity:int, age:int, horizon:int)->int:
    if age < 0 or age >= horizon:
        return 0
    return max(0,min(1000,intensity))*(horizon-age)//horizon

def rival_bias(hero:Hero, opponent:str, turn:int)->int:
    bias=0
    for m in hero.memories:
        if m.opponent != opponent: continue
        strength=_decay(m.intensity,turn-m.turn,7)
        if m.kind in ("BattleDefeat","SiegeDefeat"):
            bias -= strength*800//1000
        elif m.kind in ("BattleVictory","SiegeVictory"):
            bias += strength*250//1000
    return max(-800,min(300,bias))

def reclaim_bonus(hero:Hero, place:str, turn:int)->int:
    bonus=0
    for m in hero.memories:
        if m.kind=="PlaceLoss" and m.place==place:
            bonus += _decay(m.intensity,turn-m.turn,12)*500//1000
    return max(0,min(500,bonus))

def apply_travel(s:Logistics,cost:int,road:bool,hostile:bool)->None:
    cost=max(0,cost)
    supply_loss=cost*12
    readiness_loss=cost*8
    fatigue_gain=cost*10
    if road:
        supply_loss=supply_loss*70//100
        readiness_loss=readiness_loss*75//100
        fatigue_gain=fatigue_gain*75//100
    if hostile:
        supply_loss=supply_loss*130//100
    s.supply=max(0,min(1000,s.supply-supply_loss))
    s.readiness=max(0,min(1000,s.readiness-readiness_loss))
    s.fatigue=max(0,min(1000,s.fatigue+fatigue_gain))

def force_march(s:Logistics)->bool:
    if s.readiness < 350 or s.supply < 250:
        return False
    s.readiness=max(0,s.readiness-200)
    s.supply=max(0,s.supply-120)
    s.fatigue=min(1000,s.fatigue+250)
    return True

def end_day_logistics(s:Logistics,settlement:bool,friendly:bool)->None:
    if settlement:
        s.supply=min(1000,s.supply+350); s.readiness=min(1000,s.readiness+300)
        s.fatigue=max(0,s.fatigue-400)
    elif friendly:
        s.supply=min(1000,s.supply+120); s.readiness=min(1000,s.readiness+160)
        s.fatigue=max(0,s.fatigue-220)

    else:
        s.supply=max(0,s.supply-50); s.readiness=min(1000,s.readiness+60)
        s.fatigue=max(0,s.fatigue-100)

def make_faction(fid:str,cap:str,p:Params)->Faction:
    buildings={f"dwelling_{u}":Building(f"dwelling_{u}") for u in UNIT_LINES}
    pools={}
    for unit,(growth,capacity,cost,strength) in UNIT_LINES.items():
        pools[unit]=Pool(unit,min(growth,capacity),growth,capacity,cost,strength,f"dwelling_{unit}")
    hero=Hero(f"hero_{fid}")
    army=Army(f"army_{fid}",fid,hero.hero_id,cap,p.start_strength)
    return Faction(fid,cap,p.start_gold,army,hero,buildings,pools,{cap})

def connect(regions:dict[str,Region],a:str,b:str,road:bool=False)->None:
    regions[a].neighbors.append(b); regions[b].neighbors.append(a)
    if road:
        regions[a].roads.add(b); regions[b].roads.add(a)

def make_campaign(scale:int=2,p:Params|None=None)->Campaign:
    p=p or Params(); regions={}
    for i in range(4):
        rid=f"cap_{i}"
        regions[rid]=Region(rid,owner=f"f{i}",capital=True)
    for i in range(4):
        prev=f"cap_{i}"
        for j in range(1,scale+1):
            rid=f"route_{i}_{j}"
            regions[rid]=Region(rid,resource=(j==(scale+1)//2))
            connect(regions,prev,rid,road=(j%2==1)); prev=rid
        connect(regions,prev,f"cap_{(i+1)%4}",road=(scale%2==0))

    factions={f"f{i}":make_faction(f"f{i}",f"cap_{i}",p) for i in range(4)}
    return Campaign(1,regions,factions)

def owner_army(c:Campaign,region_id:str,exclude:str="")->Army|None:
    for f in c.factions.values():
        if f.faction_id != exclude and f.army.strength>0 and f.army.region_id==region_id:
            return f.army
    return None

def region_value(r:Region)->int:
    if r.capital: return 850
    return 240 + (280 if r.resource else 0)

def daily_income(c:Campaign,f:Faction,p:Params)->int:
    resource_count=sum(1 for rid in f.owned if c.regions[rid].resource)
    raw=p.base_income+resource_count*p.resource_income
    richest=max((sum(1 for rid in x.owned if c.regions[rid].resource) for x in c.factions.values()),default=0)
    if richest-resource_count >= 2:
        raw=max(raw,round((p.base_income+richest*p.resource_income)*p.comeback_floor))
    return raw

def threatened(c:Campaign,fid:str,rid:str)->bool:
    for n in c.regions[rid].neighbors:
        a=owner_army(c,n,exclude=fid)
        if a is not None:
            return True
    return False

def exposed(c:Campaign,r:Region,fid:str)->bool:
    if r.owner==fid or r.capital: return False
    return owner_army(c,r.region_id,exclude=fid) is None

def candidate_rows(c:Campaign,f:Faction,p:Params)->list[tuple[int,str,str]]:
    a=f.army; rows=[(200,"HOLD","")]
    if a.logistics.readiness < 700:
        rows.append((1700-a.logistics.readiness,"RECOVER",""))
    here=c.regions[a.region_id]
    for rid in sorted(here.neighbors):
        r=c.regions[rid]; strategic_travel=40
        if r.owner==f.faction_id and threatened(c,f.faction_id,rid):
            score=700+region_value(r)+650-strategic_travel
            rows.append((score,"DEFEND",rid))
        if r.owner!=f.faction_id and r.resource:
            score=250+region_value(r)+280+650-strategic_travel
            score-=max(0,700-a.logistics.readiness)
            score+=reclaim_bonus(f.hero,rid,c.day)
            rows.append((score,"CAPTURE_RESOURCE",rid))
        enemy=owner_army(c,rid,exclude=f.faction_id)
        if enemy is not None and not r.capital and a.strength*100 >= enemy.strength*65:
            edge=max(-300,min(300,(a.strength-enemy.strength)//10))
            score=2000+edge-max(0,700-a.logistics.readiness)
            score+=rival_bias(f.hero,enemy.commander_id,c.day)
            rows.append((score,"ATTACK",rid))
        if exposed(c,r,f.faction_id):
            rows.append((900+region_value(r)+650-strategic_travel+100,"SEIZE",rid))
        if r.capital and r.owner and r.owner!=f.faction_id and a.logistics.readiness>=600:
            defender=c.factions[r.owner].army
            garrison=max(250,defender.strength if defender.region_id==rid else 500)
            if a.strength*100 >= garrison*70:
                edge=max(-400,min(400,(a.strength-garrison)//10))
                rows.append((600+region_value(r)+650+edge-strategic_travel,"SIEGE",rid))
    return rows

def choose_action(c:Campaign,f:Faction,p:Params)->tuple[int,str,str]:
    rows=candidate_rows(c,f,p)
    order={a:i for i,a in enumerate(ACTION_ORDER)}
    rows.sort(key=lambda x:(-x[0],order[x[1]],x[2]))
    return rows[0]

def lab_effective_strength(f:Faction,p:Params,rng:random.Random)->float:
    a=f.army
    _,vet,_=rank_stats(a.regiment_xp)
    readiness=(500+a.logistics.readiness/2)/1000
    supply=(700+a.logistics.supply*0.3)/1000
    fatigue=(1000-a.logistics.fatigue*0.25)/1000
    hero=1+(f.hero.level-1)*p.hero_power_per_level/1000
    variance=1+rng.randint(-p.battle_variance,p.battle_variance)/1000
    return max(1,a.strength*(1+vet/1000)*readiness*supply*fatigue*hero*variance)

def record_battle_memory(winner:Faction,loser:Faction,day:int)->None:
    winner.hero.memories.append(Memory("BattleVictory",loser.hero.hero_id,turn=day))
    loser.hero.memories.append(Memory("BattleDefeat",winner.hero.hero_id,turn=day))

def resolve_battle(c:Campaign,att:Faction,defn:Faction,p:Params,rng:random.Random,region:str)->str:
    av=lab_effective_strength(att,p,rng); dv=lab_effective_strength(defn,p,rng)
    winner,loser=(att,defn) if av>=dv else (defn,att)
    ratio=max(av,dv)/max(1,min(av,dv))
    win_loss=max(120,min(420,round(330-70*min(3,ratio-1))))
    lose_loss=max(520,min(950,round(720+65*min(3,ratio-1))))
    winner.army.strength=max(1,winner.army.strength*(1000-win_loss)//1000)
    loser.army.strength=max(0,loser.army.strength*(1000-lose_loss)//1000)
    winner.battles_won+=1; loser.battles_lost+=1
    winner.army.regiment_xp+=85; loser.army.regiment_xp+=35
    winner.hero.xp+=100; loser.hero.xp+=35
    record_battle_memory(winner,loser,c.day)
    if loser.army.strength < 90:
        loser.army.strength=0; loser.army.respawn_day=c.day+p.respawn_days
    c.battle_log.append({"day":c.day,"region":region,"winner":winner.faction_id,
        "attacker":att.faction_id,"av":round(av,2),"dv":round(dv,2)})
    return winner.faction_id

def capture_region(c:Campaign,f:Faction,rid:str)->None:
    r=c.regions[rid]; old=r.owner
    if old==f.faction_id: return
    if old in c.factions:
        prior=c.factions[old]
        prior.owned.discard(rid)
        prior.hero.memories.append(Memory("PlaceLoss",f.hero.hero_id,rid,c.day,700))
    r.owner=f.faction_id; f.owned.add(rid)

def damage_dwelling(f:Faction,unit_id:str)->None:
    b=f.buildings[f"dwelling_{unit_id}"]
    b.integrity=0; b.condition="Ruined"; b.repair_days_remaining=0

def start_or_apply_repair(f:Faction,unit_id:str,p:Params)->bool:
    b=f.buildings[f"dwelling_{unit_id}"]
    if b.condition!="Ruined": return False
    if p.repair_mode=="current_free":
        b.integrity=1000; b.condition="Intact"
        return True
    pool=f.pools[unit_id]
    cost=max(1,round(pool.cost*pool.capacity*p.repair_cost_fraction))
    if f.gold < cost: return False
    f.gold-=cost; b.repair_days_remaining=max(1,p.repair_days)
    b.condition="Building"
    return True

def progress_repairs(f:Faction)->None:
    for b in f.buildings.values():
        if b.condition=="Building" and b.repair_days_remaining>0:
            b.repair_days_remaining-=1
            if b.repair_days_remaining==0:
                b.integrity=1000; b.condition="Intact"

def weekly_growth(f:Faction,p:Params)->None:
    for unit,pool in f.pools.items():
        b=f.buildings[pool.building_id]
        growth=0
        if b.operational():
            growth=(pool.weekly_growth*b.integrity+999)//1000
        pool.available=min(pool.capacity,pool.available+max(0,growth))

def recruit_at_capital(f:Faction,p:Params)->None:
    if f.army.region_id!=f.capital or f.army.strength<=0:
        return
    order=sorted(f.pools, key=lambda u:(-(f.pools[u].strength/max(1,f.pools[u].cost)),u))
    changed=True
    while changed and f.army.strength < p.target_strength:
        changed=False
        for unit in order:
            pool=f.pools[unit]; b=f.buildings[pool.building_id]
            if pool.available>0 and b.operational() and f.gold>=pool.cost:
                f.gold-=pool.cost; pool.available-=1
                f.army.strength+=pool.strength; changed=True
                if f.army.strength>=p.target_strength: break

def advance_siege_supply(supply:int,p:Params)->int:
    return max(p.siege_floor,supply-p.siege_loss)

def starvation_days_to_threshold(p:Params)->int|None:
    supply=1000
    for day in range(1,366):
        supply=advance_siege_supply(supply,p)
        if supply<=p.siege_threshold: return day
        if supply==p.siege_floor and p.siege_floor>p.siege_threshold: return None
    return None

def move_army(c:Campaign,f:Faction,target:str,p:Params)->None:
    source=c.regions[f.army.region_id]
    road=target in source.roads
    hostile=c.regions[target].owner not in ("",f.faction_id)
    apply_travel(f.army.logistics,p.move_cost,road,hostile)
    f.army.region_id=target

def execute_action(c:Campaign,f:Faction,p:Params,rng:random.Random)->bool:
    score,action,target=choose_action(c,f,p)
    f.actions[action]+=1
    c.action_log.append({"day":c.day,"faction":f.faction_id,"action":action,
                         "target":target,"score":score,"readiness":f.army.logistics.readiness})
    if action=="HOLD":
        return False
    if action=="RECOVER":
        return False
    if not target:
        return False
    move_army(c,f,target,p)
    r=c.regions[target]
    if action in ("DEFEND","CAPTURE_RESOURCE","SEIZE"):
        if action!="DEFEND": capture_region(c,f,target)
        return True
    if action=="ATTACK":
        enemy_army=owner_army(c,target,exclude=f.faction_id)
        if enemy_army is None: return True
        enemy=c.factions[enemy_army.faction_id]
        won=resolve_battle(c,f,enemy,p,rng,target)==f.faction_id
        if won:
            capture_region(c,f,target)
            enemy.army.region_id=enemy.capital
        else:
            f.army.region_id=f.capital
        return False

    if action=="SIEGE":
        defender=c.factions[r.owner]
        garrison=defender.army.strength if defender.army.region_id==target else 500
        av=lab_effective_strength(f,p,rng)
        dv=garrison*(0.85+rng.randint(-p.battle_variance,p.battle_variance)/1000)
        if av>=dv:
            f.army.strength=max(1,f.army.strength*740//1000)
            f.hero.memories.append(Memory("SiegeVictory",defender.hero.hero_id,target,c.day))
            defender.hero.memories.append(Memory("SiegeDefeat",f.hero.hero_id,target,c.day))
            if defender.army.region_id==target:
                defender.army.strength=max(0,defender.army.strength*220//1000)
            capture_region(c,f,target)
        else:
            f.army.strength=max(0,f.army.strength*420//1000)
            f.army.region_id=f.capital
        return False
    return False

def restore_army_if_due(f:Faction,c:Campaign,p:Params)->bool:
    if f.army.strength==0 and f.army.respawn_day and c.day>=f.army.respawn_day:
        f.army.strength=p.respawn_strength
        f.army.region_id=f.capital
        f.army.logistics=Logistics(p.respawn_supply,p.respawn_readiness,p.respawn_fatigue)
        if not p.respawn_preserve_veterancy:
            f.army.regiment_xp=0
        f.army.respawn_day=0
        return True
    return False

def maintain_faction(c:Campaign,f:Faction,p:Params)->None:
    progress_repairs(f)
    for unit in sorted(f.pools):
        if f.buildings[f.pools[unit].building_id].condition=="Ruined":
            start_or_apply_repair(f,unit,p)
    f.gold+=daily_income(c,f,p)
    if c.day>1 and (c.day-1)%7==0:
        weekly_growth(f,p)
    restored=restore_army_if_due(f,c,p)
    if not restored or p.respawn_recruit_same_day:
        recruit_at_capital(f,p)

def apply_shock(c:Campaign,shock:str,p:Params)->None:
    f=c.factions["f0"]
    if shock=="day14_loss" and c.day==14:
        f.snapshot14_strength=f.army.strength
        f.snapshot14_gold=f.gold
        f.shock_day=c.day
        f.army.strength=0
        f.army.respawn_day=c.day+p.respawn_days
    elif shock=="ranged_destroyed" and c.day==10:
        damage_dwelling(f,"ranged")
    elif shock=="elephant_destroyed" and c.day==10:
        damage_dwelling(f,"beast")
    elif shock=="wealth_two_resources" and c.day==3:
        resources=[r for r in c.regions.values() if r.resource][:2]
        for r in resources:
            capture_region(c,f,r.region_id)

def simulate(seed:int,days:int=56,scale:int=2,p:Params|None=None,shock:str="")->dict:
    p=p or Params()
    rng=random.Random(seed)
    c=make_campaign(scale,p)
    for day in range(1,days+1):
        c.day=day
        apply_shock(c,shock,p)
        for fid in sorted(c.factions):
            maintain_faction(c,c.factions[fid],p)
        order=sorted(c.factions)
        rng.shuffle(order)
        for fid in order:
            f=c.factions[fid]
            if f.army.strength<=0:
                continue
            for _ in range(max(0,p.ap)):
                if not execute_action(c,f,p,rng):
                    break
        for f in c.factions.values():
            if f.army.strength<=0:
                continue
            r=c.regions[f.army.region_id]
            settlement=r.capital and r.owner==f.faction_id
            end_day_logistics(f.army.logistics,settlement,r.owner==f.faction_id)
        f0=c.factions["f0"]
        if f0.snapshot14_strength>0 and f0.recovery70_day==0:
            if f0.army.strength*100 >= f0.snapshot14_strength*70:
                f0.recovery70_day=day
    return summarize(c,p,shock,seed,scale)

def gini(values:list[float])->float:
    vals=sorted(max(0.0,float(v)) for v in values)
    total=sum(vals)
    if not vals or total==0: return 0.0
    n=len(vals)
    weighted=sum((i+1)*v for i,v in enumerate(vals))
    return (2*weighted)/(n*total)-(n+1)/n

def summarize(c:Campaign,p:Params,shock:str,seed:int,scale:int)->dict:
    rows=[]
    for f in c.factions.values():
        territory=len(f.owned)
        resources=sum(1 for rid in f.owned if c.regions[rid].resource)
        income=daily_income(c,f,p)
        score=f.gold+f.army.strength+territory*120+income*10
        rows.append((f.faction_id,score,f.gold,f.army.strength,territory,resources,income))
    rows.sort()
    scores=[x[1] for x in rows]
    total=max(1,sum(scores))
    leader=max(scores) if scores else 0
    second=sorted(scores,reverse=True)[1] if len(scores)>1 else max(1,leader)
    f0=c.factions["f0"]
    f0_score=next(x[1] for x in rows if x[0]=="f0")
    recovery=(f0.army.strength/max(1,f0.snapshot14_strength)
              if f0.snapshot14_strength else None)
    recovery_days=(f0.recovery70_day-f0.shock_day
                   if f0.recovery70_day and f0.shock_day else None)
    return {"seed":seed,"scale":scale,"shock":shock,"days":c.day,
            "leader_share":leader/total,"top_second_ratio":leader/max(1,second),
            "gini":gini(scores),"runaway":leader/total>0.42,
            "battle_count":len(c.battle_log),"f0_recovery_ratio":recovery,
            "f0_recovery70_days":recovery_days,
            "f0_score_share":f0_score/total,"f0_is_leader":f0_score==leader,
            "factions":[{"id":x[0],"score":x[1],"gold":x[2],"army":x[3],
                         "territory":x[4],"resources":x[5],"income":x[6],
                         "hero_level":c.factions[x[0]].hero.level,
                         "rank":rank_stats(c.factions[x[0]].army.regiment_xp)[0]}
                        for x in rows],
            "actions":{fid:dict(c.factions[fid].actions) for fid in sorted(c.factions)}}

def memory_stress()->list[dict]:
    out=[]; turn=20
    for defeats in range(0,6):
        h=Hero("ai")
        for i in range(defeats):
            h.memories.append(Memory("BattleDefeat","rival",turn=turn-i,intensity=800))
        bias=rival_bias(h,"rival",turn)
        for readiness,edge in ((550,0),(550,300),(650,0)):
            attack=2000+edge-max(0,700-readiness)+bias
            recover=(1700-readiness) if readiness<700 else -1
            choice="ATTACK" if attack>=recover else "RECOVER"
            out.append({"defeats":defeats,"readiness":readiness,"edge":edge,
                        "bias":bias,"attack_score":attack,"recover_score":recover,"choice":choice})
    return out

def readiness_stress()->list[dict]:
    rows=[]
    seize_score=900+240+650-40+100
    for readiness in (0,100,250,400,550,650,700):
        recover=1700-readiness if readiness<700 else -1
        attack=2000-max(0,700-readiness)
        rows.append({"readiness":readiness,"recover":recover,"attack_no_memory":attack,
                     "seize_current":seize_score,
                     "chosen_between_recover_and_seize":"SEIZE" if seize_score>=recover else "RECOVER"})
    return rows

def veterancy_stress()->list[dict]:
    rows=[]
    for name,xp,combat,morale in RANKS[::-1]:
        morale_chance=min(350,abs(morale)*100)/1000
        expected_index=(1+combat/1000)*(1+morale_chance)
        rows.append({"rank":name,"xp":xp,"combat_bonus_permille":combat,
                     "morale_bonus":morale,"extra_action_chance":morale_chance,
                     "conditional_expected_action_damage_index":round(expected_index,4)})
    return rows

def siege_stress()->dict:
    trace=[]; supply=1000
    for day in range(1,31):
        supply=max(200,supply-70)
        trace.append({"day":day,"defender_supply":supply})
    sensitivity=[]
    for loss in (50,70,90):
        for threshold in (300,400,500):
            supply=1000; reached=None
            for day in range(1,31):
                supply=max(200,supply-loss)
                if supply<=threshold:
                    reached=day; break
            sensitivity.append({"loss_per_day":loss,"threshold":threshold,"days":reached})
    return {"current_day30_supply":trace[-1]["defender_supply"],
            "current_resolves_from_supply_alone":False,
            "current_trace":trace,"threshold_sensitivity":sensitivity}

def building_loss_stress(unit_id:str)->list[dict]:
    out=[]
    for mode in ("current_free","costed"):
        p=Params(repair_mode=mode)
        f=make_faction("f0","cap_0",p)
        pool=f.pools[unit_id]; pool.available=0
        starting_gold=f.gold
        for day in range(1,29):
            if day==14:
                damage_dwelling(f,unit_id)
            progress_repairs(f)
            if f.buildings[pool.building_id].condition=="Ruined":
                start_or_apply_repair(f,unit_id,p)
            if day>1 and (day-1)%7==0:
                weekly_growth(f,p)
        out.append({"unit":unit_id,"repair_mode":mode,
                    "available_day28":pool.available,
                    "gold_spent":starting_gold-f.gold,
                    "condition":f.buildings[pool.building_id].condition})
    return out

def capstone_completion_day(slots:int,start_gold:int=1200,daily_income:int=30)->int|None:
    gold=start_gold
    active=[]; complete=set(); started=set()
    prereqs=["p1","p2","p3"]
    for day in range(1,31):
        gold+=daily_income
        next_active=[]
        for name,remaining in active:
            remaining-=1
            if remaining<=0: complete.add(name)
            else: next_active.append((name,remaining))
        active=next_active
        if "capstone" in complete:
            return day
        free=max(0,slots-len(active))
        for name in prereqs:
            if free<=0: break
            if name not in started and gold>=180:
                gold-=180; active.append((name,3)); started.add(name); free-=1

        if all(x in complete for x in prereqs) and "capstone" not in started:
            if free>0 and gold>=420:
                gold-=420
                active.append(("capstone",5))
                started.add("capstone")
    return None

def capstone_stress()->list[dict]:
    rows=[]
    for slots in (1,2,99):
        for gold in (600,900,1200):
            rows.append({"construction_slots":slots,"start_gold":gold,
                         "completion_day":capstone_completion_day(slots,gold)})
    return rows

def hero_growth_stress()->list[dict]:
    rows=[]
    for wins in (0,1,2,3,5,8,12,20,30):
        xp=wins*100
        h=Hero("hero",xp)
        rows.append({"wins":wins,"xp":xp,"level":h.level,
                     "lab_strength_bonus":(h.level-1)*55})
    return rows

def aggregate(results:list[dict])->dict:
    def mean(key:str)->float:
        return statistics.fmean(r[key] for r in results) if results else 0.0
    recoveries=[r["f0_recovery_ratio"] for r in results if r["f0_recovery_ratio"] is not None]
    recovery_days=[r["f0_recovery70_days"] for r in results if r["f0_recovery70_days"] is not None]
    action_totals=Counter()
    f0_armies=[]; f0_territories=[]; f0_resources=[]
    for r in results:
        for counts in r["actions"].values():
            action_totals.update(counts)
        f0=next(x for x in r["factions"] if x["id"]=="f0")
        f0_armies.append(f0["army"]); f0_territories.append(f0["territory"])
        f0_resources.append(f0["resources"])
    total_actions=sum(action_totals.values())
    return {"runs":len(results),"mean_leader_share":mean("leader_share"),
            "mean_top_second_ratio":mean("top_second_ratio"),"mean_gini":mean("gini"),
            "runaway_rate":statistics.fmean(1.0 if r["runaway"] else 0.0 for r in results),
            "mean_battles":mean("battle_count"),
            "median_f0_recovery_ratio":statistics.median(recoveries) if recoveries else None,
            "median_f0_recovery70_days":statistics.median(recovery_days) if recovery_days else None,
            "mean_f0_score_share":mean("f0_score_share"),
            "f0_leader_rate":statistics.fmean(1.0 if r["f0_is_leader"] else 0.0 for r in results),
            "mean_f0_army":statistics.fmean(f0_armies),"mean_f0_territory":statistics.fmean(f0_territories),
            "mean_f0_resources":statistics.fmean(f0_resources),
            "action_share":{a:(action_totals[a]/total_actions if total_actions else 0.0)
                            for a in ACTION_ORDER}}

def mkparams(**kwargs)->Params:
    p=Params()
    for key,value in kwargs.items():
        setattr(p,key,value)
    return p

def scenario_definitions()->list[tuple[str,int,str,Params]]:
    return [
        ("baseline",2,"",Params()),
        ("day14_best_army_lost",2,"day14_loss",Params()),
        ("ranged_dwelling_free_repair",2,"ranged_destroyed",Params()),
        ("ranged_dwelling_costed_repair",2,"ranged_destroyed",mkparams(repair_mode="costed")),
        ("elephant_dwelling_free_repair",2,"elephant_destroyed",Params()),
        ("elephant_dwelling_costed_repair",2,"elephant_destroyed",mkparams(repair_mode="costed")),
        ("wealth_two_resources",2,"wealth_two_resources",Params()),
    ]

def sensitivity_definitions()->list[tuple[str,int,str,Params]]:
    return [
        ("map_small",1,"",Params()),
        ("map_large",4,"",Params()),
        ("ap_two",2,"",mkparams(ap=2)),
        ("ap_four",2,"",mkparams(ap=4)),
        ("travel_harsh",2,"",mkparams(move_cost=12)),
        ("resource_income_low",2,"",mkparams(resource_income=6)),
        ("resource_income_high",2,"",mkparams(resource_income=12)),
        ("recovery_generous",2,"",mkparams(respawn_days=2,respawn_supply=1000,
            respawn_readiness=1000,respawn_fatigue=0,respawn_preserve_veterancy=True,
            respawn_recruit_same_day=True)),
        ("recovery_harsh",2,"",mkparams(respawn_days=4,respawn_strength=160,
            respawn_supply=600,respawn_readiness=450,respawn_fatigue=200)),
        ("day14_loss_comeback50",2,"day14_loss",mkparams(comeback_floor=.50)),
        ("day14_loss_comeback70",2,"day14_loss",mkparams(comeback_floor=.70)),
    ]

def run_scenario(runs:int,seed_base:int,scale:int,shock:str,p:Params)->list[dict]:
    return [simulate(seed_base+i,56,scale,p,shock) for i in range(runs)]

def convergence_rows(results:list[dict])->list[dict]:
    rows=[]
    for n in (100,250,500,1000,2000):
        if n<=len(results):
            a=aggregate(results[:n])
            rows.append({"runs":n,"runaway_rate":a["runaway_rate"],
                         "mean_gini":a["mean_gini"],
                         "mean_leader_share":a["mean_leader_share"]})
    return rows

def flatten_summary(name:str,group:str,a:dict)->dict:
    row={"scenario":name,"group":group}
    for key,value in a.items():
        if key=="action_share":
            for action,share in value.items():
                row[f"action_{action.lower()}"]=share
        else:
            row[key]=value
    return row

def write_csv(path:Path,rows:list[dict])->None:
    if not rows: return
    fields=[]
    for row in rows:
        for key in row:
            if key not in fields: fields.append(key)
    with path.open("w",newline="",encoding="utf-8") as fh:
        writer=csv.DictWriter(fh,fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)

def focused_results()->dict:
    return {
        "memory_repeated_rival_defeats":memory_stress(),
        "low_readiness_choices":readiness_stress(),
        "veterancy":veterancy_stress(),
        "siege_starvation":siege_stress(),
        "ranged_building_loss":building_loss_stress("ranged"),
        "elephant_building_loss":building_loss_stress("beast"),
        "dwarf_capstone_construction":capstone_stress(),
        "hero_growth":hero_growth_stress(),
    }

def run_lab(runs:int=1000,seed_base:int=20260920)->Path:
    root=Path(__file__).resolve().parents[2]
    out=root/"Evidence"/"BalanceLab"
    out.mkdir(parents=True,exist_ok=True)
    summary_rows=[]; baseline_all=None
    for name,scale,shock,p in scenario_definitions():
        count=max(runs,2000) if name=="baseline" else runs
        results=run_scenario(count,seed_base,scale,shock,p)
        if name=="baseline": baseline_all=results
        summary_rows.append(flatten_summary(name,"adversarial",aggregate(results[:runs])))
    write_csv(out/"scenario_results.csv",summary_rows)

    sensitivity_rows=[]
    for name,scale,shock,p in sensitivity_definitions():
        results=run_scenario(runs,seed_base,scale,shock,p)
        sensitivity_rows.append(flatten_summary(name,"sensitivity",aggregate(results)))
    write_csv(out/"sensitivity.csv",sensitivity_rows)
    focused=focused_results()
    (out/"focused_results.json").write_text(json.dumps(focused,indent=2,sort_keys=True),encoding="utf-8")
    convergence=convergence_rows(baseline_all or [])
    (out/"convergence.json").write_text(json.dumps(convergence,indent=2),encoding="utf-8")
    (out/"baseline_sample.json").write_text(
        json.dumps((baseline_all or [])[:25],indent=2,sort_keys=True),encoding="utf-8")
    manifest={
        "seed_base":seed_base,"runs_per_scenario":runs,
        "baseline_convergence_runs":len(baseline_all or []),
        "soul_base_commit":"05597a337f06bb1b01de3fc9dbe829b95b4e9e6a",
        "living_strategy_reference_commit":"d82ddcfdf8f76795909934a9983b43025c34f4c2",
        "branch":"astra/soul-strategy-balance-lab-20260920",
        "default_params":asdict(Params()),
        "scenario_names":[x[0] for x in scenario_definitions()],
        "sensitivity_names":[x[0] for x in sensitivity_definitions()],
        "authority_notes":[
            "Source/ is read-only for this lane.",
            "Live-mirror formulas reproduce observed SoulCore integer rules.",
            "Aggregate battles, recovery glue, and generic roster costs are lab-only assumptions.",
            "Living Strategy informs strategic candidate/reasoning and commander-memory boundaries."
        ],
    }
    (out/"run_manifest.json").write_text(json.dumps(manifest,indent=2),encoding="utf-8")
    return out

def main()->None:
    parser=argparse.ArgumentParser()
    parser.add_argument("--runs",type=int,default=1000)
    parser.add_argument("--seed-base",type=int,default=20260920)
    args=parser.parse_args()
    out=run_lab(max(1,args.runs),args.seed_base)
    print(f"BALANCE_LAB_PASS output={out} runs={args.runs}")

if __name__=="__main__":
    main()
