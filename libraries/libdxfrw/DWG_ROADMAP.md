# DWG ACadSharp Gap-Closure Roadmap

Last reviewed: 2026-05-28.

This is the single current DWG/DXF implementation plan for LibreCAD/libdxfrw.
It replaces the earlier split roadmap and support-plan files. The cleanup scan
on 2026-05-27 found no other tracked plan files; keep future DWG planning in
this file to avoid plan drift.

## Review Scope

Primary comparison target:

- `../ACadSharp/src/ACadSharp/IO/DWG/`
- `../ACadSharp/src/ACadSharp/Entities/`
- `../ACadSharp/src/ACadSharp/Objects/`
- `../ACadSharp/src/ACadSharp/Tables/`

Local implementation surface:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/drw_objects.*`
- `libraries/libdxfrw/src/drw_interface.h`
- `libraries/libdxfrw/src/intern/dwgreader.cpp`
- `libraries/libdxfrw/src/intern/dwgwriter*.{h,cpp}`
- `libraries/libdxfrw/src/libdwgr.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.*`
- `librecad/src/lib/filters/tests/*.cpp`

Authoritative layout references:

- ODA/Open Design DWG specification: `~/doc/dwg/dwg.pdf`
- libreDWG layout specs:
  `../libreDWG/src/dwg.spec`,
  `../libreDWG/src/dwg2.spec`, and
  `../libreDWG/src/objects.inc`

## Current Baseline

LibreCAD/libdxfrw already has useful modern-DWG scaffolding:

- DWG write smoke coverage for AC1015, AC1018, AC1024, AC1027, and AC1032.
- Core entity encode/decode and bit-buffer tests under `[dwg-write]`.
- Named VIEW import/export and VIEW_CONTROL writing.
- Metadata storage for raw unsupported objects, VIEW, LIGHT, SUN, modeler
  geometry, TABLE records, associative shells, ACSH shells, MLEADER,
  MLEADERSTYLE, detail/section view styles, GROUP, IMAGEDEF_REACTOR,
  SPATIAL_FILTER, GEODATA, TABLEGEOMETRY, ACDBPLACEHOLDER, and related raw
  replay state.
- Raw replay for unchanged non-entity DWG OBJECT records, including class
  metadata, body bit size, object offset/size, raw bytes, invalidation, and
  replacement policy.
- Partial readers for DICTIONARY, DICTIONARYWDFLT, DICTIONARYVAR, XRECORD,
  FIELD, FIELDLIST, SORTENTSTABLE, RASTERVARIABLES, MATERIAL, TABLESTYLE,
  TABLE, TABLECONTENT, CELLSTYLEMAP, TABLEGEOMETRY, GROUP,
  IMAGEDEF_REACTOR, SPATIAL_FILTER, GEODATA, ACDBPLACEHOLDER, LIGHT, SUN,
  DIMASSOC, EVALUATION_GRAPH, ACDBASSOC shells, ACSH shells, MLEADER,
  MLEADERSTYLE, modeler geometry, and modern spline/table value payloads.
- Text-content MLEADER import into `LC_MLeader` and partial AC1024+ DWG text
  MLEADER writing.
- AC1032 multiline ATTRIB/ATTDEF writing supports embedded MTEXT payloads for
  type 2 and type 4 records.
- LIGHT entities can be natively written and round-tripped for AC1024+,
  including photometric metadata already captured by the reader.
- Native DWG TOLERANCE parse/write coverage for AC1015, AC1024, and AC1032.
- R2004+ HATCH gradient fields are emitted instead of flattened to
  non-gradient records.
- Decomposed MLINE sidecars are reconstructed through the native DWG MLINE
  writer when exporting DWG.
- Prior fixes for unit-system round-trip, R2007+ block insertion units,
  polyline/spline/conic fidelity, R2013/R2018 parser alignment, raw replay
  invalidation, and replay diagnostics.
- Writer-side custom class registration now deduplicates by class identity,
  rejects conflicting metadata, and accumulates raw replay instance counts.
- AC1015/R2000 writes now include an AcDb:AuxHeader locator and stream; R2004+
  page writers include AuxHeader content where the container format supports it.

ACadSharp is broader in these areas:

- Full semantic TableEntity/TableContent/TableStyle readers and writers.
- More complete MLeader context, style, block, tolerance, and override models.
- Dimension association, evaluation graph, dynamic block action/parameter/grip,
  and object-context readers.
- Modeler geometry, raster/image/underlay, plot/layout, visual style, material,
  TOLERANCE, MESH, OLE2FRAME, SHAPE, and related object writers.
- DWG file-structure details such as AuxHeader/second-header writing.
- Proxy/unknown entity/object preservation paths.

## Non-Negotiable Implementation Rules

1. Preserve bytes before decoding semantics.
2. Never replace a raw replay payload with a partial semantic shell.
3. Bounds-check every variable-length count and byte-size before allocation or
   loop consumption.
4. A malformed object must fail locally or degrade to a raw shell. It must not
   desynchronize the next object.
5. Keep DWG-only data in `LC_DwgAdvancedMetadata` until there is a real
   LibreCAD editing/rendering consumer.
6. Add native writers only when owner handles, version gates, class metadata,
   and downgrade behavior are understood.
7. Export unsupported edited data with explicit diagnostics instead of silent
   loss.
8. Keep each phase buildable and testable before moving to the next phase.

## Gap Matrix

| Area | ACadSharp capability | Local gap | Priority |
| --- | --- | --- | --- |
| File structure | Writes AuxHeader/second header by version | AC1015/R2000 and R2004+ AuxHeader streams are emitted; reader-side validation remains smoke-level | P3 |
| Raw/proxy preservation | Unknown/proxy entity and object models | Raw replay is non-entity OBJECT only; entity replay is blocked | P1 |
| Semantic tables | Reads/writes TableEntity, TableContent, cell contents, borders, breaks, styles | Local parsing is summary/metadata-first; fallback rendering and native writing are incomplete | P1 |
| MLEADER | Rich context/style/content/override handling | Text-content subset only; style writing, block/tolerance content, complex overrides missing | P1 |
| AC1032 text | R2018 attribute and MTEXT context readers/writers | Multiline ATTRIB/ATTDEF embedded MTEXT writing is covered; richer annotative payload replay and full UI integration remain | P2 |
| Modeler geometry | BODY/REGION/3DSOLID modeler data and history | Raw shell exists, but SAT/SAB segmentation and fallback wire/silhouette metadata are incomplete | P2 |
| Associative/dynamic graph | DIMASSOC, EVAL_GRAPH, ACDBASSOC*, dynamic block params/actions/grips | Shell/raw preservation only; no graph typing/evaluation or replay dependency policy beyond invalidation | P2 |
| Advanced entities | TOLERANCE, MESH, OLE2FRAME, SHAPE, richer image/raster/underlay, MLINE writing | TOLERANCE and MLINE DWG export are covered; MESH, OLE2FRAME, SHAPE, raster/underlay, and several richer entity semantics remain | P2 |
| Object metadata | Layout, PlotSettings, VisualStyle, Material, MLineStyle, UnderlayDefinition, ImageDefinition, Scale, SortEnts, GeoData, SpatialFilter writers | Some readers/shells exist; native regeneration and document integration are partial | P2 |
| LIGHT/SUN | Preserves light/sun records and view links | LIGHT and metadata-complete SUN records can be natively regenerated; view/vport lighting UI integration remains future work | P2 |
| Fixtures/interoperability | Broad fixture-driven regression potential | External fixture manifest is optional; no ACadSharp/libreDWG diff harness yet | P2 |

## Implementation Progress

Completed in the 2026-05-27 implementation pass:

- Slice 4 fixture-manifest skeleton tags were expanded and validated.
- Slice 1 class registry accounting was implemented with conflict rejection and
  raw replay instance counting.
- Slice 2 AuxHeader coverage was implemented for AC1015/R2000 and is already
  present for R2004+ page-based writers.
- Slice 13 TOLERANCE entity support was implemented for native DWG
  parse/write, public `dwgRW` writing, LibreCAD export wiring, and tests.
- Slice 14 partial advanced-entity coverage was improved:
  - HATCH gradient fields now write and round-trip.
  - MLINE sidecar reconstruction now uses native DWG MLINE export.
- Slice 10 multiline AC1032 ATTRIB/ATTDEF writing now emits embedded MTEXT
  payloads and smoke-loads the resulting AC1032 file.
- Slice 16 partial LIGHT integration: native AC1024+ LIGHT writing and
  round-trip tests were added.
- Slice 16 SUN object integration: metadata-complete AC1024+ SUN records now
  write natively, register `AcDbSun`, skip replaced raw SUN replay, and
  round-trip through the DWG reader.
- Slice 15 fixed-object shell writing: ACDBPLACEHOLDER records now write
  natively, skip replaced raw replay, and round-trip through the DWG reader.
- Slice 8/9 MLEADERSTYLE metadata support advanced: MLEADERSTYLE now decodes
  style handle references and the R2013 text-extended bit, preserves raw
  payloads for replay, exposes a native AC1024+ writer API, registers
  `AcDbMLeaderStyle`, and round-trips the parsed metadata through the reader.
- Slice 8 MLEADER graph lookup advanced: metadata exposes MLEADER and
  MLEADERSTYLE lookup by handle, style-use lookup, referenced-handle lookup,
  and graph invalidation for style, arrow, linetype, text-style, block, and
  attribute-definition dependencies.
- Slice 9 style-backed MLEADER export advanced: imported LibreCAD MLeader
  entities now preserve DWG style, leader line type, arrow, text-style, and
  block handles and feed those references back into native AC1024+ text
  MLEADER writing.
- Slice 16 visual/lighting metadata lookup support advanced: the document
  metadata store now exposes view-by-handle, light-by-handle, lights-by-owner,
  and active-sun-for-view lookups for future UI/rendering and export logic.
- Slice 5 table metadata graph advanced: TABLE/TABLECONTENT records now
  resolve TABLESTYLE handles independent of import order and expose table,
  table-style, and style-use lookup helpers for fallback rendering/writer
  follow-ups.
- Slice 5 semantic table metadata advanced: TABLE/TABLECONTENT summaries now
  retain per-cell row/column, style, override, value, text/block/field handle,
  geometry, content, and attribute details for follow-up rendering and writing.
- Slice 5 table-cell graph lookup advanced: callers can resolve cells by
  table handle/row/column and find cells that reference FIELD, block, value,
  style, or geometry handles.
- Slice 5 table reference lookup advanced: callers can find all semantic table
  records affected by a referenced TABLESTYLE, FIELD, block, value, or geometry
  handle without duplicating cell/content scans.
- Slice 5 table graph invalidation advanced: table metadata is invalidated
  when referenced style, FIELD, block, value, or geometry handles change.
- Slice 5 table raw replay safety advanced: invalidated semantic
  TABLE/TABLECONTENT records now also invalidate matching preserved raw table
  objects so stale raw payloads are not replayed after a dependent handle is
  edited.
- Slice 5 table attribute graph advanced: TABLE/TABLECONTENT cell attributes
  now preserve referenced handles, participate in cell/table dependency
  lookups, invalidate semantic/raw table replay when edited, and are counted
  in native table writer blocker diagnostics.
- Slice 5 table style handle graph advanced: TABLESTYLE text-style and border
  linetype handles are now preserved in metadata, exposed through style
  reference lookup, and invalidate matching semantic/raw TABLESTYLE replay
  when edited.
- Slice 6 table export diagnostics advanced: metadata exposes native table
  writer blocker counts and DWG export logs fallback, incomplete parse,
  unresolved style, FIELD, block, attribute, override, and geometry blockers.
- Slice 11-12 shell graph lookup support advanced: modeler geometry, ACDBASSOC
  shells, and ACSH history records now expose handle/reference lookup helpers
  and associative kind names/lookups so follow-up decoders can connect ACIS
  bodies, action dependencies, and history nodes without duplicating scans.
- Slice 12 raw queue support advanced: preserved raw objects now classify
  associative, evaluation-graph, dynamic-block, and object-context families for
  diagnostics and future targeted decoders; the metadata store also exposes
  per-family counts and DWG export logs those counts during object writing.
- Slice 4 fixture-manifest support advanced: the optional external fixture
  manifest now declares expected raw-object families alongside callback and
  replay expectations.
- Slice 11 modeler payload queue support advanced: preserved modeler raw bytes
  are now classified as likely SAT, SAB, or unknown from ACIS payload markers.
- Ready A MLEADER writer diagnostics advanced: metadata now reports MLEADER
  native-writer blocker counts for unresolved styles, missing text, block and
  tolerance content, override flags, missing leader geometry, and
  invalidated/replaced replay state; DWG export logs the summary.
- Ready B modeler payload byte-range metadata advanced: preserved ACIS/modeler
  payloads now expose recognizable SAT/SAB marker text, offset, length, and
  category while keeping unknown payloads raw-only.
- Ready C associative prefix accounting advanced: ACDBASSOC action value-param
  counts, owned-param prefix counts, and action-param prefix parse status are
  now surfaced through libdxfrw shells and LibreCAD DWG metadata.
- Ready D VIEW/UCS/visual dependency lookup advanced: VIEW records now expose
  reference lookup and invalidation for named/base UCS, background, visual
  style, sun, and live-section handles.
- Ready E fixture harness diagnostics advanced: optional DWG fixture manifest
  tests now parse expected tags, callbacks, raw-family expectations, reference
  dump paths, and load/raw-preservation expectations into typed structures.
- Ready F modeler raw split diagnostics advanced: preserved modeler payloads
  now expose body/handle-stream byte split metadata, marker section
  classification, split consistency checks, and export-time payload summaries.
- Ready G associative shell diagnostics advanced: ACDBASSOC/ACSH shell
  metadata now reports per-kind counts, value-param parse accounting,
  action-param prefix accounting, and export-time graph summaries.

Still incomplete:

- Reader-side structural validation for AuxHeader/second-header remains shallow.
- Slice 5-7 semantic table graph is coherent enough for dependency tracking;
  the next safe table work is fallback rendering/diagnostics, not native table
  writing yet.
- Slice 8-10 MLEADER style/content metadata is coherent enough for dependency
  tracking and text-content writing; the next safe MLEADER work is explicit
  writer blocker diagnostics for block/tolerance/override cases.
- Slice 11-12 ACIS/modeler and associative/action/history graph expansion has
  queryable shell records; the next safe work is bounded sub-block counts and
  typed payload summaries, not ACIS interpretation or action evaluation.
- Slice 14 remaining advanced entities: MESH, SHAPE, OLE2FRAME, raster/image/
  underlay DWG writing, and fuller HATCH semantics beyond gradients.
- Slice 15-16 remaining object metadata writers and VIEW/UCS/lighting UI
  integration; visual/lighting records are queryable by core handles, but view
  dependency invalidation and UI/rendering integration are not complete.

## Implementation-Ready Queue

Reviewed against the current codebase on 2026-05-28. These are the next
bounded items Gibbs or another implementation pass can start without needing a
new native writer contract or external fixture first.

### Ready A: MLEADER Writer Blocker Diagnostics

Why ready: `LC_DwgAdvancedMetadata::MLeaderRecord` already stores style
resolution, content-type, block-content, override flags, root/line counts,
text-content state, and dependent handles. `RS_FilterDXFRW::writeMLeader()`
already warns for block-content fallback; object writing does not yet provide a
metadata-level summary like table writing does.

Status: complete in the current implementation pass. Keep future MLEADER work
diagnostics-first unless block/tolerance ownership and downgrade contracts are
added.

Implementation steps:

1. Add `MLeaderWriterBlockerCounts` to `LC_DwgAdvancedMetadata`.
2. Count unresolved style, missing/empty text content, block content,
   tolerance content (`effectiveContentType == 3`), nonzero override flags,
   no roots/leader lines, and invalidated/replaced replay state.
3. Log the counts from `RS_FilterDXFRW::writeObjects()` next to table blocker
   diagnostics.
4. Add `entity_metadata` tests with synthetic text, block, tolerance,
   unresolved-style, override, and invalidated records.

Acceptance:

- `[entity_metadata]` proves each blocker bucket.
- `[dwg-write]` remains green.
- qmake6 remains green.

Stop before native block/tolerance writing; this slice is diagnostics only.

### Ready B: Modeler Payload Byte-Range Metadata

Why ready: `DRW_ModelerGeometry` already preserves full raw object bytes,
body bit size, object size, empty/body flags, 3DSOLID history handle, and a
coarse SAT/SAB marker classification. The safe next step is metadata indexing
inside the raw payload, not ACIS decoding.

Status: complete in the current implementation pass. The metadata exposes
marker byte ranges only; ACIS/SAB interpretation remains deferred.

Implementation steps:

1. Extend `ModelerGeometryRecord` with marker offset, marker length, and
   marker text/category for recognizable SAT/SAB headers.
2. Add a small helper that scans `rawBytes` for known markers such as
   `ACIS BinaryFile`, `Begin-of-ACIS-History`, and `ACIS`.
3. Keep unknown payloads as unknown with offset/length zero.
4. Add tests for SAB, SAT, and unknown payloads using synthetic byte vectors.
5. Update export diagnostics only if the metadata is incomplete or edited;
   do not attempt native modeler writing.

Acceptance:

- `[entity_metadata]` proves marker offsets and unknown handling.
- No parser field order changes are needed.

Stop before interpreting ACIS entities, wire bodies, or SAT/SAB records.

### Ready C: Associative Value-Param and Prefix Accounting

Why ready: `DRW_AssociativeObject::parseDwg()` already skips action value
params and action-param prefixes safely, but the skipped counts are not exposed
in metadata. ACadSharp/libreDWG parity can improve by reporting these bounded
decode queues without evaluating the graph.

Status: complete in the current implementation pass. The parser exposes
bounded counts/status only; constraint evaluation remains deferred.

Implementation steps:

1. Add fields to `DRW_AssociativeObject` for value-param count, owned-param
   prefix count where available, and booleans indicating prefix parse success.
2. Populate those fields in the existing bounded parser paths without changing
   skip order.
3. Mirror them into `LC_DwgAdvancedMetadata::AssociativeRecord`.
4. Add lookup/diagnostic tests for action records with owned params and value
   params.
5. Keep raw replay invalidation behavior unchanged.

Acceptance:

- `[entity_metadata]` proves counts reach metadata.
- Existing associative raw replay invalidation tests still pass.

Stop before evaluating constraints, dynamic-block actions, or dependency
ownership semantics.

### Ready D: VIEW/UCS/Visual Dependency Lookup

Why ready: `ViewRecord` already stores named/base UCS, background, visual
style, sun, live-section, and lighting fields. The missing piece is the same
reference lookup/invalidation support now present for TABLE and MLEADER.

Status: complete in the current implementation pass. This is metadata-only;
UI/rendering integration remains deferred.

Implementation steps:

1. Add `findViewsReferencingHandle(duint32)` to metadata.
2. Add `invalidateViewGraphForHandle(duint32)` to mark dependent views stale.
3. Include named/base UCS, background, visual style, sun, and live-section
   handles in the reference predicate.
4. Add synthetic metadata tests for UCS, visual-style, and sun references.
5. Do not add UI/rendering behavior in this slice.

Acceptance:

- `[entity_metadata]` proves lookup and invalidation.
- No writer behavior changes are required.

Stop before view/vport UI integration or visual-style rendering.

### Ready E: Fixture Harness Diagnostics

Why ready: `libraries/libdxfrw/testdata/dwg-fixtures.json` exists and is
optional. The current tests validate JSON shape; they do not yet summarize
which expectations would run when fixtures are present.

Status: complete in the current implementation pass. The default test remains
offline/optional and does not require ACadSharp or libreDWG executables.

Implementation steps:

1. Add a small manifest helper/test that parses expected tags, callback
   expectations, raw-family expectations, and optional reference dump paths
   into typed local test structures.
2. Keep missing fixture files non-fatal.
3. Emit test diagnostics showing skipped fixture paths and enabled feature
   tags.
4. Do not require ACadSharp or libreDWG executables in default CI.

Acceptance:

- `[dwg-smoke]` or `[entity_metadata][dwg_fixtures]` validates manifest
  semantics without external files.

Stop before adding mandatory fixture downloads or network access.

### Ready F: Modeler Raw Split Diagnostics

Why ready: `DRW_ModelerGeometry` already preserves the full raw object bytes
and the R2010+ body bit-size value. Ready B added marker offsets, so the next
bounded step is to report where that marker lands relative to the body/handle
stream split and to diagnose inconsistent split metadata.

Status: complete in the current implementation pass. This is byte-indexing
only; ACIS/SAB interpretation, wireframe extraction, and modeler rewriting
remain deferred.

Implementation steps:

1. Add modeler raw-byte split metadata to `ModelerGeometryRecord`: whether the
   body bit-size split is known, whether it is consistent with preserved raw
   bytes, body byte count, handle-stream byte count, and marker section.
2. Add helper methods for split calculation and payload-section naming.
3. Add aggregate modeler payload counts for SAT/SAB/unknown records,
   inconsistent splits, and marker section placement.
4. Log modeler payload summaries during DWG object writing, warning only when
   split metadata is inconsistent.
5. Add synthetic `[entity_metadata]` tests for unknown, consistent, and
   inconsistent splits plus marker-in-handle-stream placement.

Acceptance:

- `[entity_metadata]` proves split counts and marker sections.
- `[dwg-write]` remains green because export diagnostics changed.
- qmake6 remains green.

Stop before parsing ACIS sub-blocks, interpreting SAT/SAB geometry, or
rewriting modeler entities natively.

### Ready G: Associative Shell Diagnostics

Why ready: `DRW_AssociativeObject` already exposes associative kind,
value-param counts, action-param prefix parse status, dependency handles, and
raw replay state. The safe next step is an aggregate diagnostic layer; graph
evaluation remains out of scope.

Status: complete in the current implementation pass. This reports the shell
decode queue only; it does not evaluate constraints or regenerate
ACDBASSOC/action objects.

Implementation steps:

1. Add `AssociativeShellCounts` to `LC_DwgAdvancedMetadata`.
2. Count records by `AssociativeKind`, value-param records vs parsed
   value-param records, action-param records vs parsed prefixes, and the
   single-dependency/compound prefix variants.
3. Add a helper for querying a count by associative kind.
4. Log aggregate shell diagnostics from DWG object writing.
5. Extend `[entity_metadata]` tests for synthetic action and action-param
   shells.

Acceptance:

- `[entity_metadata]` proves per-kind and prefix/value-param counts.
- `[dwg-write]` remains green because export diagnostics changed.
- qmake6 remains green.

Stop before decoding dynamic-block actions, evaluating constraints, or writing
native ACDBASSOC graph objects.

### Not Ready As One-Shot Work

These roadmap items should remain deferred until the listed contract exists:

- Native semantic TABLE writing: needs a complete AC1021+ TABLE/TABLECONTENT
  writer layout, owner/dictionary handles, and a text-only fixture proving
  round-trip.
- Table fallback rendering: needs a LibreCAD-side attachment policy for mapping
  fallback grid/text entities back to native table metadata after edits.
- Native MLEADER block/tolerance writing: needs block attribute/tolerance
  content ownership and downgrade rules; diagnostics are ready first.
- ACIS/modeler interpretation: needs fixture-backed SAT/SAB byte slicing and
  a policy for fallback wire/silhouette geometry.
- Associative/action graph evaluation: needs semantics for invalidating or
  preserving constraints after geometry edits; current work should stay
  shell/metadata-only.
- MESH/SHAPE/OLE2FRAME/raster/underlay DWG writers: each needs a separate
  class/handle registration and writer contract.
- VIEW/UCS/LIGHT/SUN UI integration: metadata lookup can proceed, but rendering
  behavior requires a LibreCAD UI/design decision.

## Detailed Forward Implementation Plan

This section is the implementation-grade plan for the remaining broad feature
areas. It supersedes the short "not ready" bullets above and refines Slices
5-16 into source-backed phases that can be converted into small ready slices.

Reference sources for this section:

- ACadSharp table model and writers:
  `../ACadSharp/src/ACadSharp/Entities/TableEntity*.cs`,
  `../ACadSharp/src/ACadSharp/Objects/Table*.cs`,
  `../ACadSharp/src/ACadSharp/IO/DWG/DwgStreamWriters/DwgObjectWriter.*.cs`.
- ACadSharp modeler geometry:
  `../ACadSharp/src/ACadSharp/Entities/ModelerGeometry*.cs`,
  `../ACadSharp/src/ACadSharp/IO/DWG/DwgStreamReaders/DwgObjectReader.cs`,
  `../ACadSharp/src/ACadSharp/IO/DWG/DwgStreamWriters/DwgObjectWriter.Entities.cs`.
- ACadSharp associative/evaluation objects:
  `../ACadSharp/src/ACadSharp/Objects/DimensionAssociation*.cs` and
  `../ACadSharp/src/ACadSharp/Objects/Evaluations/*.cs`.
- libreDWG layouts:
  `../libreDWG/src/dwg.spec`, `../libreDWG/src/dwg2.spec`,
  `../libreDWG/src/acds.spec`, `../libreDWG/src/classes.inc`.
- Current LibreCAD/libdxfrw implementation:
  `libraries/libdxfrw/src/drw_entities.*`,
  `libraries/libdxfrw/src/drw_objects.*`,
  `libraries/libdxfrw/src/intern/dwgreader.cpp`,
  `libraries/libdxfrw/src/intern/dwgwriter*.{h,cpp}`,
  `libraries/libdxfrw/src/libdwgr.*`,
  `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`,
  `librecad/src/lib/filters/rs_filterdxfrw.*`.

### Forward A: Native TABLE Writing and Rendering Policy

Goal: make TABLE/TABLECONTENT visible and exportable without pretending that
LibreCAD has a full native table editor.

Current local state:

- libdxfrw parses TABLE, TABLECONTENT, TABLESTYLE, CELLSTYLEMAP, and
  TABLEGEOMETRY into metadata-rich shells.
- `LC_DwgAdvancedMetadata` has table/style lookup, cell lookup, reference
  lookup, graph invalidation, raw replay invalidation, and writer blocker
  diagnostics.
- 2026-05-28 progress: TABLESTYLE metadata now surfaces parsed flow/flag/
  margin values, named cell style IDs/names, content-format counts, text
  heights, alignments, colors, visible-border counts, and margin-style counts
  for fallback rendering and writer-blocker decisions.
- No native TABLE/TABLECONTENT writer exists. No LibreCAD rendering policy
  attaches fallback geometry back to the native table record.

Phase A1 - complete semantic metadata prerequisites:

1. Compare current `DRW_TableContent`, `DRW_Table`, `DRW_TableStyle`,
   `DRW_CellStyleMap`, and `DRW_TableGeometry` against ACadSharp table
   templates and libreDWG `TABLECONTENT_fields`, `CellStyle_fields`,
   `CellStyleMap`, and `TableGeometry` layouts.
2. Add missing non-rendering metadata before any writer work: named cell
   style IDs/names, margins, text height, alignment, colors, border
   visibility, and content-format counts are surfaced. Remaining metadata
   work: row/column style inheritance, border lineweight/double-line details,
   break metadata, override masks, and complete CELLSTYLEMAP linkage.
3. Preserve all unknown table sub-record byte ranges with count, offset,
   version gate, and parse-complete flags.
4. Add metadata tests that synthetic table cells retain text, FIELD, block,
   attribute, style, geometry, border, and override summaries.

Phase A2 - fallback rendering policy:

1. Introduce a metadata-owned fallback group ID for each imported semantic
   table. Fallback line/text entities generated from the table must store the
   table handle and cell coordinate they came from.
2. Render only stable 2D primitives: grid lines from row/column sizes, text
   cells from available content/style summaries, and placeholder text for
   FIELD, block, formula, and unknown cell content.
3. Treat edited fallback entities as edits to the table facade, not as proof
   that native table semantics can be regenerated. Mark the semantic table and
   matching raw payload invalidated when any fallback entity changes.
4. Add export diagnostics for unchanged raw replay, fallback edited/no native
   writer, incomplete table parse, unresolved TABLESTYLE, FIELD content, block
   content, attribute content, geometry cell, override cell, and unsupported
   break/style data.
5. Keep fallback geometry optional during import until UI ownership and entity
   selection behavior are tested.

Phase A3 - minimal native writer subset:

1. Define a strict writer contract: AC1021+ only, table has complete style
   handle, rectangular row/column model, text-only cells, no FIELD, no block
   content, no formulas, no unsupported overrides, no unresolved attributes,
   and no edited fallback geometry outside the table facade.
2. Register required custom classes through the writer class registry with
   deterministic instance counts.
3. Emit TABLE/TABLECONTENT in the same ownership/dictionary path ACadSharp
   uses, with version-gated payloads from ODA/libreDWG.
4. Preserve or replay TABLESTYLE/CELLSTYLEMAP only when unchanged or when the
   metadata writer can emit every referenced style handle.
5. Refuse native writing with explicit blocker counts when any contract item
   fails; never flatten a semantic table silently.

Validation for Forward A:

- `[entity_metadata]` tests for cell/style/reference/fallback invalidation.
- `[dwg-write]` smoke for unchanged raw replay and one text-only native table
  subset after the writer exists.
- Optional fixture comparison against ACadSharp/libreDWG dumps for
  TABLECONTENT cell counts, style handles, and writer blocker reduction.

Stop line:

- Do not implement a spreadsheet-like editor, formula evaluator, block-cell
  editor, or complex table regeneration until LibreCAD has a table editing
  model. Use fallback rendering plus diagnostics until then.

### Forward B: ACIS and Modeler Geometry Interpretation

Goal: turn BODY/REGION/3DSOLID payloads into inspectable, safely preserved
modeler records and optional fallback geometry without building an ACIS
kernel inside LibreCAD.

Current local state:

- REGION, 3DSOLID, and BODY are parsed as `DRW_ModelerGeometry` shells.
- Metadata preserves raw bytes, body/handle-stream split, payload marker
  category, marker offset/length, history handles, and split diagnostics.
- Native ACIS interpretation, SAT/SAB sub-block indexing, wireframe fallback,
  silhouette extraction, and modeler writing are not implemented.

Phase B1 - byte container indexing:

1. Compare `DRW_ModelerGeometry::parseDwg()` with ACadSharp
   `readModelerGeometryData`, `readWire`, `readSolid3D`, libreDWG
   `COMMON_3DSOLID`, and `acds.spec`.
2. Record modeler kind, raw data format, block-size fields, modeler version,
   SAT/SAB marker text, byte ranges for SAT/SAB data, history block ranges,
   wire block ranges, silhouette block ranges, and unknown tails.
3. Bounds-check every size before scanning. Unknown or inconsistent records
   must stay raw-preserved with a diagnostic.
4. Add tests for synthetic SAT, SAB, history, empty body, inconsistent split,
   and unknown payloads.

Phase B2 - non-kernel fallback summaries:

1. Decode only container-level wire/silhouette records when libreDWG/ACadSharp
   identify stable counts and coordinate payloads.
2. Store fallback edge/polyline summaries in `LC_DwgAdvancedMetadata`; do not
   modify the native modeler raw payload.
3. Render fallback wire/silhouette geometry as non-authoritative preview
   entities only after the same attachment/invalidation policy used for table
   fallback entities exists.
4. Mark the modeler raw payload invalidated if fallback preview geometry is
   edited, because LibreCAD cannot regenerate ACIS bodies from edited curves.

Phase B3 - write and replay policy:

1. Continue raw replay for unchanged modeler OBJECT/ENTITY records only where
   owner/block membership and class metadata are preserved.
2. For edited modeler fallback geometry, export only the fallback curves with
   an explicit diagnostic that native ACIS regeneration is unavailable.
3. Add optional hooks for a future external ACIS/SAT/SAB backend, but keep the
   default build backend-free.
4. Do not attempt to infer solid topology or Boolean/history semantics from
   SAT/SAB strings.

Validation for Forward B:

- `[entity_metadata]` tests for byte ranges, fallback summaries, and raw
  invalidation.
- `[dwg-write]` diagnostics for unchanged raw replay versus edited fallback.
- Optional fixture checks that ACadSharp/libreDWG still report the same
  modeler class/kind and payload category after round-trip when unchanged.

Stop line:

- Do not implement native 3D solid editing, ACIS Boolean evaluation, or SAT/SAB
  rewriting in this roadmap. That requires a separate geometry-kernel design.

### Forward C: Associative Graph Evaluation

Goal: decode ACDBASSOC/evaluation/action/history objects into a dependency
graph that can be invalidated and diagnosed, then evaluate only deliberately
chosen simple cases.

Current local state:

- DIMASSOC, EVALUATION_GRAPH, ACDBASSOC*, ACSH*, and related records have raw
  shells, family classification, kind names, dependency/reference lookup,
  prefix/value-param accounting, graph invalidation, and export diagnostics.
- Constraint solving, dynamic-block action evaluation, regeneration of
  associative objects, and UI editing semantics are not implemented.

Phase C1 - typed graph decode:

1. Compare local shells with ACadSharp `DimensionAssociation`,
   `EvaluationGraph`, dynamic-block evaluation objects, and libreDWG
   `AcDbAssoc*`, `AcDbEvalExpr`, and `AcDbShHistoryNode` layouts.
2. Add typed node IDs, owning network/action handles, dependency handles,
   dependent-on object handles, action-body handles, value-param summaries,
   point/osnap references, compound/path/action-param prefixes, and
   parent/child history links.
3. Store every decoded graph edge in metadata as `(source handle, edge kind,
   target handle, confidence, raw range)`.
4. Keep raw replay metadata linked to the typed record so invalidating a graph
   node invalidates the preserved raw payload for the same handle/class.
5. Add tests for DIMASSOC, EVALUATION_GRAPH, ASSOCNETWORK, ASSOCACTION,
   ASSOCDEPENDENCY, GEOMDEPENDENCY, vertex action param, osnap point ref, and
   ACSH history node synthetic records.

Phase C2 - invalidation and edit policy:

1. Any edit to a referenced entity, block, parameter, dimension, table,
   mleader, or modeler body must mark dependent associative records stale.
2. Raw replay must consult stale graph state before writing; stale associative
   payloads are diagnostic-only.
3. Expose aggregate diagnostics by family: preserved unchanged, invalidated,
   unsupported action body, unsupported value-param, unsupported dynamic-block
   evaluator, missing dependency target, and malformed prefix.
4. Keep object-context records metadata-only until a consumer needs them.

Phase C3 - limited evaluation:

1. Start with non-mutating graph inspection: dependency lists, affected
   handles, and dynamic-block parameter summaries.
2. Add optional evaluation only for cases with deterministic 2D output and no
   hidden solver requirement, such as simple DIMASSOC handle tracing or
   visibility lookup summaries.
3. Do not regenerate ACDBASSOC objects after geometry edits until a tested
   writer contract exists for every node and dependency touched by that edit.
4. Any unsupported evaluated case must degrade to stale/raw-preserved
   diagnostics, not partial graph rewriting.

Validation for Forward C:

- `[entity_metadata]` tests for edge construction, dependency invalidation,
  stale raw suppression, and aggregate diagnostics.
- Fixture diagnostics proving known ACDBASSOC/ACSH classes are no longer
  anonymous raw records where layouts are implemented.
- Optional reference dump comparison for node/edge counts against
  ACadSharp/libreDWG.

Stop line:

- Do not implement a geometric constraint solver, dynamic-block evaluator, or
  associative graph writer until the invalidation and typed-edge layers are
  complete and fixture-backed.

### Forward D: Advanced Entity Writers

Goal: add DWG writers for remaining ACadSharp-supported entity families only
when the local data model, class registration, owner handles, and downgrade
rules are complete.

Current local state:

- TOLERANCE, HATCH gradient fields, MLINE export reconstruction, LIGHT, SUN,
  MLEADERSTYLE, text-content MLEADER subsets, and many core entities are
  covered by native writers or metadata diagnostics.
- MESH/SubDMesh, SHAPE, OLE2FRAME, raster image, WIPEOUT/ImageDefinition/
  ImageDefinitionReactor/RasterVariables, and underlay definition/reference
  writing remain incomplete.

Phase D1 - writer readiness inventory:

1. For each candidate entity, compare the local `DRW_*` struct, parser, and
   LibreCAD consumer with ACadSharp writer methods and libreDWG layouts.
2. Mark each field as one of: local editable geometry, imported metadata,
   raw-only payload, computed writer field, unsupported required field, or
   version-gated optional field.
3. Add per-family writer blocker counts before native writing.
4. Keep raw replay and fallback export diagnostics active for incomplete
   families.

Phase D2 - MESH/SubDMesh:

1. Complete metadata for vertices, faces, edge/crease data, subdivision level,
   and version-specific flags.
2. Add fallback rendering as mesh/polyline preview where LibreCAD has no
   editable native mesh.
3. Add native DWG writing only for complete, unchanged or metadata-complete
   mesh records with valid owner handles and class registration.
4. Add downgrade diagnostics for versions that cannot carry the mesh payload.

Phase D3 - raster, wipeout, image, and underlay:

1. Complete ImageDefinition, ImageDefinitionReactor, RasterVariables,
   UnderlayDefinition, and UnderlayReference metadata, including file path,
   clipping, brightness, contrast, fade, display flags, definition handles,
   and reactor handles.
2. Keep file-path policy explicit: preserve path strings, do not copy external
   files, and diagnose missing local assets.
3. Write Wipeout/Image/Underlay only when definitions, reactors, owner handles,
   and clipping boundaries are complete.
4. Add tests for rectangular, polygonal, and no-boundary clipping; definition
   handle resolution; and missing-definition diagnostics.

Phase D4 - SHAPE and OLE2FRAME:

1. Implement SHAPE metadata and writer fields: shape file/style handle, shape
   name/index, insertion point, scale, rotation, oblique, width factor, and
   extrusion where version-gated.
2. Implement OLE2FRAME as raw-preserved frame metadata first: placement,
   display extents, payload byte range, and unchanged replay.
3. Do not regenerate OLE binary payloads. Edited OLE frames export as fallback
   frame geometry plus diagnostics unless a full payload is preserved.

Phase D5 - writer integration:

1. Register any needed custom classes in one place and verify instance counts.
2. Add `dwgRW` writer APIs only after libdxfrw writer support is tested.
3. Wire LibreCAD export through `RS_FilterDXFRW` with diagnostics for blocked
   entities.
4. Add one smoke test per family and one export diagnostic test per blocker
   bucket.

Validation for Forward D:

- `[dwg-write]` smoke tests per family.
- `[entity_metadata]` blocker/metadata tests.
- qmake6 build after public API changes.
- Optional fixture round-trip through local reader plus ACadSharp/libreDWG
  count comparison.

Stop line:

- Do not add a writer that silently drops required fields or emits a payload
  ACadSharp/libreDWG cannot read. Add diagnostics first, then narrow native
  writer subsets.

### Forward E: UI and Rendering Integration for Visual, UCS, Light, and Sun Metadata

Goal: connect imported DWG visual metadata to LibreCAD document/UI concepts
without promising unsupported 3D rendering.

Current local state:

- libdxfrw and metadata store VIEW, VPORT/VIEWPORT, UCS, VISUALSTYLE, LIGHT,
  and SUN records with handle lookup and dependency invalidation support.
- Native LIGHT and metadata-complete SUN writing exist for AC1024+.
- LibreCAD has view/UCS lists and 2D viewport/rendering infrastructure, but no
  photometric lighting or visual-style renderer.

Phase E1 - document mapping:

1. Map named UCS records into `LC_UCSList` with original DWG handle, origin,
   axes, elevation, orthographic type, and unresolved handle diagnostics.
2. Map named VIEW records into `LC_ViewList` or metadata-backed view records
   with center/target/direction, view height/width, twist, clipping, UCS
   reference, visual-style reference, background reference, and sun reference.
3. Keep VPORT/VIEWPORT layout settings in metadata unless there is an existing
   LibreCAD layout viewport consumer.
4. Add import-order independent resolution tests for view-to-UCS,
   view-to-visual-style, view-to-sun, and light-owner links.

Phase E2 - rendering policy:

1. Treat VISUALSTYLE, LIGHT, and SUN as metadata and UI-inspectable properties,
   not as active 3D shading inputs.
2. Map only safe 2D-affecting fields into current rendering: view extents,
   twist where supported, named UCS orientation where supported, and viewport
   clipping when LibreCAD already has a matching concept.
3. Display unsupported lighting/visual-style effects through metadata panels or
   diagnostics, not approximate geometry.
4. Preserve original handles and raw payloads so unchanged records can be
   replayed or regenerated.

Phase E3 - UI integration:

1. Add a document-level metadata access path for named views, UCSs, lights,
   sun, and visual styles so UI panels do not scan raw metadata vectors.
2. Surface read-only light/sun/visual-style summaries first: name, type, owner,
   intensity, color, shadow flags, sun status, date/time, and referenced view.
3. Add edit support only for fields that have a writer and invalidation policy.
   Editing any unsupported referenced field must mark the matching raw payload
   stale.
4. Keep UI strings and controls consistent with LibreCAD's existing view/UCS
   patterns; do not invent a new DWG-only editor surface unless necessary.

Phase E4 - export policy:

1. Regenerate named VIEW/UCS/LIGHT/SUN records only when metadata is complete
   and all referenced handles are resolved or intentionally zero.
2. Replay unchanged raw VISUALSTYLE and VPORT/VIEWPORT metadata where native
   writers are incomplete.
3. Diagnose invalidated visual/light metadata by handle and reason: edited
   referenced UCS, visual style, sun, owner/layout, background, or live-section
   handle.
4. Add smoke tests that view/UCS/light/sun metadata survives read/write when
   unchanged and is suppressed with diagnostics when stale.

Validation for Forward E:

- `[entity_metadata]` tests for view/UCS/light/sun lookup, invalidation, and
  UI-facing summaries.
- `[dwg-write]` smoke for native LIGHT/SUN and VIEW/UCS records.
- UI-level tests only after a stable document-list integration point exists.

Stop line:

- Do not implement photometric rendering, materials, shadows, or visual-style
  shaded display in this roadmap. Keep those as future renderer projects unless
  LibreCAD gains a 3D/rendering backend.

## Forward Implementation Breakdown

Use this section to pick the next implementation slice. Each item is intended
to be small enough for one reviewable pass, while still moving one of the
Forward A-E tracks toward native support.

### Ready Detail A1a: CELLSTYLEMAP Linkage and Style Inheritance

Purpose: finish the table style metadata layer before fallback rendering.

Status: implementation started on 2026-05-28. CELLSTYLEMAP objects are now
forwarded from `RS_FilterDXFRW` into `LC_DwgAdvancedMetadata`, stored as
dedicated records, searchable by handle/style ID/referenced handle, and
participate in table raw replay invalidation. Remaining work in this slice is
effective style inheritance across table defaults, row/column styles, and cell
override masks.

Files:

- `libraries/libdxfrw/src/drw_objects.h`
- `libraries/libdxfrw/src/drw_objects.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add `CellStyleMapRecord` metadata or extend `TableRecord` so CELLSTYLEMAP
   styles are distinguishable from TABLESTYLE styles.
2. Store style IDs, style class, style name, source handle, and parent
   TABLESTYLE handle when available.
3. Add lookup helpers:
   - `findCellStyleMapByHandle(handle)`;
   - `findCellStylesById(styleId)`;
   - `findTableStylesReferencingCellStyle(styleHandleOrId)`.
4. Preserve inheritance hints: table default style, row style, column style,
   cell style ID, and override-mask presence. Do not compute final effective
   style yet unless every layer is known.
5. Include CELLSTYLEMAP text-style and border linetype handles in table style
   reference lookup and invalidation.

Tests:

- Synthetic TABLESTYLE plus CELLSTYLEMAP import-order test.
- Lookup by style ID/name and invalidation by referenced text style/linetype.
- Table writer blocker count unchanged unless unsupported style inheritance is
  intentionally added as a new blocker bucket.

Stop before fallback geometry or native writing.

### Ready Detail A1b: Table Unknown-Range and Override Metadata

Purpose: make table parsing observable where semantics are still incomplete.

Files:

- `libraries/libdxfrw/src/drw_entities.h`
- `libraries/libdxfrw/src/drw_entities.cpp`
- `libraries/libdxfrw/src/drw_objects.h`
- `libraries/libdxfrw/src/drw_objects.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add lightweight `DwgSubrecordRange` structs for table/table-style skipped
   regions: name, offset, bit size or byte size, version gate, count, and
   parse-complete flag.
2. Capture ranges around currently skipped content formats, cell styles,
   border blocks, break data, override masks, and table geometry tails.
3. Surface aggregate counts in metadata:
   `unknownRangeCount`, `overrideMaskCount`, `breakRangeCount`,
   `tableGeometryTailRangeCount`.
4. Add export diagnostics for tables whose native writer is blocked by unknown
   ranges even when the visible cell text is otherwise text-only.
5. Keep range capture best-effort; a malformed range must not move the reader
   cursor differently than the current parser.

Tests:

- Synthetic metadata tests for unknown range summaries.
- Parser smoke for an existing table fixture if present; otherwise default
  tests stay synthetic.
- `[dwg-write]` if export blocker diagnostics are changed.

Stop before interpreting the unknown bytes.

### Ready Detail A2a: Table Fallback Attachment Contract

Purpose: define how fallback grid/text entities stay tied to native table
metadata after import.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/rs_filterdxfrw.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add a `TableFallbackEntityRecord` with table handle, row, column, fallback
   role (`GridLine`, `CellText`, `Placeholder`, `Boundary`), entity ID if
   available, and source handle.
2. Add metadata helpers:
   - `addTableFallbackEntity(record)`;
   - `findTableFallbackEntities(tableHandle)`;
   - `findTableByFallbackEntityId(entityId)`;
   - `invalidateTableForFallbackEntity(entityId)`.
3. Make `addTableFallback()` register records for every generated fallback
   entity, without changing the geometry yet.
4. When a fallback entity is invalidated, invalidate the semantic table and
   matching raw TABLE/TABLECONTENT payload.
5. Add export blocker counts for edited fallback entities and missing fallback
   attachment records.

Tests:

- Synthetic metadata tests for fallback lookup and invalidation.
- Import-side test can use synthetic `DRW_Table` data if no external fixture is
  available.

Stop before changing table visual output.

### Ready Detail A2b: Table Fallback Grid/Text Rendering

Purpose: make semantic tables visible through conservative 2D entities.

Files:

- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`
- Optional focused DWG import test if a table fixture is present.

Implementation:

1. Generate grid line entities from origin, table direction, row heights, and
   column widths.
2. Generate text entities for cells with plain text content only.
3. Generate placeholder text for FIELD, block, formula, unknown content, and
   incomplete parse cells. Placeholder text must be visually modest and backed
   by metadata; do not claim semantic conversion.
4. Apply available text height/alignment/style metadata only when resolved.
   Otherwise use the drawing default and record an unresolved-style diagnostic.
5. Attach every generated entity through Ready Detail A2a.

Tests:

- Fallback entity count and role tests for a synthetic two-row table.
- Text placement/alignment smoke that checks metadata role and cell coordinate,
  not pixel-perfect rendering.
- Export diagnostic test that edited fallback blocks native table writing.

Stop before adding table editing UI.

### Ready Detail A3a: Native Table Writer Blocker Matrix

Purpose: make the native TABLE writer contract executable before emitting
bytes.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Extend `TableWriterBlockerCounts` with text-only writer gates:
   unresolved CELLSTYLEMAP, unknown subrecord ranges, unsupported style
   inheritance, unsupported break data, edited fallback, unresolved text style,
   non-rectangular/merged cells, and missing owner/dictionary handles.
2. Add a `tableNativeWriterEligible(handle)` helper returning a diagnostic enum
   list, not just a boolean.
3. Log blocker summaries from `writeObjects()` with one concise line.
4. Add tests for every blocker bucket.

Stop before native TABLE/TABLECONTENT byte writing.

### Ready Detail A3b: Text-Only TABLE/TABLECONTENT Writer Scaffold

Purpose: add the narrow writer only after A3a proves eligibility.

Files:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/drw_objects.*`
- `libraries/libdxfrw/src/intern/dwgwriter*.{h,cpp}`
- `libraries/libdxfrw/src/libdwgr.*`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

Implementation:

1. Add writer APIs for metadata-complete text-only table records.
2. Register `AcDbTable`, `AcDbTableContent`, and supporting classes through
   the existing class registry. Verify instance counts with raw replay.
3. Emit only AC1021+ payloads with complete owner/dictionary handles and
   resolved TABLESTYLE.
4. Re-read the generated DWG locally and assert table callback/metadata counts.
5. Keep unsupported cells diagnostic-only.

Stop if ODA/ACadSharp/libreDWG disagree on any required field order without a
fixture resolving it.

### Ready Detail B1a: ACIS Container Block Scanner

Purpose: expose modeler sub-block byte ranges without interpreting geometry.

Files:

- `libraries/libdxfrw/src/drw_entities.h`
- `libraries/libdxfrw/src/drw_entities.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add `ModelerPayloadRange` records: kind (`Sat`, `Sab`, `History`, `Wire`,
   `Silhouette`, `UnknownTail`), offset, length, declared size, and
   consistency flag.
2. For SAB, locate `ACIS BinaryFile` and known end marker bytes only as range
   hints. Do not parse SAB entities.
3. For SAT, locate plain text ACIS header/history markers and known end text
   only as range hints.
4. For fixed-size fields already read by `DRW_ModelerGeometry`, record the
   expected body start and handle-stream split.
5. Add aggregate payload range counts and inconsistent-size diagnostics.

Tests:

- Synthetic SAT/SAB/history/unknown buffers.
- Inconsistent declared-size test must not crash or allocate unbounded memory.

Stop before ACIS topology parsing.

### Ready Detail B1b: Wire/Silhouette Summary Parser

Purpose: decode only stable count/coordinate summaries for fallback previews.

Files:

- `libraries/libdxfrw/src/drw_entities.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Mirror ACadSharp `readWire` and libreDWG modeler wire layouts for count
   fields and point arrays only.
2. Add bounded max counts for wires, edges, silhouettes, and points.
3. Store summaries as preview records: point count, edge count, bounding box,
   and source byte range.
4. Treat parser mismatch as "range known, preview unavailable" rather than
   object failure.

Stop before generating preview entities.

### Ready Detail B2a: Modeler Fallback Preview Attachment

Purpose: apply the same edit-safety policy as table fallback geometry.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add modeler fallback preview records with source modeler handle, preview
   entity ID, preview role, and source range index.
2. Invalidate modeler raw replay when a preview entity is edited.
3. Export edited previews as ordinary 2D geometry with a diagnostic that ACIS
   regeneration is unavailable.

Stop before native modeler writing.

### Ready Detail C1a: Associative Edge Metadata

Purpose: make associative shells queryable as a graph.

Files:

- `libraries/libdxfrw/src/drw_objects.h`
- `libraries/libdxfrw/src/drw_objects.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add `AssociativeEdgeRecord`: source handle, source kind, edge kind, target
   handle, raw range index, and confidence (`ExplicitHandle`, `Inferred`,
   `Unknown`).
2. Populate edges from existing dependency refs, action refs, owned params,
   owned actions, read/write dependency handles, r/d-node handles, action body
   handles, and ACSH owner/history links.
3. Add lookup helpers:
   - `findAssociativeEdgesFrom(handle)`;
   - `findAssociativeEdgesTo(handle)`;
   - `findAssociativeRecordsAffectedBy(handle)`.
4. Ensure graph invalidation walks edges in the target direction but does not
   loop forever on cycles.

Tests:

- Synthetic network/action/dependency graph with a cycle.
- Invalidation suppresses matching raw replay for all affected records.

Stop before evaluating graph semantics.

### Ready Detail C1b: Common Prefix Decode Audit

Purpose: make parser state observable for every known ACDBASSOC/ACSH prefix.

Files:

- `libraries/libdxfrw/src/drw_objects.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add prefix status fields for `AcDbAssocAction`,
   `AcDbAssocActionParam`, `AcDbAssocDependency`,
   `AcDbAssocGeomDependency`, `AcDbAssocNetwork`, `AcDbEvalExpr`, and
   `AcDbShHistoryNode`.
2. Store prefix byte/bit ranges and parse status.
3. Add diagnostics for missing prefix, partial prefix, unsupported class
   version, and bounded-count overflow.

Stop before adding new action-body semantics.

### Ready Detail C2a: Associative Invalidation Policy Matrix

Purpose: make export behavior predictable after edits.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add per-family stale counts: DIMASSOC, EVAL_GRAPH, ACDBASSOC,
   dynamic-block, object-context, ACSH.
2. Add invalidation reasons: edited entity, missing target, unsupported
   evaluator, parser partial, fallback geometry edited, and native replacement.
3. Log one object-write diagnostic line summarizing stale/preserved graph
   records.
4. Add tests for stale raw suppression by reason.

Stop before graph writers.

### Ready Detail D1a: Advanced Entity Writer Readiness Ledger

Purpose: prevent premature writers by recording each entity family's missing
fields as data.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add `AdvancedEntityWriterBlockerCounts` with buckets for MESH/SubDMesh,
   SHAPE, OLE2FRAME, raster/image, wipeout, underlay, and unknown required
   owner/class data.
2. Count current sidecar/native entity availability before adding writers.
3. Log the ledger from DWG export.
4. Add tests proving blocker counts for synthetic metadata records.

Stop before entity byte writers.

### Ready Detail D2a: MESH/SubDMesh Metadata Completeness

Purpose: prepare mesh writing and preview without committing to a writer.

Files:

- `libraries/libdxfrw/src/drw_entities.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Preserve subdivision level, crease count, edge count, face count, vertex
   count, and version flags.
2. Add mesh sidecar lookup by source handle.
3. Add writer blockers for missing crease data, unsupported subdivision data,
   and fallback-only mesh previews.

Stop before native SubDMesh writing.

### Ready Detail D3a: Raster, Wipeout, Image, and Underlay Link Graph

Purpose: make external-reference entities export-safe.

Files:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/drw_objects.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Store definition/reactor/reference handles for IMAGE, WIPEOUT, PDF/DGN/DWF
   underlays, ImageDefinition, ImageDefinitionReactor, RasterVariables, and
   UnderlayDefinition.
2. Add lookup helpers from entity to definition and definition to entities.
3. Add file-path diagnostics: empty path, relative path, missing local file,
   and intentionally external.
4. Add clipping diagnostics for no-boundary, rectangular, polygonal, and
   malformed clipping.

Stop before copying files or writing native underlay/image records.

### Ready Detail D4a: SHAPE and OLE2FRAME Metadata Shells

Purpose: classify two remaining advanced entity families before writer work.

Files:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/intern/dwgreader.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add SHAPE shell fields: name/index, style handle, insertion point, scale,
   rotation, oblique, width factor, extrusion, and raw range status.
2. Add OLE2FRAME shell fields: placement, extents, payload byte range, raw
   replay state, and preview frame status.
3. Add writer blockers for missing style handle, missing OLE payload, and
   edited preview frame.

Stop before OLE payload regeneration.

### Ready Detail E1a: VIEW/UCS Document Mapping Adapter

Purpose: connect metadata to existing LibreCAD view/UCS lists in a reversible
way.

Files:

- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/gui/` or existing UCS/view list classes only if needed
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add metadata-side document mapping records for named VIEW and UCS:
   source handle, created list item ID/name, unresolved references, and
   replay state.
2. Populate mappings during import without changing existing UI behavior.
3. Add lookup helpers from list name/ID back to DWG handle.
4. Invalidate VIEW/UCS raw replay if a mapped list item is edited and the
   writer cannot regenerate the original references.

Stop before UI panel changes.

### Ready Detail E2a: Read-Only Visual/Light Summary Model

Purpose: expose visual metadata safely to future UI without rendering it.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Implementation:

1. Add summary helpers for views, visual styles, lights, and suns:
   display name, handle, owner/layout, type, intensity/color, date/time,
   referenced sun, referenced visual style, and stale status.
2. Add aggregate counts by owner/layout and stale/replay state.
3. Ensure summaries do not require Qt UI classes so they remain testable in
   metadata tests.

Stop before rendering, material, or lighting UI work.

### Ready Detail E4a: Visual Metadata Export Diagnostics

Purpose: make VIEW/UCS/LIGHT/SUN/VISUALSTYLE export policy explicit.

Files:

- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`
- `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

Implementation:

1. Add blocker counts for unresolved UCS, visual style, sun, owner/layout,
   background, live section, and invalidated raw visual payload.
2. Log blocker summaries during DWG object writing.
3. Add smoke tests that unchanged VIEW/LIGHT/SUN remain writable and stale
   references are diagnosed.

Stop before VISUALSTYLE native writing unless every required field is present.

## Step-by-Step Implementation Queue

This queue is optimized for small, buildable changes. Each slice should be a
separate reviewable unit unless two adjacent slices touch the same small file
set and share tests.

### Per-Slice Loop

Use this loop for every slice below:

1. Check ODA, ACadSharp, and libreDWG for the exact version gates and field
   order.
2. Add or update the smallest possible test before broad implementation.
3. Preserve raw bytes and handle-stream split metadata before decoding new
   semantics.
4. Implement the parser or metadata contract.
5. Add native writing only when ownership, class registration, handles, and
   downgrade behavior are known. Otherwise add an explicit diagnostic.
6. Run the slice validation commands listed in the slice.
7. Update this roadmap if a layout or ownership assumption proves wrong.

### Slice 0: Workspace and Baseline

Goal: establish a clean implementation starting point.

Files touched: none unless baseline documentation needs correction.

Tasks:

1. Record `git status --short`.
2. Confirm this is the only plan artifact:
   `find . -path './build*' -prune -o -path './.git' -prune -o -type f \( -iname '*plan*.md' -o -iname '*roadmap*.md' -o -iname '*dwg*todo*.md' \) -print`
3. Run:
   - `cmake --build build --target librecad_tests -- -j2`
   - `./build/librecad_tests "[entity_metadata]"`
   - `./build/librecad_tests "[dwg-write]"`
   - `make -C build-qmake6-codex -j2`
   - `git diff --check`

Exit criteria:

- Baseline failures are either fixed or documented as pre-existing.
- There is still exactly one DWG roadmap file.

### Slice 1: Class Registry Accounting

Goal: make custom class emission deterministic before adding more writers.

Status: complete. Keep this section as historical context unless new custom
class writer regressions appear.

Likely files:

- `libraries/libdxfrw/src/drw_classes.*`
- `libraries/libdxfrw/src/intern/dwgwriter*.{h,cpp}`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

Tasks:

1. Centralize writer-side custom class registration.
2. Key class records by class number and record name.
3. Accumulate instance counts from native writer registrations and raw replay.
4. Reject conflicting metadata for an existing class number with a diagnostic.
5. Keep existing class numbers stable for already-tested custom classes.

Validation:

- Add focused tests for duplicate registration, conflict rejection, and raw
  replay instance counts.
- Run `[dwg-write]` and `git diff --check`.

### Slice 2: AuxHeader / Second Header

Goal: close the DWG file-structure gap independently of entity work.

Status: writer coverage complete for current smoke-test scope. Remaining work
is reader-side structural validation only, and is P3 unless interoperability
tests expose a concrete failure.

Likely files:

- `libraries/libdxfrw/src/intern/dwgwriter15.cpp`
- `libraries/libdxfrw/src/intern/dwgwriter18.cpp`
- `libraries/libdxfrw/src/intern/dwgwriter24.cpp`
- `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

Tasks:

1. Implement second-header/AuxHeader writing for AC1015+.
2. Use version-gated raw version codes from ODA/ACadSharp.
3. For AC1032, append the three trailing zero RS values.
4. Add parser-side smoke checks if local readers can validate the section.
5. Keep old readers tolerant if a file omits the section.

Validation:

- Add AC1027 and AC1032 smoke assertions.
- Run `[dwg-write]`.

### Slice 3: Raw Replay Metadata Completeness

Goal: make raw preservation a durable decode queue.

Status: complete for non-entity OBJECT replay and explicit blocker accounting.
Entity replay remains deliberately blocked until owner/block membership rewrite
rules are designed.

Likely files:

- `libraries/libdxfrw/src/drw_objects.h`
- `libraries/libdxfrw/src/intern/dwgreader.cpp`
- `libraries/libdxfrw/src/intern/dwgwriter*.{h,cpp}`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.*`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

1. Store owner handle, source section, entity/object kind, class metadata,
   body bit size, object offset/size, and handle-stream split on every raw
   replay candidate.
2. Keep non-entity OBJECT replay enabled for unchanged records.
3. Add unsupported/proxy entity caches in the LibreCAD consumer, but keep
   entity replay blocked by policy.
4. Add explicit blocker counts for entity raw blocked, invalidated, replaced,
   missing bytes, missing class metadata, and writer rejection.
5. Ensure semantic invalidation also invalidates matching raw replay records.

Validation:

- Add metadata tests for every blocker reason.
- Add replay tests for unchanged OBJECT replay and invalidated stale payload
  suppression.
- Run `[entity_metadata]` and `[dwg-write]`.

### Slice 4: Fixture and Interop Harness Skeleton

Goal: create repeatable comparison infrastructure before broad feature work.

Status: optional manifest exists. See Ready E for the next implementable
manifest-diagnostics slice.

Likely files:

- `libraries/libdxfrw/testdata/dwg-fixtures.json`
- `librecad/src/lib/filters/tests/dwg_smoke_tests.cpp`
- optional helper under `scripts/`

Tasks:

1. Extend the fixture manifest with optional feature tags:
   `table`, `mleader`, `acis`, `assoc`, `dynamic-block`, `light-sun`,
   `raster-underlay`, `r2018-text`, and `advanced-entities`.
2. Add optional ACadSharp/libreDWG reference dump paths.
3. Report loaded entity/object counts, expected callbacks, unsupported
   diagnostics, raw replay candidates, and invalidation counts.
4. Make missing external fixtures non-fatal for default CI.

Validation:

- Run `[dwg-smoke]` with and without fixture files present.

### Slice 5: Table Metadata Graph

Goal: make TABLE/TABLECONTENT/TABLESTYLE data internally coherent before
rendering or writing.

Likely files:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/drw_objects.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

Completed:

- TABLE/TABLECONTENT records resolve TABLESTYLE handles when the style appears
  before or after the content record.
- Metadata exposes table-by-handle, table-style-by-handle, and
  tables-using-style lookups.
- Metadata retains per-cell style, override, content, handle, geometry, and
  attribute summaries instead of only aggregate table counts.
- Metadata exposes table-cell coordinate lookup and cell-reference lookup by
  handle.
- Table attribute handles are included in cell/table reference lookups and
  native writer blocker diagnostics.
- TABLESTYLE text-style and border linetype handles are included in style
  reference lookups and replay invalidation.
- TABLESTYLE flow/flag/margin values, named cell style IDs/names, text
  heights, alignments, colors, visible-border counts, content-format counts,
  and margin-style counts are preserved in metadata.
- Table graph invalidation marks dependent TABLE/TABLECONTENT records stale
  when a referenced handle changes.
- Table graph invalidation also marks matching preserved raw table payloads
  stale, preventing raw replay of edited TABLE/TABLECONTENT records.

Remaining:

1. Expand preserved cell content kinds beyond current text, FIELD, block, and
   value/handle summaries when more native table payloads are decoded.
2. Preserve remaining R2007+ and R2010+ table style detail values beyond the
   current metadata: row/column style inheritance, border lineweight and
   double-line details, break metadata, override masks, unknown byte ranges,
   and complete CELLSTYLEMAP linkage.
3. Add fallback rendering only after the fallback-entity attachment policy is
   defined; keep native table writing blocked until Slice 6 has a concrete
   text-only subset.

Validation:

- Add graph assembly tests independent of rendering.
- Add synthetic tests for text, FIELD, block, merged, override, and style
  lookup cases.
- Run `[entity_metadata]`.

### Slice 6: Table Fallback Rendering and Export Policy

Goal: make imported semantic tables visible without pretending native table
editing is complete.

Likely files:

- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

1. Render fallback grid lines from row/column sizes and break data.
2. Place text cells with available style/alignment/height metadata.
3. Represent unsupported FIELD/block content with metadata-backed
   placeholders.
4. Attach native table metadata to fallback entities for invalidation.
5. Add export diagnostics:
   - unchanged native table raw replay;
   - edited fallback geometry;
   - incomplete native table writer prerequisites.

Completed:

- DWG export reports native table writer blocker counts for fallback-rendered
  tables, incomplete semantic parse, unresolved style, FIELD content, block
  content, cell overrides, and geometry cells.

Validation:

- Tests for fallback entity counts and invalidation.
- Tests for export diagnostics when table metadata is edited.

### Slice 7: Minimal Native Table Writer

Goal: add native writing only for a complete, narrow table subset.

Prerequisite: Slice 5 and Slice 6 complete.

Tasks:

1. Register required custom classes through the class registry.
2. Write a minimal text-only TABLE/TABLECONTENT subset with complete handles.
3. Keep FIELD, block content, complex overrides, and incomplete styles as
   diagnostic-only until the writer contract is complete.
4. Add version gates for AC1021+ table payload variants.

Validation:

- Synthetic text-only table write/read smoke test.
- Diagnostic tests for unsupported table features.

### Slice 8: MLeader Style and Context Metadata

Goal: ensure MLEADER metadata resolves correctly before writer expansion.

Likely files:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/drw_objects.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

Completed:

- Metadata resolves MLEADERSTYLE records independent of import order.
- Metadata exposes MLEADER/MLEADERSTYLE lookup by handle, style-use lookup,
  referenced-handle lookup, and dependency invalidation for style, arrow,
  linetype, text-style, block, and attribute-definition handles.

Remaining:

1. Preserve style defaults: arrow blocks, text style/color/height, block
   handles, dogleg, landing, scale, content type, frame, and background.
2. Preserve context data for multiple roots, leader lines, block content,
   tolerance content, overrides, and annotative/context handles.

Validation:

- Tests for leader-before-style and style-before-leader imports.
- Tests for unresolved style diagnostics.

### Slice 9: MLeader Native Writer Expansion

Goal: extend MLEADER writing in safe increments.

Prerequisite: Slice 8 complete.

Tasks:

Implementation status:

- See Ready A for the next bounded diagnostics slice.
- Native block/tolerance MLEADER writing remains blocked by ownership and
  downgrade contracts.

1. Add native MLEADERSTYLE writing.
2. Preserve current text-content MLeader writing.
3. Add style-backed text leaders.
4. Add block-content leaders only when block handles are complete.
5. Defer tolerance content until `DRW_Tolerance` DWG support is complete.
6. Emit diagnostics for unsupported overrides instead of silently flattening.

Validation:

- Writer tests for MLEADERSTYLE and style-backed text leaders.
- Diagnostic tests for block/tolerance/override cases.

### Slice 10: AC1032 Text and Attribute Writing

Goal: close the R2018 text writer gap without destabilizing older versions.

Likely files:

- `libraries/libdxfrw/src/drw_entities.*`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

Tasks:

1. Store parsed R2018 MTEXT extras needed for writing: annotative bytes,
   registered app handle, redundant geometry, frame/background fields, and
   column metadata.
2. Implement AC1032 multiline ATTRIB/ATTDEF writing with attribute type,
   embedded MTEXT, annotative bytes, registered app handle, and trailing
   version/unknown fields.
3. Add downgrade diagnostics for pre-AC1032 exports.

Validation:

- AC1032 read/write tests for single-line and multiline ATTRIB/ATTDEF.
- MTEXT R2018 smoke tests for frame/background/column preservation.

### Slice 11: Modeler Geometry Payloads

Goal: expose ACIS/modeler bytes safely before attempting entity replay.

Likely files:

- `libraries/libdxfrw/src/drw_entities.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

Completed:

- Metadata exposes modeler-geometry-by-handle and modeler-geometry-by-history
  lookups.
- Metadata classifies preserved modeler raw payloads as likely SAT, SAB, or
  unknown when recognizable ACIS markers are present.
- Metadata records recognizable payload marker offsets/lengths, raw body byte
  counts, handle-stream byte counts, marker sections, and inconsistent
  body-bit-size splits.

Remaining:

1. Split metadata into entity kind, modeler version, SAT/SAB marker, body
   bit-size split, and 3DSOLID history handle is covered; remaining work is
   to identify raw ACIS sub-blocks, wireframe bytes, and silhouette bytes
   inside the preserved payload.
2. Add bounded parsers for ACIS sub-block counts and byte ranges without
   interpreting ACIS geometry.
3. Surface coarse fallback summaries when available.
4. Invalidate raw replay when fallback geometry or dependent handles are
   edited.

Validation:

- Tests for SAT/SAB detection, byte slicing, and invalidation.

### Slice 12: Associative, Action, History, and Dynamic Block Shells

Goal: type known ACDBASSOC/ACSH/dynamic-block objects without evaluating them.

Likely files:

- `libraries/libdxfrw/src/drw_objects.*`
- `libraries/libdxfrw/src/intern/dwgreader.cpp`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

Completed:

- Metadata exposes associative-object-by-handle, associative-records
  referencing a handle, associative-records-by-kind, associative kind names,
  ACSH-by-handle, and ACSH-by-owner lookups.
- Preserved raw records are classified into associative, evaluation-graph,
  dynamic-block, object-context, or unknown families.
- Metadata exposes associative shell aggregate counts for known classes,
  value-param parse accounting, and action-param prefix accounting.

Remaining:

1. Decode common prefixes for `AcDbAssocAction`,
   `AcDbAssocActionParam`, `AcDbAssocDependency`,
   `AcDbAssocGeomDependency`, `AcDbAssocNetwork`, `AcDbEvalExpr`, and
   `AcDbShHistoryNode`.
2. Preserve status, owning action/network handles, dependency handles, value
   parameter records, point payloads, osnap references, action-body handles,
   and history parent/child links.
3. Add placeholders for annotation scale context, block reference context,
   MText attribute context, MLeader context, and block action/parameter/grip/
   visibility objects.
4. Keep every record raw-preserving and invalidate raw replay when dependent
   entity handles change.

Validation:

- Graph tests for handle links and invalidation propagation.
- Fixture diagnostics showing known classes are no longer anonymous.

### Slice 13: TOLERANCE Entity

Goal: unblock a small high-value advanced entity and future MLeader tolerance
content.

Status: complete for native DWG parse/write and current tests. Future work is
only to consume TOLERANCE from richer MLEADER content once that writer contract
exists.

Likely files:

- `libraries/libdxfrw/src/drw_entities.*`
- `libraries/libdxfrw/src/drw_interface.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp`

Tasks:

1. Implement DWG parser for `DRW_Tolerance`.
2. Implement native DWG writer.
3. Wire LibreCAD consumer/export metadata.
4. Add version and style handle validation.

Validation:

- TOLERANCE parse/write smoke test.
- Re-run MLeader diagnostic tests if tolerance content becomes writable.

### Slice 14: Remaining Advanced Entities

Goal: handle less common ACadSharp entity families one at a time.

Order:

1. `DRW_Mesh` / SubDMesh: vertices, faces, crease data, subdivision level,
   fallback mesh rendering, native writing only for complete shells.
2. `DRW_Shape`: shape name/index, insertion point, scale, rotation, oblique,
   and style handle.
3. `DRW_Ole2Frame`: placement, raw OLE bytes, placeholder frame, unchanged
   replay/write policy.
4. Raster/image/underlay: ImageDefinition/ImageDefinitionReactor/
   RasterVariables linkage, Wipeout/Image/PdfUnderlay DWG writing, clipping,
   brightness, contrast, fade, and definition handles.
5. MLINE: wire existing libdxfrw MLINE writer through LibreCAD DWG export and
   preserve style handles.
6. HATCH gradients: preserve gradient fields and write them where supported.

Validation:

- One focused parse/write smoke test per family.
- Consumer tests for fallback geometry and export diagnostics.

### Slice 15: Object Metadata Writers

Goal: regenerate safe metadata objects instead of relying only on raw replay.

Likely files:

- `libraries/libdxfrw/src/drw_objects.*`
- `libraries/libdxfrw/src/intern/dwgwriter*.{h,cpp}`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`

Tasks:

1. Layout/PlotSettings: complete storage and regenerate from paper-space data
   where possible.
2. VisualStyle/Material: preserve ACadSharp-readable fields, metadata-only
   unless a UI/rendering consumer exists.
3. MLineStyle/UnderlayDefinition/ImageDefinition/Scale/SortEnts/GeoData/
   SpatialFilter: add complete metadata structs, invalidation rules, and
   native writers only when handles are complete.
4. Current variables: preserve and update `CMLEADERSTYLE`, `CTABLESTYLE`, and
   related dictionary links when native styles are written.

Validation:

- Metadata tests for ownership and dictionary links.
- Writer tests for unchanged replay and regenerated minimal records.

### Slice 16: VIEW, UCS, LIGHT, and SUN Integration

Goal: link visual metadata coherently while keeping core geometry narrow.

Likely files:

- `libraries/libdxfrw/src/drw_objects.*`
- `librecad/src/lib/engine/document/lc_dwgadvancedmetadata.h`
- `librecad/src/lib/filters/rs_filterdxfrw.cpp`
- `librecad/src/lib/filters/tests/entity_metadata_tests.cpp`

Tasks:

1. Map VIEW, VPORT, VIEWPORT, UCS, LIGHT, and SUN handles.
2. Preserve sun handles referenced by views/vports/viewports.
3. Add metadata lookups for active sun by view, lights by owner/layout, UCS by
   view/vport, and visual style by view/vport.
4. Add native LIGHT/SUN writing for unchanged or metadata-complete records.
5. Leave rendering integration as a TODO until LibreCAD has a lighting
   consumer.

Validation:

- Metadata tests for handle resolution independent of import order.
- Writer smoke test for LIGHT/SUN class registration and object emission.

### Slice 17: Full Validation and Documentation

Goal: prove the stepwise implementation did not regress supported formats.

Run:

1. `cmake --build build --target librecad_tests -- -j2`
2. `./build/librecad_tests "[entity_metadata]"`
3. `./build/librecad_tests "[dwg-write]"`
4. `./build/librecad_tests "[dwg-smoke]"`
5. `./build/librecad_tests "[dwg-entity-roundtrip]"`
6. `make -C build-qmake6-codex -j2`
7. `git diff --check`
8. Optional with fixtures:
   - run the DWG fixture manifest runner;
   - compare ACadSharp/libreDWG reference dumps;
   - verify unsupported diagnostics decrease or become more specific.

Exit criteria:

- Default test/build gates pass.
- Every unsupported feature has either native support, raw preservation, or an
  explicit diagnostic.
- This roadmap reflects any deferred work accurately.

## Stop Conditions

Stop the implementation and update this roadmap instead of guessing when:

- ODA, ACadSharp, and libreDWG disagree on a byte layout and no local fixture
  confirms the correct version gate.
- A native writer would need handles that LibreCAD cannot currently preserve.
- Entity raw replay would require rewriting owner/block membership without a
  tested handle-translation contract.
- A feature needs UI/editor semantics rather than DWG metadata preservation.

In those cases, preserve raw bytes, expose metadata, add a diagnostic, and
record the exact missing contract in this roadmap.
