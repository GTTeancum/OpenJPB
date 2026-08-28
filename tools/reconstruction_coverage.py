#!/usr/bin/env python3
"""Measure reconstructed PDB procedure coverage without claiming parity."""

from __future__ import annotations

import argparse
import csv
import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Any


PDB_MARKER_RE = re.compile(
    r"/\*\s*0x([0-9a-f]+),.*?\*/", re.IGNORECASE | re.DOTALL
)
COMMENT_RE = re.compile(r"/\*.*?\*/|//[^\r\n]*", re.DOTALL)
RISK_RE = re.compile(
    r"\b(?:fallback|infer(?:red|ence)?|approx(?:imate|imation)?|"
    r"substitut(?:e|ion)|stub(?:bed)?|unrecover(?:ed|able)|"
    r"partial(?:ly)?)\b",
    re.IGNORECASE,
)

BUNDLED_PATH_PARTS = (
    "\\sdl2\\",
    "\\fbx\\ufbx.c",
    "\\ufbx\\",
    "\\video\\theoraplay.c",
    "\\theoraplay\\",
    "\\soundtouch\\",
)


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def load_source_map(path: Path) -> dict[tuple[str, str, str], str]:
    result: dict[tuple[str, str, str], str] = {}
    with path.open("r", encoding="utf-8", newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            result[(row["module"], row["symbol"], row["rva"].lower())] = row[
                "source"
            ]
    return result


def classify_origin(source_path: str | None) -> str:
    lowered = (source_path or "").lower()
    if "\\swjedipowerbattles\\work\\" not in lowered:
        return "toolchain"
    if any(part in lowered for part in BUNDLED_PATH_PARTS):
        return "bundled"
    return "project"


def review_claim(text: str) -> str:
    header = text[:3000].lower()
    if "complete reviewed reconstruction" in header:
        return "complete"
    if "partially reviewed reconstruction" in header or (
        "partial reviewed reconstruction" in header
    ):
        return "partial"
    if "reviewed reconstruction" in header:
        return "reviewed"
    return "unspecified"


def source_risk_terms(text: str) -> list[str]:
    terms = {match.group(0).lower() for match in RISK_RE.finditer(text)}
    return sorted(terms)


def marker_segments(text: str) -> dict[int, str]:
    matches = list(PDB_MARKER_RE.finditer(text))
    result: dict[int, str] = {}
    for index, match in enumerate(matches):
        start = match.end()
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        result[int(match.group(1), 16)] = text[start:end]
    return result


def code_has_body(code: str, symbol: str) -> bool:
    catch_handler = re.fullmatch(r"(.+)\$catch\$\d+", symbol)
    if catch_handler is not None:
        return (
            re.search(r"\bcatch\s*\(", code) is not None
            and code_has_body(code, catch_handler.group(1))
        )

    initializer = re.fullmatch(r"`dynamic initializer for '([^']+)''", symbol)
    if initializer is not None:
        definition = re.compile(
            r"(?<![A-Za-z0-9_~:])"
            + re.escape(initializer.group(1))
            + r"(?:\s*\[[^\]]*\])?\s*(?:=\s*(?:\{|[^;])|;)"
        )
        return definition.search(code) is not None

    candidates = {symbol, symbol.rsplit("::", 1)[-1]}
    candidates.update(
        re.sub(r"<[^<>]*>", "", candidate)
        for candidate in tuple(candidates)
    )
    candidates.add(
        re.sub(r"(~[^:<]+)<[^<>]*>", r"\1", symbol)
    )
    for candidate in candidates:
        if not candidate or "`" in candidate or "'" in candidate:
            continue
        pattern = re.compile(
            r"(?<![A-Za-z0-9_~:])" + re.escape(candidate) + r"\s*\("
        )
        for match in pattern.finditer(code):
            open_paren = code.find("(", match.start())
            depth = 0
            index = open_paren
            while index < len(code):
                char = code[index]
                if char == "(":
                    depth += 1
                elif char == ")":
                    depth -= 1
                    if depth == 0:
                        break
                index += 1
            if depth != 0:
                continue
            index += 1
            if re.match(r"\s*=\s*default\b", code[index:]) is not None:
                return True
            while index < len(code):
                char = code[index]
                if char == "{":
                    return True
                if char == ";":
                    break
                index += 1
    return False


def build_ledger(
    root: Path,
    functions: list[dict[str, Any]],
    source_map: dict[tuple[str, str, str], str],
    body_map: dict[tuple[str, str, str], str],
) -> dict[str, Any]:
    sources: dict[str, dict[str, Any]] = {}
    modules: dict[str, dict[str, Any]] = {}
    procedures: list[dict[str, Any]] = []

    for function in functions:
        key = (
            function["module"],
            function["name"],
            function["rva"].lower(),
        )
        relative = body_map.get(key, source_map.get(key))
        if relative is None:
            relative = f"original/{function['module']}.c"
        reconstructed = root / "src" / "reconstructed" / relative
        source_key = reconstructed.as_posix()
        if source_key not in sources:
            if reconstructed.exists():
                text = reconstructed.read_text(encoding="utf-8", errors="replace")
                sources[source_key] = {
                    "exists": True,
                    "text": text,
                    "code": COMMENT_RE.sub("", text),
                    "segments": marker_segments(text),
                    "claim": review_claim(text),
                    "risk_terms": source_risk_terms(text),
                }
            else:
                sources[source_key] = {
                    "exists": False,
                    "text": "",
                    "code": "",
                    "segments": {},
                    "claim": "missing",
                    "risk_terms": [],
                }

        source = sources[source_key]
        origin = classify_origin(function.get("source_path", ""))
        rva = int(function["rva"], 16)
        segment = source["segments"].get(rva)
        if not source["exists"]:
            state = "source_missing"
        elif code_has_body(source["code"], function["name"]):
            state = "body_present"
        elif segment is None:
            state = "marker_missing"
        else:
            state = "comment_only"

        procedure = {
            "module": function["module"],
            "name": function["name"],
            "rva": function["rva"],
            "size": int(function["size"]),
            "origin": origin,
            "state": state,
            "source": relative.replace("\\", "/"),
            "evidence_source": function.get("source_path", ""),
        }
        procedures.append(procedure)

        module = modules.setdefault(
            function["module"],
            {
                "module": function["module"],
                "source": procedure["source"],
                "claim": source["claim"],
                "risk_terms": source["risk_terms"],
                "counts": defaultdict(int),
                "bytes": defaultdict(int),
            },
        )
        count_key = f"{origin}_{state}"
        module["counts"][count_key] += 1
        module["bytes"][count_key] += procedure["size"]
        module["counts"][f"{origin}_total"] += 1
        module["bytes"][f"{origin}_total"] += procedure["size"]

    normalized_modules = []
    for module in modules.values():
        module["counts"] = dict(module["counts"])
        module["bytes"] = dict(module["bytes"])
        normalized_modules.append(module)
    normalized_modules.sort(
        key=lambda item: (
            -item["bytes"].get("project_comment_only", 0),
            -item["bytes"].get("project_marker_missing", 0),
            item["module"],
        )
    )

    totals: dict[str, int] = defaultdict(int)
    for procedure in procedures:
        totals[f"{procedure['origin']}_{procedure['state']}_count"] += 1
        totals[f"{procedure['origin']}_{procedure['state']}_bytes"] += procedure[
            "size"
        ]
        totals[f"{procedure['origin']}_total_count"] += 1
        totals[f"{procedure['origin']}_total_bytes"] += procedure["size"]

    return {
        "schema_version": 1,
        "meaning": (
            "Mechanical body coverage only. body_present does not prove an "
            "exact or reviewed reconstruction."
        ),
        "totals": dict(totals),
        "modules": normalized_modules,
        "procedures": procedures,
    }


def write_report(path: Path, ledger: dict[str, Any]) -> None:
    totals = ledger["totals"]
    lines = [
        "# Reconstruction Coverage Ledger",
        "",
        "This is a mechanical coverage inventory, not a parity claim. A present",
        "body still requires comparison against the matched PDB and executable.",
        "",
        "## Project Procedure Surface",
        "",
        f"- Procedures: {totals.get('project_total_count', 0):,}",
        f"- Bytes: {totals.get('project_total_bytes', 0):,}",
        f"- Bodies present: {totals.get('project_body_present_count', 0):,}",
        f"- Comment-only shells: {totals.get('project_comment_only_count', 0):,}",
        f"- Missing PDB markers: {totals.get('project_marker_missing_count', 0):,}",
        f"- Missing reconstructed sources: {totals.get('project_source_missing_count', 0):,}",
        "",
        "## Module Queue",
        "",
        "| Module | Claim | Bodies | Shells | Shell bytes | Marker gaps | Risk terms |",
        "|---|---|---:|---:|---:|---:|---|",
    ]
    for module in ledger["modules"]:
        counts = module["counts"]
        bytes_ = module["bytes"]
        project_total = counts.get("project_total", 0)
        if project_total == 0:
            continue
        lines.append(
            f"| `{module['module']}` | {module['claim']} | "
            f"{counts.get('project_body_present', 0)}/{project_total} | "
            f"{counts.get('project_comment_only', 0)} | "
            f"{bytes_.get('project_comment_only', 0):,} | "
            f"{counts.get('project_marker_missing', 0)} | "
            f"{', '.join(module['risk_terms']) or '-'} |"
        )

    open_procedures = [
        procedure
        for procedure in ledger["procedures"]
        if procedure["origin"] == "project"
        and procedure["state"] != "body_present"
    ]
    open_procedures.sort(key=lambda item: (-item["size"], item["rva"]))
    lines.extend(
        [
            "",
            "## Largest Mechanical Gaps",
            "",
            "| Module | Procedure | RVA | Bytes | State |",
            "|---|---|---:|---:|---|",
        ]
    )
    for procedure in open_procedures[:100]:
        lines.append(
            f"| `{procedure['module']}` | `{procedure['name']}` | "
            f"{procedure['rva']} | {procedure['size']:,} | "
            f"{procedure['state']} |"
        )
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument(
        "--functions", type=Path, default=Path("inventory/functions.json")
    )
    parser.add_argument(
        "--function-map", type=Path, default=Path("inventory/function_map.tsv")
    )
    parser.add_argument(
        "--body-map",
        type=Path,
        default=Path("inventory/reconstruction_body_map.tsv"),
    )
    parser.add_argument(
        "--output", type=Path, default=Path("inventory/reconstruction_coverage.json")
    )
    parser.add_argument(
        "--report", type=Path, default=Path("docs/PDB_COVERAGE_LEDGER.md")
    )
    args = parser.parse_args()

    root = args.root.resolve()
    ledger = build_ledger(
        root,
        load_json(args.functions),
        load_source_map(args.function_map),
        load_source_map(args.body_map) if args.body_map.exists() else {},
    )
    args.output.write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")
    write_report(args.report, ledger)
    print(
        f"Wrote {len(ledger['modules'])} modules and "
        f"{len(ledger['procedures'])} procedures."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
