"""Run Soul's complete deterministic non-UE overmap acceptance gate."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Evidence" / "WorldOvermap" / "nonue_acceptance_manifest.json"

STEPS = [
    ("world_graph", "Tools/validate_soul_world_overmap.py"),
    ("runtime_import", "Tools/validate_soul_overmap_runtime_import.py"),
    ("settlement_slots", "Tools/validate_soul_overmap_settlement_slots.py"),
    ("directed_approaches", "Tools/validate_soul_overmap_approach_profiles.py"),
    ("campaign_start_state", "Tools/validate_soul_campaign_start_states.py"),
    ("battle_handoff_build", "Tools/build_soul_overmap_battle_handoff.py"),
    ("battle_handoff_validation", "Tools/validate_soul_overmap_battle_handoff.py"),
    ("founder_campaign_stress", "Tools/analyze_soul_founder_campaign_stress.py"),
    ("founder_campaign_stress_validation", "Tools/validate_soul_founder_campaign_stress.py"),
    ("founder_assault_stress", "Tools/stress_soul_founder_assault_balance.py"),
    ("founder_assault_stress_validation", "Tools/validate_soul_founder_assault_balance.py"),
    ("founder_reaction_window_analysis", "Tools/analyze_soul_founder_reaction_windows.py"),
    ("founder_reaction_window_validation", "Tools/validate_soul_founder_reaction_windows.py"),
    ("founder_traversal", "Tools/simulate_soul_founder_overmap.py"),
    ("settlement_coverage", "Tools/analyze_soul_overmap_settlement_coverage.py"),
    ("battlefield_fidelity", "Tools/analyze_soul_overmap_battlefield_coverage.py"),
    ("opening_pressure", "Tools/analyze_soul_campaign_openings.py"),
    ("visual_anchor_build", "Tools/build_soul_overmap_visual_anchors.py"),
    ("visual_anchor_validation", "Tools/validate_soul_overmap_visual_anchors.py"),
    ("route_visual_build", "Tools/build_soul_overmap_route_visual_cues.py"),
    ("route_visual_validation", "Tools/validate_soul_overmap_route_visual_cues.py"),
    ("integrated_bundle_build", "Tools/build_soul_overmap_integrated_import.py"),
    ("integrated_bundle_validation", "Tools/validate_soul_overmap_integrated_import.py"),
    ("founder_presentation_build", "Tools/build_soul_founder_presentation_import.py"),
    ("founder_presentation_validation", "Tools/validate_soul_founder_presentation_import.py"),
    ("founder_presentation_geometry", "Tools/analyze_soul_founder_presentation_geometry.py"),
    ("founder_state_vectors_build", "Tools/build_soul_founder_presentation_state_vectors.py"),
    ("founder_state_vectors_validation", "Tools/validate_soul_founder_presentation_state_vectors.py"),
    ("founder_ue_staging_export", "Tools/export_soul_founder_ue_staging.py"),
    ("founder_ue_staging_validation", "Tools/validate_soul_founder_ue_staging.py"),
    ("founder_import_bundle_v2_build", "Tools/build_soul_founder_import_bundle_v2.py"),
    ("founder_import_bundle_v2_validation", "Tools/validate_soul_founder_import_bundle_v2.py"),
    ("ue_acceptance_fixtures_build", "Tools/build_soul_overmap_ue_acceptance_fixtures.py"),
    ("ue_acceptance_fixtures_validation", "Tools/validate_soul_overmap_ue_acceptance_fixtures.py"),
    ("founder_structural_stage_static_validation", "Tools/validate_soul_founder_overmap_structural_stage.py"),
    ("battlefield_environment_expansion_build", "Tools/build_soul_battlefield_environment_expansion.py"),
    ("battlefield_environment_expansion_validation", "Tools/validate_soul_battlefield_environment_expansion.py"),
    ("overmap_preview_build", "Tools/build_soul_overmap_preview_bundle.py"),
    ("overmap_preview_validation", "Tools/validate_soul_overmap_preview_bundle.py"),
    ("founder_state_preview_build", "Tools/build_soul_founder_state_preview.py"),
    ("founder_state_preview_validation", "Tools/validate_soul_founder_state_preview.py"),
]
results = []
for name, script in STEPS:
    proc = subprocess.run(
        [sys.executable, script],
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    stdout = proc.stdout.replace("\r\n", "\n")
    stderr = proc.stderr.replace("\r\n", "\n")
    results.append({
        "name": name,
        "script": script,
        "exit_code": proc.returncode,
        "stdout_sha256": hashlib.sha256(stdout.encode("utf-8")).hexdigest(),
        "stderr_sha256": hashlib.sha256(stderr.encode("utf-8")).hexdigest(),
        "stdout_tail": stdout.rstrip().splitlines()[-1:] or [],
        "stderr_tail": stderr.rstrip().splitlines()[-3:] if stderr.strip() else [],
    })
    state = "PASS" if proc.returncode == 0 else "FAIL"
    print(f"{state} {name}: {script}")
failed = [item for item in results if item["exit_code"] != 0]
manifest = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not failed else "fail",
    "steps_total": len(results),
    "steps_passed": len(results) - len(failed),
    "steps_failed": len(failed),
    "steps": results,
}
OUT.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
print(json.dumps({
    "status": manifest["status"],
    "steps_total": manifest["steps_total"],
    "steps_passed": manifest["steps_passed"],
    "steps_failed": manifest["steps_failed"],
    "evidence": str(OUT),
}, indent=2))
if failed:
    raise SystemExit(1)
