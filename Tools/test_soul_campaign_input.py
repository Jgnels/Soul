"""Static input-wiring regressions; UE hit testing/manual input remain required."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


def source(path):
    text = (ROOT / path).read_text(encoding="utf-8")
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)


class CampaignInputTests(unittest.TestCase):
    def setUp(self):
        self.controller = source("Source/Soul/Private/SoulFounderPlaytestPlayerController.cpp")
        self.region = source("Source/Soul/Private/SoulPlaytestRegionActor.cpp")

    def test_pressed_left_click_traces_and_routes_only_a_valid_region(self):
        self.assertRegex(self.controller, r"BindKey\(EKeys::LeftMouseButton,\s*IE_Pressed,\s*this,\s*&ASoulFounderPlaytestPlayerController::PrimaryClick\)")
        click = self.controller.split("::PrimaryClick()", 1)[1].split("::Number1()", 1)[0]
        self.assertRegex(click, r"if\s*\(!GetHitResultUnderCursor\(ECC_Visibility,\s*false,\s*Hit\)\)\s*return;")
        self.assertIn("Cast<ASoulPlaytestRegionActor>(Hit.GetActor())", click)
        self.assertRegex(click, r"if\s*\(!IsValid\(Region\)\s*\|\|\s*Region->IsHidden\(\)\s*\|\|\s*Region->RegionId.IsNone\(\)\)\s*return;")
        self.assertIn("if (auto* Campaign = GetCampaign())", click)
        self.assertEqual(click.count("Campaign->HandleRegionClicked(Region->RegionId)"), 1)
        self.assertNotIn("MovePlayerTo", click)
        self.assertNotIn("BeginBattle", click)

    def test_actor_notification_cannot_dispatch_campaign_a_second_time(self):
        header = source("Source/Soul/Public/SoulPlaytestRegionActor.h")
        self.assertNotIn("NotifyActorOnClicked", header + self.region)
        self.assertNotIn("HandleRegionClicked", self.region)
        self.assertEqual(self.controller.count("HandleRegionClicked("), 1)

    def test_cursor_ui_and_marker_collision_contract(self):
        for setting in ("bShowMouseCursor = true", "FInputModeGameAndUI Mode",
                        "Mode.SetHideCursorDuringCapture(false)",
                        "Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock)",
                        "SetInputMode(Mode)"):
            self.assertIn(setting, self.controller)
        self.assertIn('Marker->SetCollisionProfileName(TEXT("BlockAll"))', self.region)
        self.assertIn("SetActorEnableCollision(bExplored)", self.region)
        self.assertIn("Label->SetCollisionEnabled(ECollisionEnabled::NoCollision)", self.region)


if __name__ == "__main__":
    unittest.main()
