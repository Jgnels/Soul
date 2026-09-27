"""Non-UE checks on the production continuation data, not runtime acceptance."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def data(name):
    return json.loads((ROOT / "Data" / name).read_text(encoding="utf-8"))


class ContinuationDataTests(unittest.TestCase):
    def setUp(self):
        self.config = data("soul_vertical_scenario_20260925.json")
        self.scenario = data("soul_campaign_start_states_v1_20260922.json")["scenarios"]["founder_human_orc_micro"]
        self.world = data("soul_world_overmap_v1_20260922.json")

    def test_three_reachable_hostile_garrisons(self):
        regions = set(self.scenario["region_ids"])
        garrisons = self.config["hostile_garrisons"]
        self.assertGreaterEqual(len(garrisons), 3)
        self.assertTrue(set(garrisons) <= regions)
        self.assertNotIn(self.scenario["player_start_region"], garrisons)
        self.assertTrue(all(type(n) is int and 0 < n <= 2147483647 for n in garrisons.values()))
        reached = {self.scenario["player_start_region"]}
        while True:
            expanded = reached | {
                endpoint for e in self.world["edges"]
                if e["a"] in regions and e["b"] in regions
                and (e["a"] in reached or e["b"] in reached)
                for endpoint in (e["a"], e["b"])
            }
            if expanded == reached:
                break
            reached = expanded
        self.assertTrue(set(garrisons) <= reached)

    def test_continued_victory_route_uses_real_edges(self):
        edges = {frozenset((e["a"], e["b"])) for e in self.world["edges"]}
        route = ["human_capital", "crossroads", "river_ford", "orc_watch", "orc_camp", "north_pass", "forest_edge"]
        for origin, target in zip(route, route[1:]):
            self.assertIn(frozenset((origin, target)), edges)
        self.assertTrue(set(route[3:6]) <= self.config["hostile_garrisons"].keys())

    def test_existing_matchup_and_battlefield_remain_supported(self):
        self.assertEqual((self.config["player_faction"], self.config["player_unit_id"]), ("humans", "human_knight"))
        self.assertEqual((self.config["enemy_faction"], self.config["enemy_unit_id"]), ("dwarves", "dwarf_warrior"))
        self.assertEqual(self.config["battle_map"], "/Game/Dragon_graveyard/Level/L_showcase_level")
        self.assertFalse(any(candidate.get("enabled", False) for candidate in self.config["battlefield_candidates"]))
        self.assertTrue(15 <= self.config["active_cap_per_side"] <= 35)


    def test_each_encounter_source_has_secured_retreat_route(self):
        graph = {key: set() for key in self.scenario["region_ids"]}
        for edge in self.world["edges"]:
            a, b = edge["a"], edge["b"]
            if a in graph and b in graph:
                graph[a].add(b)
                graph[b].add(a)
        route = ["river_ford", "orc_watch", "orc_camp", "north_pass"]
        secured = {"human_capital", "crossroads", route[0]}
        for source, target in zip(route, route[1:]):
            reached, pending = set(), [source]
            while pending:
                current = pending.pop()
                if current in reached:
                    continue
                reached.add(current)
                pending.extend((graph[current] & secured) - reached)
            self.assertIn("human_capital", reached)
            secured.add(target)


if __name__ == "__main__":
    unittest.main()
