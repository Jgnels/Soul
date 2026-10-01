"""Physical presentation contracts for the approved terrain, not strategic rules."""
import hashlib
import json
from pathlib import Path
import unittest
import numpy as np

ROOT = Path(__file__).resolve().parents[1]


class MesaIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.profile = json.loads((ROOT/'Data/CampaignMesa/presentation.json').read_text())
        path = ROOT/'Data/CampaignMesaLocal/MesaHeight.r16'
        if not path.exists():
            raise unittest.SkipTest('Licensed local terrain not mounted; run Setup_Soul_Mesa.ps1')
        cls.raw = path.read_bytes()
        cls.z = (np.frombuffer(cls.raw,dtype='<u2').reshape(2041,2041).astype(float)-32768)*.5

    @classmethod
    def height(cls, points):
        uv = (np.asarray(points)+75000)/150000*2040
        uv = np.clip(uv,0,2039.999)
        ij = uv.astype(int)
        x,y = ij[:,0],ij[:,1]
        fx,fy = (uv-ij).T
        a,b,c,d = cls.z[y,x],cls.z[y,x+1],cls.z[y+1,x],cls.z[y+1,x+1]
        return np.where(fx>=fy,a+(b-a)*fx+(d-b)*fy,a+(d-c)*fx+(c-a)*fy)

    def test_exact_height_profile_pair(self):
        self.assertEqual(len(self.raw),2041*2041*2)
        self.assertEqual(hashlib.sha256(self.raw).hexdigest(),self.profile['height_sha256'])

    def test_exact_canonical_graph_and_route_endpoints(self):
        old = json.loads((ROOT/'Data/CampaignTerrainV2/presentation.json').read_text())
        edges = lambda p:{tuple(sorted((r['a'],r['b']))) for r in p['routes']}
        self.assertEqual(edges(old),edges(self.profile))
        self.assertEqual(set(old['regions']),set(self.profile['regions']))
        for r in self.profile['routes']:
            np.testing.assert_allclose(r['points'][0],self.profile['regions'][r['a']][:2])
            np.testing.assert_allclose(r['points'][-1],self.profile['regions'][r['b']][:2])

    def test_rendered_road_and_company_path_are_dry_and_traversable(self):
        report = []
        for r in self.profile['routes']:
            p = np.array(r['points'])
            # Twice the Mesa road mesh resolution, plus dense travel interpolation.
            alpha = np.linspace(0,1,2401)
            path = np.column_stack([np.interp(alpha,np.linspace(0,1,len(p)),p[:,axis]) for axis in (0,1)])
            z = self.height(path)
            distance = np.linalg.norm(np.diff(path,axis=0),axis=1)
            grades = np.degrees(np.arctan2(np.abs(np.diff(z)),distance))
            self.assertGreater(float(z.min()),180,r)
            self.assertLess(float(grades.max()),22.5,(r['a'],r['b']))
            tangents = np.roll(path,-1,axis=0)-np.roll(path,1,axis=0)
            tangents[0],tangents[-1] = path[1]-path[0],path[-1]-path[-2]
            side = np.column_stack([tangents[:,1],-tangents[:,0]])
            side /= np.linalg.norm(side,axis=1)[:,None]
            width = (22+4*np.sin(alpha*np.pi*7))*10
            for sign in (-1,1):
                self.assertGreater(float(self.height(path+side*width[:,None]*sign).min()),150)
            report.append(dict(a=r['a'],b=r['b'],length_m=float(distance.sum()/100),
                               min_ground_m=float(z.min()/100),max_grade_degrees=float(grades.max())))
        target = ROOT/'Evidence/CampaignIntegration-20260930/route-validation.json'
        target.parent.mkdir(exist_ok=True)
        target.write_text(json.dumps(report,indent=2)+'\n')

    def test_region_anchors_and_settlement_footprints_are_dry(self):
        for name,p in self.profile['regions'].items():
            self.assertLess(max(abs(p[0]),abs(p[1])),68000)
            self.assertAlmostEqual(float(self.height([p[:2]])[0]),p[2],places=2)
            # Covers the existing capital/tower/cottage footprint at region scale 5.
            radius = 3200 if name=='human_capital' else 2600 if name=='orc_camp' else 2200
            offset = np.linspace(-radius,radius,33)
            xx,yy = np.meshgrid(offset+p[0],offset+p[1])
            samples = np.column_stack([xx.ravel(),yy.ravel()])
            self.assertGreater(float(self.height(samples).min()),100,name)

    def test_road_triangles_do_not_disappear_under_landscape(self):
        worst = 0.
        for r in self.profile['routes']:
            p = np.array(r['points'])
            a = np.linspace(0,1,1201)
            def path(t):
                return np.column_stack([np.interp(t,np.linspace(0,1,len(p)),p[:,axis]) for axis in (0,1)])
            centers = path(a)
            tangent = path(np.minimum(a+.01,1))-path(np.maximum(a-.01,0))
            side = np.column_stack([tangent[:,1],-tangent[:,0]])
            side /= np.linalg.norm(side,axis=1)[:,None]
            width = (22+4*np.sin(a*np.pi*7))*10
            xy = (centers[:,None,:]+side[:,None,:]*width[:,None,None]*np.linspace(-1,1,9)[None,:,None]).reshape(-1,2)
            road_z = self.height(xy)+22
            start = (np.arange(1200)[:,None]*9+np.arange(8)).ravel()
            triangles = np.concatenate([np.column_stack([start,start+9,start+1]),np.column_stack([start+1,start+9,start+10])])
            for weights in ([1/3]*3,[.5,.5,0],[0,.5,.5],[.5,0,.5]):
                sample_xy = (xy[triangles]*np.array(weights)[None,:,None]).sum(axis=1)
                sample_z = (road_z[triangles]*weights).sum(axis=1)
                penetration = float(np.max(self.height(sample_xy)-sample_z))
                worst = max(worst,penetration)
                self.assertLess(penetration,1.,(r['a'],r['b']))
        print(f'Mesa road triangle worst sampled penetration: {worst:.3f} cm')


if __name__ == '__main__':
    unittest.main(verbosity=2)
