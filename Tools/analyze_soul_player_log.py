#!/usr/bin/env python3
"""Audit existing Soul battle logs. Does not launch, drive or qualify the game."""
import argparse
import json
from pathlib import Path
import re

EVENT = re.compile(r"\b(SOUL_CAMPAIGN_ENCOUNTER|SOUL_RT_RESERVES_READY|SOUL_UNIT_DEFEATED|SOUL_RT_REINFORCEMENT_WAVE|SOUL_RT_REINFORCEMENT_FAILED|SOUL_BATTLE_RESOLVED|SOUL_CAMPAIGN_RESULT|SOUL_CAMPAIGN_MANA)\b:?\s*(.*)")
FIELDS = re.compile(r"(\w+)=([^\s]+)")


def analyze(text):
    records, orphan_events = [], []
    current = None
    for line_number, line in enumerate(text.splitlines(), 1):
        match = EVENT.search(line)
        if not match:
            continue
        kind, payload = match.groups()
        fields = dict(FIELDS.findall(payload))
        if kind == "SOUL_CAMPAIGN_ENCOUNTER":
            unfinished = current is not None and current["campaign_result"] is None
            if unfinished:
                current["issues"].append(f"line {line_number}: next encounter started before campaign return")
            current = dict(encounter=fields, start_line=line_number, deaths=[[], []],
                           waves=[[], []], issues=[], active=None, reserves=None,
                           peak_active=None, battle_result=None, campaign_result=None, mana_result=None)
            if any(r["encounter"].get("id") == fields.get("id") for r in records):
                current["issues"].append(f"line {line_number}: reused encounter identity")
            if unfinished:
                current["issues"].append(f"line {line_number}: encounter overlaps an unresolved predecessor")
            try:
                if any(not fields.get(key) or fields[key] == "None" for key in ("id", "source", "target", "map")):
                    raise ValueError("missing encounter identity, geography or map")
                if "mana" in fields and not 0 <= int(fields["mana"]) <= 2147483647:
                    raise ValueError("invalid committed mana")
                if fields["source"] == fields["target"]:
                    raise ValueError("source and target must differ")
                force = [int(n) for n in fields["forces"].split("/")]
                if len(force) != 2 or any(n <= 0 or n > 2147483647 for n in force):
                    raise ValueError("invalid strategic force")
                if not 1 <= int(fields["cap"]) <= 35:
                    raise ValueError("active cap outside supported 1-35 range")
            except (KeyError, ValueError) as error:
                current["issues"].append(f"line {line_number}: malformed encounter: {error}")
            records.append(current)
            continue
        if current is None:
            orphan_events.append(dict(line=line_number, event=kind))
            continue
        r = current

        def issue(message):
            r["issues"].append(f"line {line_number}: {message}")

        try:
            force = [int(n) for n in r["encounter"]["forces"].split("/")]
            if len(force) != 2 or min(force) <= 0:
                raise ValueError("invalid strategic force")
            cap = int(r["encounter"]["cap"])
            if cap <= 0:
                raise ValueError("invalid active cap")
            if kind == "SOUL_RT_RESERVES_READY":
                if r["reserves"] is not None:
                    issue("duplicate initial reserve record")
                    continue
                r["reserves"] = [int(fields["human"]), int(fields["enemy"])]
                r["active"] = [force[s] - r["reserves"][s] for s in (0, 1)]
                r["peak_active"] = r["active"].copy()
                if r["active"] != [min(n, cap) for n in force]:
                    issue("initial deployment differs from committed force and cap")
            elif kind in ("SOUL_UNIT_DEFEATED", "SOUL_RT_REINFORCEMENT_WAVE"):
                side = int(fields["side"])
                if side not in (0, 1):
                    raise ValueError("invalid side")
                if r["active"] is None:
                    issue("physical event before initial reserve inventory")
                    continue
                if r["battle_result"] is not None:
                    issue("physical event after resolution")
                if kind == "SOUL_UNIT_DEFEATED":
                    identity = fields["id"]
                    if any(identity in deaths for deaths in r["deaths"]):
                        issue(f"duplicate defeated actor {identity}")
                        continue
                    r["deaths"][side].append(identity)
                    r["active"][side] -= 1
                else:
                    bodies, ordinal = int(fields["bodies"]), int(fields["wave"])
                    if r["active"][side] * 1000 >= cap * 700:
                        issue("reinforcement arrived before casualty threshold")
                    if r["reserves"][side] <= 0:
                        issue("reinforcement arrived without strategic reserves")
                    if not 1 <= bodies <= min(4, cap):
                        issue("reinforcement wave outside delivery bound")
                    if ordinal != len(r["waves"][side]) + 1:
                        issue("reinforcement ordinal skipped or duplicated")
                    r["waves"][side].append(bodies)
                    r["active"][side] += bodies
                    r["reserves"][side] -= bodies
                    r["peak_active"][side] = max(r["peak_active"][side], r["active"][side])
            elif kind == "SOUL_RT_REINFORCEMENT_FAILED":
                issue("runtime reported reinforcement failure")
            elif kind == "SOUL_BATTLE_RESOLVED":
                if r["battle_result"] is not None:
                    issue("duplicate battle resolution")
                r["battle_result"] = fields
                if int(fields["magic"]) < 0:
                    issue("negative magic cast count")
                if "mana" in r["encounter"]:
                    if "mana" in fields and not 0 <= int(fields["mana"]) <= int(r["encounter"]["mana"]):
                        issue("battle mana outside committed balance")
                survivors = [int(fields["playerSurvivors"]), int(fields["enemySurvivors"])]
                won = int(fields["won"])
                if won not in (0, 1) or ((survivors[1] == 0 and survivors[0] > 0) != bool(won)):
                    issue("winner contradicts surviving forces")
                if min(survivors) != 0:
                    issue("result published with both forces still alive")
                physical = survivors
                if "physical" in fields or "routed" in fields:
                    # Arena FinishBattle keeps living bodies in the physical ledger,
                    # but a routed force has zero effective campaign survivors.
                    # Both fields are required together; legacy logs remain strict.
                    physical = [int(n) for n in fields["physical"].split("/")]
                    routed = [int(n) for n in fields["routed"].split("/")]
                    if len(physical) != 2 or any(not 0 <= n <= force[s] for s, n in enumerate(physical)):
                        raise ValueError("invalid physical survivor pair")
                    if len(routed) != 2 or any(n not in (0, 1) for n in routed):
                        raise ValueError("invalid routed side pair")
                    for side in (0, 1):
                        if survivors[side] != (0 if routed[side] else physical[side]):
                            issue(f"side {side} strategic survivors differ from physical/routed resolution")
                for side in (0, 1):
                    if force[side] - len(r["deaths"][side]) != physical[side]:
                        issue(f"side {side} casualties plus physical survivors do not conserve initial force")
                    if r["active"] is not None and r["active"][side] + r["reserves"][side] != physical[side]:
                        issue(f"side {side} physical and reserve ledger differs from physical survivors")
                if [int(n) for n in fields["waves"].split("/")] != [len(w) for w in r["waves"]]:
                    issue("resolved wave counts differ from deliveries")
            elif kind == "SOUL_CAMPAIGN_MANA":
                if r["mana_result"] is not None:
                    issue("duplicate campaign mana receipt")
                r["mana_result"] = fields
                if r["campaign_result"] is not None:
                    issue("mana receipt after campaign return")
                if fields["id"] != r["encounter"]["id"]:
                    issue("mana receipt has incorrect id")
                before, after, casts = (int(fields[key]) for key in ("before", "after", "casts"))
                if "mana" not in r["encounter"] or before != int(r["encounter"]["mana"]):
                    issue("mana receipt differs from committed balance")
                if not 0 <= after <= before <= 2147483647 or casts < 0:
                    issue("invalid mana receipt balance or cast count")
                battle = r["battle_result"]
                if battle is None:
                    issue("mana receipt has no preceding physical resolution")
                elif after != int(battle["mana"]) or casts != int(battle["magic"]):
                    issue("campaign mana receipt differs from physical result")
            elif kind == "SOUL_CAMPAIGN_RESULT":
                if r["campaign_result"] is not None:
                    issue("duplicate campaign result")
                r["campaign_result"] = fields
                battle = r["battle_result"]
                if battle is None:
                    issue("campaign result has no preceding physical resolution")
                else:
                    if fields["survivors"] != f"{battle['playerSurvivors']}/{battle['enemySurvivors']}" or fields["victory"] != battle["won"]:
                        issue("campaign result differs from physical battle result")
                    expected_region = r["encounter"]["target"] if battle["won"] == "1" else r["encounter"]["source"]
                    if fields["player_region"] != expected_region:
                        issue("campaign returned to incorrect region")
                for key in ("id", "target"):
                    if fields[key] != r["encounter"][key]:
                        issue(f"campaign result has incorrect {key}")
            if r["active"] is not None:
                if any(n < 0 or n > cap for n in r["active"]):
                    issue("physical count outside active cap")
                if min(r["reserves"]) < 0:
                    issue("reserve pool overdrawn")
        except (KeyError, ValueError) as error:
            issue(f"malformed event: {error}")

    for r in records:
        complete = all(r[key] is not None for key in ("reserves", "battle_result", "campaign_result"))
        if "mana" in r["encounter"]:
            complete = complete and r["mana_result"] is not None and "mana" in (r["battle_result"] or {})
        r["log_consistency"] = "INCONSISTENT" if r["issues"] else "CONSISTENT" if complete else "INCOMPLETE"
        r["casualty_counts"] = [len(d) for d in r.pop("deaths")]
    return dict(player_loop_acceptance="UNVERIFIED_FROM_LOGS", encounters=records,
                orphan_events=orphan_events,
                limitations="Logs alone do not prove ordinary input, rendered physical behavior, HOLD inheritance, frame times, stuck actors, Firebolt timing, save recovery or process lifecycle.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    report = analyze(args.log.read_text(encoding="utf-8-sig", errors="replace"))
    print(json.dumps(report, indent=2))
    if report["orphan_events"] or any(r["issues"] for r in report["encounters"]):
        return 1
    return 0 if report["encounters"] and all(r["log_consistency"] == "CONSISTENT" for r in report["encounters"]) else 2


if __name__ == "__main__":
    raise SystemExit(main())
