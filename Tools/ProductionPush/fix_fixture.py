from pathlib import Path
import runpy,json
R=Path.cwd();p=R/'Tools/ProductionPush/export_runtime.py';t=p.read_text();t=t.replace("state['owners']={n['id']:n.get('owner') or '' for n in world['nodes']}","state['owners']={n['id']:'' for n in world['nodes']}");t=t.replace('retained founder ownership overlay plus canonical node owners outside founder slice.','retained founder ownership overlay; outer regions neutral in this qualification fixture because current campaign save/combat authority is two-faction. Six-faction gameplay is NOT claimed.');p.write_text(t);runpy.run_path(str(p),run_name='__main__')
E=R/'Evidence/ProductionPush-20261008';(E/'runtime-scope.md').write_text('''# Opt-in runtime scope
The production profile is presentation data for all 36 canonical IDs and 51 legal pairs. It does not redefine the graph.

The current founder campaign/save validator accepts only PlayerFaction, EnemyFaction and unowned regions. The initial integration fixture copied six-faction structural owners into it; its first real F9 correctly rejected that unsupported state. The failed receipt is retained under Local/runtime-input-r1.

The corrected qualification fixtures retain the established founder/Human-proof owners and leave additional regions neutral. This is an isolated, explicitly named qualification scenario, not a production ownership/balance decision. Canonical structural world data is unchanged. No save schema, accepted-owner rule, battle authority or faction rules were changed. Six-faction strategic simulation remains outside this integration proof.

Existing camera restoration recenters on the saved company region. Exact prior pan/zoom is not in the existing save schema and is not claimed.
''')
