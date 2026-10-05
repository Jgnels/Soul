"""Bake the existing 36/51 Soul geography into one owned landscape presentation.

Canonical nodes/edges are read, never changed. Licensed cropped height vocabulary
and all generated raster/height assets stay in the external preview directory.
Python requirements: numpy, Pillow, scipy (task-local dependencies supported).
"""
from __future__ import annotations

import argparse
import hashlib
import heapq
import json
import math
from pathlib import Path
import sys

sys.path.insert(0, 'D:/RefinedBadger/AssetLibraries/SoulTerrainPreview/WorldTerrain/dependencies')
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
PREVIEW = Path('D:/RefinedBadger/AssetLibraries/SoulTerrainPreview')
sys.path.insert(0, str(PREVIEW / 'WorldTerrain/dependencies'))
from scipy.ndimage import gaussian_filter, distance_transform_edt, map_coordinates, binary_dilation, label
from scipy.interpolate import CubicSpline

N = 2033
STEP = 5.0  # metres; Landscape 8x8 components, 127 quads x 2 subsections.
MINIMUM = -5080.0
EXTENT = STEP * (N - 1)
HEIGHT_UNIT_CM = 4.0
MAX_ROUTE_DEGREES = 22.0


def smooth(a, b, value):
    t = np.clip((value - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def soft_minimum(a,b,rounding):
    return np.minimum(a,b)-rounding*np.exp(-np.abs(a-b)/rounding)*.5


def world_xy(point):
    return np.array([(point[0] - 500) * 10., (475 - point[1]) * 10.])


def sample(h, xy):
    """Unreal's actual Landscape triangle diagonal, in metres."""
    p = np.asarray(xy)
    u = np.clip((p[..., 0] - MINIMUM) / STEP, 0, N - 1.00001)
    v = np.clip((p[..., 1] - MINIMUM) / STEP, 0, N - 1.00001)
    ix, iy = u.astype(int), v.astype(int)
    fx, fy = u - ix, v - iy
    a, b, c, d = h[iy, ix], h[iy, ix + 1], h[iy + 1, ix], h[iy + 1, ix + 1]
    return np.where(fx >= fy, a + (b-a)*fx + (d-b)*fy, a + (d-c)*fx + (c-a)*fy)


def resample(points, spacing=10.):
    p = np.asarray(points, dtype=float)
    lengths = np.r_[0, np.cumsum(np.linalg.norm(np.diff(p[:, :2], axis=0), axis=1))]
    keep = np.r_[True, np.diff(lengths) > 1e-7]
    p, lengths = p[keep], lengths[keep]
    t = np.linspace(0, lengths[-1], max(2, math.ceil(lengths[-1]/spacing)+1))
    return np.stack([np.interp(t, lengths, p[:, i]) for i in range(p.shape[1])], axis=1)


def polyline_distance(x, y, points):
    nearest = np.full(x.shape, np.inf, dtype=np.float32)
    surface = np.zeros(x.shape, dtype=np.float32)
    for a, b in zip(points[:-1], points[1:]):
        dx, dy = b[0]-a[0], b[1]-a[1]
        t = np.clip(((x-a[0])*dx + (y-a[1])*dy)/(dx*dx+dy*dy), 0, 1)
        distance = np.hypot(x-a[0]-t*dx, y-a[1]-t*dy)
        select = distance < nearest
        if len(a) > 2:
            surface[select] = (a[2]+t*(b[2]-a[2]))[select]
        nearest = np.minimum(nearest, distance)
    return nearest, surface


def stamp(raw, crop, bounds, x, y):
    """One source crop stretched only within a regional feather, never a tile."""
    a = raw[crop[1]:crop[3], crop[0]:crop[2]].astype(np.float32)
    lo, hi = np.percentile(a, [2, 98])
    a = np.clip((a-lo)/max(hi-lo, 1), 0, 1)
    u = np.clip((x-bounds[0])/(bounds[2]-bounds[0]), 0, 1)*(a.shape[1]-1)
    v = np.clip((y-bounds[1])/(bounds[3]-bounds[1]), 0, 1)*(a.shape[0]-1)
    result=map_coordinates(a, [v, u], order=1, mode='nearest')
    edge=np.minimum.reduce([x-bounds[0],bounds[2]-x,y-bounds[1],bounds[3]-y])
    fade=smooth(0,250,edge)
    return gaussian_filter(.5+(result-.5)*fade,2)


def flowing_curve(controls, spacing=10):
    points=np.asarray(controls,float)
    t=np.r_[0,np.cumsum(np.linalg.norm(np.diff(points[:,:2],axis=0),axis=1))]
    steps=np.unique(np.r_[t,np.linspace(0,t[-1],int(t[-1]/spacing)+1)])
    xy=CubicSpline(t,points[:,:2],bc_type='natural')(steps)
    return np.column_stack([xy,np.interp(steps,t,points[:,2])])


def raster_distance(points):
    p=resample(points,2.5)
    ix=np.clip(np.rint((p[:,0]-MINIMUM)/STEP).astype(int),0,N-1)
    iy=np.clip(np.rint((p[:,1]-MINIMUM)/STEP).astype(int),0,N-1)
    occupied=np.zeros((N,N),bool);height=np.zeros((N,N),np.float32)
    occupied[iy,ix]=True;height[iy,ix]=p[:,2]
    distance,indices=distance_transform_edt(~occupied,sampling=STEP,return_indices=True)
    return distance.astype(np.float32),height[indices[0],indices[1]]


def pathfind(h, wet, start, end, penalty=None):
    """Terrain-cost A*: dry low grades preferred; every route retains its IDs."""
    stride = 4
    hh = h[::stride, ::stride]
    ww = wet[::stride, ::stride]
    pp=penalty[::stride,::stride] if penalty is not None else np.zeros_like(hh)
    size = hh.shape[0]
    a, b = [tuple(np.rint((q[::-1]-MINIMUM)/(STEP*stride)).astype(int)) for q in (start, end)]
    queue = [(0., a)]
    scores, previous = {a: 0.}, {}
    direction = np.array(b)-np.array(a)
    squared = max(float(direction @ direction), 1.)
    while queue:
        _, current = heapq.heappop(queue)
        if current == b:
            out = [b]
            while out[-1] != a:
                out.append(previous[out[-1]])
            xy = np.array([(MINIMUM+c[1]*STEP*stride, MINIMUM+c[0]*STEP*stride) for c in out[::-1]])
            xy[0], xy[-1] = start, end
            # Round the stair steps within the traversed corridor, endpoints pinned.
            for _ in range(2):
                xy[1:-1] = (xy[:-2]+xy[1:-1]*2+xy[2:])/4
            rounded=gaussian_filter(xy, [2.5,0])
            rounded[0],rounded[-1]=start,end
            return resample(rounded)
        for oy, ox in ((0,1),(1,0),(0,-1),(-1,0),(1,1),(1,-1),(-1,1),(-1,-1)):
            v = current[0]+oy, current[1]+ox
            if not (1 <= v[0] < size-1 and 1 <= v[1] < size-1) or ww[v]:
                continue
            if ox and oy and (ww[current[0],v[1]] or ww[v[0],current[1]]):
                continue
            length = math.hypot(ox, oy)*STEP*stride
            grade = abs(float(hh[v]-hh[current]))/length
            dy,dx=v[0]-a[0],v[1]-a[1]
            t=min(1.,max(0.,(dy*direction[0]+dx*direction[1])/squared))
            deviation=math.hypot(dy-t*direction[0],dx-t*direction[1])*STEP*stride
            if deviation > 1800:
                continue
            cost = length*(1 + 40*grade*grade + (10000 if grade>.325 else 0) + float(pp[v]) + .00007*deviation)
            candidate = scores[current]+cost
            if candidate < scores.get(v, math.inf):
                scores[v], previous[v] = candidate, current
                heuristic = math.hypot(v[0]-b[0], v[1]-b[1])*STEP*stride
                heapq.heappush(queue, (candidate+heuristic, v))
    raise RuntimeError(f'No terrain route from {start} to {end}')


def route_grade(h, routes, site_xy):
    """Shared grade field avoids stacked road embankments at route junctions."""
    total = np.zeros_like(h)
    count = np.zeros_like(h)
    for points in routes:
        p = resample(points, STEP/2)
        z = sample(h, p)
        z = gaussian_filter(z, 5)
        ix = np.rint((p[:,0]-MINIMUM)/STEP).astype(int)
        iy = np.rint((p[:,1]-MINIMUM)/STEP).astype(int)
        np.add.at(total, (iy,ix), z)
        np.add.at(count, (iy,ix), 1)
    occupied = count > 0
    target = total/np.maximum(count, 1)
    distance, nearest = distance_transform_edt(~occupied, sampling=STEP, return_indices=True)
    grade = gaussian_filter(target[nearest[0],nearest[1]],2)
    # Balanced local projection permits both a small cut and a small fill.
    # A downhill-only relaxation can excavate entire connected road networks.
    inside=distance<=15
    iy,ix=np.nonzero(inside)
    ids=np.full(h.shape,-1,np.int32);ids[iy,ix]=np.arange(len(iy))
    ea=[];eb=[];limits=[]
    for oy,ox in [(0,1),(1,0),(1,1),(1,-1)]:
        valid=(iy+oy<N)&(ix+ox>=0)&(ix+ox<N)
        aa=np.nonzero(valid)[0];bb=ids[iy[valid]+oy,ix[valid]+ox]
        valid=bb>=0;ea.extend(aa[valid]);eb.extend(bb[valid]);limits.extend([.24*STEP*math.hypot(oy,ox)]*int(valid.sum()))
    ea=np.asarray(ea);eb=np.asarray(eb);limits=np.asarray(limits)
    z=grade[iy,ix].astype(float)
    for _ in range(1200):
        delta=z[eb]-z[ea]
        excess=np.sign(delta)*np.maximum(np.abs(delta)-limits,0)*.48
        correction=np.bincount(ea,weights=excess,minlength=len(z))-np.bincount(eb,weights=excess,minlength=len(z))
        degree=np.bincount(ea,weights=excess!=0,minlength=len(z))+np.bincount(eb,weights=excess!=0,minlength=len(z))
        z+=correction/np.maximum(degree,1)
        if np.max(np.abs(excess))<.001:break
    grade[iy,ix]=z
    _,near=distance_transform_edt(~inside,return_indices=True)
    grade=grade[near[0],near[1]]
    weight = 1-smooth(15,np.where(h>240,85,55),distance)
    cut_limit=np.full_like(h,12.)
    yy,xx=np.mgrid[:N,:N];xx=xx*STEP+MINIMUM;yy=yy*STEP+MINIMUM
    for name in ['north_pass','dwarf_mountain_pass','viking_snow_pass','mountain_shrine']:
        center=site_xy[name];cut_limit+=22*(1-smooth(250,700,np.hypot(xx-center[0],yy-center[1])))
    changed=h+np.clip(grade-h,-cut_limit,cut_limit)*weight
    print('Road cut/fill metres',float((changed-h).min()),float((changed-h).max()),flush=True)
    return changed,distance


def lake_basin(h,x,y):
    """A closed glacial basin below the measured 74.8 m enclosing terrain."""
    center=np.array([650.,3550.]);radius=np.array([240.,380.]);water=68.
    theta=np.arctan2((y-center[1])/radius[1],(x-center[0])/radius[0])
    radial=np.hypot((x-center[0])/radius[0],(y-center[1])/radius[1])/(1+.16*np.sin(theta*3)+.1*np.cos(theta*5))
    bed=water-12+13.5*smooth(.55,1,radial)+30*np.maximum(radial-1,0)
    blend=1-smooth(1,1.65,radial)
    result=h*(1-blend)+bed*blend
    angle=np.arange(96)*2*np.pi/96
    boundary=1+.16*np.sin(angle*3)+.1*np.cos(angle*5)
    polygon=center+np.stack([np.cos(angle)*radius[0],np.sin(angle)*radius[1]],axis=1)*boundary[:,None]
    lake=dict(center=[*center,water],radius=radius.tolist(),center_cm=[*center*100,water*100],radius_cm=(radius*100).tolist(),shoreline_cm=np.round(polygon*100,3).tolist(),shoreline_winding='ccw',shoreline_role='water mesh perimeter embedded in closed bank',depth_cm=1200)
    return result,lake,radial


def terrain(preview, nodes, features):
    axis = np.linspace(MINIMUM, -MINIMUM, N, dtype=np.float32)
    x, y = np.meshgrid(axis, axis)
    source = preview/'TerrainWork'
    mountain = np.load(source/'mountain05-height.npy')
    def rg(file):
        a = np.asarray(Image.open(source/file)).astype(np.uint16)
        return a[:,:,0]*256+a[:,:,1]
    grass = rg('grassland02-2041-rg.png')
    mesa = rg('mesa01-2041-rg.png')
    g = stamp(grass, (160,220,1860,1870), (-4300,-4400,4600,4400), x,y)
    m = stamp(mountain, (210,170,1790,1790), (-1900,1700,4650,4650), x,y)
    q = stamp(mesa, (480,400,1220,1160), (700,-2000,4750,1700), x,y)
    # The plain has long agricultural swells, with tributary hollows rather
    # than an almost constant-height tabletop beneath a material texture.
    h = 48+g*105
    heart=(1-smooth(-400,500,x))*(1-smooth(1000,2100,y))*smooth(-3100,-1300,y)
    h+=heart*(24*np.sin(x/470+y/680)+19*np.cos(y/330-x/1050)+14*np.sin(x/230+y/390))
    # Continuous Crownspine and its eastern foothills, not separate biome islands.
    spine = [world_xy(p) for p in features['mountain_belts'][0]['points']]
    d, _ = polyline_distance(x,y,spine)
    crown = np.exp(-(d/650)**1.7)
    h += crown*(240+590*m)
    # Subsidiary ribs turn off the main belt toward its two watersheds. This
    # directional intermediate relief is visible at the whole-world camera.
    ribs=(np.sin((x+.37*y)/155+1.7*np.sin(y/480))*.5+.5)**1.6
    rib_mask=smooth(90,290,d)*(1-smooth(680,1150,d))*smooth(1700,2300,y)
    h+=rib_mask*(155*ribs-45)
    d, _ = polyline_distance(x,y,[world_xy(p) for p in [(605,220),(670,300),(650,370)]])
    h += np.exp(-(d/450)**2)*(180+150*m)
    # Localized northern fjord uplands; open lower March remains the trade corridor.
    north = smooth(1700,2800,y)*(1-smooth(-1350,-600,x))
    north_stamp=stamp(mountain,(250,650,1250,1850),(-4550,1500,-750,4750),x,y)
    h += north*(25+310*north_stamp)
    # Cropped mesa relief fades across foothills into the central Heartland.
    bad = smooth(650,1400,x)*(1-smooth(1400,2200,y))*smooth(-2350,-1500,y)
    h += bad*(25+125*q)
    green = (1-smooth(-300,500,x))*(1-smooth(-850,-100,y))
    h += green*(18+24*g)
    ash = smooth(-100,850,x)*(1-smooth(-1500,-650,y))
    h += ash*(28+75*q)
    # Volcanic shoulders frame the southern broad ash plain, not a horizontal wall.
    for cx,cy,rx,ry,rise in [(1100,-3350,620,1000,180),(3900,-2900,600,900,220),(4250,-4150,850,400,160)]:
        h += rise*np.exp(-((x-cx)/rx)**2-((y-cy)/ry)**2)
    ash_ribs=ash*smooth(2000,3500,np.abs(y))*(.5+.5*np.sin(x/140+.7*np.sin(y/310)))**2
    h+=ash_ribs*70
    # West coastline and two inlet mouths: harbour and southern coastal ruins.
    coast=np.array([(-4200,4500),(-3500,4630),(-2700,4510),(-1800,4300),(-600,4630),
        (650,4510),(1850,4730),(3250,4490),(4100,4300),(4510,3550),(4320,2600),
        (4480,1750),(4170,1320),(4640,520),(4520,-400),(4250,-1550),(4630,-2700),
        (4510,-4000),(3800,-4450),(2820,-4210),(1830,-4100),(920,-4580),(-250,-4430),
        (-1350,-4200),(-2400,-4510),(-3540,-4070),(-4320,-3550),(-4170,-2400),
        (-4460,-1480),(-4550,-750),(-4110,100),(-4410,1200),(-4090,1820),(-4390,2350),
        (-4220,2530),(-3820,2510),(-3440,2660),(-3300,2840),(-3570,3060),(-3980,3120),
        (-4210,3260),(-4200,3430),(-3910,3510),(-3590,3470),(-3210,3640),(-3020,3880),
        (-3150,4090),(-3510,4140),(-3900,4070),(-4280,4190)])
    closed=np.vstack([coast,coast[0]])
    t=np.r_[0,np.cumsum(np.linalg.norm(np.diff(closed,axis=0),axis=1))]
    coast_curve=CubicSpline(t,closed,bc_type='periodic')(np.linspace(0,t[-1],1400))
    land_image=Image.new('L',(N,N));ImageDraw.Draw(land_image).polygon([((p[0]-MINIMUM)/STEP,(p[1]-MINIMUM)/STEP) for p in coast_curve],fill=255)
    land_mask=np.asarray(land_image)>0
    shore=(distance_transform_edt(land_mask)-distance_transform_edt(~land_mask))*STEP
    shore=gaussian_filter(shore,3)
    # Independent coastal catchments, not one common bevel multiplying every
    # landform. Gentle west/south estuaries grade over 0.6--1.4 km; selected
    # northern/eastern headlands retain hard rock and unequal bank heights.
    headlands=smooth(1900,3200,y)*(.35+.65*(.5+.5*np.sin(x/560+y/930)))
    headlands=np.maximum(headlands,smooth(3200,4150,x)*(.5+.5*np.cos(y/640))*.7)
    positive=np.maximum(shore,0)
    coastal_ceiling=(.025+.33*headlands)*positive+(.000065+.00011*headlands)*positive**2
    coastal_ceiling*=.75+.5*g
    coastal_ceiling+=smooth(50,500,positive)*headlands*18*np.sin(y/120+x/180)
    h=soft_minimum(h,coastal_ceiling,22)*smooth(0,70,positive)
    ocean=-24*(1-np.exp(np.minimum(shore,0)/85))
    h=np.where(shore<0,ocean,h)
    outer=smooth(0,500,5080-np.abs(x))*smooth(0,500,5080-np.abs(y))
    h=h*outer-24*(1-outer)
    # A northern glacial lake is context only, never a new region or naval rule.
    h,lake,radial=lake_basin(h,x,y)
    sea_lake = (shore<20)|(radial<1.10)|(outer<.35)
    # Repair structural river sketches at crossing IDs; physical curves own no rules.
    river_specs = [
        ('heart_river', [(505,235,245),(482,300,205),(470,360,168),(435,425,145),(455,500,118),
                         (420,560,94),(450,615,83),(495,660,74),(520,700,65),
                         (480,740,45),(400,795,28),(350,840,18),(330,865,16),(210,880,9),(45,860,0)], 20.),
        ('eastern_run',[(640,265,250),(682,328,210),(680,400,174),(705,455,157),(666,505,139),(690,560,125),
                        (720,660,100),(660,710,80),(570,730,58),(480,740,45)],16.),
    ]
    rivers=[]
    for name, controls, halfwidth in river_specs:
        points=flowing_curve([list(world_xy(p[:2]))+[p[2]] for p in controls],25)
        points[:,2]=np.minimum(points[:,2],gaussian_filter(sample(h,points[:,:2]),6)-8)
        points[:,2]=np.maximum(0,np.minimum.accumulate(points[:,2]))
        if rivers:
            parent=rivers[0]['points'];nearest=np.argmin(np.linalg.norm(parent[:,:2]-points[-1,:2],axis=1))
            points[-1,2]=parent[nearest,2]
            points[:,2]=np.maximum.accumulate(points[::-1,2])[::-1]
        d, surface=raster_distance(points)
        broad_surface=gaussian_filter(surface,14)
        # Broad catchment grading: natural valley grows into the local hills;
        # avoid a constant-width bevel trench or suspended water above low ground.
        dd=np.maximum(d-halfwidth,0)
        floodplain=70+110*(.5+.5*np.sin(x/410+y/690))**2+70*(.5+.5*np.cos(y/280-x/850))
        # Unequal, vegetated valley shoulders with 2--4x varying floodplain
        # width. A broad low-grade valley ceiling avoids two continuous cliffs.
        upper=np.maximum(dd-floodplain,0)
        valley=broad_surface+2+.045*dd+.0005*upper**2
        valley+=smooth(halfwidth+30,halfwidth+200,d)*((g-.5)*32+7*np.sin(x/130+y/230))
        feather=1-smooth(floodplain+240,floodplain+760,d)
        h=h+(soft_minimum(h,valley,16)-h)*feather
        bank=1-smooth(halfwidth+5,halfwidth+35,d)
        h=h*(1-bank)+np.maximum(h,surface+1.6)*bank
        bed=1-smooth(halfwidth*.78,halfwidth+4,d)
        h=h*(1-bed)+(surface-4)*bed
        rivers.append(dict(id=name,points=points,halfwidth=halfwidth,distance=d,surface=surface,bed=bed))
    h=np.where(shore<0,np.minimum(h,-.2),h)
    h=gaussian_filter(h,.65)
    # Terrace existing node centers. Crossing centers belong on shared bridge decks.
    crossing={'river_ford','southern_crossing','orc_broken_bridge'}
    sites=[]
    for node in nodes:
        center=world_xy((node['x'],node['y']))
        radius=140. if node['id']=='human_capital' else (100. if node['id'] in {'dwarf_hold','dark_fortress'} else (90. if node['kind']=='capital' else (45. if node['feature']=='quarry' else 30.)))
        z=float(sample(h,center))
        if node['id'] in crossing:
            continue
        z=max(z,14.)
        # Place mountain anchors on authored basin/pass terraces, rather than
        # later excavating a 100-m trench to a point arbitrarily atop a donor crag.
        if node['macro_region']=='crownspine':z=min(z,220 if node['id']=='dwarf_snow_basin' else 300)
        if node['id'] in {'viking_snow_pass','north_pass','ancient_shrine'}:z=min(z,280)
        for river in rivers:
            dist,level=polyline_distance(np.array([center[0]]),np.array([center[1]]),river['points'])
            if dist[0]<220:z=min(z,float(level[0]+max(dist[0]-river['halfwidth']-radius,5)*.2+3))
        distance=np.hypot(x-center[0],y-center[1])
        w=1-smooth(radius,radius+(550 if node['macro_region']=='crownspine' else 300),distance)
        for river in rivers:w*=smooth(river['halfwidth']+8,river['halfwidth']+35,river['distance'])
        w=np.maximum(w,1-smooth(radius,radius+2,distance))
        h=h*(1-w)+z*w
        sites.append(dict(id=node['id'],radius_m=radius,target_m=z))
    return h.astype(np.float32),rivers,sea_lake,sites,dict(grass=g,mountain=m,mesa=q,badlands=bad,greenwood=green,ash=ash),lake


def crossing_axes(positions,rivers):
    result={}
    for name in ['river_ford','southern_crossing','orc_broken_bridge']:
        center=positions[name]
        river=min(rivers,key=lambda r:np.min(np.linalg.norm(r['points'][:,:2]-center,axis=1)))
        p=river['points'];i=int(np.argmin(np.linalg.norm(p[:,:2]-center,axis=1)))
        tangent=p[min(i+2,len(p)-1),:2]-p[max(i-2,0),:2]
        unit=np.array([-tangent[1],tangent[0]])/np.linalg.norm(tangent)
        result[name]=dict(center=center,unit=unit,deck=float(p[i,2])+5,river=river['id'],halfspan=river['halfwidth']+18)
    return result


def bridges_for(routes, rivers, h, crossings):
    bridges=[]
    for name,c in crossings.items():
        route=next(r for r in routes if name in (r['a'],r['b']))
        bridges.append(dict(a=route['a'],b=route['b'],crossing=name,center=[*np.round(c['center']*100,4),c['deck']*100],yaw=math.degrees(math.atan2(c['unit'][1],c['unit'][0])),half_span=c['halfspan']*100,ford=False,river=c['river']))
    for route in routes:
        p=np.asarray(route['points'])/100
        for river in rivers:
            distance,surface=polyline_distance(p[:,0],p[:,1],river['points'])
            wet=distance<river['halfwidth']+8
            idx=np.nonzero(wet)[0]
            if not len(idx):continue
            for group in np.split(idx,np.nonzero(np.diff(idx)>1)[0]+1):
                if any(np.min(np.linalg.norm(p[group]-c['center'],axis=1))<160 for c in crossings.values()):continue
                left=max(0,int(group[0])-2);right=min(len(p)-1,int(group[-1])+2)
                a,b=p[left],p[right];point=(a+b)/2;delta=b-a
                unit=delta/max(np.linalg.norm(delta),1e-5)
                halfspan=np.linalg.norm(delta)/2+10
                deck=float(surface[group].max())+5
                bridges.append(dict(a=route['a'],b=route['b'],center=[*np.round(point*100,4),round(deck*100,4)],yaw=math.degrees(math.atan2(unit[1],unit[0])),half_span=round(halfspan*100,4),ford=False,river=river['id']))
    return bridges


def road_surface(h, xy, bridges):
    p=np.asarray(xy)
    z=sample(h,p)+.22
    for bridge in bridges:
        c=np.asarray(bridge['center'])/100
        yaw=math.radians(bridge['yaw'])
        delta=p-c[:2]
        u=delta[...,0]*math.cos(yaw)+delta[...,1]*math.sin(yaw)
        v=-delta[...,0]*math.sin(yaw)+delta[...,1]*math.cos(yaw)
        span=bridge['half_span']/100
        w=(1-smooth(span,span+45,np.abs(u)))*(1-smooth(12,24,np.abs(v)))
        lifted=z+(c[2]+.10-z)*w
        z=np.maximum(z,lifted)
    return z


def bridge_approaches(h,bridges,rivers):
    """Short dry abutments support the fixed deck ramp without an abrupt hump."""
    before=h.copy()
    for bridge in bridges:
        c=np.asarray(bridge['center'])/100;span=bridge['half_span']/100
        radius=span+140
        ix0=max(0,int((c[0]-radius-MINIMUM)/STEP));ix1=min(N,int((c[0]+radius-MINIMUM)/STEP)+1)
        iy0=max(0,int((c[1]-radius-MINIMUM)/STEP));iy1=min(N,int((c[1]+radius-MINIMUM)/STEP)+1)
        yy,xx=np.mgrid[iy0:iy1,ix0:ix1];xx=xx*STEP+MINIMUM-c[0];yy=yy*STEP+MINIMUM-c[1]
        yaw=math.radians(bridge['yaw']);u=np.abs(xx*math.cos(yaw)+yy*math.sin(yaw));v=np.abs(-xx*math.sin(yaw)+yy*math.cos(yaw))
        weight=(1-smooth(span+45,span+115,u))*(1-smooth(10,30,v))
        river=next(r for r in rivers if r['id']==bridge['river'])
        weight*=smooth(river['halfwidth']+5,river['halfwidth']+15,river['distance'][iy0:iy1,ix0:ix1])
        patch=h[iy0:iy1,ix0:ix1]
        target=c[2]-.3-.18*np.maximum(u-span,0)
        patch+=np.clip(target-patch,0,8)*weight
    print('Maximum dry abutment fill metres',float((h-before).max()),flush=True)
    return h


def qualify(h, profile, rivers, sites):
    route_receipts=[]
    for route in profile['routes']:
        points=resample(np.asarray(route['points'])/100,2.5)
        tangent=np.gradient(points,axis=0)
        normal=np.stack([-tangent[:,1],tangent[:,0]],axis=1)
        normal/=np.maximum(np.linalg.norm(normal,axis=1)[:,None],1e-6)
        maxgrade=0.
        minclearance=math.inf
        for offset in [-3,0,3]:
            path=points+normal*offset
            surface=road_surface(h,path,profile['bridges'])
            grade=np.degrees(np.arctan(np.abs(np.diff(surface))/np.maximum(np.linalg.norm(np.diff(path,axis=0),axis=1),.001)))
            maxgrade=max(maxgrade,float(grade.max()))
            # A deck may cross wet bed; clearance is judged on the effective road.
            for river in rivers:
                d,w=polyline_distance(path[:,0],path[:,1],river['points'])
                hit=d<river['halfwidth']
                if np.any(hit):minclearance=min(minclearance,float(np.min(surface[hit]-w[hit])))
        route_receipts.append(dict(a=route['a'],b=route['b'],samples=len(points),length_m=float(np.linalg.norm(np.diff(points,axis=0),axis=1).sum()),max_grade_degrees=maxgrade,min_water_clearance_m=None if math.isinf(minclearance) else minclearance))
    pads=[]
    for site in sites:
        center=np.array(profile['regions'][site['id']][:2])/100
        axis=np.arange(-site['radius_m'],site['radius_m']+1,5)
        xx,yy=np.meshgrid(axis,axis);inside=xx*xx+yy*yy<=site['radius_m']**2
        p=np.stack([center[0]+xx[inside],center[1]+yy[inside]],axis=1)
        z=sample(h,p)
        pads.append(dict(id=site['id'],radius_m=site['radius_m'],height_range_m=float(z.max()-z.min())))
    return dict(status='pass' if all(r['max_grade_degrees']<=MAX_ROUTE_DEGREES and (r['min_water_clearance_m'] is None or r['min_water_clearance_m']>=.5) for r in route_receipts) else 'fail',routes=route_receipts,footprints=pads,drainage=[dict(id=r['id'],downhill=bool(np.all(np.diff(r['points'][:,2])<=0)),outlet_m=r['points'][-1].tolist()) for r in rivers])


def add_fields(profile,h):
    """Small productive parcels on sampled Heartland terrain; no economy rules."""
    rng=np.random.default_rng(20261005)
    paths=np.concatenate([np.asarray(r['points'])/100 for r in profile['routes']])
    centers=np.asarray([p[:2] for p in profile['regions'].values()])/100
    selected=[];evidence=[]
    clusters=[(-3450,700,-12),(-2750,700,-8),(-2200,620,8),(-2700,-280,5),(-3450,-480,-10),(-1850,-430,10)]
    for attempt in range(15000):
        cx,cy,orientation=clusters[attempt%len(clusters)]
        center=np.array([cx+rng.uniform(-390,390),cy+rng.uniform(-300,300)])
        w,d=rng.uniform(50,90),rng.uniform(20,45)
        radius=math.hypot(w,d)
        if np.min(np.linalg.norm(paths-center,axis=1))<radius+25:continue
        if np.min(np.linalg.norm(centers-center,axis=1))<radius+140:continue
        if selected and min(np.linalg.norm(center-np.array(f[:2])/100) for f in selected)<150:continue
        p=center+np.array([[-w,-d],[-w,d],[w,-d],[w,d],[0,0]])
        z=sample(h,p)
        slope=max(np.degrees(np.arctan(np.abs(sample(h,p+[5,0])-sample(h,p-[5,0]))/10)).max(),np.degrees(np.arctan(np.abs(sample(h,p+[0,5])-sample(h,p-[0,5]))/10)).max())
        if slope>7.5 or z.min()<15:continue
        yaw=float(orientation+rng.uniform(-3,3))
        selected.append([round(center[0]*100,3),round(center[1]*100,3),round(w*100,3),round(d*100,3),round(yaw,3)])
        evidence.append(dict(max_grade_degrees=float(slope),minimum_elevation_m=float(z.min())))
        if len(selected)==54:break
    profile['fields']=selected
    profile['field_qualification']=evidence
    return selected


def build(preview=PREVIEW):
    out=preview/'WorldTerrain';out.mkdir(parents=True,exist_ok=True)
    source_path=ROOT/'Data/soul_world_overmap_v1_20260922.json'
    world=json.loads(source_path.read_text())
    nodes=world['nodes'];positions={n['id']:world_xy((n['x'],n['y'])) for n in nodes}
    print('Baking six macro regions and connected drainage',flush=True)
    h,rivers,wet,sites,masks,lake=terrain(preview,nodes,world['terrain_features'])
    np.save(out/'ungraded.npy',h)
    routes=[]
    crossings=crossing_axes(positions,rivers)
    axis=np.linspace(MINIMUM,-MINIMUM,N,dtype=np.float32);x,y=np.meshgrid(axis,axis)
    for c in crossings.values():wet|=np.hypot(x-c['center'][0],y-c['center'][1])<140
    for i,edge in enumerate(world['edges']):
        penalty=sum(18*(1-smooth(r['halfwidth']+5,r['halfwidth']+25,r['distance'])) for r in rivers)
        ends=[positions[edge['a']].copy(),positions[edge['b']].copy()]
        for k,name in enumerate([edge['a'],edge['b']]):
            if name in crossings:
                c=crossings[name];side=1 if np.dot(positions[edge['b'] if k==0 else edge['a']]-c['center'],c['unit'])>=0 else -1
                ends[k]=c['center']+side*c['unit']*160
        points=pathfind(h,wet,*ends,penalty)
        if edge['a'] in crossings:points=np.vstack([positions[edge['a']],points])
        if edge['b'] in crossings:points=np.vstack([points,positions[edge['b']]])
        points=resample(points)
        routes.append(dict(a=edge['a'],b=edge['b'],points=np.round(points*100,4).tolist(),road=edge['road'],chokepoint=edge['chokepoint'],route_class=edge['route'],width_cm=850 if edge['road'] else 420))
        print(f"Route {i+1}/51 {edge['a']} -> {edge['b']}: {len(points)} samples",flush=True)
    for _ in range(1):
        h,road_distance=route_grade(h,[np.asarray(r['points'])/100 for r in routes],positions)
    # Preserve channels after road grading. Every actual wet crossing gets a deck.
    for river in rivers:
        h=h*(1-river['bed'])+(river['surface']-4)*river['bed']
    # Retain authored pads. Road grade cuts are bounded to8m away from explicit
    # pass saddles; never reflatten a pad across its already-carved river bank.
    axis=np.linspace(MINIMUM,-MINIMUM,N,dtype=np.float32);x,y=np.meshgrid(axis,axis)
    for river in rivers:
        h=h*(1-river['bed'])+(river['surface']-4)*river['bed']
    for site in sites:
        c=positions[site['id']];d=np.hypot(x-c[0],y-c[1])
        w=1-smooth(site['radius_m'],site['radius_m']+65,d)
        h=h*(1-w)+site['target_m']*w
    bridges=bridges_for(routes,rivers,h,crossings)
    h=bridge_approaches(h,bridges,rivers)
    raw=np.rint(h*100/HEIGHT_UNIT_CM+32768)
    if raw.min()<=0 or raw.max()>=65535:raise ValueError('Height encoding clipped')
    raw=raw.astype('<u2');h=(raw.astype(np.float32)-32768)*HEIGHT_UNIT_CM/100
    terrain_spec=dict(resolution=N,minimum_xy_cm=[MINIMUM*100]*2,extent_xy_cm=[EXTENT*100]*2,height_unit_cm=HEIGHT_UNIT_CM,heightfield_path='Data/CampaignWorldLocal/WorldHeight.r16',map='/Game/SoulCampaignWorld/L_SoulWorld')
    profile=dict(schema=1,source='Data/soul_world_overmap_v1_20260922.json',source_sha256=hashlib.sha256(source_path.read_bytes()).hexdigest(),presentation_only=True,terrain=terrain_spec,**terrain_spec,scale=10,region_scale=2.5,regions={name:[*p*100,float(road_surface(h,p,bridges)*100-.22*100)] for name,p in positions.items()},routes=routes,fields=[],bridges=bridges,waterlines=[np.round(resample(r['points'],25)*100,4).tolist() for r in rivers],river_widths=[r['halfwidth']*100 for r in rivers],water_ids=[r['id'] for r in rivers],lake=lake,world_space=dict(map_origin=[500,475],map_unit_to_cm=1000,y_axis='map south -> UE negative Y'),playable_region_ids=world['founder_slice']['region_ids'])
    slots=json.loads((ROOT/'Data/soul_overmap_settlement_slots_v1_20260922.json').read_text())['slots']
    profile['lake']['center_cm']=(np.array(lake['center'])*100).tolist();profile['lake']['radius_cm']=(np.array(lake['radius'])*100).tolist()
    styles={'humans':'human','orcs':'orc','dwarves':'dwarf','vikings':'viking','nature':'nature','dark':'dark'}
    profile['context_settlements']=[dict(region=s['region_id'],style=styles.get(s['faction_affinity'],'human'),capital=s['tier']=='major') for s in slots if s['region_id'] not in profile['playable_region_ids']]
    add_fields(profile,h)
    (out/'WorldHeight.r16').write_bytes(raw.tobytes());Image.fromarray(raw).save(out/'WorldHeight.png')
    profile['height_sha256']=hashlib.sha256(raw.tobytes()).hexdigest()
    report=qualify(h,profile,rivers,sites)
    report.update(resolution=N,extent_m=[EXTENT,EXTENT],step_m=STEP,height_min_m=float(h.min()),height_max_m=float(h.max()),regions=len(positions),routes_count=len(routes),bridges_count=len(bridges),height_sha256=profile['height_sha256'],unreal_qualified=False)
    # Material masks: regions R badlands/G greenwood/B ash; biome R rock/G snow/B coast.
    gy,gx=np.gradient(h,STEP);slope=np.hypot(gx,gy)
    snow=smooth(260,550,h)*smooth(1800,3100,y)
    rock=smooth(.22,.65,slope)
    coast=1-smooth(0,20,h)
    region_image=Image.fromarray(np.uint8(np.clip(np.stack([masks['badlands'],masks['greenwood'],masks['ash']],axis=-1),0,1)*255))
    region_image.save(out/'WorldRegions.png');region_image.save(out/'WorldRegions.tga')
    biome_image=Image.fromarray(np.uint8(np.clip(np.stack([rock,snow,coast],axis=-1),0,1)*255))
    biome_image.save(out/'WorldBiome.png');biome_image.save(out/'WorldBiome.tga')
    Image.fromarray(np.uint8((1-smooth(4,9,road_distance))*255)).save(out/'WorldRoads.png')
    color=np.empty((N,N,3),dtype=np.float32);color[:]=[.32,.42,.21]
    for mask,tint in [(masks['greenwood'],[.15,.30,.16]),(masks['badlands'],[.48,.34,.22]),(masks['ash'],[.28,.27,.28]),(rock,[.41,.42,.42]),(snow,[.78,.82,.84])]:
        color=color*(1-mask[...,None])+np.array(tint)*mask[...,None]
    color*=.86+.14*masks['grass'][...,None]
    macro_image=Image.fromarray(np.uint8(np.clip(color,0,1)*255));macro_image.save(out/'WorldMacro.png');macro_image.save(out/'WorldMacro.tga')
    light=np.clip((.85-.6*gx-.4*gy)/np.sqrt(1+slope*slope),.25,1.15)
    color*=light[...,None]
    water=h<0
    for r in rivers:water|=r['distance']<r['halfwidth']
    water|=(((x-lake['center'][0])/lake['radius'][0])**2+((y-lake['center'][1])/lake['radius'][1])**2<2)&(h<lake['center'][2])
    color[water]=[.12,.30,.39]
    # Ground texture contains neither labels nor a permanent strategic graph.
    Image.fromarray(np.uint8(np.clip(color,0,1)*255)).save(out/'WorldColor.png')
    im=Image.fromarray(np.uint8(np.clip(color[::-1],0,1)*255));draw=ImageDraw.Draw(im)
    def uv(p):return ((p[0]/100-MINIMUM)/STEP,(MINIMUM+EXTENT-p[1]/100)/STEP)
    for r in routes:draw.line([uv(p) for p in r['points']],fill=(220,178,104),width=2)
    for name,p in profile['regions'].items():
        u,v=uv(p);draw.ellipse((u-5,v-5,u+5,v+5),fill=(250,235,180));draw.text((u+7,v-5),name,fill='white')
    im.save(out/'geography-overview.png')
    (out/'qualification.json').write_text(json.dumps(report,indent=2)+'\n')
    target=ROOT/'Data/CampaignWorldTerrain/presentation.json';target.parent.mkdir(parents=True,exist_ok=True);target.write_text(json.dumps(profile,indent=2)+'\n')
    (out/'presentation.json').write_text(json.dumps(profile,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['routes','footprints']},indent=2),flush=True)
    print('max route slope',max(r['max_grade_degrees'] for r in report['routes']),flush=True)
    return report


def repair_lake_and_outlet(preview=PREVIEW):
    """Bounded, reproducible repair of frozen R5; keep its qualified corridors."""
    out=preview/'WorldTerrain';base=out/'revisions/revision5'
    profile=json.loads((base/'presentation.json').read_text())
    before=(np.fromfile(base/'WorldHeight.r16',dtype='<u2').astype(np.float32).reshape(N,N)-32768)*.04
    assert hashlib.sha256((base/'WorldHeight.r16').read_bytes()).hexdigest()==profile['height_sha256']
    axis=np.arange(N,dtype=np.float32)*STEP+MINIMUM;x,y=np.meshgrid(axis,axis)
    h,lake,radial=lake_basin(before,x,y)
    outlet=(x<-4200)&(y>-4000)&(y<-3750)&(h>-.25)&(h<2)
    h[outlet]=-1.
    raw=np.rint(h*100/HEIGHT_UNIT_CM+32768).astype('<u2')
    h=(raw.astype(np.float32)-32768)*.04
    profile['lake']=lake;profile['height_sha256']=hashlib.sha256(raw.tobytes()).hexdigest()
    oldq=json.loads((base/'qualification.json').read_text())
    sites=[dict(id=s['id'],radius_m=s['radius_m']) for s in oldq['footprints']]
    rivers=[]
    for points,width,name in zip(profile['waterlines'],profile['river_widths'],profile['water_ids']):
        p=np.asarray(points)/100;distance,surface=raster_distance(p)
        rivers.append(dict(id=name,points=p,halfwidth=width/100,distance=distance,surface=surface))
    report=qualify(h,profile,rivers,sites)
    perimeter=resample(np.vstack([lake['shoreline_cm'],lake['shoreline_cm'][0]])/100,2.5)
    bank_clearance=sample(h,perimeter)-lake['center'][2]
    components,_=label(h<lake['center'][2])
    ci=np.rint((np.array(lake['center'][:2])[::-1]-MINIMUM)/STEP).astype(int)
    component=components[tuple(ci)];inside=components==component
    closed=bool(component and not(inside[0].any() or inside[-1].any() or inside[:,0].any() or inside[:,-1].any()))
    delta=h-before
    report.update(resolution=N,extent_m=[EXTENT,EXTENT],step_m=STEP,height_min_m=float(h.min()),height_max_m=float(h.max()),regions=36,routes_count=51,bridges_count=len(profile['bridges']),height_sha256=profile['height_sha256'],unreal_qualified=False,
        local_repair=dict(base_height_sha256=json.loads((base/'presentation.json').read_text())['height_sha256'],lake_water_m=68.,closed_basin=closed,minimum_mesh_boundary_bank_clearance_m=float(bank_clearance.min()),maximum_mesh_boundary_bank_clearance_m=float(bank_clearance.max()),water_area_sqkm=float(inside.sum()*STEP*STEP/1e6),changed_area_sqkm=float((np.abs(delta)>.001).sum()*25/1e6),outlet_cells_lowered=int(outlet.sum()),minimum_height_delta_m=float(delta.min()),maximum_height_delta_m=float(delta.max())))
    if not closed or bank_clearance.min()<.5 or report['status']!='pass' or any(s['height_range_m']>.08 for s in report['footprints']):
        raise RuntimeError(f'Local repair failed acceptance: {report["local_repair"]}')
    # Recompute only changed material texels and their immediate gradient ring.
    changed=binary_dilation(np.abs(delta)>.001,iterations=2)
    gy,gx=np.gradient(h,STEP);slope=np.hypot(gx,gy)
    snow=smooth(260,550,h)*smooth(1800,3100,y);rock=smooth(.22,.65,slope);coast=1-smooth(0,20,h)
    biome=np.asarray(Image.open(base/'WorldBiome.png')).copy()
    fresh_biome=np.uint8(np.clip(np.stack([rock,snow,coast],axis=-1),0,1)*255)
    biome[changed]=fresh_biome[changed]
    for suffix in ['png','tga']:Image.fromarray(biome).save(out/f'WorldBiome.{suffix}')
    regions=np.asarray(Image.open(base/'WorldRegions.png')).astype(float)/255
    grass=np.asarray(Image.open(preview/'TerrainWork/grassland02-2041-rg.png')).astype(np.uint16)
    grass=grass[:,:,0]*256+grass[:,:,1]
    g=stamp(grass,(160,220,1860,1870),(-4300,-4400,4600,4400),x,y)
    color=np.empty((N,N,3),dtype=np.float32);color[:]=[.32,.42,.21]
    for mask,tint in [(regions[:,:,1],[.15,.30,.16]),(regions[:,:,0],[.48,.34,.22]),(regions[:,:,2],[.28,.27,.28]),(rock,[.41,.42,.42]),(snow,[.78,.82,.84])]:color=color*(1-mask[...,None])+np.asarray(tint)*mask[...,None]
    color*=.86+.14*g[...,None]
    macro=np.asarray(Image.open(base/'WorldMacro.png')).copy()
    macro[changed]=np.uint8(np.clip(color[changed],0,1)*255)
    for suffix in ['png','tga']:Image.fromarray(macro).save(out/f'WorldMacro.{suffix}')
    color=macro.astype(float)/255
    light=np.clip((.85-.6*gx-.4*gy)/np.sqrt(1+slope*slope),.25,1.15);color*=light[...,None]
    water=(h<0)|inside
    for river in rivers:water|=river['distance']<river['halfwidth']
    color[water]=[.12,.30,.39]
    Image.fromarray(np.uint8(np.clip(color,0,1)*255)).save(out/'WorldColor.png')
    im=Image.fromarray(np.uint8(np.clip(color[::-1],0,1)*255));draw=ImageDraw.Draw(im)
    def uv(p):return ((p[0]/100-MINIMUM)/STEP,(MINIMUM+EXTENT-p[1]/100)/STEP)
    for route in profile['routes']:draw.line([uv(p) for p in route['points']],fill=(220,178,104),width=2)
    for name,p in profile['regions'].items():
        u,v=uv(p);draw.ellipse((u-5,v-5,u+5,v+5),fill=(250,235,180));draw.text((u+7,v-5),name,fill='white')
    im.save(out/'geography-overview.png')
    (out/'WorldHeight.r16').write_bytes(raw.tobytes());Image.fromarray(raw).save(out/'WorldHeight.png')
    for target in [out/'presentation.json',ROOT/'Data/CampaignWorldTerrain/presentation.json']:target.write_text(json.dumps(profile,indent=2)+'\n')
    (out/'qualification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['local_repair'],indent=2),flush=True)
    print('Height SHA256',profile['height_sha256'],flush=True)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--preview',type=Path,default=PREVIEW)
    parser.add_argument('--repair-lake',action='store_true')
    args=parser.parse_args();result=repair_lake_and_outlet(args.preview) if args.repair_lake else build(args.preview)
    if result['status']!='pass':raise SystemExit('Terrain route qualification requires repair; see qualification.json')
