"""Exercise native level decoding on every available stock level pair."""
import argparse
import json
import math
import subprocess
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('helper',type=Path)
parser.add_argument('game',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
root=args.game/'res/level'
results=[]
for name in ('council','fed','marsh','theed','palace','tato','corus1','ruins','streets','hangar','core',
             'mini1','mini2','mini3','mini4','corus2','train1','train2','train3','train5','train6','train7','train4','arena'):
    fbx=root/'jpx'/name/(name+'.fbx');j3d=root/'W3D'/(name+'.j3d')
    if not fbx.is_file() and not j3d.is_file():continue
    target=args.output/'native-scene.json'
    run=subprocess.run([str(args.helper.resolve()),str(fbx) if fbx.is_file() else '-',
                        str(j3d) if j3d.is_file() else '-','-1',str(target)],capture_output=True,text=True,timeout=180)
    assert run.returncode==0,(name,run.returncode,run.stderr)
    data=json.loads(target.read_text())
    for mesh in data['meshes']:
        assert len(mesh['vertices'])%3==0
        assert all(math.isfinite(v) for vertex in mesh['vertices'] for v in vertex)
    assert all(abs(v)<1000000 for face in data['collision'] for vertex in face['vertices'] for v in vertex),name
    if fbx.is_file():assert data['meshes'],name
    results.append({'level':name,'visual_batches':len(data['meshes']),'collision_faces':len(data['collision']),
                    'placements':len(data['placements']),'scripts':len(data['scripts'])})
    print(name,'PASS',flush=True)
(args.output/'native-validation.json').write_text(json.dumps(results,indent=2))
