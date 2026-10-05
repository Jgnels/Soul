"""Synthetic parser fixtures, never gameplay acceptance."""
import unittest
from analyze_soul_player_log import analyze


def fixture(identity="first", source="river_ford", target="orc_watch", cap=15, player=45, enemy=30):
    lines = [f"SOUL_CAMPAIGN_ENCOUNTER id={identity} source={source} target={target} map=Dragon forces={player}/{enemy} cap={cap}",
             f"SOUL_RT_RESERVES_READY: human={max(0, player-cap)} enemy={max(0, enemy-cap)}"]
    active, reserve, wave = min(cap, enemy), max(0, enemy-cap), 0
    for actor in range(enemy):
        lines.append(f"SOUL_UNIT_DEFEATED: side=1 id=Enemy_{actor}")
        active -= 1
        if active * 1000 < cap * 700 and reserve:
            bodies = min(4, reserve, cap-active)
            reserve -= bodies
            active += bodies
            wave += 1
            lines.append(f"SOUL_RT_REINFORCEMENT_WAVE: side=1 bodies={bodies} wave={wave}")
    lines += [f"SOUL_BATTLE_RESOLVED: won=1 playerSurvivors={player} enemySurvivors=0 waves=0/{wave} magic=1 contacts=45 seconds=123.50",
              f"SOUL_CAMPAIGN_RESULT id={identity} target={target} victory=1 survivors={player}/0 player_region={target}"]
    return lines


def routed_fixture(player_won=True):
    """R7 victory's exact physical ledger, mirrored for a routed defeat."""
    forces, deaths = ([46, 30], [12, 27]) if player_won else ([30, 46], [27, 12])
    lines = [f"SOUL_CAMPAIGN_ENCOUNTER id=rout source=river_ford target=orc_watch map=Dragon forces={forces[0]}/{forces[1]} cap=15 mana=80",
             f"SOUL_RT_RESERVES_READY: human={forces[0]-15} enemy={forces[1]-15}"]
    waves = []
    for side in (0, 1):
        active, reserve, wave = 15, forces[side] - 15, 0
        for actor in range(deaths[side]):
            lines.append(f"SOUL_UNIT_DEFEATED: side={side} id=Side_{side}_Actor_{actor}")
            active -= 1
            if active * 1000 < 15 * 700 and reserve:
                bodies = min(4, reserve, 15-active)
                reserve -= bodies
                active += bodies
                wave += 1
                lines.append(f"SOUL_RT_REINFORCEMENT_WAVE: side={side} bodies={bodies} wave={wave}")
        waves.append(wave)
    physical = "34/3" if player_won else "3/34"
    routed = "0/1" if player_won else "1/0"
    survivors = [34, 0] if player_won else [0, 34]
    region = "orc_watch" if player_won else "river_ford"
    lines += [f"SOUL_BATTLE_RESOLVED: won={int(player_won)} playerSurvivors={survivors[0]} enemySurvivors={survivors[1]} physical={physical} routed={routed} waves={waves[0]}/{waves[1]} magic=5 contacts=612 seconds=111.51 mana=18",
              "SOUL_CAMPAIGN_MANA id=rout before=80 after=18 casts=5",
              f"SOUL_CAMPAIGN_RESULT id=rout target=orc_watch victory={int(player_won)} survivors={survivors[0]}/{survivors[1]} player_region={region}"]
    return lines


class LogAuditTests(unittest.TestCase):
    def test_routed_victory_and_defeat_conserve_physical_force(self):
        for player_won in (True, False):
            with self.subTest(player_won=player_won):
                record = analyze("\n".join(routed_fixture(player_won)))["encounters"][0]
                self.assertEqual(record["log_consistency"], "CONSISTENT", record["issues"])
                self.assertEqual(record["casualty_counts"], [12, 27] if player_won else [27, 12])
                self.assertEqual(record["active"], [11, 3] if player_won else [3, 11])
                self.assertEqual(record["reserves"], [23, 0] if player_won else [0, 23])
                self.assertEqual([len(w) for w in record["waves"]], [2, 4] if player_won else [4, 2])

    def test_rout_does_not_waive_physical_conservation_or_wave_ledger(self):
        for token in ("id=Side_1_Actor_0", "side=1 bodies=4 wave=1"):
            lines = [line for line in routed_fixture() if token not in line]
            record = analyze("\n".join(lines))["encounters"][0]
            self.assertEqual(record["log_consistency"], "INCONSISTENT")
            expected = "physical and reserve ledger" if "id=" in token else "resolved wave counts"
            self.assertIn(expected, " ".join(record["issues"]))
        for physical in ("34/0", "34/4", "35/3"):
            lines = routed_fixture()
            lines[-3] = lines[-3].replace("physical=34/3", "physical=" + physical)
            record = analyze("\n".join(lines))["encounters"][0]
            self.assertIn("do not conserve initial force", " ".join(record["issues"]))

    def test_routed_fields_require_well_formed_paired_inventory_and_binary_flags(self):
        for original, replacement in ((" physical=34/3", ""), (" routed=0/1", ""),
                                      ("physical=34/3", "physical=34"),
                                      ("physical=34/3", "physical=34/3/0"),
                                      ("physical=34/3", "physical=34/-1"),
                                      ("physical=34/3", "physical=34/31"),
                                      ("routed=0/1", "routed=0"),
                                      ("routed=0/1", "routed=0/1/0"),
                                      ("routed=0/1", "routed=0/2"),
                                      ("routed=0/1", "routed=0/-1"),
                                      ("routed=0/1", "routed=0/1.0")):
            with self.subTest(replacement=replacement):
                lines = routed_fixture()
                lines[-3] = lines[-3].replace(original, replacement)
                record = analyze("\n".join(lines))["encounters"][0]
                self.assertEqual(record["log_consistency"], "INCONSISTENT")
                self.assertIn("malformed event", " ".join(record["issues"]))

    def test_strategic_result_must_follow_rout_and_campaign_return(self):
        for original, replacement in (("routed=0/1", "routed=0/0"),
                                      ("routed=0/1", "routed=1/1"),
                                      ("playerSurvivors=34", "playerSurvivors=33"),
                                      ("enemySurvivors=0", "enemySurvivors=3")):
            lines = routed_fixture()
            lines[-3] = lines[-3].replace(original, replacement)
            record = analyze("\n".join(lines))["encounters"][0]
            self.assertIn("strategic survivors differ", " ".join(record["issues"]))
        lines = routed_fixture()
        lines[-1] = lines[-1].replace("survivors=34/0", "survivors=34/3")
        self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_nonrouted_new_and_legacy_logs_keep_strict_conservation(self):
        for with_fields in (False, True):
            lines = fixture()
            if with_fields:
                lines[-2] += " physical=45/0 routed=0/0"
            self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "CONSISTENT")
            lines = [line for line in lines if "id=Enemy_0" not in line]
            self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_70_active_large_pool_and_three_encounters(self):
        lines = (fixture(cap=35, player=300, enemy=250)
                 + fixture("second", "orc_watch", "orc_camp", cap=35, player=300, enemy=400)
                 + fixture("third", "orc_camp", "north_pass", cap=35, player=300, enemy=500))
        report = analyze("\n".join(lines))
        self.assertEqual(len(report["encounters"]), 3)
        for record in report["encounters"]:
            self.assertEqual(record["log_consistency"], "CONSISTENT", record["issues"])
            self.assertEqual(record["peak_active"], [35, 35])

    def test_malformed_encounter_rejected_even_without_later_events(self):
        first = fixture()[0]
        for bad in (first.replace("cap=15", "cap=36"), first.replace("forces=45/30", "forces=45/0"),
                    first.replace("id=first", "id=None"), first.replace("source=river_ford", "source=orc_watch"),
                    first.replace("forces=45/30", "forces=2147483648/30")):
            self.assertEqual(analyze(bad)["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_overlapping_encounters_are_explicitly_inconsistent(self):
        report = analyze("\n".join(fixture()[:-1] + fixture("second")))
        self.assertTrue(all(r["log_consistency"] == "INCONSISTENT" for r in report["encounters"]))
        self.assertIn("before campaign return", " ".join(report["encounters"][0]["issues"]))

    def test_initial_inventory_and_early_wave_are_rejected(self):
        lines = fixture()
        lines[1] = lines[1].replace("human=30", "human=31")
        issues = analyze("\n".join(lines))["encounters"][0]["issues"]
        self.assertIn("initial deployment", " ".join(issues))
        lines = fixture()
        lines.insert(2, "SOUL_RT_REINFORCEMENT_WAVE: side=0 bodies=1 wave=1")
        issues = analyze("\n".join(lines))["encounters"][0]["issues"]
        self.assertIn("before casualty threshold", " ".join(issues))

    def test_30_active_and_second_encounter(self):
        report = analyze("\n".join(fixture() + fixture("second", "orc_watch", "orc_camp")))
        self.assertEqual(report["player_loop_acceptance"], "UNVERIFIED_FROM_LOGS")
        self.assertEqual(len(report["encounters"]), 2)
        for encounter in report["encounters"]:
            self.assertEqual(encounter["log_consistency"], "CONSISTENT")
            self.assertEqual(encounter["peak_active"], [15, 15])
            self.assertEqual(encounter["casualty_counts"], [0, 30])
            self.assertEqual(sum(encounter["waves"][1]), 15)

    def test_partial_or_no_battle_never_consistent(self):
        self.assertEqual(analyze("startup only")["encounters"], [])
        self.assertEqual(analyze("\n".join(fixture()[:-1]))["encounters"][0]["log_consistency"], "INCOMPLETE")

    def test_duplicate_death_and_wrong_target_are_detected(self):
        lines = fixture()
        lines.insert(3, lines[2])
        lines[-1] = lines[-1].replace("target=orc_watch", "target=orc_camp")
        issues = analyze("\n".join(lines))["encounters"][0]["issues"]
        self.assertTrue(any("duplicate defeated" in issue for issue in issues))
        self.assertTrue(any("incorrect target" in issue for issue in issues))

    def test_missing_death_or_wave_breaks_conservation(self):
        for token in ("id=Enemy_0", "bodies=4 wave=1"):
            lines = [line for line in fixture() if token not in line]
            self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_orphan_resolution_and_malformed_side_are_visible(self):
        self.assertEqual(len(analyze(fixture()[-2])["orphan_events"]), 1)
        lines = fixture()
        lines[2] = lines[2].replace("side=1", "side=9")
        self.assertIn("invalid side", " ".join(analyze("\n".join(lines))["encounters"][0]["issues"]))

    def test_reused_identity_and_over_cap_wave_are_detected(self):
        report = analyze("\n".join(fixture() + fixture()))
        self.assertIn("reused encounter identity", " ".join(report["encounters"][1]["issues"]))
        lines = fixture()
        lines.insert(2, "SOUL_RT_REINFORCEMENT_WAVE: side=0 bodies=4 wave=1")
        self.assertIn("physical count outside active cap", " ".join(analyze("\n".join(lines))["encounters"][0]["issues"]))
    def mana_fixture(self):
        lines = fixture()
        lines[0] += " mana=24"
        lines[-2] += " mana=16"
        lines.insert(-1, "SOUL_CAMPAIGN_MANA id=first before=24 after=16 casts=1")
        return lines

    def test_campaign_mana_receipt_matches_both_authorities(self):
        record = analyze("\n".join(self.mana_fixture()))["encounters"][0]
        self.assertEqual(record["log_consistency"], "CONSISTENT", record["issues"])
        self.assertEqual(record["mana_result"]["after"], "16")

    def test_mana_aware_log_requires_complete_receipt(self):
        lines = self.mana_fixture()
        del lines[-2]
        self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCOMPLETE")

    def test_mana_receipt_rejects_wrong_identity_balance_and_casts(self):
        for before, after in (("id=first", "id=stale"), ("before=24", "before=80"),
                              ("after=16", "after=25"), ("after=16", "after=-1"),
                              ("after=16", "after=15"), ("casts=1", "casts=2"),
                              ("after=16", "after=1.5")):
            with self.subTest(before=before, after=after):
                lines = self.mana_fixture()
                lines[-2] = lines[-2].replace(before, after)
                self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_mana_receipt_order_and_duplicates_are_rejected(self):
        for placement in (1, -1, -2):
            lines = self.mana_fixture()
            lines.insert(placement, lines[-2])
            self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")
        lines = self.mana_fixture()
        receipt = lines.pop(-2)
        lines.append(receipt)
        self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_negative_casts_and_mana_are_rejected(self):
        lines = fixture()
        lines[-2] = lines[-2].replace("magic=1", "magic=-1")
        self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")
        for balance in ("-1", "2147483648", "1.5"):
            lines = fixture()
            lines[0] += " mana=" + balance
            self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")

    def test_defeat_requires_source_return(self):
        lines = ["SOUL_CAMPAIGN_ENCOUNTER id=d source=river_ford target=orc_watch map=Dragon forces=1/4 cap=15",
                 "SOUL_RT_RESERVES_READY: human=0 enemy=0",
                 "SOUL_UNIT_DEFEATED: side=0 id=Human_0",
                 "SOUL_BATTLE_RESOLVED: won=0 playerSurvivors=0 enemySurvivors=4 waves=0/0 magic=0 contacts=3 seconds=20",
                 "SOUL_CAMPAIGN_RESULT id=d target=orc_watch victory=0 survivors=0/4 player_region=river_ford"]
        self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "CONSISTENT")
        lines[-1] = lines[-1].replace("player_region=river_ford", "player_region=orc_watch")
        self.assertEqual(analyze("\n".join(lines))["encounters"][0]["log_consistency"], "INCONSISTENT")


if __name__ == "__main__":
    unittest.main(verbosity=2)
