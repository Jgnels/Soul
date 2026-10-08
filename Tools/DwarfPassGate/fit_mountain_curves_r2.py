"""Bounded spline optimization at three reviewed mountain-road defects; no terrain edits."""
import sys,json,time
from pathlib import Path
import numpy as np
R=Path.cwd();E=R/'Evidence/DwarfPassGate-20261007';B=R/'Evidence/MapFinalPolish-20261007';sys.path[:0]=[str(R/'Tools/MapPolish'),str(R/'Evidence/MapPolish-20261007/Local/python-libs')]
from route_surface import dense,assess,sample
from scipy.interpolate import CubicSpline
from scipy.optimize import minimize
from scipy.ndimage import gaussian_filter1d
D=json.loads((B/'Local/routes-presentation-final5.json').read_text());rows=[]
for ids,center,half in [(['dwarf_mountain_pass','north_pass'],[2058.8,1070.6],100),(['dwarf_mountain_pass','north_pass'],[2024.5,1111.8],55),(['dwarf_forge_approach','north_pass'],[2347.1,1139.2],70)]:
 r=next(r for r in D['routes'] if [r['a'],r['b']]==ids);s=r['segments'][0];p=dense(s['points_m'],.5);i=np.argmin(np.linalg.norm(p-center,axis=1));lo=max(8,i-round(half*2));hi=min(len(p)-9,i+round(half*2));old=p[lo:hi+1];arc=np.r_[0,np.cumsum(np.linalg.norm(np.diff(old,axis=0),axis=1))];L=arc[-1];t=np.linspace(0,L,19);base=np.c_[np.interp(t,arc,old[:,0]),np.interp(t,arc,old[:,1])];left=(p[lo+3]-p[lo-3]);left/=np.linalg.norm(left);right=(p[hi+3]-p[hi-3]);right/=np.linalg.norm(right);evalt=np.linspace(0,L,int(L*12)+1)
 def curve(v):
  k=base.copy();k[1:-1]=v.reshape(-1,2);return CubicSpline(t,k,axis=0,bc_type=((1,left),(1,right)))(evalt)
 def objective(v):
  q=curve(v);ds=np.linalg.norm(np.diff(q,axis=0),axis=1);v2=np.diff(q,n=2,axis=0);return 2500*np.sum(v2*v2)+.05*ds.sum()+.001*np.sum((v-base[1:-1].ravel())**2)
 def constraints(v):
  q=curve(v);z=sample(q);ds=np.linalg.norm(np.diff(q,axis=0),axis=1);return np.tan(np.radians(20.5))*ds-np.sqrt(np.diff(z)**2+1e-10)
 best=None;started=time.monotonic()
 for sigma in [1.0,0.0]:
  k=gaussian_filter1d(base,sigma,axis=0) if sigma else base.copy();k[0]=base[0];k[-1]=base[-1];v=k[1:-1].ravel();bounds=list(zip(base[1:-1].ravel()-30,base[1:-1].ravel()+30));res=minimize(objective,v,method='SLSQP',bounds=bounds,constraints=[{'type':'ineq','fun':constraints}],options={'maxiter':180,'ftol':.0001,'eps':.002});q=curve(res.x);m=assess(q,.1);trial=np.r_[p[:lo],q,p[hi+1:]];allm=assess(trial,.25);row=dict(route=ids,center_xy_m=center,success=bool(res.success),message=res.message,iterations=int(res.nit),local=m,route_check=allm,max_control_shift_m=float(np.linalg.norm(res.x.reshape(-1,2)-base[1:-1],axis=1).max()),elapsed_s=time.monotonic()-started)
  if m['max_grade_deg']<=22.1 and not m['invalid_samples'] and allm['max_grade_deg']<=22.1 and not allm['invalid_samples']:
   s['points_m']=trial.tolist();row['accepted']=True;rows.append(row);best=q;break
  row['accepted']=False;rows.append(row)
 if best is None:print('REJECTED',ids,center,flush=True)
 else:print('ACCEPTED',ids,center,m,flush=True)
 (E/'mountain-curves-r2.json').write_text(json.dumps(dict(terrain_changed=False,trials=rows),indent=2))
(E/'Local/routes-curves-r2.json').write_text(json.dumps(D));print('COMPLETE',flush=True)
