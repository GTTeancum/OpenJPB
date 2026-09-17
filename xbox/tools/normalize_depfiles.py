"""Repair Windows Clang dependency paths before nxdk's GNU Make reads them."""
from pathlib import Path
import re


root = Path(__file__).resolve().parents[1]
for dep in root.rglob("*.d"):
    contents = dep.read_text()
    fixed = re.sub(r"\\(?=[^\s])", "/", contents)
    if fixed != contents:
        dep.write_text(fixed)
