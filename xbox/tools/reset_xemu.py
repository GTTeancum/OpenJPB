"""Reset or stop only the isolated XEMU guest through its monitor."""
import argparse
import socket

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--quit", action="store_true", help="Exit instead of resetting")
a = p.parse_args()

with socket.create_connection(("127.0.0.1", 9247), timeout=5) as monitor:
    monitor.settimeout(5)

    def response():
        data = b""
        while not data.endswith(b"(qemu) "):
            chunk = monitor.recv(4096)
            if not chunk:
                raise ConnectionError("Monitor closed")
            data += chunk
        return data

    response()
    monitor.sendall(b"quit\n" if a.quit else b"system_reset\n")
    try:
        print(response().decode(errors="replace").strip())
    except (ConnectionError, OSError):
        if not a.quit:
            raise
