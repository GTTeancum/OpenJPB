"""Losslessly index XLV v1 geometry; validate every reconstructed vertex."""
import argparse
import json
import struct
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
data = args.source.read_bytes()
magic, version, level, batches, total = struct.unpack_from('<5I', data)
assert magic == 0x564c584a and version == 1
offset = 20
records = []
unique_total = 0
decoded_total = 0
for batch in range(batches):
    count, render_pass, mesh_index, mesh_count = struct.unpack_from('<4I', data, offset)
    offset += 16
    names = data[offset:offset+384]
    offset += 384
    assert count % 3 == 0 and render_pass <= 2 and mesh_index < mesh_count
    assert len(names) == 384 and names[255] == 0 and names[383] == 0
    source = data[offset:offset+44*count]
    assert len(source) == count*44
    offset += len(source)
    unique, lookup, indices = [], {}, []
    for i in range(count):
        vertex = source[i*44:(i+1)*44]
        if vertex not in lookup:
            lookup[vertex] = len(unique)
            unique.append(vertex)
        indices.append(lookup[vertex])
    assert len(unique) <= 65536
    assert b''.join(unique[i] for i in indices) == source
    records.append(struct.pack('<5I', count, render_pass, mesh_index, mesh_count,
                               len(unique)) + names + b''.join(unique) +
                   struct.pack(f'<{count}H', *indices))
    unique_total += len(unique)
    decoded_total += count
assert offset == len(data) and decoded_total == total
output = struct.pack('<6I', magic, 2, level, batches, total, unique_total) + b''.join(records)
args.output.write_bytes(output)
print(json.dumps(dict(vertices=total, unique_vertices=unique_total, triangles=total//3,
                      input_bytes=len(data), output_bytes=len(output),
                      vertex_memory_saved=(total-unique_total)*44-total*2,
                      roundtrip='every vertex byte-identical'), indent=2))
