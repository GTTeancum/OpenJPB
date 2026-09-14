"""Build one installable .py, retaining the existing Blender module identity."""
from pathlib import Path
import argparse
import shutil


def build(destination):
    root=Path(__file__).resolve().parent
    addon=(root/'io_bmd_v3_6_1.py').read_text(encoding='utf-8')
    codec=(root/'jpb_cad.py').read_text(encoding='utf-8')
    operators=(root/'cad_operators.py').read_text(encoding='utf-8')
    operators=operators.replace('from jpb_cad import Cad, Huffman, wrap\n','')
    start=addon.index('# The codec is shared')
    end=addon.index('# ---------------------------------------------------------------------------\n# BMD Export',start)
    addon=addon[:start]+codec+'\n\n'+operators+'\n\n'+addon[end:]
    addon=addon.replace('from level_operators import ImportJPBLevel, JPBLevelInspector, GAMEPLAY_CLASSES, register_camera_browser, unregister_camera_browser',
        (root/'level_operators.py').read_text(encoding='utf-8'))
    addon=addon.replace('from level_gameplay import GAMEPLAY_CLASSES, import_gameplay, load_level_sidecars, archive_inventory, register_camera_browser, unregister_camera_browser',
        (root/'level_gameplay.py').read_text(encoding='utf-8'))
    compile(addon,str(destination),'exec')
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_text(addon,encoding='utf-8')


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('destination',type=Path)
    parser.add_argument('--level-helper',type=Path,help='Bundle the built native level reader beside the add-on')
    args=parser.parse_args()
    build(args.destination)
    if args.level_helper:
        shutil.copy2(args.level_helper,args.destination.parent/'jpb_level_import.exe')
