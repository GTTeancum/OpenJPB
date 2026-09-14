from pathlib import Path
import json, subprocess, os, shutil, re, concurrent.futures, argparse
parser=argparse.ArgumentParser();parser.add_argument('--game-root',type=Path,required=True);parser.add_argument('--exe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--variants',choices=('both','current'),default='both');args=parser.parse_args()
root=args.game_root.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
mods=[json.loads(p.read_text()) for p in sorted((root/'mods').glob('*/mod.json')) if json.loads(p.read_text()).get('type')=='character']
cases=[(m,slot) for m in mods for slot in ([2] if args.variants=='current' else range(2 if m['isJedi'] else 1))]
def worker(worker_id):
 stage=out/f'run{worker_id}';stage.mkdir(exist_ok=True)
 shutil.copy2(args.exe,stage/'OpenJPB.exe')
 for dll in ('SDL2.dll','SDL2_mixer.dll','SDL2_ttf.dll'):shutil.copy2(root/dll,stage/dll)
 if not (stage/'res').exists():
  subprocess.run(['powershell','-NoProfile','-Command',f"New-Item -ItemType Junction -Path '{stage/'res'}' -Target '{root/'res'}' | Out-Null"],check=True)
 results=[]
 for mod,slot in cases[worker_id::2]:
  name=f"{mod['id']}-{('default','alternate','current')[slot]}";result=dict(id=mod['id'],slot=slot)
  base=[str(stage/'OpenJPB.exe'),'--mods-root',str(root),'--hidden-window','--control-harness','--player-model',str(mod['modelId']),'--framebuffer-size','640','360']
  if mod['isJedi']:base+=['--player-saber-color',('canon','legacy','current')[slot]]
  for kind in ('menu','handoff','combat'):
   phases=[]
   if kind=='menu':extra=['--review-menu','14'];phases=[('none',20)]
   elif kind=='handoff':
    extra=['--review-menu','14'];phases=[('none',40),('a',1),('none',50),('a',1),('none',80),('a',1),('none',47)]
   else:
    extra=['--quickload','fed','--control-scheme','p1','classic','--review-disable-ai','--spawn-position','20224','3328','-14848']
    phases=[('none',20),('y',1),('none',60),('b',1),('none',60),('x',1),('none',97),('right',30)]
   command=base+extra+['--frames',str(sum(n for _,n in phases)),'--output',str(out/f'{name}-{kind}.ppm')]
   for button,n in phases:command+=['--headless-xinput-phase',button,str(n)]
   p=subprocess.run(command,cwd=stage,env=dict(os.environ,SDL_AUDIODRIVER='dummy'),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=100)
   text=p.stdout.decode(errors='replace');(out/f'{name}-{kind}.log').write_text(text)
   native=(stage/'jpb_pc_game.log').read_text();(out/f'{name}-{kind}-native.log').write_text(native)
   result[kind]=p.returncode==0 and 'frame failed' not in native
   if kind=='handoff':result['handoff']=result[kind] and 'front end completed; gameplay initialized' in native and 'hardware model timing frames=' in native
   if kind=='combat':
    result['movementRecovered']=bool(re.search(r'locomotion:29/1,direction:29',text))
    if mod['isJedi']:
     match=re.search(r'player_weapon=\(model=\d+,saber=1,color=([0-9a-f]+),outer=(\d+),trail=(\d+),core=(\d+),attached=(\d+),unmatched=(\d+)',text)
     expected=(int(mod['colors'][slot],16)&0xffffff)|0x7f000000
     result['saberVerified']=bool(match and int(match[1],16)==expected and int(match[4])>0 and int(match[5])>0 and int(match[6])==0)
    else:result['saberVerified']=bool(re.search(r'player_weapon=\(model=\d+,saber=0,',text))
  results.append(result);(out/f'results{worker_id}.json').write_text(json.dumps(results,indent=2));print(json.dumps(result),flush=True)
 return results
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool: results=sum(list(pool.map(worker,range(2))),[])
(out/'results.json').write_text(json.dumps(results,indent=2))
assert len(results)==len(cases) and all(all(r[k] for k in ('menu','handoff','combat','movementRecovered','saberVerified')) for r in results)
print('ALL_MOD_SMOKE_PASS',len(results),flush=True)
