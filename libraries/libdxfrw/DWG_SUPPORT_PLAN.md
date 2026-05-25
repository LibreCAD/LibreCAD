# DWG Support Plan Through AC1027

Scope: improve LibreCAD/libdxfrw DWG import for releases before AC1032 (R2018).
AC1032 itself is intentionally out of scope for this plan; the target range is
R13/R14 through AC1027 (R2013).

References reviewed:

- `libraries/libdxfrw/src/intern/dwgreader.cpp` and `libraries/libdxfrw/src/drw_objects.*`.
- `../ACadSharp/src/ACadSharp/IO/DWG/DwgStreamReaders/DwgObjectReader.cs`.
- `../libredwg/src/objects.inc`, `../libredwg/src/dwg.spec`, and `../libredwg/src/dwg2.spec`.
- ODA/Open Design DWG specification in `~/doc/dwg/dwg.pdf`, especially sections
  20.4.44, 20.4.45, 20.4.74, 20.4.77, 20.4.91, 20.4.93,
  20.4.101, and 20.4.104.

## Current State

LibreCAD already has useful pre-AC1032 coverage for the model-space path:
entities, common tables, blocks, layouts, MLINE/MLINESTYLE, underlays, WIPEOUT,
MLEADER/MLEADERSTYLE, VISUALSTYLE, SCALE, DBCOLOR, PlotSettings, and
ARC_DIMENSION are either parsed directly or tolerated well enough to preserve
2D geometry.

The remaining gap is mostly the OBJECTS section. Those objects usually do not
render by themselves, but they preserve named-object dictionaries, current style
variables, table/material metadata, sort order, fields, and xrecord payloads.
Without them, newer-but-still-pre-AC1032 drawings can load geometry while losing
metadata or reporting object-read failures.

## Spec-Checked Priority

1. **Dictionary graph and XRECORD carriers**

   Implement/verify DICTIONARY, DICTIONARYWDFLT, DICTIONARYVAR, and XRECORD.
   These are the backbone for named objects and round-trip metadata. The ODA
   sections confirm their layouts:

   - 20.4.44 DICTIONARY: item count, entry names, and item handles.
   - 20.4.45 DICTIONARYWDFLT: DICTIONARY plus one default-entry handle.
   - 20.4.74 DICTIONARYVAR: schema byte and string value.
   - 20.4.104 XRECORD: byte-counted DXF group-code/value stream plus object-id
     handles.

   Status: implemented in this pass.

2. **Low-risk handle container objects**

   Add parsers for FIELDLIST and SORTENTSTABLE next. Both are largely handle
   containers in the spec and are present in ACadSharp/libredwg as standard
   OBJECTS support. These should be preserved before richer style/material work.

   Status: implemented in this pass.

3. **Raster and display variables**

   Add RASTERVARIABLES. This is compact, user-visible for image behavior, and
   already isolated from the core entity geometry path.

   Status: implemented in this pass.

4. **Style/material preservation**

   Add TABLESTYLE and MATERIAL enough to avoid dropping named-object dictionary
   entries. These formats are larger and version-sensitive, especially around
   R2010/R2013 table styles, so the first milestone should preserve object
   identity, handles, names, and core flags before attempting complete styling.
   The ODA TABLESTYLE section and libredwg `dwg2.spec` both show that full
   R2010+ cell style data is a nested format, so this plan deliberately keeps
   that as a later table-specific slice.

   Status: identity/core-field preservation implemented in this pass; full
   material maps and table cell-style semantics remain future work.

5. **Field objects and table companion data**

   Add FIELD/FIELDLIST expansion and table companion objects after the core
   metadata graph is stable. These should be tested against files that actually
   use fields or tables because the payload has nested handles and expressions.

## Implementation Rules

- Treat unknown object metadata as non-fatal when the object is size-bounded and
  core drawing geometry has already been read. This matches LibreCAD's import
  priority: preserve 2D geometry first, then preserve metadata opportunistically.
- Keep every new parser behind typed callbacks on `DRW_Interface` so callers can
  opt in without breaking existing integrations.
- Parse enough handles to keep the dictionary graph navigable, even when the
  full semantic model is not yet represented in LibreCAD.
- Validate each slice with:
  - `scripts/dwg-iterate.sh build`
  - `scripts/dwg-iterate.sh golden check`
  - `scripts/dwg-iterate.sh test`
  - focused sample files from `~/doc/dwg`, `~/doc/dwg2`, and `~/dev/dwg_samples`

## Completed Slice

This pass implements priorities 1 through 3 and the first milestone of
priority 4:

- DICTIONARY now records entry names and target handles.
- DICTIONARYWDFLT records the default-entry handle.
- DICTIONARYVAR records the schema byte and string value.
- XRECORD reads common DXF-style scalar, string, point, binary, and handle
  payloads, then preserves trailing object-id handles. The group-code typing
  follows the ODA XRECORD description and was cross-checked against libredwg's
  `dwg_resbuf_value_type()` and ACadSharp's `GroupCodeValue.TransformValue()`,
  including byte codes 280-289 and the wider point/string ranges.
- `dwgReader` dispatches fixed XRECORD type 0x4f and class-map custom objects
  for `DICTIONARYVAR`, `DICTIONARYWDFLT`/`ACDBDICTIONARYWDFLT`, and `XRECORD`.
- FIELDLIST records referenced field handles.
- SORTENTSTABLE records sort handles, the owner block handle, and entity handles;
  per ODA 20.4.93, the sort handles are read from the main data stream before
  the common object handle stream.
- RASTERVARIABLES records class version, image frame display, image quality, and
  raster units.
- MATERIAL records name and description.
- TABLESTYLE records name and pre-R2010 core flags/margins; R2010+ table
  cell-style semantics are intentionally deferred.

The parsers are deliberately tolerant at metadata tails because older DWG
versions vary in handle packing and because these objects are not geometry
producers. Sanity checks still reject clearly impossible dictionary item counts.
