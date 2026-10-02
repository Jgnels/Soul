import re, pathlib, collections, json
root=pathlib.Path(__file__).parent
lines=(root/'SmallBattleRuntime.log').read_text(errors='replace').splitlines()
pat=re.compile(r'i=(\d+) side=(\d+) role=(\w+) hp=([\d.]+) hidden=(\d+) visible=(\d+) actor=V\(X=([\d.-]+), Y=([\d.-]+), Z=([\d.-]+)\) root=V\(X=([\d.-]+), Y=([\d.-]+), Z=([\d.-]+)\)')
rows=[]
for line in lines:
 m=pat.search(line)
 if m: rows.append(m.groups())
bad=[r for r in rows if float(r[3])>0 and (r[4]!='0' or r[5]!='1' or float(r[8]) < -200)]
offset=[r for r in rows if float(r[3])>0 and abs(float(r[8])-float(r[11]))>600]
print(json.dumps({'samples':len(rows),'living_hidden_or_below_ground':bad[:20],'large_root_z_offsets':offset[:20]},indent=2))
