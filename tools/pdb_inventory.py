#!/usr/bin/env python3
"""Build a reproducible reconstruction inventory from game.exe + game.pdb.

Only Python's standard library is required. llvm-pdbutil supplies CodeView
records; this tool joins them with PE section RVAs and emits stable JSON/TSV,
a human-readable report, and one source shell per original game module.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import struct
import subprocess
import sys
import uuid
from dataclasses import dataclass
from pathlib import Path, PureWindowsPath
from typing import Any, Iterable


CORE_OBJECT_MARKER = "\\winver\\obj\\x64\\steam_release\\"
PROJECT_ROOT_MARKER = "\\swjedipowerbattles\\"
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
GENERATED_SCAFFOLD_MARKER = (
    "GENERATED RECONSTRUCTION SHELL - no function bodies recovered here."
)

MODULE_RE = re.compile(r"^\s*Mod\s+(\d+)\s+\|\s+`([^`]+)`:")
PROC_RE = re.compile(
    r"^\s*\d+\s+\|\s+S_([GL])PROC32(?:_ID)?\s+\[.*\]\s+`(.*)`\s*$"
)
DATA_RE = re.compile(
    r"^\s*\d+\s+\|\s+S_([GL])DATA32\s+\[.*\]\s+`(.*)`\s*$"
)
ADDRESS_RE = re.compile(
    r"addr\s*=\s*(\d+):(\d+),\s*code size\s*=\s*(\d+)"
)
DATA_INFO_RE = re.compile(
    r"type\s*=\s*(0x[0-9A-Fa-f]+)\s+\((.*?)\),"
    r"\s*addr\s*=\s*(\d+):(\d+)"
)
TYPE_RE = re.compile(r"type\s*=\s*`(0x[0-9A-Fa-f]+)\s+\((.*)\)`")
LOCAL_RE = re.compile(r"^\s*\d+\s+\|\s+S_LOCAL\s+\[.*\]\s+`(.*)`\s*$")
LOCAL_TYPE_RE = re.compile(
    r"type\s*=\s*(0x[0-9A-Fa-f]+)\s+\((.*?)\),\s*flags\s*=\s*(.*)"
)
FILE_RE = re.compile(
    r"^-\s+\(SHA-256:\s*([0-9A-Fa-f]{64})\)\s+(.*)$"
)
LINE_FILE_RE = re.compile(
    r"^(.*?)\s+\(SHA-256:\s*([0-9A-Fa-f]{64})\)\s*$"
)
LINE_RANGE_RE = re.compile(
    r"^\s+(\d+):([0-9A-Fa-f]+)-([0-9A-Fa-f]+),"
    r"\s*line/addr entries\s*=\s*(\d+)"
)


@dataclass(frozen=True)
class Section:
    index: int
    name: str
    virtual_address: int
    virtual_size: int
    raw_address: int
    raw_size: int


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def is_core_object(path: str) -> bool:
    windows = PureWindowsPath(path)
    return (
        CORE_OBJECT_MARKER in path.lower()
        and windows.suffix.lower() == ".obj"
    )


def read_pe(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    if data[:2] != b"MZ":
        raise ValueError(f"{path} is not a PE executable")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError(f"{path} has no PE signature")

    coff = pe_offset + 4
    machine, section_count, _, _, _, optional_size, _ = struct.unpack_from(
        "<HHIIIHH", data, coff
    )
    optional = coff + 20
    magic = struct.unpack_from("<H", data, optional)[0]
    if magic == 0x20B:
        image_base = struct.unpack_from("<Q", data, optional + 24)[0]
        directory_count_offset = optional + 108
        directories_offset = optional + 112
    elif magic == 0x10B:
        image_base = struct.unpack_from("<I", data, optional + 28)[0]
        directory_count_offset = optional + 92
        directories_offset = optional + 96
    else:
        raise ValueError(f"unsupported PE optional-header magic 0x{magic:X}")

    section_table = optional + optional_size
    sections: list[Section] = []
    for index in range(section_count):
        offset = section_table + index * 40
        name = data[offset : offset + 8].split(b"\0", 1)[0].decode(
            "ascii", errors="replace"
        )
        virtual_size, virtual_address, raw_size, raw_address = struct.unpack_from(
            "<IIII", data, offset + 8
        )
        sections.append(
            Section(
                index=index + 1,
                name=name,
                virtual_address=virtual_address,
                virtual_size=virtual_size,
                raw_address=raw_address,
                raw_size=raw_size,
            )
        )

    def rva_to_file_offset(rva: int) -> int:
        for section in sections:
            extent = max(section.virtual_size, section.raw_size)
            if section.virtual_address <= rva < section.virtual_address + extent:
                return section.raw_address + (rva - section.virtual_address)
        raise ValueError(f"RVA 0x{rva:X} is outside PE sections")

    codeview: dict[str, Any] | None = None
    directory_count = struct.unpack_from("<I", data, directory_count_offset)[0]
    if directory_count > 6:
        debug_rva, debug_size = struct.unpack_from(
            "<II", data, directories_offset + 6 * 8
        )
        if debug_rva and debug_size:
            debug_offset = rva_to_file_offset(debug_rva)
            for item_offset in range(
                debug_offset, debug_offset + debug_size, 28
            ):
                (
                    _,
                    _,
                    _,
                    _,
                    debug_type,
                    size_of_data,
                    _,
                    pointer_to_raw_data,
                ) = struct.unpack_from("<IIHHIIII", data, item_offset)
                if debug_type != 2 or size_of_data < 24:
                    continue
                record = data[
                    pointer_to_raw_data : pointer_to_raw_data + size_of_data
                ]
                if record[:4] != b"RSDS":
                    continue
                guid = str(uuid.UUID(bytes_le=record[4:20])).upper()
                age = struct.unpack_from("<I", record, 20)[0]
                pdb_path = record[24:].split(b"\0", 1)[0].decode(
                    "utf-8", errors="replace"
                )
                codeview = {"guid": guid, "age": age, "pdb_path": pdb_path}
                break

    return {
        "machine": f"0x{machine:04X}",
        "image_base": image_base,
        "sections": [section.__dict__ for section in sections],
        "codeview": codeview,
    }


def run_pdbutil(pdbutil: Path, pdb: Path, *options: str) -> str:
    command = [str(pdbutil), "dump", *options, str(pdb)]
    result = subprocess.run(
        command,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8",
        errors="replace",
    )
    if result.returncode:
        raise RuntimeError(
            f"llvm-pdbutil failed ({result.returncode}): "
            + result.stderr.strip()
        )
    return result.stdout


def parse_pdb_summary(text: str) -> dict[str, Any]:
    def field(name: str) -> str:
        match = re.search(rf"^\s*{re.escape(name)}:\s*(.*?)\s*$", text, re.M)
        if not match:
            raise ValueError(f"PDB summary is missing {name!r}")
        return match.group(1)

    guid = field("GUID").strip("{}").upper()
    return {
        "guid": guid,
        "age": int(field("Age")),
        "has_debug_info": field("Has Debug Info").lower() == "true",
        "has_types": field("Has Types").lower() == "true",
        "has_ids": field("Has IDs").lower() == "true",
        "has_globals": field("Has Globals").lower() == "true",
        "has_publics": field("Has Publics").lower() == "true",
        "is_stripped": field("Is stripped").lower() == "true",
    }


def parse_symbols(
    text: str, sections: dict[int, Section], image_base: int
) -> tuple[
    list[dict[str, Any]],
    list[dict[str, Any]],
    list[dict[str, Any]],
]:
    modules: list[dict[str, Any]] = []
    functions: list[dict[str, Any]] = []
    globals_: list[dict[str, Any]] = []
    current_module: dict[str, Any] | None = None
    current_function: dict[str, Any] | None = None
    pending_local: dict[str, Any] | None = None
    pending_data: dict[str, Any] | None = None

    def flush_function() -> None:
        nonlocal current_function
        if current_function is not None:
            functions.append(current_function)
            current_function = None

    for line in text.splitlines():
        module_match = MODULE_RE.match(line)
        if module_match:
            flush_function()
            module_id = int(module_match.group(1))
            object_path = module_match.group(2)
            current_module = {
                "id": module_id,
                "object_path": object_path,
                "object_name": PureWindowsPath(object_path).name,
                "stem": PureWindowsPath(object_path).stem,
                "language": None,
                "is_core": is_core_object(object_path),
                "files": [],
            }
            modules.append(current_module)
            pending_local = None
            pending_data = None
            continue

        if current_module is None:
            continue

        language_match = re.search(r"\blanguage\s*=\s*([^,\s]+)", line)
        if language_match and current_module["language"] is None:
            current_module["language"] = language_match.group(1).lower()

        data_match = DATA_RE.match(line)
        if data_match:
            pending_data = {
                "origin": "module_symbols",
                "module_id": current_module["id"],
                "module": current_module["stem"],
                "linkage": (
                    "global" if data_match.group(1) == "G" else "local"
                ),
                "name": data_match.group(2),
                "type_index": None,
                "type": None,
                "section": None,
                "section_offset": None,
                "rva": None,
                "va": None,
            }
            globals_.append(pending_data)
            continue

        if pending_data is not None:
            data_info_match = DATA_INFO_RE.search(line)
            if data_info_match:
                section_index = int(data_info_match.group(3))
                section_offset = int(data_info_match.group(4), 10)
                section = sections.get(section_index)
                pending_data.update(
                    {
                        "type_index": data_info_match.group(1).upper(),
                        "type": data_info_match.group(2),
                        "section": section_index,
                        "section_offset": section_offset,
                    }
                )
                if section is not None:
                    rva = section.virtual_address + section_offset
                    pending_data["rva"] = rva
                    pending_data["va"] = image_base + rva
                pending_data = None
                continue

        proc_match = PROC_RE.match(line)
        if proc_match:
            flush_function()
            current_function = {
                "module_id": current_module["id"],
                "module": current_module["stem"],
                "linkage": "global" if proc_match.group(1) == "G" else "local",
                "name": proc_match.group(2),
                "section": None,
                "section_offset": None,
                "rva": None,
                "va": None,
                "size": None,
                "type_index": None,
                "signature": None,
                "locals": [],
                "line_ranges": [],
                "source_path": None,
            }
            pending_local = None
            continue

        if current_function is None:
            continue

        address_match = ADDRESS_RE.search(line)
        if address_match and current_function["rva"] is None:
            section_index = int(address_match.group(1))
            section_offset = int(address_match.group(2), 10)
            code_size = int(address_match.group(3), 10)
            section = sections.get(section_index)
            current_function["section"] = section_index
            current_function["section_offset"] = section_offset
            current_function["size"] = code_size
            if section is not None:
                rva = section.virtual_address + section_offset
                current_function["rva"] = rva
                current_function["va"] = image_base + rva
            continue

        type_match = TYPE_RE.search(line)
        if type_match and current_function["signature"] is None:
            current_function["type_index"] = type_match.group(1).upper()
            current_function["signature"] = type_match.group(2)
            continue

        local_match = LOCAL_RE.match(line)
        if local_match:
            pending_local = {"name": local_match.group(1)}
            current_function["locals"].append(pending_local)
            continue

        if pending_local is not None:
            local_type_match = LOCAL_TYPE_RE.search(line)
            if local_type_match:
                pending_local.update(
                    {
                        "type_index": local_type_match.group(1).upper(),
                        "type": local_type_match.group(2),
                        "flags": local_type_match.group(3).strip(),
                    }
                )
                pending_local = None

    flush_function()
    return modules, functions, globals_


def parse_global_data(
    text: str, sections: dict[int, Section], image_base: int
) -> list[dict[str, Any]]:
    symbols: list[dict[str, Any]] = []
    pending: dict[str, Any] | None = None

    for line in text.splitlines():
        data_match = DATA_RE.match(line)
        if data_match:
            pending = {
                "origin": "global_stream",
                "module_id": None,
                "module": None,
                "linkage": (
                    "global" if data_match.group(1) == "G" else "local"
                ),
                "name": data_match.group(2),
                "type_index": None,
                "type": None,
                "section": None,
                "section_offset": None,
                "rva": None,
                "va": None,
            }
            continue

        if pending is None:
            continue
        data_info_match = DATA_INFO_RE.search(line)
        if not data_info_match:
            continue

        section_index = int(data_info_match.group(3))
        section_offset = int(data_info_match.group(4), 10)
        section = sections.get(section_index)
        pending.update(
            {
                "type_index": data_info_match.group(1).upper(),
                "type": data_info_match.group(2),
                "section": section_index,
                "section_offset": section_offset,
            }
        )
        if section is not None:
            rva = section.virtual_address + section_offset
            pending["rva"] = rva
            pending["va"] = image_base + rva
            symbols.append(pending)
        pending = None

    return symbols


def merge_data_symbols(
    linked: Iterable[dict[str, Any]],
    module_owned: Iterable[dict[str, Any]],
) -> list[dict[str, Any]]:
    by_identity = {
        (item["name"], item["rva"]): item
        for item in linked
        if item["rva"] is not None
    }
    for item in module_owned:
        if item["rva"] is not None:
            by_identity[(item["name"], item["rva"])] = item
    return sorted(
        by_identity.values(),
        key=lambda item: (item["rva"], item["name"], item["linkage"]),
    )


def parse_files(text: str) -> dict[int, list[dict[str, str]]]:
    result: dict[int, list[dict[str, str]]] = {}
    current_module: int | None = None
    for line in text.splitlines():
        module_match = MODULE_RE.match(line)
        if module_match:
            current_module = int(module_match.group(1))
            result.setdefault(current_module, [])
            continue
        file_match = FILE_RE.match(line)
        if file_match and current_module is not None:
            result[current_module].append(
                {
                    "sha256": file_match.group(1).upper(),
                    "path": file_match.group(2),
                }
            )
    return result


def parse_line_ranges(
    text: str, sections: dict[int, Section]
) -> list[dict[str, Any]]:
    ranges: list[dict[str, Any]] = []
    current_module: int | None = None
    current_source: str | None = None
    for line in text.splitlines():
        module_match = MODULE_RE.match(line)
        if module_match:
            current_module = int(module_match.group(1))
            current_source = None
            continue
        source_match = LINE_FILE_RE.match(line)
        if source_match and not line.lstrip().startswith("- ("):
            current_source = source_match.group(1)
            continue
        range_match = LINE_RANGE_RE.match(line)
        if range_match and current_module is not None and current_source:
            section_index = int(range_match.group(1))
            section = sections.get(section_index)
            if section is None:
                continue
            start = section.virtual_address + int(range_match.group(2), 16)
            end = section.virtual_address + int(range_match.group(3), 16)
            ranges.append(
                {
                    "module_id": current_module,
                    "source_path": current_source,
                    "section": section_index,
                    "start_rva": start,
                    "end_rva": end,
                    "entry_count": int(range_match.group(4)),
                }
            )
    return ranges


def choose_source(module: dict[str, Any]) -> str | None:
    project_sources = [
        item["path"]
        for item in module["files"]
        if PROJECT_ROOT_MARKER in item["path"].lower()
        and PureWindowsPath(item["path"]).suffix.lower() in SOURCE_SUFFIXES
    ]
    stem = module["stem"].lower()
    matching = [
        path
        for path in project_sources
        if PureWindowsPath(path).stem.lower() == stem
    ]
    candidates = matching or project_sources
    if not candidates:
        return None
    candidates.sort(
        key=lambda path: (
            "\\work\\include\\" in path.lower(),
            len(PureWindowsPath(path).parts),
            path.lower(),
        )
    )
    return candidates[0]


def scaffold_relative_path(module: dict[str, Any]) -> Path:
    source = module.get("primary_source")
    if source:
        windows = PureWindowsPath(source)
        lower_parts = [part.lower() for part in windows.parts]
        if "work" in lower_parts:
            index = lower_parts.index("work")
            relative = Path(*windows.parts[index + 1 :])
            if relative.parts:
                return relative
    suffix = ".c" if module.get("language") == "c" else ".cpp"
    return Path(module["stem"] + suffix)


def attach_line_ranges(
    functions: list[dict[str, Any]], ranges: list[dict[str, Any]]
) -> None:
    by_module: dict[int, list[dict[str, Any]]] = {}
    for item in ranges:
        by_module.setdefault(item["module_id"], []).append(item)
    for function in functions:
        start = function.get("rva")
        size = function.get("size")
        if start is None or size is None:
            continue
        end = start + size
        overlaps = []
        for item in by_module.get(function["module_id"], []):
            if item["start_rva"] < end and item["end_rva"] > start:
                overlaps.append(item)
        if overlaps:
            function["line_ranges"] = overlaps
            function["source_path"] = overlaps[0]["source_path"]


def hexify_function(function: dict[str, Any]) -> dict[str, Any]:
    result = dict(function)
    for key in ("rva", "va", "section_offset"):
        if result.get(key) is not None:
            result[key] = f"0x{result[key]:X}"
    result["line_ranges"] = [
        {
            **item,
            "start_rva": f"0x{item['start_rva']:X}",
            "end_rva": f"0x{item['end_rva']:X}",
        }
        for item in function["line_ranges"]
    ]
    return result


def hexify_data_symbol(symbol: dict[str, Any]) -> dict[str, Any]:
    result = dict(symbol)
    for key in ("rva", "va", "section_offset"):
        if result.get(key) is not None:
            result[key] = f"0x{result[key]:X}"
    return result


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, sort_keys=False) + "\n", encoding="utf-8"
    )


def tsv_clean(value: Any) -> str:
    return str(value if value is not None else "").replace("\t", " ").replace(
        "\n", " "
    )


def write_function_map(
    path: Path,
    functions: Iterable[dict[str, Any]],
    module_lookup: dict[int, dict[str, Any]],
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(["va", "rva", "size", "module", "source", "symbol"])
        for function in functions:
            module = module_lookup[function["module_id"]]
            relative = Path("original") / scaffold_relative_path(module)
            writer.writerow(
                [
                    f"0x{function['va']:X}",
                    f"0x{function['rva']:X}",
                    function["size"],
                    tsv_clean(function["module"]),
                    relative.as_posix(),
                    tsv_clean(function["name"]),
                ]
            )


def write_scaffold(
    root: Path,
    modules: list[dict[str, Any]],
    functions: list[dict[str, Any]],
) -> None:
    functions_by_module: dict[int, list[dict[str, Any]]] = {}
    for function in functions:
        functions_by_module.setdefault(function["module_id"], []).append(function)

    generated_paths: set[Path] = set()
    for module in modules:
        relative = scaffold_relative_path(module)
        output = root / "original" / relative
        if output in generated_paths:
            output = output.with_name(
                f"{output.stem}_mod{module['id']:04d}{output.suffix}"
            )
        generated_paths.add(output)
        if output.exists():
            existing = output.read_text(encoding="utf-8", errors="replace")
            if GENERATED_SCAFFOLD_MARKER not in existing:
                print(f"Preserving reviewed source: {output}", flush=True)
                continue
        output.parent.mkdir(parents=True, exist_ok=True)
        module_functions = sorted(
            functions_by_module.get(module["id"], []),
            key=lambda function: function["rva"] or 0,
        )
        lines = [
            "/*",
            f" * {GENERATED_SCAFFOLD_MARKER}",
            f" * PDB module: {module['id']:04d}",
            f" * Object: {module['object_path']}",
            f" * Primary source: {module.get('primary_source') or 'unknown'}",
            f" * Compiler language: {module.get('language') or 'unknown'}",
            f" * Emitted procedures: {len(module_functions)}",
            " *",
            " * Use inventory/function_map.tsv with ExportReconstruction.java.",
            " */",
            "",
        ]
        for function in module_functions:
            signature = function.get("signature") or "unknown"
            local_count = len(function["locals"])
            source = function.get("source_path") or "no line mapping"
            lines.extend(
                [
                    f"/* 0x{function['rva']:X}, {function['size']} bytes, "
                    f"{function['linkage']}, {local_count} named locals",
                    f" * {function['name']}",
                    f" * PDB type: {signature}",
                    f" * Source: {source}",
                    " */",
                    "",
                ]
            )
        output.write_text("\n".join(lines), encoding="utf-8")


def merged_coverage_bytes(ranges: Iterable[tuple[int, int]]) -> int:
    ordered = sorted(ranges)
    if not ordered:
        return 0
    total = 0
    start, end = ordered[0]
    for next_start, next_end in ordered[1:]:
        if next_start <= end:
            end = max(end, next_end)
        else:
            total += end - start
            start, end = next_start, next_end
    return total + end - start


def write_report(
    path: Path,
    pair: dict[str, Any],
    core_modules: list[dict[str, Any]],
    core_functions: list[dict[str, Any]],
    linked_globals: list[dict[str, Any]],
    core_globals: list[dict[str, Any]],
    line_ranges: list[dict[str, Any]],
) -> None:
    functions_with_lines = sum(
        1 for function in core_functions if function["line_ranges"]
    )
    procedure_bytes = sum(function["size"] or 0 for function in core_functions)
    covered_intersections: list[tuple[int, int]] = []
    for function in core_functions:
        if function["rva"] is None or function["size"] is None:
            continue
        fn_start = function["rva"]
        fn_end = fn_start + function["size"]
        for item in function["line_ranges"]:
            start = max(fn_start, item["start_rva"])
            end = min(fn_end, item["end_rva"])
            if start < end:
                covered_intersections.append((start, end))
    covered_bytes = merged_coverage_bytes(covered_intersections)
    locals_count = sum(len(function["locals"]) for function in core_functions)
    unique_project_files = {
        item["path"].lower()
        for module in core_modules
        for item in module["files"]
        if PROJECT_ROOT_MARKER in item["path"].lower()
    }
    unique_sources = {
        item["path"].lower()
        for module in core_modules
        for item in module["files"]
        if PROJECT_ROOT_MARKER in item["path"].lower()
        and PureWindowsPath(item["path"]).suffix.lower() in SOURCE_SUFFIXES
    }
    core_module_ids = {module["id"] for module in core_modules}
    lines = [
        "# PDB reconstruction inventory",
        "",
        f"- Pairing: **{'exact match' if pair['matches'] else 'MISMATCH'}**",
        f"- GUID: `{pair['pdb']['guid']}`",
        f"- Age: `{pair['pdb']['age']}`",
        f"- Core modules: **{len(core_modules):,}**",
        f"- Core procedures: **{len(core_functions):,}**",
        f"- Linked data symbols: **{len(linked_globals):,}**",
        f"- Data symbols attributed directly to core modules: "
        f"**{len(core_globals):,}**",
        f"- Named parameters/locals: **{locals_count:,}**",
        f"- Procedures intersecting direct `DEBUG_S_LINES` ranges: "
        f"**{functions_with_lines:,} / "
        f"{len(core_functions):,}**",
        f"- Procedure bytes intersecting direct line ranges: "
        f"**{covered_bytes:,} / "
        f"{procedure_bytes:,}**",
        f"- Unique project files referenced: **{len(unique_project_files):,}**",
        f"- Unique project source files referenced: **{len(unique_sources):,}**",
        f"- Core line ranges: **{sum(1 for item in line_ranges if item['module_id'] in core_module_ids):,}**",
        "",
        "This report is generated from the reference binary and PDB. The module",
        "source files are organizational shells, not claims of recovered source.",
        "",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--pdb", type=Path, required=True)
    parser.add_argument("--pdbutil", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=Path("inventory"))
    parser.add_argument(
        "--scaffold", type=Path, default=Path("src/reconstructed")
    )
    args = parser.parse_args()

    for label, path in (
        ("EXE", args.exe),
        ("PDB", args.pdb),
        ("llvm-pdbutil", args.pdbutil),
    ):
        if not path.is_file():
            parser.error(f"{label} does not exist: {path}")

    print("Reading PE identity and sections...", flush=True)
    pe = read_pe(args.exe)
    sections = {
        item["index"]: Section(**item) for item in pe["sections"]
    }

    print("Reading PDB summary...", flush=True)
    pdb_summary = parse_pdb_summary(
        run_pdbutil(args.pdbutil, args.pdb, "--summary")
    )
    codeview = pe["codeview"]
    matches = bool(
        codeview
        and codeview["guid"] == pdb_summary["guid"]
        and codeview["age"] == pdb_summary["age"]
    )
    pair = {
        "matches": matches,
        "exe": {
            "path": str(args.exe.resolve()),
            "sha256": sha256_file(args.exe),
            "machine": pe["machine"],
            "image_base": f"0x{pe['image_base']:X}",
            "codeview": codeview,
        },
        "pdb": {
            "path": str(args.pdb.resolve()),
            "sha256": sha256_file(args.pdb),
            **pdb_summary,
        },
    }
    if not matches:
        write_json(args.output / "pairing.json", pair)
        raise RuntimeError(
            "EXE CodeView identity does not match the supplied PDB"
        )

    print("Parsing module symbols and named locals...", flush=True)
    modules, functions, globals_ = parse_symbols(
        run_pdbutil(args.pdbutil, args.pdb, "--symbols"),
        sections,
        pe["image_base"],
    )
    print("Parsing source-file checksums...", flush=True)
    files_by_module = parse_files(
        run_pdbutil(args.pdbutil, args.pdb, "--files")
    )
    for module in modules:
        module["files"] = files_by_module.get(module["id"], [])
        module["primary_source"] = choose_source(module)

    print("Parsing source-line ranges...", flush=True)
    line_ranges = parse_line_ranges(
        run_pdbutil(args.pdbutil, args.pdb, "-l"), sections
    )
    attach_line_ranges(functions, line_ranges)
    print("Parsing linked global data symbols...", flush=True)
    global_stream_data = parse_global_data(
        run_pdbutil(args.pdbutil, args.pdb, "--globals"),
        sections,
        pe["image_base"],
    )

    core_modules = [module for module in modules if module["is_core"]]
    core_module_ids = {module["id"] for module in core_modules}
    core_functions = [
        function
        for function in functions
        if function["module_id"] in core_module_ids
        and function["rva"] is not None
    ]
    core_globals = [
        symbol
        for symbol in globals_
        if symbol["module_id"] in core_module_ids
        and symbol["rva"] is not None
    ]
    linked_globals = merge_data_symbols(global_stream_data, core_globals)
    module_lookup = {module["id"]: module for module in modules}

    print("Writing inventory and source scaffold...", flush=True)
    write_json(args.output / "pairing.json", pair)
    write_json(args.output / "modules.json", core_modules)
    write_json(
        args.output / "functions.json",
        [hexify_function(function) for function in core_functions],
    )
    write_json(
        args.output / "globals.json",
        [hexify_data_symbol(symbol) for symbol in linked_globals],
    )
    write_json(
        args.output / "line_ranges.json",
        [
            {
                **item,
                "start_rva": f"0x{item['start_rva']:X}",
                "end_rva": f"0x{item['end_rva']:X}",
            }
            for item in line_ranges
            if item["module_id"] in core_module_ids
        ],
    )
    write_function_map(
        args.output / "function_map.tsv", core_functions, module_lookup
    )
    write_scaffold(args.scaffold, core_modules, core_functions)
    write_report(
        args.output / "REPORT.md",
        pair,
        core_modules,
        core_functions,
        linked_globals,
        core_globals,
        line_ranges,
    )

    print(
        f"Exact pair verified; emitted {len(core_modules)} modules and "
        f"{len(core_functions)} core procedures plus "
        f"{len(linked_globals)} linked data symbols.",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
