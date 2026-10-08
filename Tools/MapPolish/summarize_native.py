"""Report native collision grades with explicit sampling/noise limits."""
import json,math,hashlib
from route_surface import OUT
p=OUT/'Local/native-route-hits-final.json';d=json.loads(p.read_text())
rows=[];delta=[];misses=0
for r in d['routes']:
    grades=[];raw=[];excluded=0;missing=0
    for s in r['segments']:
        h=s['hits'];missing+=sum(x['z_m'] is None for x in h)
        delta.extend(abs(x['z_m']-x['expected_landscape_z_m']) for x in h if x['z_m'] is not None and x['landscape'])
        for a,b in zip(h[:-1],h[1:]):
            distance=b['station_m']-a['station_m']
            if distance<1e-7 or a['z_m'] is None or b['z_m'] is None:continue
            g=math.degrees(math.atan2(abs(b['z_m']-a['z_m']),distance))
            entry=dict(grade_deg=g,distance_m=distance,chord_distance_m=math.dist(a['xy_m'],b['xy_m']),from_xy_m=a['xy_m'],to_xy_m=b['xy_m'],actors=[a['actor'],b['actor']])
            raw.append(entry)
            if distance>=.2:grades.append(entry)
            else:excluded+=1
    worst=max(grades,key=lambda x:x['grade_deg']);raw_worst=max(raw,key=lambda x:x['grade_deg'])
    rows.append(dict(a=r['a'],b=r['b'],max_grade_deg=worst['grade_deg'],worst=worst,raw_maximum_including_sub_20cm_intervals=raw_worst,sub_20cm_intervals_excluded_from_acceptance=excluded,misses=missing,passes=worst['grade_deg']<=22.1 and missing==0))
    misses+=missing
result=dict(route_count=len(rows),passes=sum(x['passes'] for x in rows),misses=misses,points=sum(len(s['hits']) for r in d['routes'] for s in r['segments']),max_landscape_height_delta_m=max(delta),method='Native simple visibility collision, 1 m travel stations and 0.25 m crossing stations. Grade uses travelled horizontal arc distance, not the chord across intervening route bends. Overlapping station sets create short intervals: intervals under 0.20 m retained in raw diagnostics, excluded from acceptance to avoid millimetre ray-query error amplification. Analytical 0.25 m inventory independently samples every polyline segment, including its vertices. This is sampled qualification, not a continuous-grade or turning-radius guarantee.',source_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),routes=rows)
(OUT/'native-route-final-summary.json').write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='routes'},indent=2))
print('FAILURES',[(r['a'],r['b'],r['max_grade_deg']) for r in rows if not r['passes']])
print('FORD',next(r['max_grade_deg'] for r in rows if r['a']=='crossroads' and r['b']=='river_ford'))
