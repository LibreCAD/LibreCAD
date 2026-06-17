#!/usr/bin/env python3
"""Generate a static adjacent-reference DWG/DXF coverage inventory.

This complements ``dwgts_coverage_inventory.py`` by comparing libdxfrw's local
dispatch/class surfaces against nearby reference implementations:

* ACadSharp: fixed DWG type enum plus custom reader cases.
* ezdxf: registered DXF entity/object ``DXFTYPE`` declarations.
* LibreDWG: fixed and known custom DWG type names from ``include/dwg.h``.

The report is intentionally static source analysis.  It identifies planning
gaps and validation priorities; it does not prove byte-level DWG correctness.
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path


DEFAULT_OUTPUT = Path("libraries/libdxfrw/DWG_REFERENCE_COVERAGE_STATUS.md")


@dataclass(frozen=True)
class RefRow:
    source: str
    area: str
    family: str
    key: str
    name: str
    libdxfrw_status: str
    priority: str
    implementation_slice: str
    evidence: str


def read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        raise SystemExit(f"error: cannot read {path}: {exc}") from exc


def extract_function(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        return ""
    body_start = text.find("{", start)
    if body_start < 0:
        return ""
    depth = 0
    for index in range(body_start, len(text)):
        ch = text[index]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    return text[start:]


def normalize_name(name: str) -> str:
    name = name.strip().upper()
    name = name.removeprefix("DWG_TYPE_")
    if name.startswith("_") and len(name) > 1 and name[1].isdigit():
        name = name[1:]
    replacements = {
        "FACE3D": "3DFACE",
        "_3DFACE": "3DFACE",
        "SOLID3D": "3DSOLID",
        "_3DSOLID": "3DSOLID",
        "DIMENSION_ANG_2_LN": "DIMENSION_ANG2LN",
        "DIMENSION_ANG_3_PT": "DIMENSION_ANG3PT",
        "DIMENSION_ANG2LN": "DIMENSION_ANG2LN",
        "DIMENSION_ANG3PT": "DIMENSION_ANG3PT",
        "BLOCK_HEADER": "BLOCK_RECORD",
        "BLOCK_CONTROL_OBJ": "BLOCK_CONTROL",
        "LAYER_CONTROL_OBJ": "LAYER_CONTROL",
        "STYLE_CONTROL_OBJ": "STYLE_CONTROL",
        "LTYPE_CONTROL_OBJ": "LTYPE_CONTROL",
        "VIEW_CONTROL_OBJ": "VIEW_CONTROL",
        "UCS_CONTROL_OBJ": "UCS_CONTROL",
        "VPORT_CONTROL_OBJ": "VPORT_CONTROL",
        "APPID_CONTROL_OBJ": "APPID_CONTROL",
        "DIMSTYLE_CONTROL_OBJ": "DIMSTYLE_CONTROL",
        "VP_ENT_HDR_CTRL_OBJ": "VX_CONTROL",
        "VP_ENT_HDR": "VX_TABLE_RECORD",
        "PLACEHOLDER": "ACDBPLACEHOLDER",
        "PROXY_ENTITY": "ACAD_PROXY_ENTITY",
        "PROXY_OBJECT": "ACAD_PROXY_OBJECT",
        "TABLE": "ACAD_TABLE",
        "MULTILEADER": "MULTILEADER",
        "MLEADER": "MULTILEADER",
        "NURBSURFACE": "NURBSSURFACE",
        "DICTIONARYWDFLT": "ACDBDICTIONARYWDFLT",
        "LAYERFILTER": "LAYER_FILTER",
        "PDFREFERENCE": "PDFUNDERLAY",
        "DGNREFERENCE": "DGNUNDERLAY",
        "DWFREFERENCE": "DWFUNDERLAY",
    }
    return replacements.get(name, name)


def candidate_names(name: str) -> set[str]:
    normalized = normalize_name(name)
    out = {normalized}
    extra = {
        "3DFACE": {"FACE3D", "_3DFACE"},
        "3DSOLID": {"SOLID3D", "_3DSOLID"},
        "MULTILEADER": {"MLEADER"},
        "NURBSSURFACE": {"NURBSURFACE"},
        "ACDBDICTIONARYWDFLT": {"DICTIONARYWDFLT"},
        "ACDBPLACEHOLDER": {"PLACEHOLDER"},
        "ACAD_TABLE": {"TABLE"},
        "PDFUNDERLAY": {"PDFREFERENCE"},
        "DGNUNDERLAY": {"DGNREFERENCE"},
        "DWFUNDERLAY": {"DWFREFERENCE"},
    }
    out.update(extra.get(normalized, set()))
    if normalized.startswith("ACDB"):
        out.add(normalized[4:])
    else:
        out.add("ACDB" + normalized)
    return out


def normalized_set(names: set[str]) -> set[str]:
    out: set[str] = set()
    for name in names:
        out.update(candidate_names(name))
    return out


def parse_number(value: str) -> int:
    value = value.strip().rstrip(",")
    if value.lower().startswith("0x"):
        return int(value, 16)
    return int(value, 10)


def extract_cpp_cases(block: str) -> set[int]:
    return {int(number) for number in re.findall(r"\bcase\s+(\d+)\s*:", block)}


def extract_cpp_string_names(block: str) -> set[str]:
    names = set(
        re.findall(
            r"\b(?:nextentity|nextobject|rn|recName|className)\s*==\s*\"([^\"]+)\"",
            block,
        )
    )
    names.update(re.findall(r"\{\s*\"([A-Za-z0-9_:]+)\"\s*,\s*\"AcDb", block))
    return names


def extract_class_table_names(text: str) -> set[str]:
    return set(re.findall(r"\{\s*\"([A-Za-z0-9_:]+)\"\s*,\s*\"AcDb", text))


def extract_acadsharp_fixed_types(root: Path) -> dict[int, str]:
    enum_path = root / "src/ACadSharp/Types/ObjectType.cs"
    if not enum_path.is_file():
        return {}
    text = read_text(enum_path)
    body_match = re.search(r"public\s+enum\s+ObjectType[^{]*\{(.*?)\n\s*\}", text, re.S)
    if not body_match:
        return {}
    value = -1
    out: dict[int, str] = {}
    for raw_line in body_match.group(1).splitlines():
        line = raw_line.split("//", 1)[0].strip()
        if not line or line.startswith("["):
            continue
        match = re.match(r"([A-Za-z0-9_]+)\s*(?:=\s*([^,]+))?,?", line)
        if not match:
            continue
        name, explicit = match.groups()
        if explicit is not None:
            value = parse_number(explicit)
        else:
            value += 1
        if value >= 0:
            out[value] = normalize_name(name)
    return out


def extract_acadsharp_custom_cases(root: Path) -> set[str]:
    names: set[str] = set()
    src = root / "src"
    if not src.is_dir():
        return names
    for path in sorted(src.rglob("*.cs")):
        text = read_text(path)
        for name in re.findall(r"case\s+\"([A-Za-z0-9_:]+)\"\s*:", text):
            if "." not in name:
                names.add(normalize_name(name))
    return names


def extract_ezdxf_types(root: Path) -> set[str]:
    names: set[str] = set()
    src = root / "src/ezdxf/entities"
    if not src.is_dir():
        return names
    for path in sorted(src.rglob("*.py")):
        text = read_text(path)
        names.update(normalize_name(n) for n in re.findall(r"DXFTYPE\s*=\s*[\"']([^\"']+)[\"']", text))
    return names


def extract_libredwg_types(root: Path) -> dict[int, str]:
    path = root / "include/dwg.h"
    if not path.is_file():
        return {}
    text = read_text(path)
    enum_match = re.search(r"typedef\s+enum\s+_?Dwg_Object_Type\s*\{(.*?)\n\s*\}\s*Dwg_Object_Type", text, re.S)
    if not enum_match:
        enum_match = re.search(r"DWG_TYPE_UNUSED\s*=.*?// all known UNHANDLED classes", text, re.S)
    if not enum_match:
        return {}
    body = enum_match.group(1) if enum_match.lastindex else enum_match.group(0)
    value = -1
    out: dict[int, str] = {}
    for raw_line in body.splitlines():
        line = raw_line.split("//", 1)[0].split("/*", 1)[0].strip()
        if not line:
            continue
        match = re.match(r"DWG_TYPE_([A-Za-z0-9_]+)\s*(?:=\s*([^,]+))?,?", line)
        if not match:
            continue
        name, explicit = match.groups()
        if explicit is not None:
            value = parse_number(explicit)
        else:
            value += 1
        if value >= 0:
            out[value] = normalize_name(name)
    return out


def local_surfaces(repo: Path) -> dict[str, object]:
    libdxfrw = read_text(repo / "libraries/libdxfrw/src/libdxfrw.cpp")
    dwg_reader = read_text(repo / "libraries/libdxfrw/src/intern/dwgreader.cpp")

    entity_fn = extract_function(dwg_reader, "bool dwgReader::readDwgEntity")
    object_fn = extract_function(dwg_reader, "bool dwgReader::readDwgObject")
    dxf_entities_fn = extract_function(libdxfrw, "bool dxfRW::processEntities")
    dxf_objects_fn = extract_function(libdxfrw, "bool dxfRW::processObjects")

    class_names = normalized_set(extract_class_table_names(libdxfrw))
    dxf_entities = normalized_set(extract_cpp_string_names(dxf_entities_fn)) - {
        "ENDSEC",
        "ENDBLK",
        "SEQEND",
    }
    dxf_objects = normalized_set(extract_cpp_string_names(dxf_objects_fn)) - {"ENDSEC"}
    dwg_names = normalized_set(extract_cpp_string_names(entity_fn) | extract_cpp_string_names(object_fn))
    dxf_table_records = normalized_set(
        {
            "APPID",
            "BLOCK",
            "BLOCK_RECORD",
            "DIMSTYLE",
            "ENDBLK",
            "LAYER",
            "LTYPE",
            "STYLE",
            "UCS",
            "VIEW",
            "VPORT",
        }
    )
    dwg_table_object_ids = {
        0x30,  # BLOCK_CONTROL
        0x31,  # BLOCK_RECORD
        0x32,  # LAYER_CONTROL
        0x33,  # LAYER
        0x34,  # STYLE_CONTROL
        0x35,  # STYLE
        0x38,  # LTYPE_CONTROL
        0x39,  # LTYPE
        0x3C,  # VIEW_CONTROL
        0x3D,  # VIEW
        0x3E,  # UCS_CONTROL
        0x3F,  # UCS
        0x40,  # VPORT_CONTROL
        0x41,  # VPORT
        0x42,  # APPID_CONTROL
        0x43,  # APPID
        0x44,  # DIMSTYLE_CONTROL
        0x45,  # DIMSTYLE
        0x46,  # VX_CONTROL / VP_ENT_HDR_CTRL
    }
    dwg_child_entity_ids = {
        0x04,  # BLOCK
        0x05,  # ENDBLK
        0x0A,  # VERTEX_2D
        0x0B,  # VERTEX_3D
        0x0C,  # VERTEX_MESH
        0x0D,  # VERTEX_PFACE
        0x0E,  # VERTEX_PFACE_FACE
    }

    return {
        "dwg_entity_cases": extract_cpp_cases(entity_fn) | dwg_child_entity_ids,
        "dwg_object_cases": extract_cpp_cases(object_fn) | dwg_table_object_ids,
        "dwg_names": dwg_names,
        "dxf_entities": dxf_entities | dxf_table_records,
        "dxf_objects": dxf_objects,
        "dxf_sections": {"CLASSES"} if "bool dxfRW::processClasses" in libdxfrw else set(),
        "class_names": class_names,
    }


ENTITY_FIXED_IDS = set(range(1, 42)) | {44, 45, 46, 47, 77, 78, 101, 498}
OBJECT_FIXED_IDS = set(range(42, 83)) | {499}


def area_for_fixed_id(type_id: int) -> str:
    if type_id in ENTITY_FIXED_IDS:
        return "DWG fixed entity"
    if type_id in OBJECT_FIXED_IDS:
        return "DWG fixed object"
    return "DWG custom class"


def family_for(name: str) -> str:
    upper = normalize_name(name)
    if upper == "CLASS" or upper == "CLASSES":
        return "classes/raw-compat"
    if upper.startswith("ACDS") or "ACDSPROTOTYPE" in upper:
        return "data-storage/classes"
    if upper in {"TEXT", "MTEXT", "DIMENSION", "ARC_DIMENSION", "LARGE_RADIAL_DIMENSION"}:
        return "annotation/context"
    if "DIM" in upper or "MLEADER" in upper or upper in {"ACAD_TABLE", "TABLESTYLE", "FIELD", "FIELDLIST"}:
        return "annotation/context"
    if "POINTCLOUD" in upper or "NAVISWORKS" in upper:
        return "point-cloud/model-reference"
    if any(token in upper for token in ("UNDERLAY", "IMAGE", "WIPEOUT", "RASTER", "PDF", "DGN", "DWF")):
        return "raster-underlay"
    if any(token in upper for token in ("SURFACE", "ACSH", "3DSOLID", "BODY", "REGION", "MESH", "ACIS")):
        return "3D/modeler"
    if upper.startswith("ASSOC") or "PARAMETER" in upper or "ACTION" in upper or "GRIP" in upper:
        return "parametric/dynamic-block"
    if any(token in upper for token in ("MATERIAL", "VISUAL", "RENDER", "BACKGROUND", "SUN", "LIGHT")):
        return "render/material"
    if upper.startswith("GEO") or "MAP" in upper:
        return "geospatial"
    if "PROXY" in upper or "OLE" in upper:
        return "proxy/embedded"
    if upper in {
        "BLOCK",
        "ENDBLK",
        "SEQEND",
        "DICTIONARY",
        "XRECORD",
        "GROUP",
        "LAYOUT",
        "STYLE",
        "LAYER",
        "LTYPE",
        "DIMSTYLE",
        "VPORT",
        "VIEW",
        "UCS",
        "APPID",
        "BLOCK_RECORD",
        "ACDBPLACEHOLDER",
    } or upper.endswith("_CONTROL"):
        return "database/table"
    if upper.startswith("ACAM") or upper.startswith("ACA") or upper.startswith("AEC"):
        return "vertical/proprietary"
    return "core-geometry"


def priority_for(name: str, family: str, status: str) -> str:
    upper = normalize_name(name)
    if status.startswith("typed"):
        return "covered"
    if family in {"classes/raw-compat", "data-storage/classes"}:
        return "P0"
    p0 = {"ACDSRECORD", "ACDSSCHEMA", "ACDB:ACDSPROTOTYPE_1B"}
    p1 = {
        "ACAD_TABLE",
        "ARC_DIMENSION",
        "LARGE_RADIAL_DIMENSION",
        "LIGHT",
        "MESH",
        "MPOLYGON",
        "MULTILEADER",
        "POINTCLOUD",
        "POINTCLOUDEX",
        "POINTCLOUDDEF",
        "POINTCLOUDDEFEX",
        "POINTCLOUDDEF_REACTOR",
        "POINTCLOUDDEF_REACTOR_EX",
        "POINTCLOUDCOLORMAP",
        "NAVISWORKSMODEL",
        "NAVISWORKSMODELDEF",
    }
    if upper in p0:
        return "P0"
    if upper in p1 or family in {"point-cloud/model-reference"}:
        return "P1"
    if family in {
        "annotation/context",
        "raster-underlay",
        "3D/modeler",
        "parametric/dynamic-block",
        "proxy/embedded",
        "render/material",
        "geospatial",
    }:
        return "P2"
    if family == "vertical/proprietary":
        return "P4"
    return "P3"


def implementation_slice_for(name: str, family: str, priority: str) -> str:
    upper = normalize_name(name)
    if priority == "covered":
        return "no immediate plan item"
    if family == "classes/raw-compat":
        return "P0: parse/re-emit DXF CLASSES with class aliases and instance counts"
    if upper.startswith("ACDS") or "ACDSPROTOTYPE" in upper:
        return "P0: preserve AC1027 data-storage sections before typed conversion"
    if priority == "P1" and upper == "LIGHT":
        return "P1: typed DXF/DWG shell and callback/write-path parity"
    if family == "point-cloud/model-reference":
        return "P1: raw/classed shells plus handle/owner preservation for external refs"
    if upper in {"MESH", "MPOLYGON", "ACAD_TABLE", "MULTILEADER", "ARC_DIMENSION", "LARGE_RADIAL_DIMENSION"}:
        return "P1: typed DXF/DWG shell and callback/write-path parity"
    if family == "raster-underlay":
        return "P2: unify image/underlay definition-reference-reactor graphs"
    if family == "3D/modeler":
        return "P2: ACIS/SAB raw shell first, typed geometry later"
    if family == "parametric/dynamic-block":
        return "P2: preserve assoc/eval/dynamic-block object graphs"
    if family == "annotation/context":
        return "P2: preserve annotation context and object-context data"
    if family == "render/material":
        return "P2: preserve render/material environment objects"
    if family == "geospatial":
        return "P2: preserve geodata/object-context dependencies"
    if family == "proxy/embedded":
        return "P2: preserve proxy/OLE payloads and owners"
    if family == "vertical/proprietary":
        return "P4: defer proprietary vertical classes to raw same-version replay"
    return "P3: classify, preserve raw, then decide typed support"


def dxf_status(name: str, surface: set[str], class_names: set[str]) -> str:
    candidates = candidate_names(name)
    if candidates & surface:
        return "typed-dispatch"
    if candidates & class_names:
        return "raw-preserved-classed"
    return "not-dispatched"


def dwg_status(type_id: int | None, name: str, local: dict[str, object]) -> str:
    candidates = candidate_names(name)
    if type_id is not None:
        if type_id in local["dwg_entity_cases"] or type_id in local["dwg_object_cases"]:  # type: ignore[operator]
            return "typed-fixed-dispatch"
    if candidates & local["dwg_names"]:  # type: ignore[operator]
        return "named-dwg-route"
    if candidates & local["class_names"]:  # type: ignore[operator]
        return "dxf-classed-raw"
    return "not-named"


def add_row(
    rows: list[RefRow],
    source: str,
    area: str,
    key: str,
    name: str,
    status: str,
    evidence: str,
) -> None:
    family = family_for(name)
    priority = priority_for(name, family, status)
    rows.append(
        RefRow(
            source=source,
            area=area,
            family=family,
            key=key,
            name=normalize_name(name),
            libdxfrw_status=status,
            priority=priority,
            implementation_slice=implementation_slice_for(name, family, priority),
            evidence=evidence,
        )
    )


def build_rows(repo: Path, acadsharp: Path, ezdxf: Path, libredwg: Path) -> list[RefRow]:
    local = local_surfaces(repo)
    rows: list[RefRow] = []

    for type_id, name in sorted(extract_acadsharp_fixed_types(acadsharp).items()):
        if name in {"INVALID", "UNDEFINED", "UNLISTED"} or name.startswith("UNKNOW"):
            continue
        add_row(
            rows,
            "ACadSharp",
            area_for_fixed_id(type_id),
            str(type_id),
            name,
            dwg_status(type_id, name, local),
            "../ACadSharp/src/ACadSharp/Types/ObjectType.cs",
        )

    for name in sorted(extract_acadsharp_custom_cases(acadsharp)):
        add_row(
            rows,
            "ACadSharp",
            "DWG custom class",
            name,
            name,
            dwg_status(None, name, local),
            "../ACadSharp/src/** case \"...\"",
        )

    for name in sorted(extract_ezdxf_types(ezdxf)):
        if name in {"DXFENTITY", "DXFGFX", "CLASS"}:
            continue
        status = dxf_status(name, local["dxf_entities"] | local["dxf_objects"], local["class_names"])  # type: ignore[operator]
        area = "DXF record"
        add_row(
            rows,
            "ezdxf",
            area,
            name,
            name,
            status,
            "../ezdxf/src/ezdxf/entities/**/*.py DXFTYPE",
        )

    for type_id, name in sorted(extract_libredwg_types(libredwg).items()):
        if name in {"UNUSED"} or name.startswith("UNKNOWN"):
            continue
        area = area_for_fixed_id(type_id)
        add_row(
            rows,
            "LibreDWG",
            area,
            str(type_id),
            name,
            dwg_status(type_id, name, local),
            "../libredwg/include/dwg.h",
        )

    add_row(
        rows,
        "ODA spec",
        "DWG data section",
        "AcDb:AcDsPrototype_1b",
        "AcDb:AcDsPrototype_1b",
        "raw-preserved",
        "~/doc/dwg/dwg.pdf and local DWGTS_COVERAGE_STATUS.md",
    )
    classes_status = (
        "typed-dispatch"
        if "CLASSES" in local["dxf_sections"]  # type: ignore[operator]
        else "not-dispatched"
    )
    add_row(
        rows,
        "ezdxf",
        "DXF section",
        "CLASSES",
        "CLASSES",
        classes_status,
        "../ezdxf/src/ezdxf/sections/classes.py",
    )
    return deduplicate_rows(rows)


def deduplicate_rows(rows: list[RefRow]) -> list[RefRow]:
    best: dict[tuple[str, str, str, str], RefRow] = {}
    status_rank = {
        "typed-fixed-dispatch": 0,
        "typed-dispatch": 0,
        "named-dwg-route": 1,
        "raw-preserved-classed": 2,
        "dxf-classed-raw": 2,
        "raw-preserved": 3,
        "not-dispatched": 4,
        "not-named": 4,
    }
    for row in rows:
        key = (row.source, row.area, row.key, row.name)
        old = best.get(key)
        if old is None or status_rank.get(row.libdxfrw_status, 99) < status_rank.get(old.libdxfrw_status, 99):
            best[key] = row
    return sorted(best.values(), key=lambda r: (r.source, r.area, r.priority, r.family, r.name, r.key))


def count_by(rows: list[RefRow], *attrs: str) -> list[tuple[tuple[str, ...], int]]:
    counts: dict[tuple[str, ...], int] = {}
    for row in rows:
        key = tuple(getattr(row, attr) for attr in attrs)
        counts[key] = counts.get(key, 0) + 1
    return sorted(counts.items())


def markdown_table(rows: list[RefRow]) -> str:
    lines = [
        "| Source | Area | Family | Key | Name | libdxfrw status | Priority | Implementation slice | Evidence |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        lines.append(
            f"| {row.source} | {row.area} | {row.family} | `{row.key}` | `{row.name}` | "
            f"{row.libdxfrw_status} | {row.priority} | {row.implementation_slice} | {row.evidence} |"
        )
    return "\n".join(lines)


def render_count_table(rows: list[RefRow]) -> str:
    lines = [
        "| Source | Area | libdxfrw status | Count |",
        "| --- | --- | --- | ---: |",
    ]
    for (source, area, status), count in count_by(rows, "source", "area", "libdxfrw_status"):
        lines.append(f"| {source} | {area} | {status} | {count} |")
    return "\n".join(lines)


def render_family_table(rows: list[RefRow]) -> str:
    gap_status = {"not-dispatched", "not-named", "raw-preserved", "raw-preserved-classed", "dxf-classed-raw"}
    gaps = [row for row in rows if row.libdxfrw_status in gap_status and row.priority != "covered"]
    lines = [
        "| Priority | Family | Count | Primary slice |",
        "| --- | --- | ---: | --- |",
    ]
    grouped: dict[tuple[str, str, str], int] = {}
    for row in gaps:
        key = (row.priority, row.family, row.implementation_slice)
        grouped[key] = grouped.get(key, 0) + 1
    for (priority, family, implementation_slice), count in sorted(grouped.items()):
        lines.append(f"| {priority} | {family} | {count} | {implementation_slice} |")
    return "\n".join(lines)


def availability_line(label: str, path: Path | None) -> str:
    if path is not None and path.exists():
        return f"- {label}: `{path}`"
    return f"- {label}: not found"


def find_oda(default: str | None) -> Path | None:
    candidates = []
    if default:
        candidates.append(Path(default).expanduser())
    candidates.extend(
        [
            Path("/Applications/ODAFileConverter.app/Contents/MacOS/ODAFileConverter"),
            Path("/Applications/ODA File Converter.app/Contents/MacOS/ODAFileConverter"),
        ]
    )
    which = shutil.which("ODAFileConverter")
    if which:
        candidates.append(Path(which))
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def generate_markdown(
    rows: list[RefRow],
    repo: Path,
    acadsharp: Path,
    ezdxf: Path,
    libredwg: Path,
    oda: Path | None,
) -> str:
    gap_status = {"not-dispatched", "not-named", "raw-preserved", "raw-preserved-classed", "dxf-classed-raw"}
    high_priority = [
        row
        for row in rows
        if row.priority in {"P0", "P1", "P2"} and row.libdxfrw_status in gap_status
    ]
    high_priority.sort(key=lambda r: (r.priority, r.family, r.source, r.name, r.key))

    lines = [
        "# Adjacent Reference Coverage Status",
        "",
        "Generated by `scripts/dwg_reference_coverage_inventory.py`.",
        "",
        "This inventory refines the DWG/DXF roadmap against adjacent references",
        "without treating any one project as authoritative.  The ODA PDF remains",
        "the byte-layout authority; these projects are used to expose neighboring",
        "feature families, class names, and practical validation gaps.",
        "",
        "## Inputs",
        "",
        f"- LibreCAD root: `{repo}`",
        availability_line("ACadSharp", acadsharp),
        availability_line("ezdxf", ezdxf),
        availability_line("LibreDWG", libredwg),
        availability_line("ODA File Converter", oda),
        "",
        "## Status Semantics",
        "",
        "- `typed-fixed-dispatch`: fixed DWG type is explicitly dispatched by libdxfrw.",
        "- `typed-dispatch`: DXF record is explicitly parsed by libdxfrw.",
        "- `named-dwg-route`: DWG custom class name is recognized, usually as typed metadata or raw replay.",
        "- `raw-preserved-classed` / `dxf-classed-raw`: class metadata exists, but semantic editing is not implemented.",
        "- `not-dispatched` / `not-named`: the adjacent reference exposes a class/record not named by libdxfrw's current surfaces.",
        "",
        "## Summary",
        "",
        render_count_table(rows),
        "",
        "## Refined Implementation Slices",
        "",
        render_family_table(rows),
        "",
        "## Implementation-Ready Plan",
        "",
        "1. Keep P0 structural gates first: AC1027 data-storage preservation, DWG frame/page validation, and external-reader validation must stay ahead of new typed feature work.",
        "2. Make DXF `CLASSES` and raw/custom handle remapping part of that P0 spine: custom entities/objects must carry class aliases, owner graphs, and instance counts before semantic widening.",
        "3. Implement P1 feature shells before deep geometry: MESH, MPOLYGON, ACAD_TABLE/MULTILEADER adjunct data, point-cloud/Navisworks references, LIGHT, and large/arc dimension adjunct records need classed raw preservation, stable owners, and DXF/DWG write hooks.",
        "4. Fold adjacent P2 families into shared preservation paths: raster/underlay/image graphs, annotation context data, ACIS/SAB surface shells, render/material objects, geodata, proxy/OLE payloads, and assoc/eval/dynamic-block graphs should use common raw-object and owner-graph infrastructure.",
        "5. Defer P4 vertical/proprietary classes to same-version raw replay unless there is a concrete LibreCAD editing surface; do not widen callbacks for AEC/Mechanical classes without fixtures and ODA/libreDWG validation.",
        "6. Graduate DWG write gates one version at a time: keep AC1015 hard-gated today, then promote AC1018, AC1024, and AC1027 only after external `dwgread`/ODA smoke validation is green.",
        "7. Add every new typed slice with a paired validation fixture: libdxfrw round trip, ezdxf audit for DXF, libreDWG read for DWG where reliable, and optional/local ODA conversion/audit for interop.",
        "",
        "## Oracle Pitfalls",
        "",
        "- ACadSharp is useful for ownership and feature-shape comparison, but the ODA PDF remains the byte-layout authority.",
        "- ezdxf is the strongest DXF tag/class oracle here, but some registered types are intentionally partial or read-only.",
        "- LibreDWG exposes a broad class registry and a practical external decoder, but warnings are noisy and R2010+ coverage is not proof of writer correctness.",
        "- ODA File Converter is a black-box smoke validator: a successful conversion means the file is readable enough, not that unknown/custom bytes or DWG-only sections round-trip losslessly.",
        "",
        "## P0/P1/P2 Gaps",
        "",
        markdown_table(high_priority),
        "",
        "## Full Inventory",
        "",
        markdown_table(rows),
        "",
    ]
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--acadsharp-root", type=Path, default=None)
    parser.add_argument("--ezdxf-root", type=Path, default=None)
    parser.add_argument("--libredwg-root", type=Path, default=None)
    parser.add_argument("--oda-file-converter", default=None)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true", help="fail if output is stale")
    args = parser.parse_args(argv)

    repo = args.repo_root.resolve()
    acadsharp = (args.acadsharp_root or (repo.parent / "ACadSharp")).resolve()
    ezdxf = (args.ezdxf_root or (repo.parent / "ezdxf")).resolve()
    libredwg = (args.libredwg_root or (repo.parent / "libredwg")).resolve()
    if not acadsharp.is_dir():
        raise SystemExit(f"error: ACadSharp root not found: {acadsharp}")
    if not ezdxf.is_dir():
        raise SystemExit(f"error: ezdxf root not found: {ezdxf}")
    if not libredwg.is_dir():
        alt = repo.parent / "libreDWG"
        if alt.is_dir():
            libredwg = alt.resolve()
        else:
            raise SystemExit(f"error: LibreDWG root not found: {libredwg}")

    rows = build_rows(repo, acadsharp, ezdxf, libredwg)
    text = generate_markdown(
        rows,
        repo=repo,
        acadsharp=acadsharp,
        ezdxf=ezdxf,
        libredwg=libredwg,
        oda=find_oda(args.oda_file_converter),
    )
    output = args.output if args.output.is_absolute() else repo / args.output

    if args.check:
        current = output.read_text(encoding="utf-8") if output.exists() else ""
        if current != text:
            sys.stderr.write(f"error: adjacent reference inventory is stale: {output}\n")
            return 1
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")
    print(f"wrote {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
