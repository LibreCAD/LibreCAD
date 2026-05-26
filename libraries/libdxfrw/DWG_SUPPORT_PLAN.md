# DWG Support Plan

Scope: track LibreCAD/libdxfrw DWG import/export work for R13/R14 through
AC1032, with extra focus on AC1027/R2013 and AC1032/R2018 compatibility,
metadata preservation, and geometry that LibreCAD can render or round-trip
without silently degrading.

References reviewed:

- `libraries/libdxfrw/src/intern/dwgreader.cpp` and `libraries/libdxfrw/src/drw_objects.*`.
- `../ACadSharp/src/ACadSharp/IO/DWG/DwgStreamReaders/DwgObjectReader.cs`.
- `../libreDWG/src/objects.inc`, `../libreDWG/src/dwg.spec`, and `../libreDWG/src/dwg2.spec`.
- ODA/Open Design DWG specification in `~/doc/dwg/dwg.pdf`, especially sections
  20.4.44, 20.4.45, 20.4.74, 20.4.77, 20.4.91, 20.4.93,
  20.4.101, and 20.4.104.

## Current State

LibreCAD already has useful coverage for the model-space path through the
currently targeted versions:
entities, common tables, blocks, layouts, MLINE/MLINESTYLE, underlays, WIPEOUT,
MLEADER/MLEADERSTYLE, VISUALSTYLE, SCALE, DBCOLOR, PlotSettings, and
ARC_DIMENSION are either parsed directly or tolerated well enough to preserve
2D geometry.

The remaining gap is mostly the OBJECTS section. Those objects usually do not
render by themselves, but they preserve named-object dictionaries, current style
variables, table/material metadata, sort order, fields, and xrecord payloads.
Without them, newer drawings can load geometry while losing metadata or
reporting object-read failures.

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
   containers in the spec and are present in ACadSharp/libreDWG as standard
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
   The ODA TABLESTYLE section and libreDWG `dwg2.spec` both show that full
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

## Implementation Sequencing After Review

The review tightened the order of work. Safety and shared representation changes
should land before semantic rendering features, because later table/spline/conic
work depends on them.

1. **Shared parser safety and value fidelity**

   Fix CadValue point `dataSize`, XRECORD 64-bit integer storage, and
   variable-count bounds checks first. These are stream-alignment and data-loss
   bugs that can invalidate any higher-level parser.

2. **Shared preservation metadata**

   Define a small, documented sidecar convention for fields LibreCAD cannot
   render natively yet: non-2D conic data, LWPOLYLINE width/extrusion/id data,
   fit-point spline metadata, high-degree spline payloads, mesh/polyface source
   topology, and complex table sub-record tails.

3. **Structural writer correctness**

   Fix old-style POLYLINE DWG export, `SEQEND` emission, vertex subtype
   tracking, AC1027/AC1032 spline scenario flags, and hyperbola rational flags.
   These produce files that external readers can classify correctly.

4. **Semantic imports**

   Add R2007 table cell contents, TABLESTYLE style substructures, rational
   ellipse-arc recognition, fit-point spline metadata, and conic/polyline
   sidecar rehydration.

5. **Rendering and graceful degradation**

   Only after the exact payload is preserved, add visible fallbacks for
   high-degree splines, mesh/polyface records, and unsupported table styling.
   Any fallback must be marked as derived so export can choose either original
   payload or edited visible geometry.

6. **AC1032 completion**

   Revisit R2018-specific custom-entity tails, annotative MTEXT context data,
   associative metadata objects, and optional AuxHeader emission after the
   shared readers/writers above are stable.

## Cross-Cutting Validation Plan

Every implementation slice should include validation at four layers. The goal is
to prove both bit-stream alignment and LibreCAD-level behavior, not just that a
single happy-path fixture loads.

1. **Spec and reference-reader alignment**

   - For every changed DWG layout, cite the ODA section and compare field order
     against at least one of ACadSharp or libreDWG.
   - For R2013+/R2018 conditional fields, include the exact version gate in the
     test name or fixture note.
   - When the plan deliberately preserves unsupported payloads as sidecar data,
     document which semantic fields remain unrendered.

2. **Parser safety**

   - Add malformed-count tests for every variable-length array added or changed.
   - Verify bad payloads fail locally without desynchronizing the following
     entity/object.
   - Bounds-check allocation counts before `reserve()` and before consuming
     payload bytes.
   - Add diagnostics that identify the failing record type and field count.

3. **Round-trip fidelity**

   - For low-level `DRW_*::encodeDwg()` changes, add encode/decode self-tests for
     all versions with distinct layout: AC1015, AC1018, AC1024, AC1027, and
     AC1032 where supported.
   - For filter-boundary changes, test import into LibreCAD entities and export
     back to `DRW_*`, including sidecar-preserved fields.
   - For metadata-only objects, assert object identity, handles, owner links, and
     raw payload values rather than expecting visible geometry.

4. **Fixture and interoperability smoke**

   - Run `scripts/dwg-iterate.sh build`, `scripts/dwg-iterate.sh golden check`,
     and `scripts/dwg-iterate.sh test`.
   - Run targeted fixture passes against `~/doc/dwg`, `~/doc/dwg2`, and
     `~/dev/dwg_samples` for the entity/object family being changed.
   - Where local tools are available, compare at least one generated DWG with
     ACadSharp or libreDWG for parser acceptance and key field values.
   - Record any remaining expected unsupported-object warnings so future reviews
     can distinguish known gaps from regressions.

## Completed Slice

This pass implements priorities 1 through 3 and the first milestone of
priority 4:

- DICTIONARY now records entry names and target handles.
- DICTIONARYWDFLT records the default-entry handle.
- DICTIONARYVAR records the schema byte and string value.
- XRECORD reads common DXF-style scalar, string, point, binary, and handle
  payloads, then preserves trailing object-id handles. The group-code typing
  follows the ODA XRECORD description and was cross-checked against libreDWG's
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

## Table, FIELD, And XRECORD Follow-Up Plan

### Scope Reviewed

- `DRW_CadValue` decoding in `drw_objects.cpp` and the duplicate table-side
  decoder in `drw_entities.cpp`.
- `FIELD`, `TABLE`, `TABLECONTENT`, `TABLESTYLE`, and `XRECORD` payload handling.
- ODA table/FIELD value sections, libreDWG table value fields, and ACadSharp's
  table/FIELD/XRECORD readers.

### Findings And Fixes

1. **P1: CadValue point payloads must read the BL data-size word**

   ODA, libreDWG, and ACadSharp read a BL payload size before `Point` and
   `3D Point` CadValue payloads. Both libdxfrw CadValue decoders must consume
   that size before the coordinate doubles. Reading doubles immediately
   desynchronizes any FIELD or TABLECONTENT value stream containing type 16 or
   type 32 point data.

   Fix:

   - Centralize CadValue point decoding so FIELD and table paths share the same
     data-size-aware implementation.
   - Read and store `m_dataSize` for point values, validate it is at least the
     expected coordinate byte count, and preserve any extra bytes as raw data.
   - Add tests for 2D and 3D point CadValues in both FIELD and TABLECONTENT
     contexts.

2. **P2: R2007 table cells are still skeleton-only**

   AC1021/R2007 table parsing currently records dimensions, row heights, column
   widths, and empty cell shells. ODA and ACadSharp read per-cell text/block
   content, attributes, value fields, content/border overrides, and table-level
   override blocks after the basic grid data. The current behavior keeps the
   parser aligned only for simple tables and leaves semantic rendering
   incomplete.

   Fix:

   - Implement the R2007 per-cell payload reader in small sub-record helpers:
     content, attributes, value fields, cell overrides, and border overrides.
   - Store unsupported sub-record tails as raw cell metadata when the LibreCAD
     model cannot render them yet.
   - Mark `m_semanticContentComplete` false only when a specific unsupported
     sub-record is encountered, not merely because R2007 content exists.
   - Add fixture tests with text cells, block cells, attributes, merged ranges,
     and override flags.

3. **P2: TABLESTYLE loses R2007/R24 style semantics**

   TABLESTYLE currently preserves only the legacy/core fields. R2007/R24 cell
   styles, content formats, borders, margins, colors, text style handles, and
   CELLSTYLEMAP-linked custom styles are needed for meaningful native table
   rendering and round-trip.

   Fix:

   - Add versioned TABLESTYLE substructures for cell styles, content formats,
     border formats, and style-map handles.
   - Preserve text style/color/alignment/height/margin/border fields even if
     rendering support lands later.
   - Connect parsed style handles to table cell content during semantic table
     rendering.
   - Add tests that parse at least one default TABLESTYLE and one custom
     R2007/R2013 style with overridden borders and text formatting.

4. **P2: XRECORD Int64 values truncate to 32 bits**

   DXF group codes 160-169 are signed/unsigned 64-bit integer values. The
   current XRECORD path stores them through a 32-bit value slot, losing high
   bits for round-trip records and metadata payloads.

   Fix:

   - Add a 64-bit integer value variant for `DRW_Variant`/record payloads or a
     dedicated XRECORD scalar storage path.
   - Decode and encode group codes 160-169 without masking.
   - Keep legacy 32-bit accessors available for existing callers, but make
     truncation explicit rather than silent.
   - Add boundary tests for `INT64_MIN`, `INT64_MAX`, and values above
     `UINT32_MAX`.

### Validation Steps

- Add unit tests for shared CadValue point decoding that feed byte streams with
  valid size, exact-size payloads, extra bytes, and undersized payloads.
- Add FIELD/TABLECONTENT parser tests proving a point-valued item is followed by
  another value and the second value still parses at the correct offset.
- Add R2007 table fixture tests for semantic cell content and assert both
  `m_hasSemanticContent` and `m_semanticContentComplete`.
- Add TABLESTYLE fixture tests that assert style names, text style handles,
  text height, alignment, margins, border flags, and color data.
- Add XRECORD tests for 64-bit group codes and verify DXF/DWG round-trip values
  preserve high bits.
- Run the cross-cutting validation commands and compare at least one table-heavy
  fixture with ACadSharp or libreDWG field counts.

## AC1032 Read/Write Plan

Scope: add R2018/AC1032 read/write coverage without widening LibreCAD's DWG
model beyond the currently supported 2D geometry/table-record surface.

Reviewed sources:

- ODA `~/doc/dwg/dwg.pdf`, chapter 8: R2018 is structurally identical to
  R2013, with AC1032-specific payload changes rather than a new section
  container.
- ODA chapter 9: the HEADER section keeps the R2010/R2013 high-size long
  unconditionally for R2018.
- ODA 20.4.46 and libreDWG `dwg.spec` `DWG_ENTITY(MTEXT)`: R2018 adds the
  MTEXT non-annotative/context-data tail after background data.
- ODA 20.4.73 and libreDWG `DWG_OBJECT(MLINESTYLE)`: R2018 MLINESTYLE elements
  include a linetype index in the data stream while linetype handles remain in
  the handle stream.
- ODA chapter 27: R2018 auxiliary-file-header additions are not emitted because
  the current R2004+ writer does not emit an `AcDb:AuxHeader` section.

Refined implementation plan:

1. Reuse the AC1027 page container for AC1032, emit the `"AC1032"` file
   version string in a thin writer subclass, and include the R2018-mandatory
   HEADER/CLASSES high-size long.
2. Permit AC1032 through the public `dwgRW::write` gate and through shared
   HEADER/common-entity encoders.
3. Keep the existing AC1032 reader dispatch and consume R2018 MTEXT tail fields
   so the following common/entity-specific handle data stays aligned.
4. Preserve the existing MLINESTYLE R2018 read behavior: consume the R2018
   per-element linetype index from the data stream and defer linetype handles to
   the handle stream.
5. Fix ATTRIB/ATTDEF writing for all R2010+ versions by emitting the version
   byte and lock-position bit that their readers already consume.
6. Match the R2007+ MTEXT body by writing the rectangle-height field before
   text height; otherwise AC1032's added MTEXT tail is read from the wrong bit.
7. Add R2018 smoke tests that write AC1032 files, read them back, and verify
   version dispatch plus representative geometry/MTEXT recovery.

Status: implemented in this pass. Remaining AC1032 work is deeper semantic
preservation of annotative MTEXT context data, proxy-entity payloads, and full
auxiliary-header emission if the writer later grows an `AcDb:AuxHeader` section.

## Polyline Support Review And Fix Plan

Scope: improve DWG/DXF polyline fidelity for `LWPOLYLINE`, old-style
`POLYLINE_2D`, `POLYLINE_3D`, `POLYLINE_MESH`, `POLYLINE_PFACE`, their
`VERTEX_*` children, and `SEQEND` chain termination.

Reviewed sources:

- ODA `~/doc/dwg/dwg.pdf`, especially the entity type map, common entity handle
  rules for `VERTEX`/`SEQEND`, and the `LWPOLYLINE` proxy subtype note.
- `../ACadSharp/src/ACadSharp/IO/DWG/DwgStreamReaders/DwgObjectReader.cs`:
  `readLWPolyline()`, `readPolyline2D()`, `readPolyline3D()`,
  `readPolylineMesh()`, `readPolyfaceMesh()`, `readVertex2D()`, and
  `readVertex3D()`.
- `../ACadSharp/src/ACadSharp/Entities/*Polyline*.cs`,
  `VertexFaceMesh.cs`, and `VertexFaceRecord.cs`.
- `../libreDWG/src/dwg.spec`: `DWG_ENTITY(LWPOLYLINE)`,
  `POLYLINE_2D`, `POLYLINE_3D`, `POLYLINE_MESH`, `POLYLINE_PFACE`,
  `VERTEX_2D`, `VERTEX_3D`, `VERTEX_MESH`, `VERTEX_PFACE`,
  `VERTEX_PFACE_FACE`, and `SEQEND`.
- LibreCAD/libdxfrw paths:
  - `libraries/libdxfrw/src/drw_entities.{h,cpp}`
  - `libraries/libdxfrw/src/intern/dwgreader.cpp`
  - `libraries/libdxfrw/src/libdwgr.cpp`
  - `libraries/libdxfrw/src/libdxfrw.cpp`
  - `librecad/src/lib/filters/rs_filterdxfrw.cpp`
  - `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

### Current State

- DWG and DXF `LWPOLYLINE` basic points and bulges are supported.
- DWG old-style `POLYLINE` reader dispatch supports 2D, 3D, mesh, and polyface
  fixed object types and walks owned `VERTEX` handles.
- The low-level `dwgRW::writePolyline()` path exists and can round-trip a simple
  R2010 2D polyline through `DRW_Polyline`.
- LibreCAD's normal DWG export path writes most `RS_Polyline` objects as
  `LWPOLYLINE`.
- Mesh and polyface imports are decomposed to LibreCAD 2D `RS_Polyline`
  children because LibreCAD has no native 3D mesh/polyface entity.

### Findings

1. **P1: Polyface mesh vertices are classified incorrectly in the LibreCAD
   filter**

   ACadSharp represents polyface coordinate vertices as `VertexFaceMesh`
   with flags `64 | 128`, and face records as `VertexFaceRecord` with flag
   `128`. The LibreCAD filter currently treats `(flags & 0x40) == 0` as a
   coordinate vertex and `(flags & 0x40) != 0` as a face. That reverses the
   meaningful distinction: coordinate vertices with bit 64 set are not added
   to the vertex pool, while face records with only bit 128 are skipped by the
   face loop. Polyface geometry can therefore disappear or be reconstructed
   from the wrong records.

   Fix:

   - Classify face records by `flags & 0x80` without `flags & 0x40`.
   - Classify coordinate vertices by `flags & 0x40`.
   - Preserve invisible-edge sign on face indices for later rendering, even if
     first-pass LibreCAD output still draws full closed outlines.
   - Add a focused filter-level test using a `DRW_Polyline` with two
     coordinate vertices flagged `0xC0` and one face record flagged `0x80`.

   Status: coordinate-vs-face classification fixed in
   `RS_FilterDXFRW::addPolyline()`; invisible-edge rendering and focused
   filter tests remain follow-up.

2. **P1: DWG export silently drops old-style polyline fallback cases**

   `RS_FilterDXFRW::writeLWPolyline()` falls back to `writePolyline()` when an
   `RS_Polyline` contains ellipse segments or when the target is R12. But
   `RS_FilterDXFRW::writePolyline()` immediately returns when `m_dwgW` is set,
   even though `dwgRW::writePolyline()` and `DRW_Polyline::encodeDwg()` exist.
   Result: ellipse-containing polylines are skipped on DWG export.

   Fix:

   - Remove the DWG early-return from `RS_FilterDXFRW::writePolyline()`.
   - Route DWG output to `m_dwgW->writePolyline(&pol)` after the existing
     `DRW_Polyline` construction.
   - Keep DXF behavior unchanged.
   - Add a DWG export test where an `RS_Polyline` contains an ellipse segment
     and verify at least a fallback `POLYLINE_2D` is emitted/read back.

   Status: implemented for the existing old-style polyline construction path;
   dedicated ellipse-fallback fixture coverage remains follow-up.

3. **P1: Generated DWG `POLYLINE` chains omit a real `SEQEND` object**

   The spec, ACadSharp templates, and libreDWG all model `POLYLINE_*` as owning
   vertex objects plus a `SEQEND` hard-owner terminator. The writer currently
   emits vertex objects and stores a null `seqEndH` handle in
   `DRW_Polyline::encodeDwg()`. LibreCAD can read its own simple round-trip
   because the reader tolerates null/erased `SEQEND`, but interoperability
   readers can expect a valid owned terminator.

   Fix:

   - Add a minimal `DRW_SeqEnd` entity or an internal writer helper that emits
     fixed type `6` using common entity data.
   - Allocate the polyline handle, vertex handles, and seqend handle together
     in `dwgRW::writePolyline()`.
   - Set vertex/seqend owner to the parent polyline where common entity handle
     data requires it.
   - Write the real seqend handle in the polyline's handle stream.
   - Extend the existing R2010 polyline round-trip test to assert that the
     handle map contains `SEQEND` and that the polyline's seqend handle is
     non-null.

4. **P1: Vertex DWG subtype selection is based only on DXF flags**

   `DRW_Vertex::encodeDwg()` chooses `VERTEX_PFACE_FACE` whenever bit 128 is
   set, and otherwise maps flags `64 | 32 | 16` to `VERTEX_3D`. That cannot
   distinguish `VERTEX_3D`, `VERTEX_MESH`, `VERTEX_PFACE`, and
   `VERTEX_PFACE_FACE` correctly. Polyface coordinate vertices commonly have
   both bits 64 and 128, so they are especially at risk.

   Fix:

   - Add an explicit vertex subtype enum to `DRW_Vertex`, populated during DWG
     parse from `oType` and during DXF parse from the parent polyline mode.
   - Use the parent polyline type when writing vertex children:
     `POLYLINE_2D -> VERTEX_2D`, `POLYLINE_3D -> VERTEX_3D`,
     `POLYLINE_MESH -> VERTEX_MESH`, `POLYLINE_PFACE` coordinate records
     -> `VERTEX_PFACE`, and face records -> `VERTEX_PFACE_FACE`.
   - Keep DXF group 70 flags as data, not as the sole source of object type.

5. **P2: LWPOLYLINE vertex IDs are parsed and discarded**

   R2010+ `LWPOLYLINE` can carry a vertex-id count and one BL id per vertex.
   The parser consumes these values into a local variable and drops them. The
   writer never emits the `0x400` flag or vertex id array because
   `DRW_Vertex2D` has no id field. DXF code 91 support exists for old-style
   `VERTEX`, so this is a model gap rather than a format unknown.

   Fix:

   - Add `identifier` to `DRW_Vertex2D`, matching `DRW_Vertex::identifier`.
   - Preserve parsed R2010+ ids.
   - Emit the `0x400` DWG flag and id count only when at least one id is
     non-zero or when round-trip preservation explicitly requires ids.
   - Write DXF group 91 for `LWPOLYLINE` vertices when present.
   - Add AC1024/AC1027 tests for id preservation.

   Status: model, DWG read/write, and DXF read/write preservation implemented.
   AC1024+ low-level self-tests are still blocked by the current entity test
   harness' single-buffer handle-section limitation.

6. **P2: LWPOLYLINE counts are not bounded against the vertex array**

   `DRW_LWPolyline::parseDwg()` reserves only `vertexnum`; it then reads
   `bulgesnum`, `vertexIdCount`, and `widthsnum` items regardless of whether
   those counts exceed `vertexnum`. The values are discarded when the index is
   outside the vertex vector, but the stream read still advances. A malformed
   or unexpected file can overrun the object payload and desynchronize common
   entity handle parsing.

   Fix:

   - Reject negative or excessive `vertexnum` before allocation.
   - Reject optional counts larger than `vertexnum` unless a fixture proves
     AutoCAD legitimately writes sparse tail arrays.
   - Bound all optional array reads by remaining object bytes.
   - Keep diagnostics explicit: report which optional array count was invalid.

   Status: non-negative vertex count and optional-array count checks landed;
   remaining-byte diagnostics remain follow-up.

7. **P2: LWPOLYLINE width/elevation/thickness/extrusion are lost at the
   LibreCAD model boundary**

   `DRW_LWPolyline` stores constant width, per-vertex widths, elevation,
   thickness, and extrusion, but `RS_FilterDXFRW::addLWPolyline()` maps only
   XY points and bulges into `RS_Polyline`. Normal LibreCAD rendering may not
   support every width/thickness concept, but import/export should not silently
   discard them when round-trip metadata is already used for MLINE and
   underlay placeholders.

   Fix:

   - Preserve unsupported LWPOLYLINE metadata in `RS_Polyline::drwExtData`
     under a `LibreCAD_LWPOLYLINE` app marker.
   - Include elevation, thickness, extrusion, constant width, per-vertex
     start/end widths, and vertex ids.
   - Update `writeLWPolyline()` to reconstruct these fields before writing
     DWG/DXF.
   - For visual rendering, decide separately whether constant/per-vertex width
     should affect `RS_Pen` or remain file-format metadata only.

8. **P2: POLYLINE mesh and polyface import is lossy by design but not
   round-trip-observable**

   Polygon meshes are decomposed into row/column 2D polylines. Polyfaces are
   decomposed into one closed 2D polyline per face. That is appropriate for a
   2D editor, but there is currently no sidecar metadata to reconstruct the
   original mesh/polyface on save.

   Fix:

   - Add optional `LibreCAD_POLYLINE_MESH` and `LibreCAD_POLYLINE_PFACE`
     XDATA sidecars when decomposing.
   - Store parent handle/id, original flags, M/N counts, density, curve type,
     vertex coordinates, face indices, and invisible-edge sign bits.
   - Reconstruct original `DRW_Polyline` only when the decomposed child set is
     complete and unmodified in a topology-changing way.
   - Otherwise fall back to plain 2D polylines.

9. **P3: Basic polyline tests cover only one simple R2010 2D path**

   The existing DWG write test proves a closed 2D `DRW_Polyline` can round-trip
   three XY vertices. It does not cover LWPOLYLINE bulges/widths/ids, old-style
   vertex widths/tangent, 3D polylines, mesh, polyface, R2000 first/last handle
   chains, R2007+ owned-object handle vectors, or malformed optional counts.

   Fix:

   - Add a matrix of focused tests:
     - `LWPOLYLINE` open/closed, bulges, const width, per-vertex widths,
       elevation, extrusion, and vertex ids.
     - `POLYLINE_2D` with default widths, per-vertex widths, bulge, tangent,
       and closed final segment bulge.
     - `POLYLINE_3D` with closed flag and spline-fit flags.
     - `POLYLINE_MESH` row/column decomposition.
     - `POLYLINE_PFACE` coordinate/face classification and invisible-edge
       index sign preservation.
     - Invalid optional counts rejected without corrupting the next entity.
   - Run each writer round-trip for at least AC1015, AC1018, AC1024, AC1027,
     and AC1032 where the format path differs.

### Priority Order

1. Fix polyface coordinate/face classification in `RS_FilterDXFRW::addPolyline()`.
2. Enable DWG old-style polyline fallback from `RS_FilterDXFRW::writePolyline()`.
3. Emit a real `SEQEND` object for generated DWG `POLYLINE_*` chains.
4. Add explicit `DRW_Vertex` subtype tracking and parent-aware vertex writing.
5. Preserve and write LWPOLYLINE vertex ids.
6. Bound-check LWPOLYLINE optional counts before consuming arrays.
7. Preserve unsupported LWPOLYLINE width/elevation/thickness/extrusion metadata
   through the LibreCAD filter.
8. Add mesh/polyface sidecar metadata for loss-aware round-trip.
9. Expand focused tests across versions and polyline subtypes.

### Acceptance Criteria

- Polyface coordinate vertices and face records are classified according to
  ACadSharp/libreDWG flag semantics and produce visible face outlines.
- DWG export no longer drops ellipse-containing `RS_Polyline` fallback cases.
- Generated DWG old-style polylines include valid vertex ownership and a valid
  `SEQEND` handle.
- LWPOLYLINE ids, widths, elevation, thickness, and extrusion can round-trip
  through import/export, either natively or through explicit sidecar metadata.
- Malformed LWPOLYLINE optional counts fail locally without desynchronizing the
  following entity.
- Tests cover basic LWPOLYLINE, 2D/3D old-style polyline, mesh, and polyface
  paths for representative DWG versions.

### Validation Steps

- Add low-level encode/decode tests for `LWPOLYLINE`, `POLYLINE_2D`,
  `POLYLINE_3D`, `POLYLINE_MESH`, `POLYLINE_PFACE`, `VERTEX_*`, and `SEQEND`
  where writer support exists.
- Add filter-level tests that import polyface/mesh records and assert the
  visible LibreCAD fallback geometry plus sidecar metadata.
- Add malformed optional-count tests for LWPOLYLINE bulge, width, and vertex-id
  arrays; assert the next entity still parses.
- Write generated old-style polylines and inspect the handle map for owned
  vertex handles and a non-null `SEQEND`.
- Run the same core cases against AC1015, AC1018, AC1024, AC1027, and AC1032
  because ownership and optional arrays differ by version.

## Spline And Bezier Support Review And Fix Plan

### Scope Reviewed

- LibreCAD/libdxfrw spline model and DWG/DXF paths:
  `DRW_Spline::parseDwg()`, `DRW_Spline::encodeDwg()`,
  `dxfRW::writeSpline()`, `RS_FilterDXFRW::addSpline()`,
  `RS_FilterDXFRW::writeSpline()`, `RS_FilterDXFRW::writeSplinePoints()`,
  and hatch spline edge import/export.
- LibreCAD spline engine:
  `RS_Spline`, `LC_SplinePoints`, `LC_HyperbolaSpline`, and
  `LC_ParabolaSpline`.
- Reference readers/writers:
  ACadSharp `readSpline()`/`writeSpline()` and libreDWG `SPLINE` in
  `dwg.spec`.
- ODA `SPLINE` section, especially R2013+ `Spline flags 1`,
  `knot parameter`, scenario 1 control-point splines, and scenario 2
  fit-point/Bezier splines.

### Current State

- Core DWG `SPLINE` parsing and writing exists for scenario 1
  control-point splines and scenario 2 fit-point splines.
- DXF spline import/export handles knots, control points, weights, fit points,
  and tangent vectors.
- LibreCAD can represent degree 1-3 `RS_Spline` and interpolation-style
  `LC_SplinePoints`; exact hyperbola/parabola conversion exists for some
  rational quadratic conics.
- Existing encode self-tests cover AC1015/AC1018 control-point and fit-point
  splines, but not AC1027/AC1032 R2013+ scenario flags, rational conics,
  hatch-boundary spline weights, or LibreCAD filter round-trip behavior.

### Findings And Fixes

1. **P1: R2013+ fit-point/Bezier DWG writer emits inconsistent scenario flags**

   ODA and ACadSharp treat R2013+ scenario selection as controlled by
   `Spline flags 1` and `knot parameter`: a fit-point/Bezier spline must set
   both `MethodFitPoints` and `UseKnotParameter`, with a non-custom knot
   parameter. `DRW_Spline::encodeDwg()` currently sets only bit 0 for scenario
   2 and writes `knotParam = 0`. LibreCAD's own reader still accepts that
   because it only checks bit 0, but ACadSharp will classify the same payload
   as scenario 1 when `UseKnotParameter` is absent, causing it to read fit
   tolerance/tangents/counts as rational/closed/periodic and knot/control
   fields.

   Fix:

   - Add explicit R2013+ spline metadata to `DRW_Spline`:
     `m_splineFlags1`, `m_knotParam`, and a parsed/writer-selected scenario.
   - For scenario 2, emit `MethodFitPoints | UseKnotParameter` and preserve
     closed/CV-frame bits where available.
   - For scenario 1, clear `UseKnotParameter` unless the stored metadata proves
     AutoCAD expects it, and write `Custom` knot parameter for custom
     control-point splines.
   - Extend encode round-trip tests to AC1027 and AC1032, and include an
     ACadSharp-compatible assertion for scenario 2 flag bits.

   Status: implemented in `DRW_Spline` metadata and the R2013+ encoder flag
   path. AC1027/AC1032 tests remain follow-up because the current low-level
   entity round-trip harness cannot model split handle sections.

2. **P1: R2013+ spline reader ignores the full scenario selection rule**

   `DRW_Spline::parseDwg()` reads `Spline flags 1` and `knot parameter` for
   AC1027+, but it changes to scenario 2 only when bit 0 is set and ignores the
   `UseKnotParameter` bit and `Custom` knot parameter. ACadSharp uses the ODA
   rule: `Custom` or missing `UseKnotParameter` means scenario 1; otherwise
   scenario 2. The current reader can therefore desynchronize on valid R2013+
   files whose flags are more precise than the old pre-R2013 scenario word.

   Fix:

   - Apply the ACadSharp/ODA scenario decision:
     `scenario = (knotParam == Custom || !(flags1 & UseKnotParameter)) ? 1 : 2`.
   - Preserve `MethodFitPoints`, `CVFrameShow`, `Closed`, and
     `UseKnotParameter` bits rather than discarding them after alignment.
   - Map R2013+ closed flag into normal DXF flag bit 0 before downstream
     LibreCAD conversion.
   - Add AC1027/AC1032 fixture tests that include both fit-point and
     control-point splines with non-default knot parameter values.

   Status: implemented in the DWG reader; fixture coverage remains follow-up.

3. **P1: Rational quadratic Bezier ellipse arcs lose their weights on import**

   `RS_FilterDXFRW::handleQuadraticConicSpline()` handles some rational
   degree-2 three-control-point splines as hyperbola or parabola. If the middle
   weight indicates an ellipse arc or the conic is otherwise not classified,
   import falls through to the generic degree-2 branch, which creates
   `LC_SplinePoints` and copies only XY control/fit points. That drops the
   rational weight and turns an exact conic/Bezier segment into a different
   non-rational quadratic curve.

   Fix:

   - For degree-2 rational splines, prefer exact `RS_Spline` preservation
     unless a conic converter proves it can create an exact native entity.
   - Add ellipse-arc recognition for canonical rational quadratic Bezier
     conics where feasible; otherwise keep the weighted `RS_Spline`.
   - Do not route weighted degree-2 control-point splines into
     `LC_SplinePoints`.
   - Add visual/geometry tests for weight `< 1`, `== 1`, and `> 1` middle
     weights.

4. **P2: Fit-point/Bezier tangents and knot parametrization are not preserved at
   the LibreCAD model boundary**

   DWG scenario 2 stores fit tolerance, start tangent, end tangent, and
   R2013+ knot parameter. When `RS_FilterDXFRW::addSpline()` rebuilds a
   LibreCAD `RS_Spline` from fit points, it calls `setFitPoints()` and ignores
   the stored tangents and knot parameter. On export, `writeSplinePoints()` also
   clears the fit list for DWG and forces a control-point scenario, discarding
   interpolation intent and Bezier/fit metadata.

   Fix:

   - Add sidecar metadata on imported `RS_Spline`/`LC_SplinePoints` for
     `fitTolerance`, start/end tangents, R2013+ knot parameter, and original
     scenario.
   - Teach `writeSpline()` and `writeSplinePoints()` to reconstruct scenario 2
     when the entity originated as a fit-point/Bezier spline and the fit data is
     still valid.
   - Keep the existing control-point export path for edited entities whose fit
     metadata is stale or incomplete.
   - Add round-trip tests for cubic Bezier-style fit-point splines with explicit
     tangents.

5. **P2: Hatch-boundary rational spline weights are stored inconsistently**

   DWG hatch spline edge parsing stores rational boundary weights in
   `controllist[i]->z`; DXF spline edge parsing stores weights in
   `weightlist`; normal entity splines use `weightlist`. Export helpers mirror
   some weights into both places, but import helpers such as
   `buildHatchSplineEdge()` consume only XY control points for degree-2
   pass-through and sampling branches. Exact rational hatch-boundary Bezier
   edges can therefore become non-rational during import.

   Fix:

   - Normalize `DRW_Spline` weight storage after hatch edge parsing:
     if rational and `weightlist` is empty, copy `controllist[i]->z` into
     `weightlist`.
   - Update `buildHatchSplineEdge()` to preserve rational weights when it
     creates an `RS_Spline`, or explicitly sample only when exact storage is not
     possible.
   - Keep export helpers writing both `weightlist` and hatch-edge `z` weights
     for compatibility with existing parser assumptions.
   - Add hatch tests for rational quadratic and cubic boundary splines.

6. **P2: Unsupported spline degrees are dropped instead of degraded visibly or
   preserved**

   ACadSharp and the DWG payload do not cap `SPLINE.degree` at 3, but
   `RS_FilterDXFRW::addSpline()` drops degree values outside 1-3. That is
   reasonable for LibreCAD's native spline engine, but it means valid DWG/DXF
   splines can disappear with only a warning.

   Fix:

   - Preserve high-degree spline payloads in explicit sidecar metadata.
   - Render them as sampled polylines or `LC_SplinePoints` for visibility when
     native exact representation is unavailable.
   - On export, reconstruct the original `DRW_Spline` only when the sampled
     representation is still untouched; otherwise export the visible fallback.
   - Add tests for degree-4 and degree-5 splines to prove they remain visible
     and do not silently vanish.

7. **P2: Spline array counts are not bounded against object body size**

   `DRW_Spline::parseDwg()` reserves arrays for knots, control points, weights,
   and fit points, but it does not validate negative or excessive counts against
   the remaining object body before consuming. A malformed spline can force
   large allocation attempts or desynchronize common entity handle parsing.

   Fix:

   - Reject negative counts and cap counts to a sane upper bound before
     allocation.
   - Validate count combinations against degree:
     `numCtrl >= degree + 1` for control-point splines, knot count consistent
     with control count and degree where applicable, and fit count non-zero for
     scenario 2.
   - Bound all point/weight reads by remaining payload bytes.
   - Add malformed-count tests that prove the following entity still parses.

   Status: non-negative and upper-bound checks landed for knot/control/fit
   arrays; relationship checks, remaining-byte checks, and malformed-stream
   tests remain follow-up.

8. **P3: Spline/Bezier tests do not cover modern DWG semantics**

   Current self-tests cover only AC1015/AC1018 entity encode/decode and do not
   exercise AC1027/AC1032 flag semantics, LibreCAD filter conversion, rational
   weights, hatch-boundary splines, or high-degree fallback behavior.

   Fix:

   - Add focused tests for:
     - AC1027/AC1032 scenario 1 control-point splines with custom knot
       parameter.
     - AC1027/AC1032 scenario 2 fit-point/Bezier splines with
       `UseKnotParameter`.
     - Rational quadratic Bezier conics with weights `< 1`, `== 1`, and `> 1`.
     - Hatch-boundary rational spline edge import/export.
     - Degree > 3 fallback visibility and metadata preservation.
     - Malformed count rejection.
   - Include at least one cross-reader fixture or byte-level assertion modeled
     on ACadSharp's R2013+ scenario logic.

### Priority Order

1. Fix R2013+ scenario 2 writer flags for AC1027/AC1032 compatibility.
2. Fix R2013+ scenario selection and metadata preservation in the reader.
3. Preserve rational quadratic Bezier/conic weights on import.
4. Preserve fit-point/Bezier tangents and knot parametrization through
   LibreCAD import/export sidecars.
5. Normalize hatch-boundary rational spline weight storage.
6. Add visible fallback plus sidecar preservation for degree > 3 splines.
7. Bound-check spline counts before allocation/consumption.
8. Expand spline/Bezier tests across AC1015, AC1018, AC1027, and AC1032.

### Acceptance Criteria

- AC1027/AC1032 fit-point/Bezier splines written by LibreCAD are classified as
  scenario 2 by ACadSharp's R2013+ rules.
- AC1027/AC1032 splines with custom knot parameters parse without
  desynchronization and retain their flag/knot metadata.
- Rational quadratic Bezier conics retain their weights or convert to exact
  native conic entities; they are not downgraded to unweighted
  `LC_SplinePoints`.
- Imported fit-point/Bezier splines preserve tangents and knot parametrization
  when exported unchanged.
- Hatch-boundary rational splines retain weights across import/export.
- Valid high-degree splines remain visible, and unchanged originals can
  round-trip through sidecar metadata.
- Malformed spline counts fail locally without corrupting the next entity.

### Validation Steps

- Add byte-level assertions for AC1027/AC1032 `Spline flags 1` and
  `knot parameter` so scenario 1 and scenario 2 selection matches ACadSharp.
- Add low-level encode/decode tests for weighted scenario 1 splines,
  fit-point/Bezier scenario 2 splines, and high-degree unsupported splines.
- Add filter-level tests for rational quadratic Bezier weights `< 1`, `== 1`,
  and `> 1`, verifying native conic conversion or exact weighted-spline
  preservation.
- Add hatch-boundary tests that compare DXF and DWG weight storage paths.
- Add malformed knot/control/fit count tests and assert parser failure does not
  corrupt the next object.

## Conics Support Review And Fix Plan

### Scope Reviewed

- Native DWG/DXF conic entities: `CIRCLE`, `ARC`, and `ELLIPSE`.
- Spline-backed conics used by LibreCAD: rational quadratic Bezier splines for
  hyperbola, parabola, and ellipse-arc cases.
- LibreCAD filter conversion points:
  `addCircle()`, `addArc()`, `addEllipse()`, `writeCircle()`, `writeArc()`,
  `writeEllipse()`, `writeHyperbola()`, `writeSplinePoints()`, and
  `handleQuadraticConicSpline()`.
- Reference behavior from ACadSharp `readArc()`, `readCircle()`,
  `readEllipse()`, `writeArc()`, `writeCircle()`, and `writeEllipse()`, plus
  libreDWG/ODA layouts for `ARC`, `CIRCLE`, `ELLIPSE`, and `SPLINE`.

### Current State

- Low-level libdxfrw DWG readers/writers for `CIRCLE`, `ARC`, and `ELLIPSE`
  match the core ACadSharp/ODA field order: center, radius/axis, thickness,
  extrusion/normal, and arc parameters where applicable.
- LibreCAD imports native circle/arc/ellipse geometry into 2D entities and
  writes them back through `DRW_Circle`, `DRW_Arc`, and `DRW_Ellipse`.
- Hyperbola export exists through `LC_HyperbolaSpline::hyperbolaToSpline()`.
- Parabola import/export uses `LC_Parabola`/`LC_SplinePoints` and the generic
  spline-points writer.
- Ellipse arcs represented as rational quadratic Bezier splines are currently
  not converted to native `RS_Ellipse`; they fall through to the generic
  degree-2 spline-points path unless the spline plan changes that routing.

### Findings And Fixes

1. **P1: Native conics lose normal/thickness/Z metadata at the LibreCAD filter
   boundary**

   `DRW_Circle`, `DRW_Arc`, and `DRW_Ellipse` carry center Z, thickness, and
   extrusion/normal. `RS_FilterDXFRW::addCircle()`, `addArc()`, and
   `addEllipse()` import only XY geometry. The matching writers emit default
   Z/thickness/normal values because `writeCircle()`, `writeArc()`, and
   `writeEllipse()` reconstruct `DRW_*` objects only from LibreCAD's 2D
   geometry. That is acceptable for ordinary 2D editing, but unchanged
   non-planar or thick conics cannot round-trip against ACadSharp/ODA fields.

   Fix:

   - Add explicit conic sidecar metadata for imported circle/arc/ellipse
     entities: original center Z, thickness, extrusion/normal, and original
     DWG/DXF type.
   - Rehydrate these fields in the writer when the conic's 2D geometry has not
     been transformed in a way that invalidates the original 3D metadata.
   - For transformed entities, either update the sidecar through the transform
     or deliberately drop it and export the current 2D geometry.
   - Add AC1015/AC1024/AC1027/AC1032 tests for non-default extrusion and
     thickness on circle, arc, and ellipse.

2. **P1: Hyperbola export writes weighted data without the rational spline flag**

   `LC_HyperbolaSpline::hyperbolaToSpline()` emits weights
   `{1, cosh(delta/2), 1}` but sets `DRW_Spline::flags = 8`, whose comment
   incorrectly labels the value as rational. Bit `0x04` is the DXF/DWG rational
   flag; `0x08` is planar. `DRW_Spline::encodeDwg()` still writes the weight
   array because `weightlist` is non-empty, but the rational bit is false.
   Other readers can interpret this as an inconsistent weighted-but-not-rational
   spline, and LibreCAD's own conic classification relies on the weight list
   rather than the flag.

   Fix:

   - Set hyperbola spline flags to `0x08 | 0x04` when weights are present.
   - Normalize this in `DRW_Spline::encodeDwg()` too: if a complete non-default
     `weightlist` is emitted, set the rational bit unless explicitly forbidden.
   - Add tests that inspect both `flags & 0x04` and `weightlist` after
     hyperbola-to-spline conversion and DWG round-trip.

   Status: implemented; hyperbola spline tests now assert planar+rational
   flags. DWG round-trip inspection remains follow-up.

3. **P1: Rational quadratic ellipse arcs are not recognized as native conics**

   The standard rational quadratic conic rule is already documented in the
   code: middle weight `< 1` is an ellipse arc, `== 1` is a parabola, and
   `> 1` is a hyperbola. The dispatcher handles hyperbola and parabola, but
   leaves the ellipse-arc branch to generic degree-2 import, which drops
   weights in `LC_SplinePoints`. This makes an exact ellipse arc visually and
   semantically drift after import/export.

   Fix:

   - Add `LC_EllipseSpline` or equivalent helper for rational quadratic Bezier
     ellipse arcs with weights `{1, w, 1}`, `0 < w < 1`, and canonical knots.
   - Convert recognized ellipse arcs to `RS_Ellipse` when the recovered conic is
     valid and bounded.
   - If exact native conversion fails, preserve as weighted `RS_Spline` rather
     than `LC_SplinePoints`.
   - Add tests for rotated, translated, small-span, and near-semicircle ellipse
     arcs.

4. **P2: Parabola export uses the generic spline-points path instead of the
   explicit parabola converter**

   `RS_FilterDXFRW::writeEntity()` routes `EntityParabola` through
   `writeSplinePoints()` because `LC_Parabola` inherits `LC_SplinePoints`.
   That can work for simple cases, but it bypasses
   `LC_ParabolaSpline::parabolaToSpline()`, which is the explicit canonical
   non-rational quadratic Bezier representation and already has dedicated
   tests.

   Fix:

   - Add `writeParabola(LC_Parabola*)` and route `EntityParabola` to it.
   - Use `LC_ParabolaSpline::parabolaToSpline()` so DXF/DWG exports have
     canonical degree-2, 3-control-point, `{0,0,0,1,1,1}` knot data.
   - Keep `writeSplinePoints()` for ordinary interpolation splines.
   - Add a filter-level round-trip test proving imported parabola splines
     export as canonical quadratic splines.

5. **P2: Circle/arc radius and ellipse ratio validation is weaker than
   ACadSharp**

   ACadSharp clamps non-positive circle/arc radii to an epsilon while reading.
   LibreCAD stores raw values from DWG and can construct invalid native
   entities or write invalid geometry back. Ellipse ratio and axis-vector
   validation are also deferred; `correctAxis()` is used in the DXF writer but
   the DWG writer writes the raw ratio and axis from the filter.

   Fix:

   - Validate native conic payloads during `parseDwg()` and at filter import:
     reject or clamp non-positive radii, zero-length ellipse axes, and invalid
     ratio values.
   - Normalize ellipse ratios greater than 1.0 consistently for both DWG and
     DXF export, or preserve the original form through sidecar metadata when
     round-trip fidelity is more important.
   - Emit diagnostics that distinguish malformed geometry from unsupported 3D
     metadata.
   - Add malformed conic tests that fail locally without corrupting following
     entities.

6. **P2: Ellipse arc direction and full-ellipse semantics need explicit
   round-trip tests**

   LibreCAD represents full ellipses with `angle1 == angle2 == 0`, while DWG
   stores start/end parameters. The filter maps `0..2pi` to `0..0` on import
   and reverses arcs by swapping parameters on export. This is plausible but
   currently under-tested across DWG versions and hatch-boundary edge paths.

   Fix:

   - Add tests for full ellipse, clockwise/counterclockwise ellipse arcs, and
     arcs crossing the `0` parameter.
   - Include hatch-boundary ellipse arcs, where `isccw` is explicit and the
     path data is 2D.
   - Ensure `RS_Ellipse` reversal maps to valid DWG start/end parameters without
     changing the intended sweep.

7. **P3: Conic test coverage is split and misses filter-level DWG behavior**

   Existing tests cover low-level encode/decode for circle, arc, and ellipse
   and engine-level parabola/hyperbola math. They do not prove that LibreCAD's
   filter layer preserves conic metadata, routes parabola/hyperbola/ellipse
   spline conics correctly, or remains interoperable with AC1027/AC1032
   readers.

   Fix:

   - Add a focused conic DWG matrix:
     - Native circle/arc/ellipse with default and non-default normal/thickness.
     - Hyperbola as rational quadratic spline with rational flag set.
     - Parabola as canonical non-rational quadratic spline.
     - Ellipse arc as rational quadratic spline with middle weight `< 1`.
     - Full ellipse and reversed/cross-zero ellipse arc cases.
   - Run at least AC1015, AC1018, AC1024, AC1027, and AC1032 where writer
     support exists.

### Priority Order

1. Preserve native conic Z/thickness/normal metadata through import/export
   sidecars.
2. Set and validate the rational flag for hyperbola weighted splines.
3. Add rational quadratic ellipse-arc recognition and native conversion or
   exact weighted-spline preservation.
4. Add explicit `writeParabola()` using `LC_ParabolaSpline::parabolaToSpline()`.
5. Harden radius, axis, ratio, and count validation for conic payloads.
6. Expand full/reversed/cross-zero ellipse arc and hatch-boundary tests.
7. Add cross-version filter-level conic round-trip coverage.

### Acceptance Criteria

- Unchanged imported circle/arc/ellipse entities with non-default
  normal/thickness/Z export those fields again.
- Hyperbola exports contain both the rational flag and the weight array, and
  read back as the same conic class.
- Rational quadratic ellipse arcs no longer degrade into unweighted
  `LC_SplinePoints`.
- Parabola exports use canonical quadratic spline data.
- Invalid native conic payloads are clamped or rejected without desynchronizing
  subsequent entities.
- Full and reversed ellipse arcs round-trip consistently across entity and
  hatch-boundary paths.

### Validation Steps

- Add native circle/arc/ellipse tests for default and non-default
  Z/thickness/normal across AC1015, AC1018, AC1024, AC1027, and AC1032.
- Add filter-boundary tests proving imported conic sidecars rehydrate unchanged
  exports and are invalidated or transformed after geometry edits.
- Add rational quadratic conic tests for ellipse arc, parabola, and hyperbola
  classification from spline weights.
- Add malformed native conic payload tests for non-positive radius, zero-length
  ellipse axis, invalid ratio, and bad start/end parameters.
- Add hatch-boundary arc/ellipse tests for clockwise, counterclockwise,
  full-ellipse, and cross-zero cases.
