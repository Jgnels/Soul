import unittest

from Tools.BalanceLab.balance_lab import (
    Params, Hero, Memory, Logistics, apply_travel, force_march, end_day_logistics,
    xp_for_level, rank_stats, rival_bias, simulate, siege_stress,
    building_loss_stress, memory_stress, capstone_completion_day,
    make_campaign, restore_army_if_due, choose_action, mkparams,
)

class LiveMirrorTests(unittest.TestCase):
    def test_logistics_exact_road_and_hostile_values(self):
        road=Logistics(); wild=Logistics()
        apply_travel(road,10,True,False)
        apply_travel(wild,10,False,True)
        self.assertEqual((road.supply,road.readiness,road.fatigue),(916,940,75))
        self.assertEqual((wild.supply,wild.readiness,wild.fatigue),(844,920,100))

    def test_force_march_threshold_and_cost(self):
        s=Logistics(supply=916,readiness=940,fatigue=75)
        self.assertTrue(force_march(s))
        self.assertEqual((s.supply,s.readiness,s.fatigue),(796,740,325))
        self.assertFalse(force_march(Logistics(supply=249,readiness=1000)))

    def test_end_day_recovery(self):
        s=Logistics(supply=700,readiness=650,fatigue=300)
        end_day_logistics(s,True,True)
        self.assertEqual((s.supply,s.readiness,s.fatigue),(1000,950,0))

    def test_hero_curve_and_veterancy_thresholds(self):
        self.assertEqual(xp_for_level(2),250)
        self.assertEqual(xp_for_level(3),750)
        self.assertEqual(rank_stats(99)[0],"Recruit")
        self.assertEqual(rank_stats(100)[0],"Seasoned")
        self.assertEqual(rank_stats(300),( "Veteran",50,1))
        self.assertEqual(rank_stats(1500),("Legendary",100,2))

    def test_memory_is_bounded_and_decays(self):
        h=Hero("ai")
        h.memories=[
            Memory("BattleDefeat","rival",turn=10,intensity=800),
            Memory("BattleDefeat","rival",turn=10,intensity=800),
        ]
        self.assertEqual(rival_bias(h,"rival",10),-800)
        self.assertEqual(rival_bias(h,"rival",17),0)

    def test_siege_supply_floor_is_inert_without_resolution_rule(self):
        s=siege_stress()
        self.assertEqual(s["current_day30_supply"],200)
        self.assertFalse(s["current_resolves_from_supply_alone"])

    def test_destroyed_dwelling_current_repair_erases_growth_loss(self):
        ranged={r["repair_mode"]:r for r in building_loss_stress("ranged")}
        self.assertGreater(ranged["current_free"]["available_day28"],
                           ranged["costed"]["available_day28"])

    def test_memory_changes_choice_but_does_not_force_obsession(self):
        rows=memory_stress()
        cautious=next(r for r in rows if r["defeats"]==2 and r["readiness"]==550 and r["edge"]==0)
        strong=next(r for r in rows if r["defeats"]==2 and r["readiness"]==550 and r["edge"]==300)
        self.assertEqual(cautious["choice"],"RECOVER")
        self.assertEqual(strong["choice"],"ATTACK")

    def test_parallel_construction_can_accelerate_capstone(self):
        one=capstone_completion_day(1,1200)
        unlimited=capstone_completion_day(99,1200)
        self.assertIsNotNone(one); self.assertIsNotNone(unlimited)
        self.assertLess(unlimited,one)

    def test_same_seed_same_campaign_summary(self):
        self.assertEqual(simulate(9917,days=28,scale=2),
                         simulate(9917,days=28,scale=2))

    def test_default_has_no_invented_comeback_subsidy(self):
        self.assertEqual(Params().comeback_floor,0.0)

    def test_live_mirror_still_seizes_at_zero_readiness(self):
        p=Params(); c=make_campaign(2,p); f=c.factions["f0"]
        f.army.logistics.readiness=0
        self.assertEqual(choose_action(c,f,p)[1],"SEIZE")

    def test_candidate_seize_penalty_makes_zero_readiness_recover(self):
        p=mkparams(seize_readiness_penalty=True)
        c=make_campaign(2,p); f=c.factions["f0"]; f.army.logistics.readiness=0
        self.assertEqual(choose_action(c,f,p)[1],"RECOVER")

    def test_candidate_readiness_floor_blocks_low_readiness_seize(self):
        p=mkparams(seize_min_readiness=400)
        c=make_campaign(2,p); f=c.factions["f0"]; f.army.logistics.readiness=250
        self.assertEqual(choose_action(c,f,p)[1],"RECOVER")
        f.army.logistics.readiness=400
        self.assertEqual(choose_action(c,f,p)[1],"SEIZE")

class CampaignStressSmokeTests(unittest.TestCase):
    def test_medium_map_reaches_conflict(self):
        result=simulate(1,days=56,scale=2)
        self.assertGreater(result["battle_count"],0)
        action_total=sum(sum(v.values()) for v in result["actions"].values())
        self.assertGreater(action_total,0)

class RecoveryGlueTests(unittest.TestCase):
    def test_bounded_replacement_loses_regiment_veterancy_and_full_readiness(self):
        p=Params()
        c=make_campaign(2,p)
        c.day=4
        f=c.factions["f0"]
        f.army.strength=0
        f.army.respawn_day=4
        f.army.regiment_xp=1500
        self.assertTrue(restore_army_if_due(f,c,p))
        self.assertEqual(f.army.regiment_xp,0)
        self.assertEqual((f.army.logistics.supply,f.army.logistics.readiness,
                          f.army.logistics.fatigue),
                         (p.respawn_supply,p.respawn_readiness,p.respawn_fatigue))

if __name__=="__main__":
    unittest.main()
