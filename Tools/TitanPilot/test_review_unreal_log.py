import unittest
from review_unreal_log import review


class LogReviewTests(unittest.TestCase):
    def test_new_page_is_not_an_error_but_missing_packages_are(self):
        self.assertEqual(review("LoadErrors: New Page: New Map")["unresolved_load_errors"], [])
        self.assertEqual(len(review("LoadErrors: While trying to load package /Game/Missing")["unresolved_load_errors"]), 1)

    def test_only_exact_editor_template_warning_with_loaded_module_is_classified(self):
        warning = ("LogLinker: Warning: [AssetLog] C:/Engine/Content/Maps/Templates/HLODs/HLODLayer_Merged.uasset: "
                   "VerifyImport: Failed to find script package for import object 'Package /Script/WorldPartitionHLODUtilities'")
        self.assertEqual(len(review(warning)["unresolved_load_errors"]), 1)
        loaded = "\nLogModuleManager: InternalLoadLibrary: 'WorldPartitionHLODUtilities'"
        self.assertEqual(len(review(warning + loaded)["resolved_editor_warnings"]), 1)
        self.assertEqual(len(review(warning.replace("/Script/WorldPartitionHLODUtilities", "/Script/Titan") + loaded)["unresolved_load_errors"]), 1)
        self.assertEqual(len(review(warning.replace("Engine/Content/Maps/Templates/HLODs/HLODLayer_Merged", "Game/Environment/Missing") + loaded)["unresolved_load_errors"]), 1)

    def test_runtime_pass_requires_marker_without_failure(self):
        self.assertFalse(review("Exit 0")["runtime_pass"])
        self.assertTrue(review("SOUL_TITAN_PASS good")["runtime_pass"])
        self.assertFalse(review("SOUL_TITAN_PASS good\nSOUL_TITAN_FAIL bad")["runtime_pass"])
