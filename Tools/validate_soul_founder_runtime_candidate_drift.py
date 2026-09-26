"""Validate the read-only founder runtime candidate drift audit snapshot.

This validator deliberately stays outside the deterministic overmap master gate because
its input is the volatile dirty primary checkout owned by another lane.
"""
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE = Path(r"D:\RefinedBadger\Games\Soul")
AUDIT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_drift.json"
OUT = ROOT / "Evidence" / "WorldOvermap" / "founder_runtime_candidate_drift_validation.json"

audit = json.loads(AUDIT.read_text(encoding="utf-8"))
errors: list[str] = []
warnings: list[str] = []

if audit.get("status") != "READ_ONLY_RUNTIME_CANDIDATE_DRIFT_AUDIT":
    errors.append("audit status mismatch")
if audit.get("authority", {}).get("candidate_authority") != "UNTRACKED_DIRTY_PRIMARY_PROTOTYPE_NOT_CANONICAL":
    errors.append("candidate authority boundary missing")

expected_counts = {
    "accepted_regions": 9,
    "candidate_regions": 9,
    "accepted_edges": 10,
    "candidate_edges": 10,
    "edge_drift_items": 0,
}
for key, expected in expected_counts.items():
    actual = audit.get("counts", {}).get(key)
    if actual != expected:
        errors.append(f"{key}: expected {expected}, got {actual}")

checks = {x["id"]: x for x in audit.get("checks", [])}
required_checks = {
    "candidate_is_uncommitted",
    "topology_matches_current_founder_ids",
    "edge_graph_matches",
    "region_metadata_matches",
    "hostile_nonsettlement_entry_gated_before_occupation",
    "battle_recipe_uses_directed_handoff",
    "battle_victory_returns_to_actual_region",
    "neutral_rewards_are_data_driven",
    "enemy_force_model_matches_reinforcement_analysis",
}
missing = required_checks - set(checks)
if missing:
    errors.append(f"missing audit checks: {sorted(missing)}")

for cid in ("candidate_is_uncommitted", "topology_matches_current_founder_ids", "edge_graph_matches"):
    if cid in checks and not checks[cid].get("pass"):
        errors.append(f"expected audit boundary/topology check to pass: {cid}")

for cid in (
    "hostile_nonsettlement_entry_gated_before_occupation",
    "battle_recipe_uses_directed_handoff",
    "battle_victory_returns_to_actual_region",
):
    if cid in checks and checks[cid].get("pass"):
        errors.append(f"audit no longer detects required runtime convergence issue: {cid}")

critical_ids = {
    x["id"] for x in audit.get("checks", [])
    if x.get("severity") == "critical" and not x.get("pass")
}
if critical_ids != {
    "hostile_nonsettlement_entry_gated_before_occupation",
    "battle_victory_returns_to_actual_region",
}:
    errors.append(f"critical failure set drift: {sorted(critical_ids)}")

metadata = checks.get("region_metadata_matches", {}).get("evidence", [])
metadata_keys = {
    (x.get("region_id"), x.get("field"), x.get("accepted"), x.get("candidate"))
    for x in metadata
}
expected_metadata = {
    ("ancient_shrine", "biome", "mountain_forest", "temperate"),
    ("ancient_shrine", "landform", "shrine_terrace", "ridge"),
    ("ancient_shrine", "feature", "ancient_shrine", "shrine"),
    ("orc_camp", "landform", "ruined_city_edge", "mesa"),
}
if metadata_keys != expected_metadata:
    errors.append(f"candidate metadata drift set changed: {sorted(metadata_keys, key=str)}")

def git_status(rel: str) -> str:
    proc = subprocess.run(
        ["git", "-C", str(CANDIDATE), "status", "--short", "--", rel],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return proc.stdout.strip()

current_hashes = {}
for rel, expected_hash in audit.get("candidate_hashes", {}).items():
    path = CANDIDATE / rel
    if not path.exists():
        errors.append(f"candidate file missing since audit: {rel}")
        continue
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    current_hashes[rel] = actual
    if actual != expected_hash:
        warnings.append(f"candidate changed since audit; rerun audit before relying on details: {rel}")
    status = git_status(rel)
    if not status.startswith("??"):
        warnings.append(f"candidate Git state changed since audit: {rel} -> {status or 'tracked clean'}")

convergence = audit.get("convergence_order", [])
if len(convergence) < 6:
    errors.append("safe convergence order is incomplete")
if convergence and "Preserve the candidate files" not in convergence[0]:
    errors.append("safe convergence order must preserve dirty-primary work first")

result = {
    "schema": 1,
    "generated": "2026-09-22",
    "status": "pass" if not errors else "fail",
    "audit_snapshot_valid": not errors,
    "candidate_snapshot_unchanged": not warnings,
    "candidate_is_external_volatile_input": True,
    "master_gate_eligible": False,
    "critical_failures_confirmed": sorted(critical_ids),
    "current_candidate_hashes": current_hashes,
    "warnings": warnings,
    "errors": errors,
}
OUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
if errors:
    raise SystemExit(1)
