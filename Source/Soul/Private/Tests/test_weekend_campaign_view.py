"""Presentation source invariants only; Unreal tests and rendered evidence are required."""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[4]
def source(name): return (ROOT/'Source/Soul'/name).read_text(encoding='utf-8-sig')
class CampaignViewTests(unittest.TestCase):
    def test_locations_inside_continuous_surface(self):
        world=source('Private/SoulCampaignWorldActor.cpp')
        header=source('Public/SoulCampaignWorldActor.h')
        locations=re.findall(r'\{TEXT\("([a-z_]+)"\),\s*FVector\(([^)]+)\)\}',world)
        self.assertEqual(len(locations),9)
        width=float(re.search(r'HalfWidth\s*=\s*([\d.]+)',header)[1])
        depth=float(re.search(r'HalfDepth\s*=\s*([\d.]+)',header)[1])
        for name,vec in locations:
            x,y,z=map(float,vec.split(','))
            with self.subTest(region=name):
                self.assertLess(abs(x)+300,width)
                self.assertLess(abs(y)+300,depth)
        self.assertIn('CreateMeshSection_LinearColor',world)
    def test_perspective_camera_and_bounded_navigation(self):
        camera=source('Private/SoulCampaignCamera.cpp')
        header=source('Public/SoulCampaignCamera.h')
        self.assertIn('ECameraProjectionMode::Perspective',camera)
        self.assertNotIn('ECameraProjectionMode::Orthographic',camera)
        minimum=float(re.search(r'MinDistance\s*=\s*([\d.]+)',header)[1])
        maximum=float(re.search(r'MaxDistance\s*=\s*([\d.]+)',header)[1])
        self.assertGreater(minimum,500)
        self.assertGreater(maximum,minimum)
        self.assertIn('GetMinimumDistance(), GetMaximumDistance()',camera)
        for key in ['W','A','S','D','MiddleMouseButton']:
            self.assertIn('EKeys::'+key,camera)
        control=source('Private/SoulFounderPlaytestPlayerController.cpp')
        self.assertIn('EKeys::MouseScrollUp',control)
        self.assertIn('EKeys::MouseScrollDown',control)
        self.assertIn('HeightAt(Position.X*Scale,Position.Y*Scale)/Scale',camera)
    def test_roads_come_from_authoritative_neighbors(self):
        world=source('Private/SoulCampaignWorldActor.cpp')
        campaign=source('Private/SoulFounderPlaytestCampaignActor.cpp')
        self.assertIn('State->World.Regions',world)
        self.assertIn('Region.Value.Neighbors',world)
        self.assertNotIn('DrawDebugLine',world+campaign)
        self.assertIn('HeightAt(Edge.X,Edge.Y)',world)
        self.assertIn('FSoulWorldRules::IsExplored',world)
    def test_location_collision_is_hidden_and_not_a_sphere(self):
        region=source('Private/SoulPlaytestRegionActor.cpp')
        self.assertNotIn('BasicShapes/Sphere',region)
        self.assertIn('Marker->SetHiddenInGame(true)',region)
        self.assertIn('SetActorEnableCollision(bExplored)',region)
        self.assertIn('SetCollisionProfileName(TEXT("BlockAll"))',region)
        self.assertIn('Label->SetCollisionEnabled(ECollisionEnabled::NoCollision)',region)
    def test_party_motion_does_not_write_campaign_state(self):
        world=source('Private/SoulCampaignWorldActor.cpp')
        for forbidden in ['MovePlayerTo(', 'BeginBattle(', 'State->PlayerRegion=', 'State->World=']:
            self.assertNotIn(forbidden,world)
        self.assertIn('RoadPoint(TravelFrom,TravelTo',world)

    def test_campaign_travel_is_discoverable(self):
        hud=source('Private/SoulFounderPlaytestHUD.cpp')
        campaign=source('Private/SoulFounderPlaytestCampaignActor.cpp')
        self.assertIn('MOVEMENT %d / %d',hud)
        self.assertIn('SELECT YOUR ARMY',hud)
        self.assertIn('highlighted neighbouring place',hud)
        self.assertIn('bLegalDestination',campaign)
        self.assertIn('movement remaining',campaign)

    def test_normal_launcher_keeps_player_control(self):
        launcher=(ROOT/'PLAY_SOUL_VERTICAL_SLICE.cmd').read_text(encoding='utf-8-sig')
        battle=(ROOT/'Source/SoulRealtimeBattle/Private/SoulRealtimeBattleArena.cpp').read_text(encoding='utf-8-sig')
        self.assertNotIn('SoulRealtimeMagicProof',launcher)
        self.assertNotIn('SoulAutobattle',launcher)
        self.assertIn('DEPLOYMENT PAUSED',battle)
        self.assertIn('EKeys::F1',battle)
        self.assertIn('ToggleBattleCamera',battle)
        self.assertIn('EKeys::X',battle)
        self.assertIn('ToggleFirstPersonCamera',battle)
        arm=re.search(r'TargetArmLength = bFirstPersonCamera \? 0\.0f : ([0-9.]+)f',battle)
        self.assertIsNotNone(arm)
        self.assertGreater(float(arm.group(1)),200.0)
        self.assertIn('SetOwnerNoSee(bFirstPersonCamera)',battle)
        self.assertIn('/SK_Elephant.SK_Elephant',battle)
        self.assertNotIn('/SK_ElephantTusksBig.SK_ElephantTusksBig',battle)

    def test_opt_in_benchmark_retains_warmup_hold_and_measured_duration(self):
        mode=source('Private/SoulFounderPlaytestGameMode.cpp')
        header=source('Public/SoulFounderPlaytestGameMode.h')
        self.assertIn('int32 BenchmarkSampleSeconds=60',header)
        self.assertIn('TEXT("SoulTerrainBenchmarkSeconds=")',mode)
        self.assertIn('FMath::Clamp(BenchmarkSampleSeconds,20,60)',mode)
        self.assertIn('const double SampleEnd=20.+BenchmarkSampleSeconds',mode)
        self.assertIn('if(Age>=20&&Age<SampleEnd)Samples.Add(Frame)',mode)
        self.assertIn('if(Age>SampleEnd+3)',mode)
        self.assertIn('N,Sum/1000.,BenchmarkSampleSeconds',mode)
        self.assertIn('requested_sample_seconds',mode)
        self.assertIn('for(double V:Samples){Sum+=V;',mode)
        self.assertIn('TEXT("SoulTerrainBenchmark")',mode)

if __name__=='__main__':unittest.main(verbosity=2)
