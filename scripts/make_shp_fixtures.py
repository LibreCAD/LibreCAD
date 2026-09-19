#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate the corpus-gap fixtures called out in Phase 4a of
docs/plan_shp_native_filter.md, and one for the DBF LTYPE column.

Produces (all under test_data/shp/, all genuinely NEW filenames — never
touches existing pinned fixtures):

  polygonz.shp / .shx     -- one SHPT_POLYGONZ record (single ring, 5 vertices,
                             Z-populated) exercising the POLYGONZ path in
                             RS_FilterSHP that shapelib itself covers only
                             indirectly.
  polylinez.shp / .shx    -- one SHPT_ARCZ record (2-part polyline, Z-populated)
                             for the ARCZ open-polyline path.
  multipointz.shp / .shx  -- one SHPT_MULTIPOINTZ record with 4 vertices.
  multipatch.shp / .shx   -- one SHPT_MULTIPATCH record with three parts:
                             an OUTER_RING, an INNER_RING (i.e. hole boundary),
                             and a TRIANGLE_STRIP (wireframe simplification
                             path in the filter).
  dos_npoints.shp / .shx  -- crafted hostile: header valid, single record
                             content whose nPoints field claims 60_000_000
                             (> the 50 M cap in shapelib/shpopen.cpp:2259);
                             SHPReadObject must return null without
                             allocating.  CVE-2023-30259-class regression
                             guard.
  dos_nparts.shp / .shx   -- same as above but nParts=15_000_000
                             (> the 10 M cap).
  ltype_point.shp/.shx/.dbf -- SHPT_POINT records whose .dbf carries an
                             LTYPE C(32) column, one LTYPE_POINT_VALUES
                             entry each, for the linetype name RS_FilterSHP
                             puts on the entity pen.

Pure-stdlib `struct`-packing.  No GDAL / no shapelib dependency; the
generator is intentionally read-only against the corpus dir (it only
creates files with new names) so re-running it can never regenerate an
existing fixture.

SHP file layout — quick reference (ESRI whitepaper, cross-checked
against libraries/shapelib/src/shpopen.cpp):

  File header (100 bytes)
      int32 BE  9994          (file code)
      int32 BE  0             (5 unused ints)
      ...
      int32 BE  fileLength    (in 16-bit words including header)
      int32 LE  1000          (version)
      int32 LE  shapeType     (SHPT_*)
      double LE Xmin, Ymin, Xmax, Ymax  (bounding box)
      double LE Zmin, Zmax, Mmin, Mmax  (0.0 when unused)

  Per record:
      int32 BE  recordNumber  (1-based)
      int32 BE  contentLength (in 16-bit words, NOT bytes)
      int32 LE  shapeType     (repeated in the record content)
      <shape-type-specific payload>

  SHX layout is a 100-byte header identical to SHP but with fileLength
  reflecting the .shx size, followed by one 8-byte index record per
  SHP record: <int32 BE offset in 16-bit words> <int32 BE contentLength>.
"""
import argparse
import json
import os
import struct
import sys
from pathlib import Path


# ---- shapelib SHPT_* constants (from shapefil.h) ---------------------------
SHPT_NULL = 0
SHPT_POINT = 1
SHPT_ARC = 3
SHPT_POLYGON = 5
SHPT_MULTIPOINT = 8
SHPT_POINTZ = 11
SHPT_ARCZ = 13
SHPT_POLYGONZ = 15
SHPT_MULTIPOINTZ = 18
SHPT_POINTM = 21
SHPT_ARCM = 23
SHPT_POLYGONM = 25
SHPT_MULTIPOINTM = 28
SHPT_MULTIPATCH = 31

# MULTIPATCH part types (from shapefil.h)
SHPP_TRISTRIP = 0
SHPP_TRIFAN = 1
SHPP_OUTERRING = 2
SHPP_INNERRING = 3
SHPP_FIRSTRING = 4
SHPP_RING = 5


# ---- primitive packers ----------------------------------------------------

def pack_be_i32(n: int) -> bytes:
    return struct.pack(">i", n)


def pack_le_i32(n: int) -> bytes:
    return struct.pack("<i", n)


def pack_le_double(d: float) -> bytes:
    return struct.pack("<d", d)


# ---- header + record helpers ----------------------------------------------

def file_header(shp_type: int, file_len_words: int, bbox: tuple) -> bytes:
    """Build the 100-byte SHP/SHX header.

    file_len_words is the total file length in 16-bit words, per ESRI spec.
    bbox is (Xmin, Ymin, Xmax, Ymax, Zmin, Zmax, Mmin, Mmax).
    """
    parts = [
        pack_be_i32(9994),           # file code
        pack_be_i32(0), pack_be_i32(0), pack_be_i32(0),
        pack_be_i32(0), pack_be_i32(0),
        pack_be_i32(file_len_words),
        pack_le_i32(1000),           # version
        pack_le_i32(shp_type),
    ]
    for v in bbox:
        parts.append(pack_le_double(v))
    header = b"".join(parts)
    assert len(header) == 100, f"header={len(header)} bytes"
    return header


def record_header(rec_num: int, content_len_words: int) -> bytes:
    return pack_be_i32(rec_num) + pack_be_i32(content_len_words)


def shx_index_record(offset_words: int, content_len_words: int) -> bytes:
    return pack_be_i32(offset_words) + pack_be_i32(content_len_words)


# ---- payload builders ------------------------------------------------------

def build_polygonz_payload(rings: list) -> bytes:
    """rings is list of list of (x, y, z) tuples.
    Assumes rings are already closed (last vertex == first).
    """
    xs = [pt[0] for r in rings for pt in r]
    ys = [pt[1] for r in rings for pt in r]
    zs = [pt[2] for r in rings for pt in r]
    n_parts = len(rings)
    n_points = sum(len(r) for r in rings)
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = min(ys), max(ys)
    zmin, zmax = min(zs), max(zs)
    part_starts = []
    running = 0
    for r in rings:
        part_starts.append(running)
        running += len(r)

    body = b"".join([
        pack_le_double(xmin), pack_le_double(ymin),
        pack_le_double(xmax), pack_le_double(ymax),
        pack_le_i32(n_parts),
        pack_le_i32(n_points),
        b"".join(pack_le_i32(s) for s in part_starts),
        b"".join(pack_le_double(x) + pack_le_double(y)
                 for x, y in zip(xs, ys)),
        pack_le_double(zmin), pack_le_double(zmax),
        b"".join(pack_le_double(z) for z in zs),
        # M values are optional; skip them for these fixtures (nEntitySize
        # will therefore not include an M block).
    ])
    return body


def build_arcz_payload(parts: list) -> bytes:
    """parts is list of list of (x, y, z).  Parts are NOT auto-closed."""
    # Structurally identical to POLYGONZ except semantics; same wire format.
    return build_polygonz_payload(parts)


def build_multipointz_payload(points: list) -> bytes:
    """points is list of (x, y, z)."""
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    zs = [p[2] for p in points]
    body = b"".join([
        pack_le_double(min(xs)), pack_le_double(min(ys)),
        pack_le_double(max(xs)), pack_le_double(max(ys)),
        pack_le_i32(len(points)),
        b"".join(pack_le_double(x) + pack_le_double(y)
                 for x, y in zip(xs, ys)),
        pack_le_double(min(zs)), pack_le_double(max(zs)),
        b"".join(pack_le_double(z) for z in zs),
    ])
    return body


def build_multipatch_payload(parts: list) -> bytes:
    """parts is list of (part_type, [(x, y, z), ...])."""
    xs = [pt[0] for _, r in parts for pt in r]
    ys = [pt[1] for _, r in parts for pt in r]
    zs = [pt[2] for _, r in parts for pt in r]
    n_parts = len(parts)
    n_points = sum(len(r) for _, r in parts)
    part_starts = []
    running = 0
    for _, r in parts:
        part_starts.append(running)
        running += len(r)
    body = b"".join([
        pack_le_double(min(xs)), pack_le_double(min(ys)),
        pack_le_double(max(xs)), pack_le_double(max(ys)),
        pack_le_i32(n_parts),
        pack_le_i32(n_points),
        b"".join(pack_le_i32(s) for s in part_starts),
        b"".join(pack_le_i32(t) for t, _ in parts),
        b"".join(pack_le_double(x) + pack_le_double(y)
                 for x, y in zip(xs, ys)),
        pack_le_double(min(zs)), pack_le_double(max(zs)),
        b"".join(pack_le_double(z) for z in zs),
    ])
    return body


def build_point_payload(x: float, y: float) -> bytes:
    """SHPT_POINT content payload: just the X and Y doubles (no bbox, no
    part/point counts) -- the shape type itself is prefixed by write_shp_shx."""
    return pack_le_double(x) + pack_le_double(y)


# ---- top-level file writer -----------------------------------------------

def write_shp_shx(basename: Path, shp_type: int, records: list, bbox=None):
    """records is list of (shape_type_int, payload_bytes).
    Writes basename.shp and basename.shx.
    """
    # Build record blocks with headers (each record: 8-byte header + 4-byte
    # shape-type-in-content + payload).
    record_bytes = []
    shx_records = []
    running_offset_words = 50  # SHP header = 100 bytes = 50 words
    for i, (rtype, payload) in enumerate(records, start=1):
        content = pack_le_i32(rtype) + payload
        content_len_words = len(content) // 2
        rec_hdr = record_header(i, content_len_words)
        record_bytes.append(rec_hdr + content)
        shx_records.append(shx_index_record(running_offset_words,
                                            content_len_words))
        running_offset_words += 4 + content_len_words  # 8-byte header = 4 words

    body = b"".join(record_bytes)
    shp_file_len_words = 50 + len(body) // 2

    # Bounding box: derive from records if not given; assume 2D box only
    # (Z/M ranges are zeros — matches the many real .shp files that emit 0.0
    # here regardless of Z contents).
    if bbox is None:
        bbox = (0.0, 0.0, 100.0, 100.0, 0.0, 0.0, 0.0, 0.0)

    shp_header = file_header(shp_type, shp_file_len_words, bbox)
    shx_body = b"".join(shx_records)
    shx_file_len_words = 50 + len(shx_body) // 2
    shx_header = file_header(shp_type, shx_file_len_words, bbox)

    basename.with_suffix(".shp").write_bytes(shp_header + body)
    basename.with_suffix(".shx").write_bytes(shx_header + shx_body)


# Default field list: a single 'FID' integer column, values 1..n_records.
# A field is (name, type_char, width, decimals, value), where value is either a
# constant string or a callable taking the 1-based record number.
DEFAULT_DBF_FIELDS = [("FID", "N", 11, 0, lambda i: str(i))]


def write_minimal_dbf(basename: Path, n_records: int, fields=None):
    """Write a minimal dBase III file with `fields` and n_records rows.

    Required because shapelib's open path is happy without a .dbf, but the
    RS_FilterSHP-level tests can exercise the codepage/label/linetype paths
    better if a .dbf is present.  The default field list reproduces the
    original single-'FID' layout byte for byte, so fixtures generated before
    this parameter existed regenerate identically.

    Numeric ('N') values are right-justified and character ('C') values
    left-justified in their field width, per dBase III.
    """
    if fields is None:
        fields = DEFAULT_DBF_FIELDS
    # Header (32 bytes):
    #   byte 0: version 0x03
    #   bytes 1-3: YMD (Y = year since 1900)
    #   bytes 4-7: LE int32 nRecords
    #   bytes 8-9: LE int16 header length
    #   bytes 10-11: LE int16 record length
    #   bytes 12-31: reserved zero
    n_fields = len(fields)
    header_len = 32 + 32 * n_fields + 1  # +1 for terminator
    record_len = 1 + sum(f[2] for f in fields)  # deletion flag + field widths
    header = struct.pack("<BBBBIHH20s",
                         0x03, 100, 1, 1, n_records, header_len, record_len,
                         b"\x00" * 20)
    # Field descriptor (32 bytes): name (11 bytes, null-padded), type,
    #   4 reserved, length (1 byte), decimals (1 byte), 14 reserved
    field_descriptors = b""
    for name, type_char, width, decimals, _value in fields:
        name_bytes = name.encode("ascii")
        assert len(name_bytes) <= 10, f"DBF field name too long: {name}"
        field_descriptors += struct.pack(
            "<11sBI B B14s",
            name_bytes.ljust(11, b"\x00"), ord(type_char), 0,
            width, decimals, b"\x00" * 14)
    terminator = b"\x0D"
    records = b""
    for i in range(1, n_records + 1):
        row = b" "  # deletion flag ' ' (not deleted)
        for _name, type_char, width, _decimals, value in fields:
            text = value(i) if callable(value) else value
            cell = text.rjust(width) if type_char == "N" else text.ljust(width)
            assert len(cell) == width, f"value {text!r} overflows width {width}"
            row += cell.encode("ascii")
        records += row
    eof = b"\x1A"
    basename.with_suffix(".dbf").write_bytes(header + field_descriptors
                                             + terminator + records + eof)


# ---- fixtures --------------------------------------------------------------

def gen_polygonz(root: Path):
    # One triangle ring, closed (first == last), Z varies per vertex.
    ring = [(0.0, 0.0, 10.0),
            (5.0, 0.0, 20.0),
            (2.5, 4.0, 30.0),
            (0.0, 0.0, 10.0)]
    write_shp_shx(root / "polygonz", SHPT_POLYGONZ, [(SHPT_POLYGONZ,
                                                     build_polygonz_payload([ring]))])
    write_minimal_dbf(root / "polygonz", n_records=1)


def gen_polylinez(root: Path):
    # Two-part 3D polyline: staircase up.
    part1 = [(0.0, 0.0, 0.0), (10.0, 0.0, 5.0), (10.0, 10.0, 10.0)]
    part2 = [(20.0, 0.0, 15.0), (30.0, 10.0, 20.0)]
    write_shp_shx(root / "polylinez", SHPT_ARCZ, [(SHPT_ARCZ,
                                                   build_arcz_payload([part1, part2]))])
    write_minimal_dbf(root / "polylinez", n_records=1)


def gen_multipointz(root: Path):
    points = [(0.0, 0.0, 1.0), (10.0, 0.0, 2.0),
              (10.0, 10.0, 3.0), (0.0, 10.0, 4.0)]
    write_shp_shx(root / "multipointz", SHPT_MULTIPOINTZ, [(SHPT_MULTIPOINTZ,
                                                            build_multipointz_payload(points))])
    write_minimal_dbf(root / "multipointz", n_records=1)


def gen_multipatch(root: Path):
    # Part 0: OUTER_RING - a square (closed).
    outer = [(0.0, 0.0, 0.0), (10.0, 0.0, 0.0),
             (10.0, 10.0, 0.0), (0.0, 10.0, 0.0),
             (0.0, 0.0, 0.0)]
    # Part 1: INNER_RING - a smaller square inside (hole).
    inner = [(2.0, 2.0, 0.0), (2.0, 8.0, 0.0),
             (8.0, 8.0, 0.0), (8.0, 2.0, 0.0),
             (2.0, 2.0, 0.0)]
    # Part 2: TRIANGLE_STRIP - two triangles sharing an edge.
    strip = [(20.0, 0.0, 0.0), (20.0, 5.0, 5.0),
             (25.0, 0.0, 0.0), (25.0, 5.0, 5.0)]
    parts = [
        (SHPP_OUTERRING, outer),
        (SHPP_INNERRING, inner),
        (SHPP_TRISTRIP,  strip),
    ]
    write_shp_shx(root / "multipatch", SHPT_MULTIPATCH, [(SHPT_MULTIPATCH,
                                                          build_multipatch_payload(parts))])
    write_minimal_dbf(root / "multipatch", n_records=1)


def build_polygon2d_payload_with_declared_counts(n_parts_declared: int,
                                                 n_points_declared: int,
                                                 real_parts: list) -> bytes:
    """Build a SHPT_POLYGON payload that *declares* pathological
    nParts/nPoints in the record but only actually stores the smaller
    `real_parts` data.  Shapelib will read the declaration, apply the
    50M/10M caps, and reject with SHPReadObject returning null.

    This is what the DoS fixtures exercise: the on-disk file stays tiny
    (kilobytes) but the record header lies about record content size.
    """
    xs = [pt[0] for r in real_parts for pt in r]
    ys = [pt[1] for r in real_parts for pt in r]
    part_starts = []
    running = 0
    for r in real_parts:
        part_starts.append(running)
        running += len(r)
    body = b"".join([
        pack_le_double(min(xs) if xs else 0.0),
        pack_le_double(min(ys) if ys else 0.0),
        pack_le_double(max(xs) if xs else 0.0),
        pack_le_double(max(ys) if ys else 0.0),
        pack_le_i32(n_parts_declared),   # POISONED
        pack_le_i32(n_points_declared),  # POISONED
        b"".join(pack_le_i32(s) for s in part_starts),
        b"".join(pack_le_double(x) + pack_le_double(y)
                 for x, y in zip(xs, ys)),
    ])
    return body


def gen_dos_npoints(root: Path):
    """Header valid; single POLYGON record whose content declares
    nPoints = 60_000_000 (over the 50M cap).  Shapelib rejects at the
    corruption check in shpopen.cpp:2258 without allocating."""
    dummy_ring = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 0.0)]
    payload = build_polygon2d_payload_with_declared_counts(
        n_parts_declared=1, n_points_declared=60_000_000,
        real_parts=[dummy_ring])
    write_shp_shx(root / "dos_npoints", SHPT_POLYGON,
                  [(SHPT_POLYGON, payload)])


def gen_dos_nparts(root: Path):
    """Header valid; single POLYGON record whose content declares
    nParts = 15_000_000 (over the 10M cap)."""
    dummy_ring = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 0.0)]
    payload = build_polygon2d_payload_with_declared_counts(
        n_parts_declared=15_000_000, n_points_declared=4,
        real_parts=[dummy_ring])
    write_shp_shx(root / "dos_nparts", SHPT_POLYGON,
                  [(SHPT_POLYGON, payload)])


# One POINT each: a name that is no built-in, a built-in in another case, an
# ISO alias, a built-in padded with a tab, a tab only, and a blank value.
LTYPE_POINT_VALUES = ["VENDOR_TAB", "Dashed", "ACAD_ISO02W100", "DASHED\t",
                      "\t", ""]


def gen_ltype_point(root: Path):
    """One POINT per LTYPE_POINT_VALUES entry, at x = 10, 20, ... and y = 20.

    RS_FilterSHP puts a LINETYPE or LTYPE column on the entity pen.  shapelib
    trims the spaces that pad a C field, but not tabs."""
    xs = [10.0 * i for i in range(1, len(LTYPE_POINT_VALUES) + 1)]
    y = 20.0
    write_shp_shx(root / "ltype_point", SHPT_POINT,
                  [(SHPT_POINT, build_point_payload(x, y)) for x in xs],
                  bbox=(xs[0], y, xs[-1], y, 0.0, 0.0, 0.0, 0.0))
    write_minimal_dbf(root / "ltype_point", n_records=len(xs),
                      fields=[("FID", "N", 11, 0, lambda i: str(i)),
                              ("LTYPE", "C", 32, 0,
                               lambda i: LTYPE_POINT_VALUES[i - 1])])


# ---- inventory update ------------------------------------------------------

def update_inventory(root: Path):
    """Add the newly-generated fixtures to test_data/shp_inventory.json under
    the "generated_z_types", "generated_hostile" and "generated_linetype"
    buckets so their expected sizes/types can be cross-checked
    programmatically."""
    inv_path = root.parent / "shp_inventory.json"
    if not inv_path.exists():
        print(f"[warn] {inv_path} not found — skipping inventory update",
              file=sys.stderr)
        return
    with inv_path.open("r", encoding="utf-8") as f:
        inv = json.load(f)

    def sz(name):
        p = root / name
        return p.stat().st_size if p.exists() else 0

    inv["generated_z_types"] = [
        {"name": "polygonz.shp",    "shp_size": sz("polygonz.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py"},
        {"name": "polylinez.shp",   "shp_size": sz("polylinez.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py"},
        {"name": "multipointz.shp", "shp_size": sz("multipointz.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py"},
        {"name": "multipatch.shp",  "shp_size": sz("multipatch.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py"},
    ]
    inv["generated_hostile"] = [
        {"name": "dos_npoints.shp", "shp_size": sz("dos_npoints.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py",
         "expect": "SHPReadObject returns null (nPoints=60M > 50M cap)"},
        {"name": "dos_nparts.shp",  "shp_size": sz("dos_nparts.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py",
         "expect": "SHPReadObject returns null (nParts=15M > 10M cap)"},
    ]
    inv["generated_linetype"] = [
        {"name": "ltype_point.shp", "shp_size": sz("ltype_point.shp"),
         "has_shx": True, "has_prj": False,
         "generator": "scripts/make_shp_fixtures.py",
         "expect": "each DBF LTYPE value reaches its point's pen"},
    ]
    with inv_path.open("w", encoding="utf-8") as f:
        json.dump(inv, f, indent=2)
        f.write("\n")


# ---- main ------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path,
                    default=Path(__file__).resolve().parent.parent
                    / "test_data" / "shp",
                    help="Output directory (default: test_data/shp/)")
    ap.add_argument("--force", action="store_true",
                    help="Overwrite existing files with the same name")
    args = ap.parse_args()

    root: Path = args.out
    if not root.exists():
        print(f"[err] {root} does not exist", file=sys.stderr)
        return 1

    # Refuse to overwrite unless --force explicitly given.  Guard against
    # accidental corpus mutation.
    generated = [
        "polygonz.shp", "polygonz.shx", "polygonz.dbf",
        "polylinez.shp", "polylinez.shx", "polylinez.dbf",
        "multipointz.shp", "multipointz.shx", "multipointz.dbf",
        "multipatch.shp", "multipatch.shx", "multipatch.dbf",
        "dos_npoints.shp", "dos_npoints.shx",
        "dos_nparts.shp", "dos_nparts.shx",
        "ltype_point.shp", "ltype_point.shx", "ltype_point.dbf",
    ]
    existing = [f for f in generated if (root / f).exists()]
    if existing and not args.force:
        print(f"[err] {len(existing)} target file(s) already exist under {root} "
              f"(pass --force to overwrite): {existing}", file=sys.stderr)
        return 1

    gen_polygonz(root)
    gen_polylinez(root)
    gen_multipointz(root)
    gen_multipatch(root)
    gen_dos_npoints(root)
    gen_dos_nparts(root)
    gen_ltype_point(root)
    update_inventory(root)
    print(f"Wrote {len(generated)} fixture file(s) to {root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
