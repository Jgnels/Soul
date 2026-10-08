"""Chart existing thermal telemetry; no third-party dependency and no benchmark launch."""
from pathlib import Path
import json, datetime
from html import escape
R=Path(__file__).resolve().parents[2]; E=R/'Evidence/ProductionContinuation-20261008'; P=E/'Local/runtime-profile-60s-r1/runtime'
s=json.loads((P/'summary.json').read_text()); rows=[json.loads(v) for v in (P/'telemetry.jsonl').read_text().splitlines()]
ready=(datetime.datetime.fromisoformat(s['ready_utc'])-datetime.datetime.fromisoformat(s['started_utc'])).total_seconds()
xmax=max(v['elapsed_seconds'] for v in rows); left=85; width=990
svg=['<svg xmlns="http://www.w3.org/2000/svg" width="1160" height="670" viewBox="0 0 1160 670"><rect width="1160" height="670" fill="#15191d"/><g font-family="sans-serif" fill="#eee">', '<text x="45" y="38" font-size="23">Corrected 60-second attempt: thermal cutoff before completion</text>', '<text x="45" y="66" font-size="15">Existing telemetry only. No complete mean, P95 or P99; no retry.</text>']
def text(x,y,label,size=13,color='#bbb'):svg.append(f'<text x="{x}" y="{y}" fill="{color}" font-size="{size}">{escape(label)}</text>')
def line(x1,y1,x2,y2,color='#444',dash=''):svg.append(f'<path d="M{x1:.2f},{y1:.2f}L{x2:.2f},{y2:.2f}" fill="none" stroke="{color}" stroke-dasharray="{dash}"/>')
def panel(top,height,lo,hi,ticks,series,title):
 def xp(x):return left+x/xmax*width
 def yp(y):return top+height-(y-lo)/(hi-lo)*height
 text(left,top-13,title,16,'#eee')
 for tick in ticks:line(left,yp(tick),left+width,yp(tick));text(43,yp(tick)+4,str(tick))
 for sec in range(0,int(xmax)+1,20):line(xp(sec),top,xp(sec),top+height,'#292f35');text(xp(sec)-7,top+height+19,str(sec))
 line(xp(ready),top,xp(ready),top+height,'#ccc','4 4')
 for values,color in series:
  points=' '.join(f'{xp(r["elapsed_seconds"]):.2f},{yp(v):.2f}' for r,v in zip(rows,values))
  svg.append(f'<polyline points="{points}" fill="none" stroke="{color}" stroke-width="2.5"/>')
 return xp,yp
xp,yp=panel(115,185,40,90,[40,50,60,70,80,85,90],[([max(g['temperature_c'] for g in r['gpu']) for r in rows],'#eb967a')],'GPU temperature (C)')
line(left,yp(85),left+width,yp(85),'#ff766e','7 4');text(860,yp(85)-7,'Hard cutoff 85 C',13,'#ffaaa4');text(xp(ready)+6,137,'Map-ready log marker',12)
panel(390,175,0,8,[0,2,4,6,8],[([max(g['memory_used_mib'] for g in r['gpu'])/1024 for r in rows],'#8fb8df'),([r.get('process_memory',{}).get('private_commit_mib',0)/1024 for r in rows],'#b7d28e')],'Memory (GiB)')
text(85,350,'Blue: device-wide VRAM, including other apps. Green: owned game process private commit.',14)
text(400,620,'Seconds since launch',16);text(85,650,'Requested 20-second warmup + 60-second sample. Map-ready marker is not the sample-start timestamp.',13)
svg.append('</g></svg>');(E/'performance-thermal.svg').write_text('\n'.join(svg),encoding='utf-8');print('Existing telemetry charted; no benchmark launched.')
