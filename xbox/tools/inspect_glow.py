"""Read the frame-650 Xbox glow packet and render its affine-UV reference.

Diagnostic only: software bilinear sampling of the submitted vertices and
the staged source texture. Does not drive the desktop or alter game state.
"""
import json
from pathlib import Path
import re
import socket
import struct
import numpy as np
from PIL import Image

symbols=Path('xbox/build/OpenJPB.map').read_text()
def address(name):
    return int(re.search(r'\s+\d{4}:[\da-fA-F]+\s+_'+name+r'\s+([\da-fA-F]+)',symbols)[1],16)
with socket.create_connection(('127.0.0.1',9247),timeout=2) as monitor:
    monitor.settimeout(3)
    def response():
        data=b''
        while not data.endswith(b'(qemu) '):
            block=monitor.recv(32768)
            if not block: raise RuntimeError('Monitor closed')
            data+=block
        return data.decode(errors='replace')
    response()
    def read(addr,words):
        monitor.sendall(f'x /{words}wx 0x{addr:x}\n'.encode())
        result=[]
        for line in response().splitlines():
            if re.match(r'^[\da-fA-F]+:',line):
                result.extend(int(x,16) for x in re.findall(r'0x([\da-fA-F]{8})',line))
        if len(result)!=words: raise RuntimeError('Incomplete guest-memory read')
        return result
    count=read(address('jpb_XboxGlowCaptureCount'),1)[0]
    if not 0<count<=384: raise RuntimeError(f'Capture not ready: {count}')
    words=read(address('jpb_XboxGlowCaptureVertices'),count*11)
vertices=np.array(struct.unpack('<'+'f'*len(words),struct.pack('<'+'I'*len(words),*words))).reshape(-1,11)
out=Path('xbox/build/xemu/glow-all-vertices')
out.mkdir(parents=True,exist_ok=True)
(out/'vertices.json').write_text(json.dumps(vertices.tolist(),indent=2))
texture=np.asarray(Image.open('C:/Games/OpenJPB-Xbox/res/default/a_glow.tga').convert('RGBA'),dtype=float)/255
height,width=texture.shape[:2]
canvas=np.zeros((960,1280,3))
for tri in vertices.reshape(-1,3,11):
    xy=tri[:,:2]*4
    low=np.maximum(np.floor(xy.min(axis=0)).astype(int),0)
    high=np.minimum(np.ceil(xy.max(axis=0)).astype(int),[1279,959])
    if np.any(low>high): continue
    yy,xx=np.mgrid[low[1]:high[1]+1,low[0]:high[0]+1]
    matrix=np.vstack([xy.T,np.ones(3)])
    if abs(np.linalg.det(matrix))<1e-8: continue
    weights=np.linalg.solve(matrix,np.array([xx.ravel()+.5,yy.ravel()+.5,np.ones(xx.size)]))
    inside=(weights>=-1e-6).all(axis=0)
    values=weights.T@tri
    uv=np.clip(values[:,5:7]*[width,height]-.5,[0,0],[width-1,height-1])
    x0,y0=np.floor(uv).astype(int).T
    x1,y1=np.minimum(x0+1,width-1),np.minimum(y0+1,height-1)
    dx,dy=(uv-np.floor(uv)).T
    sample=(texture[y0,x0]*(1-dx[:,None])+texture[y0,x1]*dx[:,None])*(1-dy[:,None])
    sample+=(texture[y1,x0]*(1-dx[:,None])+texture[y1,x1]*dx[:,None])*dy[:,None]
    color=sample*values[:,7:11]/255
    rgb=color[:,:3]*color[:,3,None] # class 1: SrcAlpha/One
    canvas[yy.ravel()[inside],xx.ravel()[inside]]+=rgb[inside]
Image.fromarray(np.uint8(np.clip(canvas,0,1)*255)).save(out/'glow-reference.png')
print(json.dumps(dict(vertices=int(count),bounds=[vertices[:,:2].min(axis=0).tolist(),vertices[:,:2].max(axis=0).tolist()],image=str(out/'glow-reference.png'))))
