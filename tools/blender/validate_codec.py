"""Compare Python CAD decoding and edited exports with the native frame probe."""
import argparse
import json
import struct
import subprocess
from pathlib import Path
from jpb_cad import Cad, Huffman, wrap


def compare(cad, huffman, binary):
    offset = 0
    frame_count = 0
    for index in range(cad.count):
        for frame in cad.decode(index, huffman):
            frame_count += 1
            for joint, vector in enumerate(frame):
                actual = struct.unpack_from('<hhhH', binary, offset)
                offset += 8
                actual = [wrap(v,13 if joint == 0 and axis == 1 else 12)
                          if axis < 3 else v for axis,v in enumerate(actual)]
                assert vector == actual, (index, frame_count, joint, vector, actual)
    assert offset == len(binary)
    return frame_count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('assets', type=Path)
    parser.add_argument('probe', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    huffman = Huffman(args.assets)
    results = []
    for filename in ('adi.cad','obi_wan.cad','qui_gon.cad','mace.cad','plo.cad',
                     'darth_ma.cad','aayla.cad','kitfisto.cad','shaakti.cad'):
        path = args.assets / filename
        if not path.exists():
            continue
        cad = Cad.load(path)
        decoded = {i:cad.decode(i,huffman) for i in range(cad.count)}
        rebuilt = cad.export(decoded,huffman)
        candidate = args.output / filename
        candidate.write_bytes(rebuilt)
        result = {'file':filename,'clips':cad.count}
        for label,source in (('original',path),('reencoded',candidate)):
            binary = args.output / (filename+'.'+label+'.bin')
            subprocess.run([str(args.probe.resolve()),str(source.resolve()),
                            str(args.assets.resolve()),str(binary.resolve())],check=True)
            result[label+'_frames'] = compare(cad,huffman,binary.read_bytes())
        assert Cad(rebuilt).motions == cad.motions
        results.append(result)
        print(result, flush=True)
    (args.output/'codec-results.json').write_text(json.dumps(results,indent=2),encoding='utf-8')


if __name__ == '__main__':
    main()
