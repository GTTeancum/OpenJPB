"""Small format-boundary regressions; real-asset/native sweeps live in tools/blender."""
import struct
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/blender'))
from jpb_cad import Bits, Cad, Huffman, Writer, wrap


class CadCodecTests(unittest.TestCase):
    def test_lsb_bits_cross_byte_and_bounds(self):
        writer=Writer()
        writer.write(0x345,11)
        writer.write(0xbeef,16)
        bits=Bits(writer.finish())
        self.assertEqual(bits.read(11),0x345)
        self.assertEqual(bits.read(16),0xbeef)
        with self.assertRaises(ValueError): Bits(b'\x00').read(9)

    def test_angle_and_root_wrapping(self):
        self.assertEqual(wrap(2048),-2048)
        self.assertEqual(wrap(-2049),2047)
        self.assertEqual(wrap(4096,13),-4096)

    def test_raw_event_and_zero_delta_roundtrip(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            (root/'huffman.opt').write_bytes(struct.pack('<II',0,0)*256)
            (root/'huffman.val').write_bytes(bytes(2))
            (root/'huffman.tab').write_bytes(bytes(4))
            huffman=Huffman(root)
            # Minimal valid archive. One joint, one template, preserved Motion.
            template=struct.pack('<Ihhhh',0,0,3,1,0)+bytes(20)
            motion=bytes(68)+b'fixture'+bytes(25)
            header=struct.pack('<IIIHHHH',52,20,60,1,0,1,0)
            payload=header+template+bytes(8)+motion
            cad=Cad(struct.pack('<I',len(payload))+payload)
            frames=[[[4,20,-6,0x1234],[0,0,0,0]] for _ in range(3)]
            result=Cad(cad.export({0:frames},huffman))
            self.assertEqual(result.decode(0,huffman),frames)
            self.assertEqual(result.motions,cad.motions)
            with self.assertRaises(ValueError): cad.export({0:frames[:2]},huffman)

    def test_invalid_archives_are_rejected(self):
        for data in (b'',bytes(24),struct.pack('<I',20)+bytes(20)):
            with self.assertRaises(ValueError): Cad(data)


if __name__=='__main__': unittest.main()
