"""Synthetic parser fixtures, never gameplay acceptance."""
import unittest
from analyze_soul_player_log import analyze


def fixture(identity="first", source="river_ford", target="orc_watch"):
    lines = [f"SOUL_CAMPAIGN_ENCOUNTER id={identity} source={source} target={target} map=Dragon forces=45/30 cap=15",
             "SOUL_RT_RESERVES_READY: human=30 enemy=15"]
    active, reserve, wave = 15, 15, 0
    for actor in range(30):
        lines.append(f"SOUL_UNIT_DEFEATED: side=1 id=Enemy_{actor}")
        active -= 1
        if active < 10.5 and reserve:
            bodies = min(4, reserve)
            reserve -= bodies
            active += bodies
            wave += 1
            lines.append(f"SOUL_RT_REINFORCEMENT_WAVE: side=1 bodies={bodies} wave={wave}")
    lines += [f"SOUL_BATTLE_RESOLVED: won=1 playerSurvivors=45 enemySurvivors=0 waves=0/{wave} magic=1 contacts=45 seconds=123.50",
              f"SOUL_CAMPAIGN_RESULT id={identity} target={target} victory=1 survivors=45/0 player_region={target}"]
    return lines


class LogAuditTests(unittest.TestCase):
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
