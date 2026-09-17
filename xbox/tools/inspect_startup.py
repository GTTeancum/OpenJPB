"""Read Xbox startup/video diagnostic words from this project's XEMU monitor."""

import argparse
import re
import socket
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--map", type=Path, default=Path("xbox/build/OpenJPB.map"))
parser.add_argument("--port", type=int, default=9247)
args = parser.parse_args()
mapping = args.map.read_text()


def address(name: str) -> int:
    match = re.search(
        rf"(?m)^\s+0000:[0-9a-fA-F]+\s+_{name}\s+([0-9a-fA-F]+)",
        mapping,
    )
    if not match:
        raise ValueError(f"Missing symbol: {name}")
    return int(match[1], 16)


with socket.create_connection(("127.0.0.1", args.port), timeout=3) as monitor:
    monitor.settimeout(3)

    def response() -> str:
        data = b""
        while not data.endswith(b"(qemu) "):
            data += monitor.recv(16384)
        return data.decode(errors="replace")

    response()
    for name in (
        "jpb_XboxStartupPhase",
        "jpb_XboxVideoModeResult",
        "jpb_XboxGpuInitResult",
        "jpb_XboxLastLoadResult",
        "jpb_XboxLastLoadFailureCode",
        "jpb_XboxFreePagesBeforeConstructor",
        "jpb_XboxFreePagesAfterConstructor",
        "jpb_XboxConstructorFileNotFound",
        "jpb_XboxVisualLevelLoaded",
        "jpb_XboxFreePagesAfterVisualLevel",
        "jpb_XboxLevelLoadStage",
        "jpb_XboxLevelLoadBatch",
        "jpb_XboxLevelResidentVertexBytes",
        "jpb_XboxLevelFreePagesBeforeVertices",
        "jpb_XboxLevelFreePagesAfterVertices",
        "jpb_XboxSmokeFrame",
        "jpb_XboxFramePhase",
    ):
        monitor.sendall(f"x /1wx 0x{address(name):x}\n".encode())
        result = response()
        match = re.search(r":\s+0x([0-9a-fA-F]+)", result)
        print(f"{name}={int(match[1], 16) if match else result.strip()}")
