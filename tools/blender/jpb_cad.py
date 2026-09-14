"""CAD codec matched to original/unpack.c, original/anim.c and portable/cad.c.

No Blender dependency. Offsets are relative to the payload (after size u32).
Export retains templates and Motion records, including gameplay metadata.
"""
import struct
from pathlib import Path


def wrap(value, bits=12):
    return ((int(value) + (1 << (bits - 1))) & ((1 << bits) - 1)) - (1 << (bits - 1))


class Bits:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def read(self, count):
        if self.pos + count > len(self.data) * 8:
            raise ValueError('Truncated CAD bitstream')
        start, shift = divmod(self.pos, 8)
        value = int.from_bytes(self.data[start:start + (shift + count + 7) // 8], 'little')
        self.pos += count
        return (value >> shift) & ((1 << count) - 1)

    def peek(self, count):
        pos = self.pos
        result = self.read(count)
        self.pos = pos
        return result


class Writer:
    def __init__(self):
        self.data = bytearray()
        self.pending = 0
        self.count = 0

    def write(self, value, count):
        self.pending |= (value & ((1 << count) - 1)) << self.count
        self.count += count
        while self.count >= 8:
            self.data.append(self.pending & 255)
            self.pending >>= 8
            self.count -= 8

    def finish(self):
        if self.count:
            self.data.append(self.pending & 255)
        # Original decoder prefetches two words; keep a guard within each clip.
        return bytes(self.data) + bytes((-len(self.data)) % 4 + 8)


class Huffman:
    def __init__(self, directory):
        directory = Path(directory)
        self.options = (directory / 'huffman.opt').read_bytes()
        self.values = (directory / 'huffman.val').read_bytes()
        self.tree = (directory / 'huffman.tab').read_bytes()
        if len(self.options) != 2048 or len(self.tree) % 4 or len(self.values) % 2:
            raise ValueError('Invalid Huffman table sizes')
        self.codes = None

    def word(self, bits, pending):
        if pending:
            return pending.pop(0)
        vals, goto = struct.unpack_from('<II', self.options, bits.peek(8) * 8)
        if not vals & 0x80000000:
            bits.read((7 - ((goto >> 28) & 7)) or 8)
            count, offset = (vals >> 28) + 1, (vals & 0x7fff) * 2
            pending.extend(struct.unpack_from('<' + 'H' * count, self.values, offset))
            return pending.pop(0)
        bits.read(8)
        node = goto & 0x7fff
        for _ in range(64):
            branches = struct.unpack_from('<I', self.tree, node * 4)[0]
            node = (branches >> (16 * bits.read(1))) & 0xffff
            if node & 0x8000:
                return node & 0x7fff
        raise ValueError('Cyclic Huffman tree')

    def build_codes(self):
        codes = {}
        def add(words, value, length):
            if words not in codes or length < codes[words][1]:
                codes[words] = (value, length)
        def walk(node, value, length, visited):
            if node in visited or length > 64:
                raise ValueError('Cyclic Huffman tree')
            branches = struct.unpack_from('<I', self.tree, node * 4)[0]
            for bit in range(2):
                child = (branches >> (bit * 16)) & 0xffff
                code = value | (bit << length)
                if child & 0x8000:
                    add((child & 0x7fff,), code, length + 1)
                else:
                    walk(child, code, length + 1, visited | {node})
        for prefix in range(256):
            vals, goto = struct.unpack_from('<II', self.options, prefix * 8)
            if vals & 0x80000000:
                walk(goto & 0x7fff, prefix, 8, set())
            else:
                length = (7 - ((goto >> 28) & 7)) or 8
                count = (vals >> 28) + 1
                words = struct.unpack_from('<' + 'H' * count, self.values, (vals & 0x7fff) * 2)
                add(words, prefix & ((1 << length) - 1), length)
        self.codes = {}
        for words, code in codes.items():
            self.codes.setdefault(words[0], []).append((words, code))

    def encode(self, writer, words):
        if self.codes is None:
            self.build_codes()
        # Optimal segmentation matters: a fast code can emit several words.
        costs = [float('inf')] * (len(words) + 1)
        chosen = [None] * len(words)
        costs[-1] = 0
        for i in range(len(words) - 1, -1, -1):
            for sequence, code in self.codes.get(words[i], ()):
                n = len(sequence)
                if tuple(words[i:i+n]) == sequence and code[1] + costs[i+n] < costs[i]:
                    costs[i] = code[1] + costs[i+n]
                    chosen[i] = (n, code)
        if costs[0] == float('inf'):
            raise ValueError('Animation contains a value not representable by the supplied Huffman tables')
        i = 0
        while i < len(words):
            n, code = chosen[i]
            writer.write(*code)
            i += n


class Cad:
    def __init__(self, data):
        self.data = bytes(data)
        if len(data) < 24 or struct.unpack_from('<I', data)[0] != len(data) - 4:
            raise ValueError('Invalid CAD size')
        self.bit_offset, self.seq_offset, self.motion_offset = struct.unpack_from('<III', data, 4)
        self.parts, self.count = struct.unpack_from('<H2xH', data, 16)
        if (self.seq_offset < 20 or self.seq_offset + self.count * 32 != self.bit_offset
                or self.motion_offset + self.count * 100 != len(data) - 4
                or self.bit_offset > self.motion_offset
                or any(x % 4 for x in (self.seq_offset, self.bit_offset, self.motion_offset))):
            raise ValueError('Invalid CAD layout')
        self.templates = [bytes(data[4+self.seq_offset+i*32:4+self.seq_offset+(i+1)*32]) for i in range(self.count)]
        self.motions = [bytes(data[4+self.motion_offset+i*100:4+self.motion_offset+(i+1)*100]) for i in range(self.count)]

    @classmethod
    def load(cls, path):
        return cls(Path(path).read_bytes())

    def name(self, index):
        for motion in self.motions:
            if struct.unpack_from('<H', motion, 4)[0] == index:
                return motion[68:100].split(b'\0')[0].decode('ascii', 'replace')
        return f'anim_{index}'

    def decode(self, index, huffman):
        offset, first, last, parts, preroll = struct.unpack_from('<Ihhhh', self.templates[index])
        if not 0 <= parts <= 32 or not 0 <= first <= last:
            raise ValueError(f'Invalid CAD template {index}')
        # unpack_seekcontext floors the seek to a word. The native context's
        # readable window includes the trailing Motion records.
        bits = Bits(self.data[4+self.bit_offset+(offset & ~3):])
        frames, delta, pending = [], None, []
        for frame in range(last):
            vectors = []
            for j in range(parts + 1):
                if frame == 0:
                    has_pad = bits.read(1)
                    xyz = [bits.read(11) for _ in range(3)]
                    pad = bits.read(16) if has_pad else 0
                else:
                    x = huffman.word(bits, pending)
                    pad = huffman.word(bits, pending) if x == 4095 else 0
                    if x == 4095:
                        x = huffman.word(bits, pending)
                    xyz = [x, huffman.word(bits, pending), huffman.word(bits, pending)]
                xyz = [wrap(xyz[0], 11), wrap(xyz[1], 11)*2, wrap(xyz[2], 11)] if j == 0 else [wrap(x*2) for x in xyz]
                vectors.append(xyz + [pad])
            if frame == 0:
                pose = vectors
            else:
                if frame == 1:
                    delta = [v[:3] for v in vectors]
                else:
                    delta = [[wrap(a+b) for a,b in zip(d,v)] for d,v in zip(delta,vectors)]
                pose = [[wrap(p[k]+d[k], 13 if j == 0 and k == 1 else 12) for k in range(3)]
                        + [p[3] ^ v[3]] for j,(p,d,v) in enumerate(zip(pose,delta,vectors))]
            frames.append([list(v) for v in pose])
        return frames

    def encode_clip(self, frames, huffman):
        if not frames:
            return bytes(8)
        writer, words, previous_delta = Writer(), [], None
        for fi, frame in enumerate(frames):
            deltas = []
            for j, vector in enumerate(frame):
                if fi == 0:
                    xyz = [vector[0], vector[1]//2, vector[2]] if j == 0 else [x//2 for x in vector[:3]]
                    if j == 0 and any(not -1024 <= x <= 1023 for x in xyz):
                        raise ValueError('Initial root translation exceeds CAD 11-bit range')
                    writer.write(bool(vector[3]), 1)
                    for x in xyz:
                        writer.write(x, 11)
                    if vector[3]:
                        writer.write(vector[3], 16)
                    continue
                delta = [wrap(vector[k]-frames[fi-1][j][k], 13 if j == 0 and k == 1 else 12) for k in range(3)]
                deltas.append(delta)
                change = delta if fi == 1 else [wrap(a-b) for a,b in zip(delta,previous_delta[j])]
                xyz = [change[0],change[1]//2,change[2]] if j == 0 else [x//2 for x in change]
                if j == 0 and any(not -1024 <= x <= 1023 for x in xyz):
                    raise ValueError(f'Root delta at frame {fi+1} exceeds CAD range')
                pad = vector[3] ^ frames[fi-1][j][3]
                if pad:
                    words.extend((4095, pad))
                words.extend(x & 2047 for x in xyz)
            if fi:
                previous_delta = deltas
        huffman.encode(writer, words)
        return writer.finish()

    def export(self, replacements, huffman):
        # Keep untouched clip bitstreams verbatim, including unusual pre-roll.
        stream = bytearray() if len(replacements) == self.count else bytearray(self.data[4+self.bit_offset:4+self.motion_offset])
        templates = []
        for i, raw in enumerate(self.templates):
            template = bytearray(raw)
            if i in replacements:
                struct.pack_into('<I', template, 0, len(stream))
                frames = replacements[i]
                if len(frames) != struct.unpack_from('<h', raw, 6)[0]:
                    raise ValueError('Keep the original clip length to preserve gameplay event timing')
                stream.extend(self.encode_clip(frames, huffman))
            templates.append(template)
        header = bytearray(self.data[4:4+self.seq_offset])
        struct.pack_into('<I', header, 8, self.bit_offset + len(stream))
        payload = header + b''.join(templates) + stream + b''.join(self.motions)
        if len(payload) + 4 > 0x40000:
            if len(replacements) != self.count:
                complete = {i:replacements.get(i) if i in replacements else self.decode(i,huffman)
                            for i in range(self.count)}
                return self.export(complete,huffman)
            raise ValueError('Encoded CAD exceeds the runtime 256 KiB capacity')
        return struct.pack('<I', len(payload)) + payload
