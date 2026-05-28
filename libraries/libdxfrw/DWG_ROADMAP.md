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

Still incomplete:

- Reader-side structural validation for AuxHeader/second-header remains shallow.
- Slice 5-7 semantic table graph still needs cell-level style/detail
  expansion, fallback rendering, and narrow native table writing.
- Slice 8-10 remaining MLEADER work: dictionary/current-style linkage, richer
  block/tolerance content, complex override diagnostics, and AC1032 annotative
  attribute side payload replay.
- Slice 11-12 ACIS/modeler and associative/action/history graph expansion;
  shell records are queryable by handle/reference, but richer byte decoding and
  graph evaluation remain.
- Slice 14 remaining advanced entities: MESH, SHAPE, OLE2FRAME, raster/image/
  underlay DWG writing, and fuller HATCH semantics beyond gradients.
- Slice 15-16 remaining object metadata writers and VIEW/UCS/lighting UI
  integration; visual/lighting records are queryable but not yet rendered.

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
- Table graph invalidation marks dependent TABLE/TABLECONTENT records stale
  when a referenced handle changes.
- Table graph invalidation also marks matching preserved raw table payloads
  stale, preventing raw replay of edited TABLE/TABLECONTENT records.

Remaining:

1. Normalize table entity, table content, table style, cell style map, FIELD,
   block content, and attribute handles into a single metadata graph.
2. Expand preserved cell content kinds beyond current text, FIELD, block, and
   value/handle summaries when more native table payloads are decoded.
3. Preserve row/column sizes, breaks, merged ranges, override cells, geometry
   cells, and style handles.
4. Preserve R2007+ and R2010+ table style details: named cell styles, borders,
   margins, alignment, text style, text height, colors, and CELLSTYLEMAP
   entries.

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

Remaining:

1. Split metadata into entity kind, modeler version, SAT/SAB marker, body
   bit-size split, raw ACIS bytes, wireframe bytes, silhouette bytes,
   3DSOLID history handle, and empty-body flag.
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
