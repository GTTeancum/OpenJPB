"""Exercise installed additions through title/menu input, never direct selection."""
from pathlib import Path
import argparse, concurrent.futures, json, os, re, shutil, subprocess
ap=argparse.ArgumentParser()
ap.add_argument('--game-root',type=Path,required=True)
ap.add_argument('--exe',type=Path,required=True)
ap.add_argument('--output',type=Path,required=True)
ap.add_argument('--stage-root',type=Path,required=True)
ap.add_argument('--only',nargs='*')
ap.add_argument('--workers',type=int,default=2)
a=ap.parse_args();root=a.game_root.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
mods=sorted((json.loads(p.read_text()) for p in (root/'mods').glob('*/mod.json')),key=lambda m:m['modelId'])
mods=[m for m in mods if m.get('enabled',True) and m.get('type')=='character']
def worker(w):
 stage=a.stage_root.resolve()/str(w);stage.mkdir(parents=True,exist_ok=True)
 shutil.copy2(a.exe,stage/'OpenJPB.exe')
 for dll in root.glob('SDL2*.dll'):shutil.copy2(dll,stage/dll.name)
 if not (stage/'res').exists():
  subprocess.run(['powershell','-NoProfile','-Command',f"New-Item -ItemType Junction -Path '{stage/'res'}' -Target '{root/'res'}' | Out-Null"],check=True)
 results=[]
 for m in mods[w::a.workers]:
  if a.only and m['id'] not in a.only:continue
  case=out/m['id'];case.mkdir(exist_ok=True);save=case/'save';save.mkdir(exist_ok=True)
  shutil.copy2(root/'SAVEDATA0'/'Options',save/'Options')
  phases=[];marks={}
  def wait(n=30,label=None):
   phases.append(('none',n))
   if label:marks[label]=len(phases)-1
  def tap(b,n=30,label=None):phases.append((b,1));wait(n,label)
  wait(180)
  for _ in range(7):tap('a',90)
  # Physical left wraps from Obi-Wan through all installed additions.
  for _ in range(len(mods)-mods.index(m)):tap('left')
  wait(30,'portrait-current')
  for _ in range(4):tap('a',90)
  wait(720,'gameplay-current')
  phases.append(('down',120));wait(45)
  for _ in range(5):tap('y',60)
  phases.append(('right',30));wait(30,'combat-current')
  tap('start')
  for _ in range(4):tap('down',20)
  tap('a');tap('down');tap('a',90)
  # Saved progression makes Continue the first item; choose New Game.
  tap('down')
  for _ in range(3):tap('a',90)
  tap('left');tap('right')
  if m['isJedi']:tap('x')
  wait(30,'portrait-toggled')
  for _ in range(4):tap('a',90)
  wait(720,'gameplay-toggled')
  phases.append(('down',120));wait(45)
  for _ in range(5):tap('y',60)
  phases.append(('right',30));wait(30,'combat-toggled')
  assert len(phases)<=128
  (case/'phases.json').write_text(json.dumps(dict(phases=phases,marks=marks),indent=2))
  cmd=[str(stage/'OpenJPB.exe'),'--mods-root',str(root),'--hidden-window','--control-harness','--persistence-directory',str(save),'--frames',str(sum(n for b,n in phases)),'--framebuffer-size','960','540','--session-review-prefix',str(case/'frame'),'--output',str(case/'final.ppm')]
  for b,n in phases:cmd+=['--headless-xinput-phase',b,str(n)]
  (case/'command.json').write_text(json.dumps(cmd,indent=2))
  p=subprocess.run(cmd,cwd=stage,env=dict(os.environ,SDL_AUDIODRIVER='dummy'),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=240)
  (case/'stdout.log').write_bytes(p.stdout);native=(stage/'jpb_pc_game.log').read_text();(case/'native.log').write_text(native)
  identities=re.findall(r'mod-identity id=(\S+) actual=(\w+) expected=(\w+) match=(\d)',native)
  sabers=re.findall(r'mod-saber id=(\S+) color=(\w+) expected=(\w+) core=(\d+) attached=(\d+) unmatched=(\d+)',native)
  colors={int(c,16) for id,c,e,core,attach,unmatch in sabers if id==m['id'] and c==e and int(core)>0 and int(attach)>0 and int(unmatch)==0}
  expected={(int(c,16)&0xffffff)|0x7f000000 for c in m['colors'][:2]}
  portraits=all(re.search(rf'session-review phase={marks[label]} frame=\d+ title=1 menu=14 selected={m["modelId"]} ',native) for label in ('portrait-current','portrait-toggled'))
  counts={int(phase):int(count) for phase,count in re.findall(r'session-combat phase=(\d+) attack_frames=(\d+)',native)}
  attacked=all(counts.get(marks['combat-'+variant],0)>counts.get(marks['gameplay-'+variant],0) for variant in ('current','toggled'))
  r=dict(attacksObserved=attacked,id=m['id'],exit=p.returncode,portraitsSelected=portraits,identity=bool(identities) and all(id==m['id'] and actual==expected and match=='1' for id,actual,expected,match in identities),saberVariants=not m['isJedi'] or expected<=colors,noLoadErrors='menu texture load rejected' not in native and 'frame failed' not in native,handoffs=native.count('front end completed; gameplay initialized'))
  r['pass']=r['exit']==0 and all(r[k] for k in ('portraitsSelected','identity','saberVariants','noLoadErrors','attacksObserved')) and r['handoffs']==2
  results.append(r);(case/'result.json').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True)
 return results
with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:results=sum(list(pool.map(worker,range(a.workers))),[])
(out/'results.json').write_text(json.dumps(results,indent=2))
raise SystemExit(0 if all(r['pass'] for r in results) else 1)

