#!/usr/bin/env python3
#
# This file is part of the LibreCAD project, a 2D CAD program
#
# Copyright (C) 2026 LibreCAD (librecad.org)
# Copyright (C) 2026 Dongxu Li (github.com/dxli)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Generate a deterministic DWG/DXF dispatch inventory.

The inventory is source analysis, not a byte-level conformance claim.  It
records the local libdxfrw dispatch surfaces and compares DXF registrations and
DWG fixed/custom names with the adjacent TypeScript parser and writer trees.
Rows are deliberately classified as typed, raw-preserved, or unsupported so a
missing fixture cannot be mistaken for feature completeness.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

from dwg_inventory_common import (
    candidate_names,
    extract_function,
    family_for,
    local_reader_surfaces,
    markdown_cell,
    normalize_name,
    read_text,
    repo_root_from_script,
    write_or_check,
)
from dwg_version_inventory import dispatch_by_code


DEFAULT_OUTPUT = Path("libraries/libdxfrw/DWG_DXF_COVERAGE_INVENTORY.md")

LICENSE_HEADER = """<!--
This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2026 LibreCAD (librecad.org)
Copyright (C) 2026 Dongxu Li (github.com/dxli)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
-->

"""


def register_names(root: Path, function_name: str) -> set[str]:
    """Collect literal registerEntity/registerObject names from a TS tree."""
    names: set[str] = set()
    source = root / "src/dxf"
    if not source.is_dir():
        return names
    pattern = re.compile(
        rf"\b{function_name}\(\s*['\"]([^'\"]+)['\"]"
    )
    for path in sorted(source.rglob("*.ts")):
        names.update(normalize_name(name) for name in pattern.findall(read_text(path)))
    return names


def fixed_dwg_names(root: Path) -> dict[int, str]:
    """Read the reference fixed DXF-name map keyed by DWG object type."""
    path = root / "src/dwg/sections/DwgObjectsReader.ts"
    if not path.is_file():
        return {}
    text = read_text(path)
    match = re.search(
        r"const\s+FIXED_DXF_NAMES\s*=\s*new Map<number, string>\(\[(.*?)\]\);",
        text,
        re.S,
    )
    if not match:
        return {}
    return {
        int(number): normalize_name(name)
        for number, name in re.findall(
            r"\[\s*(\d+)\s*,\s*['\"]([^'\"]+)['\"]\s*\]", match.group(1)
        )
    }


def fixed_object_ids(root: Path) -> set[int]:
    """Collect reference non-entity parser keys from DwgObjectsReader."""
    path = root / "src/dwg/sections/DwgObjectsReader.ts"
    if not path.is_file():
        return set()
    text = read_text(path)
    match = re.search(
        r"const\s+NON_ENTITY_OBJECT_PARSERS\s*=\s*new Map<number, NonEntityObjectParser>\(\[(.*?)\]\);",
        text,
        re.S,
    )
    if not match:
        return set()
    return {int(number) for number in re.findall(r"\[\s*(\d+)\s*,", match.group(1))}


def fixed_object_names(root: Path) -> dict[int, str]:
    """Read semantic non-entity names for reference fixed object IDs."""
    path = root / "src/dwg/sections/DwgObjectsReader.ts"
    if not path.is_file():
        return {}
    text = read_text(path)
    match = re.search(
        r"const\s+NON_ENTITY_OBJECT_PARSERS\s*=\s*new Map<number, NonEntityObjectParser>\(\[(.*?)\]\);",
        text,
        re.S,
    )
    if not match:
        return {}
    return {
        int(number): normalize_name(name)
        for number, name in re.findall(
            r"\[\s*(\d+)\s*,.*?objectTypeName:\s*['\"]([^'\"]+)['\"]",
            match.group(1),
        )
    }


def custom_class_names(root: Path) -> set[str]:
    """Collect intentional raw custom class names from the TS helper."""
    path = root / "src/helpers/rawCustom.ts"
    if not path.is_file():
        return set()
    text = read_text(path)
    match = re.search(
        r"INTENTIONAL_RAW_CUSTOM_CLASSES\s*=\s*new Map<string, [^{]+>\(\[(.*?)\]\);",
        text,
        re.S,
    )
    if not match:
        return set()
    return {
        normalize_name(name)
        for name in re.findall(r"['\"]([A-Z][A-Z0-9_]+)['\"]\s*,", match.group(1))
    }


def interface_callbacks(repo: Path) -> tuple[int, int]:
    interface_text = read_text(repo / "libraries/libdxfrw/src/drw_interface.h")
    filter_text = read_text(repo / "librecad/src/lib/filters/rs_filterdxfrw.cpp")
    callbacks = set(re.findall(r"virtual void\s+(add\w+)\s*\(", interface_text))
    overrides = set(re.findall(r"RS_FilterDXFRW::(add\w+)\s*\(", filter_text))
    return len(callbacks), len(callbacks & overrides)


def local_dxf_names(repo: Path) -> tuple[set[str], set[str]]:
    surfaces = local_reader_surfaces(repo)
    return set(surfaces["dxf_entities"]), set(surfaces["dxf_objects"])


def local_fixed_dwg_ids(repo: Path) -> tuple[set[int], set[int]]:
    """Resolve enum-qualified reader cases to their fixed DWG IDs.

    The reader dispatches through ``dwgType::`` and ``dwgObjType::`` rather
    than numeric case labels.  Inventorying the enum definitions keeps this
    report coupled to the actual local type table instead of duplicating a
    second hard-coded map in the script.
    """
    util = read_text(repo / "libraries/libdxfrw/src/intern/dwgutil.h")
    source = read_text(repo / "libraries/libdxfrw/src/intern/dwgreader.cpp")
    base_header = read_text(repo / "libraries/libdxfrw/src/drw_base.h")
    objects_header = read_text(repo / "libraries/libdxfrw/src/drw_objects.h")

    def enum_values(namespace: str, enum_name: str) -> dict[str, int]:
        match = re.search(
            rf"namespace\s+{namespace}\s*\{{\s*enum\s+{enum_name}\s*\{{(.*?)\n\s*\}};",
            util,
            re.S,
        )
        if not match:
            return {}
        return {
            name: int(value)
            for name, value in re.findall(
                r"\b([A-Za-z0-9_]+)\s*=\s*(-?\d+)", match.group(1)
            )
        }

    entity_values = enum_values("dwgType", "Entity")
    object_values = enum_values("dwgObjType", "Object")
    entities = extract_function(source, "bool dwgReader::readDwgEntity(")
    objects = extract_function(source, "bool dwgReader::readDwgObject(")
    blocks = extract_function(source, "bool dwgReader::readDwgBlocks(")
    legacy_polyline = extract_function(
        source, "DwgMappedEntityOutcome dwgReader::stageLegacyPolylineChain("
    )
    tables = extract_function(source, "bool dwgReader::readDwgTables(")

    def cases(function: str, namespace: str, values: dict[str, int]) -> set[int]:
        return {
            values[name]
            for name in re.findall(
                rf"\bcase\s+{namespace}::([A-Za-z0-9_]+)\s*:", function
            )
            if name in values
        }

    def referenced_entity_types(function: str) -> set[int]:
        return {
            entity_values[name]
            for name in re.findall(r"\bdwgType::([A-Za-z0-9_]+)\b", function)
            if name in entity_values
        }

    def table_object_types() -> set[int]:
        """Resolve R2000 table IDs only when the reader uses the table route.

        ``readDwgTables`` has one shared parser for table controls and records,
        so they do not appear in the normal fixed-object switch.  The shared
        fixed-type constants and reader descriptors are the authoritative
        in-tree contract; requiring both parser calls prevents a declaration
        without a reader route from being reported as read support.
        """
        if not tables:
            return set()

        values = {
            name: int(value, 0)
            for name, value in re.findall(
                r"inline\s+constexpr\s+std::int16_t\s+"
                r"(Dwg[A-Za-z0-9_]+ObjectType)\s*=\s*"
                r"(0x[0-9A-Fa-f]+|\d+)",
                base_header,
            )
        }
        descriptors = {
            name: (control, record)
            for name, control, record in re.findall(
                r"constexpr\s+DwgTableDescriptor\s+(k[A-Za-z0-9_]+)\s*"
                r"\{\s*DRW::(Dwg[A-Za-z0-9_]+ObjectType)\s*,\s*"
                r"DRW::(Dwg[A-Za-z0-9_]+ObjectType)",
                source,
            )
        }
        parsed_controls = set(re.findall(
            r"parseControl\s*\(\s*oc\s*,\s*(k[A-Za-z0-9_]+)", tables
        ))
        parsed_records = set(re.findall(
            r"parseTableRecord\s*\(\s*oc\s*,\s*(k[A-Za-z0-9_]+)", tables
        ))
        table_types = {
            values[type_name]
            for descriptor in parsed_controls & parsed_records
            for type_name in descriptors.get(descriptor, ())
            if type_name in values
        }
        control_match = re.search(
            r"kDwgControlType\s*=\s*(0x[0-9A-Fa-f]+|\d+)", objects_header
        )
        record_match = re.search(
            r"kDwgType\s*=\s*(0x[0-9A-Fa-f]+|\d+)", objects_header
        )
        if "DRW_ViewportEntityHeader::kDwgControlType" in tables and control_match:
            table_types.add(int(control_match.group(1), 0))
            if record_match:
                table_types.add(int(record_match.group(1), 0))
        return table_types

    return (
        cases(entities, "dwgType", entity_values)
        | referenced_entity_types(blocks)
        | referenced_entity_types(legacy_polyline),
        cases(objects, "dwgObjType", object_values)
        | cases(entities, "dwgObjType", object_values)
        | table_object_types(),
    )


def reference_names(root: Path) -> dict[str, set[str]]:
    return {
        "dxf_entities": register_names(root, "registerEntity"),
        "dxf_objects": register_names(root, "registerObject"),
        "custom_classes": custom_class_names(root),
    }


def local_custom_names(repo: Path) -> set[str]:
    surfaces = local_reader_surfaces(repo)
    return set(surfaces["dwg_names"]) | set(surfaces["dwg_typed_names"]) | set(surfaces["class_names"])


def names_match(name: str, candidates: set[str]) -> bool:
    """Match class names that differ only by DWG/DXF separator spelling."""
    key = re.sub(r"[^A-Z0-9]", "", normalize_name(name))
    return any(
        re.sub(r"[^A-Z0-9]", "", normalize_name(candidate)) == key
        for candidate in candidates
    )


def is_dynamic_block_name(name: str) -> bool:
    """Mirror DRW_DynamicBlockObject::isDynamicBlockRecName()."""
    if name in {"BLOCK", "BLOCK_HEADER", "BLOCK_CONTROL", "BLOCK_RECORD"}:
        return False
    if "BLOCK" not in name:
        return False
    return (
        name.endswith(("PARAMETER", "ACTION", "GRIP", "COMPONENT"))
        or "GRIPLOCATIONCOMPONENT" in name
        or "PROXYNODE" in name
        or "PURGEPREVENTER" in name
        or "PARAMDEPENDENCYBODY" in name
        or "PROPERTIESTABLE" in name
        or "REPRESENTATION" in name
    )


def has_dynamic_block_route(name: str, surfaces: dict[str, object]) -> bool:
    return bool(surfaces["has_dwg_dynamic_block_callback"]) and any(
        is_dynamic_block_name(candidate) for candidate in candidate_names(name)
    )


def has_associative_route(name: str, surfaces: dict[str, object]) -> bool:
    if not surfaces["has_dwg_associative_callback"]:
        return False
    return any(
        candidate.startswith("ACDBASSOC")
        or candidate == "ACDBPERSSUBENTMANAGER"
        or (surfaces["has_dwg_centerline_associative_callback"]
            and candidate == "ACDBCENTERLINEACTIONBODY")
        for candidate in candidate_names(name)
    )


def has_acsh_history_route(name: str, surfaces: dict[str, object]) -> bool:
    return bool(surfaces["has_dwg_acsh_history_callback"]) and any(
        candidate.startswith("ACSH_") for candidate in candidate_names(name)
    )


def local_status(domain: str, name: str, surfaces: dict[str, object]) -> str:
    if domain == "DXF entity":
        if names_match(name, surfaces["dxf_entities"]):
            return "typed-dispatch"
        return "raw-preserved" if surfaces["has_dxf_raw_entity"] else "unsupported"
    if domain == "DXF object":
        if names_match(name, surfaces["dxf_objects"]):
            return "typed-dispatch"
        return "raw-preserved" if surfaces["has_dxf_raw_object"] else "unsupported"
    if (has_dynamic_block_route(name, surfaces)
            or has_associative_route(name, surfaces)
            or has_acsh_history_route(name, surfaces)):
        return "typed-custom"
    if names_match(name, surfaces["dwg_typed_names"]):
        return "typed-custom"
    if names_match(name, surfaces["dwg_validated_raw_custom_object_names"]):
        return "raw-preserved"
    if (names_match(name, surfaces["dwg_names"])
            or names_match(name, surfaces["class_names"])):
        return "raw-or-classed"
    return "unsupported"


def registration_rows(
    repo: Path,
    references: list[Path],
    reference_surfaces: list[dict[str, set[str]]],
) -> list[tuple[str, str, str, str, str, str, str]]:
    surfaces = local_reader_surfaces(repo)
    local_entities = set(surfaces["dxf_entities"])
    local_objects = set(surfaces["dxf_objects"])
    ref_entities = set().union(*(surface["dxf_entities"] for surface in reference_surfaces)) if references else set()
    ref_objects = set().union(*(surface["dxf_objects"] for surface in reference_surfaces)) if references else set()
    local_custom = set(surfaces["dwg_names"]) | set(surfaces["dwg_typed_names"]) | set(surfaces["class_names"])
    ref_custom = set().union(*(surface["custom_classes"] for surface in reference_surfaces)) if references else set()

    rows: list[tuple[str, str, str, str, str, str, str]] = []
    for domain, local, ref in (
        ("DXF entity", local_entities, ref_entities),
        ("DXF object", local_objects, ref_objects),
        ("DWG custom class", local_custom, ref_custom),
    ):
        reference_key = "dxf_entities" if domain == "DXF entity" else "dxf_objects" if domain == "DXF object" else "custom_classes"
        for name in sorted(local | ref):
            parser = "yes" if reference_surfaces and name in reference_surfaces[0][reference_key] else "no"
            writer = "yes" if len(reference_surfaces) > 1 and name in reference_surfaces[1][reference_key] else "no"
            rows.append((domain, name, local_status(domain, name, surfaces), parser, writer, family_for(name), "fixture-needed"))
    return rows


def fixed_rows(repo: Path, references: list[Path]) -> list[tuple[str, str, str, str, str, str, str]]:
    surfaces = local_reader_surfaces(repo)
    local_entity_ids, local_object_ids = local_fixed_dwg_ids(repo)
    fixed_shell_types = (
        set(surfaces["dwg_fixed_entity_shell_types"])
        | set(surfaces["dwg_fixed_object_shell_types"])
    )
    ref_entity_maps = [fixed_dwg_names(root) for root in references]
    ref_object_ids = [fixed_object_ids(root) for root in references]
    ref_object_maps = [fixed_object_names(root) for root in references]
    fixed_ids = set().union(*(mapping.keys() for mapping in ref_entity_maps)) if ref_entity_maps else set()
    object_ids = set().union(*(ids for ids in ref_object_ids)) if references else set()
    rows: list[tuple[str, str, str, str, str, str, str]] = []
    for number in sorted(fixed_ids):
        name = next((mapping[number] for mapping in ref_entity_maps if number in mapping), f"OBJECT_{number}")
        local = "typed-fixed-dispatch" if (
            number in local_entity_ids or number in local_object_ids
        ) else ("raw-preserved" if number in fixed_shell_types
                else local_status("DWG custom class", name, surfaces))
        parser = "yes" if ref_entity_maps and number in ref_entity_maps[0] else "no"
        writer = "yes" if len(ref_entity_maps) > 1 and number in ref_entity_maps[1] else "no"
        rows.append(("DWG fixed type", f"{number}:{name}", local, parser, writer, family_for(name), "fixture-needed"))
    for number in sorted(object_ids - fixed_ids):
        name = next((mapping[number] for mapping in ref_object_maps if number in mapping), f"OBJECT_{number}")
        local = "typed-fixed-dispatch" if number in local_object_ids else local_status(
            "DWG custom class", name, surfaces
        )
        parser = "yes" if ref_object_ids and number in ref_object_ids[0] else "no"
        writer = "yes" if len(ref_object_ids) > 1 and number in ref_object_ids[1] else "no"
        rows.append(("DWG fixed object", f"{number}:{name}", local, parser, writer, family_for(name), "fixture-needed"))
    return rows


def display_path(repo: Path, path: Path) -> str:
    """Render paths relative to the checkout for reproducible output."""
    return "." if path == repo else Path(os.path.relpath(path, repo)).as_posix()


def render(repo: Path, references: list[Path]) -> str:
    surfaces = local_reader_surfaces(repo)
    callback_count, override_count = interface_callbacks(repo)
    reference_surfaces = [reference_names(root) for root in references]
    rows = fixed_rows(repo, references) + registration_rows(repo, references, reference_surfaces)
    readers, writers, _ = dispatch_by_code(repo)

    version_lines = [
        "| version | localReader | localWriter |",
        "| --- | --- | --- |",
    ]
    for version in sorted(set(readers) | set(writers)):
        version_lines.append(
            f"| `{version}` | `{readers.get(version, 'unsupported')}` | `{writers.get(version, 'unsupported')}` |"
        )

    row_lines = [
        "| domain | key | libdxfrwStatus | dwg-parser | dwg-writer | family | evidence |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        row_lines.append("| " + " | ".join(markdown_cell(value) for value in row) + " |")

    return LICENSE_HEADER + "\n".join(
        [
            "# DWG/DXF Coverage Inventory",
            "",
            "Generated by `scripts/dwg_dxf_coverage_inventory.py`; this is a deterministic source inventory, not a byte-level conformance claim.",
            "",
            "## Inputs",
            "",
            f"- libdxfrw: `{display_path(repo, repo)}`",
            *[f"- reference: `{display_path(repo, root)}`" for root in references],
            "",
            "## Local surface summary",
            "",
            f"- DWG fixed ID probes (legacy/common): entities **{len(surfaces['dwg_entity_cases'])}**, objects/tables **{len(surfaces['dwg_object_cases'])}**",
            f"- DWG named/custom routes: **{len(surfaces['dwg_names'])}**",
            f"- DWG named fixed entity/object cases: **{sum(len(ids) for ids in local_fixed_dwg_ids(repo))}**",
            f"- DWG validated fixed entity shells: **{len(surfaces['dwg_fixed_entity_shell_types'])}**",
            f"- DWG validated fixed object shells: **{len(surfaces['dwg_fixed_object_shell_types'])}**",
            f"- DXF entity names: **{len(surfaces['dxf_entities'])}**",
            f"- DXF object names: **{len(surfaces['dxf_objects'])}**",
            f"- DXF class-table names: **{len(surfaces['class_names'])}**",
            f"- DRW interface callbacks: **{callback_count}**, filter overrides: **{override_count}**",
            f"- raw DXF ENTITY fallback: **{bool(surfaces['has_dxf_raw_entity'])}**",
            f"- raw DXF OBJECT fallback: **{bool(surfaces['has_dxf_raw_object'])}**",
            "",
            "## Version gates",
            "",
            *version_lines,
            "",
            "## Registration comparison",
            "",
            *row_lines,
            "",
            "### Classification",
            "",
            "- `typed-*`: an explicit local parser/route was found in the dispatch surface.",
            "- `raw-preserved`: a checked DXF or DWG raw carrier path retains the record for same-version replay.",
            "- `raw-or-classed`: a named DWG/custom route or class shell exists; fixture proof is still required.",
            "- `unsupported`: no local route was found by this inventory.",
            "- `fixture-needed`: source registration alone does not close a feature claim.",
            "",
        ]
    )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=repo_root_from_script(__file__))
    parser.add_argument("--reference", action="append", type=Path, default=None)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="fail if the generated report is stale")
    parser.add_argument("--require-reference", action="store_true", help="fail if an adjacent reference tree is absent")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    requested = args.reference or [repo.parent / "dwg-parser", repo.parent / "dwg-writer"]
    references: list[Path] = []
    for root in requested:
        root = root.resolve()
        if root.is_dir():
            references.append(root)
        elif args.require_reference:
            raise SystemExit(f"error: reference tree not found: {root}")
        else:
            print(f"warning: reference tree not found, skipping: {root}", file=sys.stderr)

    output = args.output if args.output.is_absolute() else repo / args.output
    return write_or_check(
        output,
        render(repo, references),
        args.check,
        "DWG/DXF coverage inventory",
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
