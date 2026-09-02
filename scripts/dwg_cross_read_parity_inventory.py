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

"""Generate the LibreCAD side of the DWG/DXF cross-read parity matrix.

The report deliberately separates source-derived route facts from the
human-maintained semantic-depth seed. It can therefore be regenerated after
dispatch changes without silently upgrading a row from raw to fully typed.
When the sibling ``dwg-parser`` checkout is present, its fixed DWG map, DXF
registrations, object-family list, and class registrations are joined by the
canonical DXF name. ACadSharp is intentionally not used as a primary join
source; its custom/unlisted list is only a separate reference inventory.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT / "libraries/libdxfrw/CROSS_READ_PARITY_STATUS.md"
DEFAULT_SEED = ROOT / "libraries/libdxfrw/cross_read_parity_seed.json"
DEFAULT_DWG_PARSER = ROOT.parent / "dwg-parser"


@dataclass(frozen=True)
class RouteRow:
    format: str
    name: str
    fixed_type: str
    class_name: str
    lib_dispatched: bool
    lib_filter_override: bool
    dwg_registered: bool
    dwg_cadjson_default: bool | None
    lib_s: str
    lib_l: str
    dwg_s: str
    dwg_d: str
    direction: str
    priority: str
    notes: str
    evidence: str


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def normalize(name: str) -> str:
    value = name.strip().upper()
    aliases = {
        "_3DFACE": "3DFACE",
        "FACE3D": "3DFACE",
        "SOLID3D": "3DSOLID",
        "_3DSOLID": "3DSOLID",
        "MLEADER": "MULTILEADER",
        "NURBSURFACE": "NURBSURFACE",
        "NURBSSURFACE": "NURBSURFACE",
        "DICTIONARYWDFLT": "ACDBDICTIONARYWDFLT",
        "DICTIONARYWITHDEFAULT": "ACDBDICTIONARYWDFLT",
        "DIMENSIONASSOCIATION": "DIMASSOC",
        "EVALUATIONGRAPH": "ACAD_EVALUATION_GRAPH",
        "POINTCLOUDDEFINITION": "POINTCLOUDDEF",
        "POINTCLOUDDEFINITIONEX": "POINTCLOUDDEFEX",
        "POINTCLOUDDEFREACTOR": "POINTCLOUDDEF_REACTOR",
        "POINTCLOUDDEFREACTOREX": "POINTCLOUDDEF_REACTOR_EX",
        "NAVISWORKSMODELDEF": "NAVISWORKSMODELDEF",
        "PLACEHOLDER": "ACDBPLACEHOLDER",
        "PDFREFERENCE": "PDFUNDERLAY",
        "DGNREFERENCE": "DGNUNDERLAY",
        "DWFREFERENCE": "DWFUNDERLAY",
        "TABLE": "ACAD_TABLE",
        "DIM_ALIGNED": "DIMENSION_ALIGNED",
        "DIM_ANGULAR": "DIMENSION_ANGULAR",
        "DIM_ANGULAR3P": "DIMENSION_ANGULAR3P",
        "DIM_DIAMETRIC": "DIMENSION_DIAMETRIC",
        "DIM_LINEAR": "DIMENSION_LINEAR",
        "DIM_ORDINATE": "DIMENSION_ORDINATE",
        "DIM_RADIAL": "DIMENSION_RADIAL",
        "DIMENSION_ANG_2_LN": "DIMENSION_ANG2LN",
        "DIMENSION_ANG_3_PT": "DIMENSION_ANG3PT",
        "DIMENSION_RADIUS": "DIMENSION_RADIAL",
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
        "LWPLINE": "LWPOLYLINE",
        "BINRECORD": "AECDBBINRECORD",
        "AECBDBINRECORD": "AECDBBINRECORD",
        "AEC_CLEANUP_GROUP_DEF": "AEC_CLEANUP_GROUP",
    }
    return aliases.get(value, value)


def candidates(name: str) -> set[str]:
    value = normalize(name)
    result = {value}
    if value.startswith("ACDB"):
        result.add(value[4:])
    else:
        result.add("ACDB" + value)
    return result


def extract_function(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        return ""
    brace = text.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    return text[start:]


def interface_callbacks(text: str) -> set[str]:
    return set(re.findall(r"virtual\s+void\s+(add\w+)\s*\(", text))


def filter_overrides(cpp: str, header: str) -> set[str]:
    # The header declaration is included because an inline override is a
    # valid implementation; the qualified definition catches the normal
    # out-of-line implementation.
    return set(re.findall(r"RS_FilterDXFRW::(add\w+)\s*\(", cpp)) | set(
        re.findall(r"\bvoid\s+(add\w+)\s*\([^;]*?\)\s*override\b", header)
    )


def enum_values(text: str, enum_name: str) -> dict[str, int]:
    match = re.search(
        rf"enum\s+{re.escape(enum_name)}\s*\{{(.*?)\n\s*\}};", text, re.S
    )
    if not match:
        return {}
    values: dict[str, int] = {}
    current = -1
    for raw in match.group(1).splitlines():
        line = raw.split("//", 1)[0].strip().rstrip(",")
        if not line:
            continue
        item = re.match(r"([A-Za-z0-9_]+)\s*=\s*(-?\d+)", line)
        if item:
            current = int(item.group(2))
            values[item.group(1)] = current
            continue
        item = re.match(r"([A-Za-z0-9_]+)", line)
        if item:
            current += 1
            values[item.group(1)] = current
    return values


def local_dwg_routes(root: Path) -> tuple[dict[int, str], set[int], set[int]]:
    util = read(root / "libraries/libdxfrw/src/intern/dwgutil.h")
    reader = read(root / "libraries/libdxfrw/src/intern/dwgreader.cpp")
    objects_header = read(root / "libraries/libdxfrw/src/drw_objects.h")
    entities = enum_values(util, "Entity")
    objects = enum_values(util, "Object")
    entity_fn = extract_function(reader, "bool dwgReader::readDwgEntity(")
    object_fn = extract_function(reader, "bool dwgReader::readDwgObject(")

    def dispatched(fn: str, namespace: str, values: dict[str, int]) -> set[int]:
        return {
            values[name]
            for name in re.findall(
                rf"\bcase\s+{namespace}::([A-Za-z0-9_]+)\s*:", fn
            )
            if name in values
        }

    names = {value: normalize(name) for name, value in {**entities, **objects}.items()}
    constants = {
        name: int(value)
        for name, value in re.findall(
            r"\b(?:static\s+)?constexpr\s+(?:std::int\d+_t|int)\s+"
            r"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(\d+)",
            objects_header,
        )
    }

    def shell_routes(signature: str) -> dict[int, str]:
        function = extract_function(objects_header, signature)
        routes: dict[int, str] = {}
        for label, name in re.findall(
            r"\bcase\s+([A-Za-z_][A-Za-z0-9_]*|\d+)\s*:\s*"
            r"return\s+\"([A-Za-z0-9_:]+)\"", function
        ):
            type_id = int(label) if label.isdigit() else constants.get(label)
            if type_id is not None:
                routes[type_id] = normalize(name)
        return routes

    entity_shells = shell_routes("static const char* fixedEntityShellName")
    object_shells = shell_routes("static const char* fixedObjectShellName")
    names.update(entity_shells)
    names.update(object_shells)
    return (
        names,
        dispatched(entity_fn, "dwgType", entities) | set(entity_shells),
        dispatched(object_fn, "dwgObjType", objects) | set(object_shells),
    )


def local_dxf_routes(root: Path) -> tuple[set[str], set[str]]:
    source = read(root / "libraries/libdxfrw/src/libdxfrw.cpp")
    entities = extract_function(source, "bool dxfRW::processEntities")
    objects = extract_function(source, "bool dxfRW::processObjects")
    pattern = r"(?:nextentity|nextobject)\s*==\s*\"([^\"]+)\""
    return (
        {normalize(name) for name in re.findall(pattern, entities)},
        {normalize(name) for name in re.findall(pattern, objects)},
    )


def local_class_names(root: Path) -> dict[str, str]:
    source = read(root / "libraries/libdxfrw/src/libdxfrw.cpp")
    return {
        normalize(record): class_name
        for record, class_name in re.findall(
            r"\{\s*\"([A-Za-z0-9_:]+)\"\s*,\s*\"(AcDb[^\"]*)\"", source
        )
    }


def reference_dxf_routes(root: Path) -> tuple[set[str], set[str]]:
    entities: set[str] = set()
    objects: set[str] = set()
    source = root / "src/dxf"
    if not source.is_dir():
        return entities, objects
    for path in sorted(source.rglob("*.ts")):
        text = read(path)
        entities.update(normalize(name) for name in re.findall(
            r"\bregisterEntity\(\s*['\"]([^'\"]+)['\"]", text
        ))
        objects.update(normalize(name) for name in re.findall(
            r"\bregisterObject\(\s*['\"]([^'\"]+)['\"]", text
        ))
    return entities, objects


def reference_dwg_routes(root: Path) -> tuple[dict[int, str], set[int], set[str]]:
    path = root / "src/dwg/sections/DwgObjectsReader.ts"
    if not path.is_file():
        return {}, set(), set()
    source = read(path)
    fixed_match = re.search(
        r"const\s+FIXED_DXF_NAMES\s*=\s*new Map<number, string>\(\[(.*?)\]\);",
        source,
        re.S,
    )
    fixed: dict[int, str] = {}
    if fixed_match:
        fixed = {
            int(number): normalize(name)
            for number, name in re.findall(
                r"\[\s*(\d+)\s*,\s*['\"]([^'\"]+)['\"]\s*\]",
                fixed_match.group(1),
            )
        }
    entity_ids = {
        int(number)
        for number in re.findall(r"ENTITY_PARSERS\[(\d+)\]\s*=", source)
    }
    object_match = re.search(
        r"const\s+NON_ENTITY_OBJECT_PARSERS\s*=\s*new Map<number, NonEntityObjectParser>\(\[(.*?)\]\);",
        source,
        re.S,
    )
    object_ids: set[int] = set()
    object_names: set[str] = set()
    if object_match:
        body = object_match.group(1)
        object_ids = {int(number) for number in re.findall(r"\[\s*(\d+)\s*,", body)}
        object_names = {
            normalize(name)
            for name in re.findall(r"objectTypeName:\s*['\"]([^'\"]+)", body)
        }
    return fixed, entity_ids | object_ids, object_names


def reference_class_names(root: Path) -> set[str]:
    path = root / "src/helpers/rawCustom.ts"
    if not path.is_file():
        return set()
    source = read(path)
    match = re.search(
        r"INTENTIONAL_RAW_CUSTOM_CLASSES\s*=\s*new Map<[^>]+>\(\[(.*?)\]\);",
        source,
        re.S,
    )
    if not match:
        return set()
    return {
        normalize(name)
        for name in re.findall(r"\[\s*['\"]([^'\"]+)['\"]\s*,", match.group(1))
    }


def reference_json_keys(root: Path) -> set[str]:
    path = root / "src/api/cadToJson.ts"
    if not path.is_file():
        return set()
    source = read(path)
    match = re.search(
        r"const\s+OBJECT_FAMILIES[^=]*=\s*\[(.*?)\]\s*as\s+const;",
        source,
        re.S,
    )
    if not match:
        return set()
    return set(re.findall(r"['\"]([A-Za-z0-9]+)['\"]\s*,", match.group(1)))


def object_json_key(name: str) -> str | None:
    aliases = {
        "DICTIONARY": "dictionaries",
        "ACDBDICTIONARYWDFLT": "dictionariesWithDefault",
        "DICTIONARYVAR": "dictionaryVars",
        "GROUP": "groups",
        "XRECORD": "xRecords",
        "LAYOUT": "layouts",
        "MATERIAL": "materials",
        "DBCOLOR": "dbColors",
        "FIELD": "fields",
        "FIELDLIST": "fieldLists",
        "DIMASSOC": "dimensionAssociations",
        "ACAD_EVALUATION_GRAPH": "evaluationGraphs",
        "POINTCLOUDDEF": "pointCloudDefinitions",
        "POINTCLOUDDEFEX": "pointCloudDefinitions",
        "POINTCLOUDDEF_REACTOR": "pointCloudDefReactors",
        "POINTCLOUDDEF_REACTOR_EX": "pointCloudDefReactors",
        "POINTCLOUDCOLORMAP": "pointCloudColorMaps",
        "NAVISWORKSMODELDEF": "navisworksModelDefs",
        "RASTERVARIABLES": "rasterVariables",
        "WIPEOUTVARIABLES": "wipeoutVariables",
        "SPATIAL_FILTER": "spatialFilters",
        "SPATIAL_INDEX": "spatialIndexes",
        "LAYER_INDEX": "layerIndexes",
        "SCALE": "scales",
        "SORTENTSTABLE": "sortEntitiesTables",
        "ACDBPLACEHOLDER": "placeholders",
        "MLINESTYLE": "mlineStyles",
        "MLEADERSTYLE": "mleaderStyles",
    }
    return aliases.get(normalize(name))


def callback_for(name: str, callbacks: set[str]) -> str | None:
    explicit = {
        "3DFACE": "add3dFace",
        "3DSOLID": "addModelerGeometry",
        "ACAD_TABLE": "addTable",
        "ACDBDICTIONARYWDFLT": "addDictionaryWithDefault",
        "ACDBPLACEHOLDER": "addAcDbPlaceholder",
        "ARC_DIMENSION": "addDimArc",
        "DIMASSOC": "addDimensionAssociation",
        "DICTIONARY": "addDictionary",
        "DICTIONARYVAR": "addDictionaryVar",
        "EVALUATION_GRAPH": "addEvaluationGraph",
        "DIMENSION_ALIGNED": "addDimAlign",
        "DIMENSION_LINEAR": "addDimLinear",
        "DIMENSION_ORDINATE": "addDimOrdinate",
        "DIMENSION_RADIAL": "addDimRadial",
        "DIMENSION_DIAMETRIC": "addDimDiametric",
        "DIMENSION_ANGULAR": "addDimAngular",
        "DIMENSION_ANGULAR3P": "addDimAngular3P",
        "LARGE_RADIAL_DIMENSION": "addDimRadial",
        "IMAGEDEFREACTOR": "addImageDefinitionReactor",
        "MULTILEADER": "addMLeader",
        "NAVISWORKSMODEL": "addNavisworksModel",
        "NAVISWORKSMODELDEF": "addNavisworksModelDef",
        "POINTCLOUD": "addPointCloud",
        "POINTCLOUDEX": "addPointCloudEx",
        "POINTCLOUDDEF": "addPointCloudDef",
        "POINTCLOUDDEFEX": "addPointCloudDef",
        "POINTCLOUDDEF_REACTOR": "addPointCloudDef",
        "POINTCLOUDDEF_REACTOR_EX": "addPointCloudDef",
        "POINTCLOUDCOLORMAP": "addPointCloudColorMap",
        "SECTIONOBJECT": "addSectionObject",
        "XRECORD": "addXRecord",
        "IMAGEDEFREACTOR": "addImageDefinitionReactor",
    }
    canonical = normalize(name)
    if canonical in explicit:
        return explicit[canonical]
    compact = canonical.replace("_", "")
    for callback in callbacks:
        candidate = normalize(callback[3:]).replace("_", "")
        if candidate == compact:
            return callback
    return None


def seed_for(seed: dict[str, dict[str, str]], name: str, callback: str | None) -> dict[str, str]:
    return seed.get(name, seed.get(callback or "", {}))


def bool_cell(value: bool | None) -> str:
    return "unknown" if value is None else ("true" if value else "false")


def markdown_cell(value: object) -> str:
    return str(value).replace("|", "\\|").replace("\n", " ")


def build_rows(root: Path, dwg_parser: Path | None, seed: dict[str, dict[str, str]]) -> list[RouteRow]:
    iface = interface_callbacks(read(root / "libraries/libdxfrw/src/drw_interface.h"))
    overrides = filter_overrides(
        read(root / "librecad/src/lib/filters/rs_filterdxfrw.cpp"),
        read(root / "librecad/src/lib/filters/rs_filterdxfrw.h"),
    )
    local_dxf_entities, local_dxf_objects = local_dxf_routes(root)
    local_fixed, local_entity_ids, local_object_ids = local_dwg_routes(root)
    local_dwg_names = {
        local_fixed[type_id]
        for type_id in local_entity_ids | local_object_ids
        if type_id in local_fixed
    }
    local_classes = local_class_names(root)

    ref_dxf_entities: set[str] = set()
    ref_dxf_objects: set[str] = set()
    ref_fixed: dict[int, str] = {}
    ref_dwg_ids: set[int] = set()
    ref_object_names: set[str] = set()
    ref_classes: set[str] = set()
    json_keys: set[str] = set()
    if dwg_parser is not None and dwg_parser.is_dir():
        ref_dxf_entities, ref_dxf_objects = reference_dxf_routes(dwg_parser)
        ref_fixed, ref_dwg_ids, ref_object_names = reference_dwg_routes(dwg_parser)
        ref_classes = reference_class_names(dwg_parser)
        json_keys = reference_json_keys(dwg_parser)

    names = (
        local_dxf_entities
        | local_dxf_objects
        | set(local_fixed.values())
        | set(local_classes)
        | ref_dxf_entities
        | ref_dxf_objects
        | set(ref_fixed.values())
        | ref_classes
    )

    rows: list[RouteRow] = []
    for name in sorted(names):
        canonical = normalize(name)
        fixed = next(
            (str(number) for number, value in {**ref_fixed, **local_fixed}.items()
             if value == canonical),
            "",
        )
        callback = callback_for(canonical, iface)
        callback_override = callback in overrides if callback else False
        is_entity = canonical in local_dxf_entities or canonical in ref_dxf_entities
        is_object = canonical in local_dxf_objects or canonical in ref_dxf_objects
        if fixed:
            type_id = int(fixed)
            fmt = "dwg-fixed-entity" if type_id < 42 or type_id in local_entity_ids else "dwg-fixed-object"
        elif is_entity:
            fmt = "dxf-entity"
        elif is_object:
            fmt = "dxf-object"
        else:
            fmt = "dwg-custom"
        local_typed_route = (
            canonical in local_dxf_entities
            or canonical in local_dxf_objects
            or canonical in local_dwg_names
            or callback is not None
        )
        local_route = (
            local_typed_route
            or canonical in local_classes
        )
        reference_typed_route = (
            canonical in ref_dxf_entities
            or canonical in ref_dxf_objects
            or canonical in ref_fixed.values()
        )
        reference_route = reference_typed_route or canonical in ref_classes
        default_json: bool | None
        if is_entity or (fixed and int(fixed) < 42):
            default_json = reference_route if dwg_parser is not None else None
        elif is_object or (fixed and int(fixed) >= 42):
            key = object_json_key(canonical)
            default_json = key in json_keys if key is not None and dwg_parser is not None else None
        else:
            default_json = None
        srow = seed_for(seed, canonical, callback)
        rows.append(RouteRow(
            format=fmt,
            name=canonical,
            fixed_type=fixed,
            class_name=local_classes.get(canonical, ""),
            lib_dispatched=local_route,
            lib_filter_override=callback_override,
            dwg_registered=reference_route,
            dwg_cadjson_default=default_json,
            lib_s=srow.get("S", "T" if local_typed_route else ("R" if local_route else "U")),
            lib_l=srow.get("L", "metadata-sidecar" if callback_override else ("libdxfrw-only" if local_route else "none")),
            dwg_s=srow.get("dwg_S", "T" if reference_typed_route else ("R" if reference_route else "U")),
            dwg_d=srow.get("dwg_D", "cad-json-default" if default_json else ("cad-document" if reference_typed_route else ("raw-shell" if reference_route else "none"))),
            direction=srow.get("direction", "both" if local_route and reference_route else ("→librecad" if reference_route else "→dwg-parser")),
            priority=srow.get("priority", "P2" if local_route or reference_route else "P3"),
            notes=srow.get("notes", ""),
            evidence=srow.get("evidence", "source-dispatch-scrape"),
        ))

    # Keep interface callbacks visible even where no canonical name can be
    # inferred (for example the generic background callback). These rows are
    # explicitly marked as callbacks and do not masquerade as DXF names.
    for callback in sorted(iface):
        if any(callback_for(row.name, iface) == callback for row in rows):
            continue
        srow = seed.get(callback, {})
        rows.append(RouteRow(
            format="interface-callback",
            name=callback,
            fixed_type="",
            class_name="",
            lib_dispatched=True,
            lib_filter_override=callback in overrides,
            dwg_registered=False,
            dwg_cadjson_default=None,
            lib_s=srow.get("S", "T" if callback in overrides else "R"),
            lib_l=srow.get("L", "metadata-sidecar" if callback in overrides else "libdxfrw-only"),
            dwg_s=srow.get("dwg_S", "U"),
            dwg_d=srow.get("dwg_D", "none"),
            direction=srow.get("direction", "→librecad"),
            priority=srow.get("priority", "P2" if callback in overrides else "P1"),
            notes=srow.get("notes", "generic interface callback; canonical family mapping unavailable"),
            evidence=srow.get("evidence", "drw_interface.h / rs_filterdxfrw.cpp"),
        ))
    return sorted(rows, key=lambda row: (row.priority, row.format, row.name))


def render(root: Path, dwg_parser: Path | None, seed_path: Path) -> str:
    seed: dict[str, dict[str, str]] = {}
    if seed_path.is_file():
        seed = json.loads(read(seed_path))
    rows = build_rows(root, dwg_parser, seed)
    missing_callbacks = sorted(
        interface_callbacks(read(root / "libraries/libdxfrw/src/drw_interface.h"))
        - filter_overrides(
            read(root / "librecad/src/lib/filters/rs_filterdxfrw.cpp"),
            read(root / "librecad/src/lib/filters/rs_filterdxfrw.h"),
        )
    )
    reference_label = (
        os.path.relpath(dwg_parser, root) if dwg_parser is not None else "not found"
    )
    lines = [
        "# Cross-Read Parity Status (LibreCAD)",
        "",
        "Generated by `scripts/dwg_cross_read_parity_inventory.py`.",
        "",
        f"- dwg-parser reference: `{reference_label}`",
        f"- Matrix rows: **{len(rows)}**",
        f"- Interface callbacks without a filter override: **{len(missing_callbacks)}**",
        "",
        "## Missing filter overrides (auto)",
        "",
    ]
    lines.extend(f"- `{name}`" for name in missing_callbacks)
    lines += [
        "",
        "## Matrix (auto + curated seed)",
        "",
        "| format | name | fixedType | className | lib_dispatched | lib_filter_override | dwg_registered | dwg_cadjson_default | lib_S | lib_L | dwg_S | dwg_D | direction | priority | notes | evidence |",
        "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        if row.priority in {"P0", "P1"} or not row.lib_filter_override or row.format == "interface-callback":
            lines.append("| " + " | ".join([
                markdown_cell(row.format), markdown_cell(row.name), markdown_cell(row.fixed_type),
                markdown_cell(row.class_name), str(row.lib_dispatched).lower(),
                str(row.lib_filter_override).lower(), str(row.dwg_registered).lower(),
                bool_cell(row.dwg_cadjson_default), markdown_cell(row.lib_s), markdown_cell(row.lib_l),
                markdown_cell(row.dwg_s), markdown_cell(row.dwg_d), markdown_cell(row.direction),
                markdown_cell(row.priority), markdown_cell(row.notes), markdown_cell(row.evidence),
            ]) + " |")
    p0p1 = [row for row in rows if row.priority in {"P0", "P1"}]
    lines += ["", f"## P0/P1 rows: {len(p0p1)}", ""]
    lines.extend(
        f"- `{row.name}`: {'exposed' if row.lib_filter_override else 'open'} "
        f"(lib S={row.lib_s}, L={row.lib_l}; dp S={row.dwg_s}, D={row.dwg_d}) {row.notes}"
        for row in p0p1
    )
    lines += [
        "",
        "## Interpretation",
        "",
        "- `lib_dispatched`, `lib_filter_override`, `dwg_registered`, and `dwg_cadjson_default` are source-derived facts.",
        "- `lib_S`, `lib_L`, `dwg_S`, `dwg_D`, direction, and priority come from the checked-in seed or conservative defaults.",
        "- The inventory never auto-assigns fully structured (`F`) or linked-graph (`G`) semantic depth.",
        "- `unknown` means the sibling checkout was unavailable or the route could not be mapped; it is not evidence of support.",
    ]
    return "\n".join(lines) + "\n"


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--dwg-parser", type=Path, default=DEFAULT_DWG_PARSER)
    parser.add_argument("--seed", type=Path, default=DEFAULT_SEED)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--write", action="store_true", help="write the report")
    parser.add_argument("--check", action="store_true", help="fail if the report is stale")
    args = parser.parse_args(argv)

    root = args.repo_root.resolve()
    output = args.output if args.output.is_absolute() else root / args.output
    seed = args.seed if args.seed.is_absolute() else root / args.seed
    reference = args.dwg_parser.resolve() if args.dwg_parser.is_dir() else None
    if reference is None:
        print(f"warning: dwg-parser reference not found: {args.dwg_parser}", file=sys.stderr)
    generated = render(root, reference, seed)
    if args.check:
        if not output.is_file() or read(output) != generated:
            print(f"stale cross-read parity report: {output}", file=sys.stderr)
            return 1
        return 0
    if args.write:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(generated, encoding="utf-8")
        print(f"wrote {output}")
    else:
        sys.stdout.write(generated)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
