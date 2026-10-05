"""Acceptance contracts for the full-world terrain without launching Unreal."""
import hashlib
import json
from pathlib import Path
import unittest
import numpy as np
import bake_world as bake

ROOT=Path(__file__).resolve().parents[2]
LOCAL=Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/WorldTerrain')


class WorldTerrainContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.p=json.loads((ROOT/'Data/CampaignWorldTerrain/presentation.json').read_text())
        cls.world=json.loads((ROOT/'Data/soul_world_overmap_v1_20260922.json').read_text())
        cls.raw=(LOCAL/'WorldHeight.r16').read_bytes()
        cls.h=(np.frombuffer(cls.raw,dtype='<u2').astype(float).reshape(2033,2033)-32768)*.04
        cls.q=json.loads((LOCAL/'qualification.json').read_text())

    def test_exact_graph_and_unchanged_authority(self):
        self.assertEqual(len(self.p['regions']),36)
        self.assertEqual(len(self.p['routes']),51)
        expected={frozenset((e['a'],e['b'])) for e in self.world['edges']}
        actual={frozenset((r['a'],r['b'])) for r in self.p['routes']}
        self.assertEqual(actual,expected)
        self.assertEqual(self.p['source_sha256'],hashlib.sha256((ROOT/self.p['source']).read_bytes()).hexdigest())
        self.assertEqual(self.p['playable_region_ids'],self.world['founder_slice']['region_ids'])
        self.assertEqual(len(self.p['playable_region_ids']),9)
        self.assertTrue(self.p['presentation_only'])

    def test_exact_existing_world_transform_and_route_endpoints(self):
        for n in self.world['nodes']:
            np.testing.assert_allclose(self.p['regions'][n['id']][:2],[(n['x']-500)*1000,(475-n['y'])*1000],atol=.001)
        for route in self.p['routes']:
            p=np.array(route['points'])
            np.testing.assert_allclose(p[0],self.p['regions'][route['a']][:2],atol=.001)
            np.testing.assert_allclose(p[-1],self.p['regions'][route['b']][:2],atol=.001)
            self.assertLessEqual(np.linalg.norm(np.diff(p,axis=0),axis=1).max(),1000.01)
            self.assertLess(np.abs(p).max(),508000)

    def test_height_import_dimensions_encoding_and_margins(self):
        self.assertEqual(self.p['resolution'],8*2*127+1)
        self.assertEqual(self.p['extent_xy_cm'],[1016000,1016000])
        self.assertEqual(self.p['minimum_xy_cm'],[-508000,-508000])
        self.assertEqual(len(self.raw),2033*2033*2)
        self.assertEqual(hashlib.sha256(self.raw).hexdigest(),self.p['height_sha256'])
        self.assertGreater(self.h.max(),600)
        self.assertLess(self.h.max(),1310.6)
        self.assertLess(max(self.h[0].max(),self.h[-1].max(),self.h[:,0].max(),self.h[:,-1].max()),0)

    def test_actual_route_grades_and_crossing_clearances(self):
        self.assertEqual(len(self.q['routes']),51)
        self.assertEqual(self.q['status'],'pass')
        for route in self.q['routes']:
            self.assertLessEqual(route['max_grade_degrees'],22.,f"{route['a']} -> {route['b']}")
            clearance=route['min_water_clearance_m']
            self.assertTrue(clearance is None or clearance>=.5,f"Flooded route {route}")
        edge_set={frozenset((e['a'],e['b'])) for e in self.world['edges']}
        for bridge in self.p['bridges']:
            self.assertIn(frozenset((bridge['a'],bridge['b'])),edge_set)
            self.assertGreater(bridge['half_span'],0)

    def test_level_settlement_footprints(self):
        for pad in self.q['footprints']:
            self.assertLessEqual(pad['height_range_m'],.08,pad['id'])
        capital=next(p for p in self.q['footprints'] if p['id']=='human_capital')
        self.assertGreaterEqual(capital['radius_m'],140)

    def test_named_crossings_have_one_shared_deck_and_bank_approach(self):
        for name in ['river_ford','southern_crossing','orc_broken_bridge']:
            decks=[b for b in self.p['bridges'] if b.get('crossing')==name]
            self.assertEqual(len(decks),1,name)
            bridge=decks[0]
            center=np.asarray(bridge['center'][:2])/100
            yaw=np.radians(bridge['yaw'])
            normal=np.array([-np.sin(yaw),np.cos(yaw)])
            for route in self.p['routes']:
                if name not in (route['a'],route['b']):continue
                points=np.asarray(route['points'])/100
                local=points[np.linalg.norm(points-center,axis=1)<100]
                self.assertGreater(len(local),5)
                self.assertLess(np.max(np.abs((local-center)@normal)),.01,f"{route['a']}->{route['b']}")

    def test_productive_hinterland_is_substantial_and_dry(self):
        self.assertGreaterEqual(len(self.p['fields']),40)
        self.assertLessEqual(len(self.p['fields']),60)
        for field,evidence in zip(self.p['fields'],self.p['field_qualification']):
            self.assertTrue(5000<=field[2]<=9000)
            self.assertTrue(2000<=field[3]<=4500)
            self.assertLessEqual(evidence['max_grade_degrees'],7.5)
            self.assertGreaterEqual(evidence['minimum_elevation_m'],15)

    def test_connected_downhill_drainage(self):
        self.assertEqual(len(self.p['waterlines']),len(self.p['river_widths']))
        heart,east=[np.array(p) for p in self.p['waterlines']]
        for river in [heart,east]:
            self.assertTrue(np.all(np.diff(river[:,2])<=.001))
        self.assertAlmostEqual(heart[-1,2],0.)
        endpoint=east[-1]
        errors=[]
        for a,b in zip(heart[:-1],heart[1:]):
            d=b-a;t=np.clip(np.dot(endpoint-a,d)/np.dot(d,d),0,1)
            errors.append(np.linalg.norm(endpoint-(a+t*d)))
        self.assertLess(min(errors),1000.)

    def test_lake_has_a_closed_basin_and_supported_radial_mesh(self):
        lake=self.p['lake'];center=np.asarray(lake['center_cm'])/100
        polygon=np.asarray(lake['shoreline_cm'])/100
        self.assertEqual(lake['shoreline_winding'],'ccw')
        self.assertGreaterEqual(len(polygon),64)
        vectors=polygon-center[:2]
        following=np.roll(vectors,-1,axis=0)
        cross=vectors[:,0]*following[:,1]-vectors[:,1]*following[:,0]
        self.assertTrue(np.all(cross>0),'Center fan must be ordered and star-shaped')
        perimeter=bake.resample(np.vstack([polygon,polygon[0]]),2.5)
        clearance=bake.sample(self.h,perimeter)-center[2]
        self.assertGreaterEqual(clearance.min(),.5,'Water mesh edge must end beneath terrain')
        components,_=bake.label(self.h<center[2])
        index=np.rint((center[:2][::-1]-bake.MINIMUM)/bake.STEP).astype(int)
        component=components[tuple(index)]
        self.assertGreater(component,0)
        self.assertNotIn(component,np.r_[components[0],components[-1],components[:,0],components[:,-1]])
        self.assertGreater(center[2]-bake.sample(self.h,center[:2]),8)

    def test_context_settlements_do_not_create_gameplay_regions(self):
        founder=set(self.p['playable_region_ids'])
        for s in self.p['context_settlements']:
            self.assertNotIn(s['region'],founder)
            self.assertIn(s['region'],self.p['regions'])
            self.assertIn(s['style'],{'human','orc','viking','dwarf','nature','dark'})


if __name__=='__main__':unittest.main(verbosity=2)
