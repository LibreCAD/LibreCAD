# Plan: Refactor & Harden ESRI Shapefile (SHP) Support — Native Filter + C++17

> **Status**: Approved plan, ready for implementation. Deep-researched 2026-07-19; every
> file:line reference below verified against the working tree on that date.
> **Decisions locked in**:
> 1. **UI integration**: SHP becomes a **native `RS_FilterInterface` filter** (`RS_FilterSHP`)
>    surfaced through File→Open (and format auto-detection), replacing the plugin path.
> 2. **C→C++17**: vendored shapelib sources are **renamed `.c` → `.cpp` and compiled as
>    C++17** with zero-to-minimal source edits (upstream structure preserved for CVE resync).
> **Supersedes**: `docs/plan_importshp_port_back.md`, `docs/plan_importshp_fix_all.md`,
> `docs/plan_importshp_refactor.md` (the plugin those describe is retired in Phase 5).

---

## 0. Verified Ground Truth (what the plan builds on)

### 0.1 Current state

| Item | State | Evidence |
|---|---|---|
| importshp plugin restored + modernized | Yes, but **imports nothing** — `ImportShp::execComm()` never calls `procesFile()`; the entire import engine is dead code | `plugins/importshp/importshp.cpp:40-45`; `procesFile` has zero call sites. Working pattern: `plugins/asciifile/asciifile.cpp:49-51` |
| Plugin styling ceiling | `Document_Interface` direct API (`addPoint`/`addPolyline`/`addText`) attaches **no pen** → layer-only, strictly 2D. Color/linetype/width UI controls are dead (`(void)colorF;…` at `importshp.cpp:520-522`) | `doc_plugin_interface.cpp:851-949` |
| shapelib 1.6.3 (vendored) | Pure C files, but **already dual-language by design**: cast macros expand to `static_cast`/`reinterpret_cast`/`const_cast`/`nullptr` under `__cplusplus`; `extern "C"` guards in `shapefil.h`; **empirically compiles clean** with `clang++ -std=c++17 -Wall -Wextra -Wpedantic` (0 diagnostics on all three files) | `plugins/importshp/shapelib/shapefil_private.h:19-29`, `shapefil.h:25-28,580` |
| Filter registry | Hard-coded factory list `RS_FileIO::getFilters()` returning `{LFF, DXFRW, CXF, JWW, DXF1}::createFilter` | `librecad/src/lib/fileio/rs_fileio.cpp` (`getFilters`, bottom of file) |
| Format detection | Extension→`RS2::FormatType` map in `RS_FileIO::detectFormat()`: `{"dxf","cxf","lff","dwg"}` | `rs_fileio.cpp` (`detectFormat`) |
| Format enum | `RS2::FormatType` | `librecad/src/lib/engine/rs.h:109-128` |
| Open-file dialog | `QG_FileDialog`: `getExtension()`, `hasExtension()` (supported-suffix lists at :73/:75), `getType()` (filter-string→FormatType), name filters built ~:140, `getOpenFile()` at :156 | `librecad/src/ui/dialogs/file/qg_filedialog.cpp` (NOT `ui/generic/` — old docs are wrong) |
| Open call chain | `LC_DocumentsStorage::loadGraphic()` → `RS_FileIO::fileImport()` → `getImportFilter()` → `filter->fileImport(graphic, file, type)` | `librecad/src/ui/main/persistence/lc_documentsstorage.cpp:162-176` |
| File→Import submenu | Exists (`DrawImage`, `BlocksImport`) | `librecad/src/ui/main/init/lc_menufactory_main.cpp:296-299` |
| Native-filter entity model | `RS_Layer(name)` + `layer->setPen(pen)` + `graphic->addLayer(layer)`; entities `new RS_Point/RS_Line/RS_Polyline(container, data)` + `container->addEntity(e)`; per-entity `setPen()` | `librecad/src/lib/filters/rs_filterjww.cpp:190-212,287-291,398-404` |
| Test framework | Catch2 v3, `librecad_tests` target (top `CMakeLists.txt` `BUILD_TESTS` block, ~:2345-2465), links `librecad_lib`, defines `LIBRECAD_SOURCE_DIR` + `LIBRECAD_TEST_DIR`, ctest runs with `QT_QPA_PLATFORM=offscreen` | `CMakeLists.txt:2345-2465` |
| Sample corpus | **Already downloaded**, 31 `.shp` fixtures in `test_data/shp/` (+ `test_data/shp_inventory.json`) — currently **untracked in git** and consumed by no test | `git status`; `test_data/shp/` |
| Library vendoring model | jwwlib: sources listed directly in top `CMakeLists.txt` (~:391) + include dir (~:157); qmake: static lib in `libraries/libraries.pro:14` + `librecad/src/src.pro:45,48,110,115` | verified |
| Toolchain | macOS arm64, Apple clang, Qt6 at `/opt/homebrew`, `qmake6` + `cmake` + `ninja` present, 12 cores | verified |

### 0.2 Corpus inventory (test_data/shp/)

| Group | Files | Exercises |
|---|---|---|
| Basic types | `points`, `polylines`, `polygons`, `multipoints`, `pointz` | POINT(1), ARC(3), POLYGON(5), MULTIPOINT(8), POINTZ(11) |
| Z/M variants | `z_m_types/{pointm,polylinem,polygonm,multipointm}` | POINTM(21), ARCM(23), POLYGONM(25), MULTIPOINTM(28) |
| Multi-part | `multi_part_polyline`, `multi_part/{bc_hospitals,multipoint}` | part splitting, real POINT data w/ DBF |
| Real world | `real_world/{Chi-SDOH, TM_WORLD_BORDERS-0.2, world_borders, …}` | large POLYGON sets (791 / 246 records) |
| DBF properties | `real_world/{utf8,latin1,boolean,date,number,number-null,string,mixed,ignore}-propert*` | codepage + field-type handling |
| Edge cases | `null_shape`, `real_world/{null,empty,singleton}`, `missing_shx` | SHPT_NULL, 0-record, no-.shx linear scan |
| Hostile | `malformed_dbf`, `malformed_truncated` | CVE-2023-30259-class robustness |
| **Gaps** | — | POLYGONZ(15), ARCZ(13), MULTIPOINTZ(18), MULTIPATCH(31), crafted `nPoints>50M` DoS header, >255-char filename |

---

## 1. Architecture Target

```
libraries/shapelib/               ← vendored shapelib 1.6.3, compiled as C++17
├── shapelib.pro                  ← qmake static lib (mirrors libraries/jwwlib)
└── src/
    ├── shapefil.h                ← unchanged (extern "C" guards stay)
    ├── shapefil_private.h        ← unchanged
    ├── shpopen.cpp               ← was shpopen.c   (git mv, zero source edits)
    ├── dbfopen.cpp               ← was dbfopen.c   (minus duplicate CPL defines)
    ├── safileio.cpp              ← was safileio.c  (16 old-style casts fixed)
    └── LICENSE-MIT / LICENSE-LGPL / ChangeLog / README.librecad

librecad/src/lib/filters/
├── rs_filtershp.h                ← NEW: RS_FilterSHP : RS_FilterInterface
├── rs_filtershp.cpp              ← NEW: import implementation (import-only)
└── tests/
    ├── shp_shapelib_tests.cpp    ← NEW: Phase-0 shapelib API tests (safety net)
    └── shp_import_filter_tests.cpp ← NEW: Phase-4 filter-level corpus tests

RS2::FormatSHP                    ← NEW enum value, rs.h FormatType
RS_FileIO::getFilters()           ← + RS_FilterSHP::createFilter
RS_FileIO::detectFormat()         ← + {"shp", RS2::FormatSHP}
QG_FileDialog                     ← + "ESRI Shapefile (*.shp)" (open path only)

plugins/importshp/                ← REMOVED in Phase 5 (superseded)
```

Data-flow after this plan: **File→Open → `QG_FileDialog` (`*.shp`) → `LC_DocumentsStorage::loadGraphic` → `RS_FileIO::fileImport` → `RS_FilterSHP::fileImport` → native `RS_Point`/`RS_Polyline`/`RS_MText` with per-entity `RS_Pen` + DBF-driven `RS_Layer`s.**

Why native filter (recap): only this path can honor per-entity color/linetype/width AND appears in the standard Open flow; the plugin's `Document_Interface` is layer-only/2D by API design, not fixable at the plugin layer.

### 1.1 Dual Build-System Policy (qmake6 + CMake) — hard requirement

Both build systems are **first-class and must stay green at every commit** of the ladder.
Neither is "the real one": CI/packaging uses CMake; the macOS bundle deployment
(`DESTDIR = ../../LibreCAD.app/...`) and several developer workflows use qmake6. Any
change that adds, renames, moves, or deletes a source file **must touch both systems in
the same commit** — a one-sided edit silently breaks the other build.

**Canonical per-commit verification pair** (run both before every commit):
```bash
# CMake (tests live here — qmake has no test target)
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build-test -j12
ctest --test-dir build-test -R librecad_tests --output-on-failure

# qmake6 (compile+link gate; TEMPLATE=subdirs recurses on its own — no -r needed)
qmake6 librecad.pro && make -j12
```

**Per-phase touchpoint matrix** (what each phase edits in each system):

| Phase | CMake touchpoints | qmake6 touchpoints |
|---|---|---|
| 0 | top `CMakeLists.txt`: `librecad_tests` sources (+ shapelib sources & include dir, temporary) | none (tests are CMake-only) — gate is full build stays green |
| 1a (move) | top `CMakeLists.txt` test wiring path; `plugins/importshp/CMakeLists.txt` paths | `plugins/importshp/importshp.pro` paths |
| 1b (.cpp) | filename refs in the above; add shapelib sources + include dir to `librecad_lib` (jwwlib pattern, ~:157/~:391) | **new** `libraries/shapelib/shapelib.pro` (static lib, mirror `libraries/jwwlib/*.pro`); `libraries/libraries.pro` `SUBDIRS` (:14); `librecad/src/src.pro` PRE_TARGETDEPS/`-lshapelib`/`INCLUDEPATH` (:45,:48,:110,:115 pattern) |
| 2 | `rs_filtershp.{h,cpp}` into `librecad_lib` source lists (beside the other `rs_filter*`) | `librecad/src/src.pro` `HEADERS` (~:538-543) + `SOURCES` (~:1176-1180) |
| 3 | none (edits to already-registered `qg_filedialog.cpp`, `rs.h`, `rs_fileio.cpp` — both builds pick them up automatically) | none |
| 4 | `shp_import_filter_tests.cpp` into `librecad_tests` sources | none (CMake-only tests) |
| 5 | remove `add_subdirectory(importshp)` (`plugins/CMakeLists.txt:109`) | remove `importshp \` (`plugins/plugins.pro:23`) |
| 6 | — | translation `TRANSLATIONS`/ts updates if strings move |

**Known asymmetries** (accepted, documented — do not "fix" one-sidedly):
- **Tests**: Catch2 `librecad_tests` exists only in CMake (`BUILD_TESTS=ON`). The qmake
  gate for test-bearing phases is compile+link green; behavioral validation always runs
  through ctest.
- **shapelib linkage**: CMake compiles shapelib sources directly into `librecad_lib`
  (matching how jwwlib sources are listed in the top `CMakeLists.txt`); qmake builds it as
  a separate static lib under `libraries/` (matching how qmake handles jwwlib). Same
  sources, two linkage shapes — keep both wired to `libraries/shapelib/src/`.
- **Plugin deployment** (until Phase 5): only qmake's `DESTDIR` auto-deploys the plugin
  dylib into `LibreCAD.app/Contents/Resources/plugins/`; CMake requires the manual copy
  noted in Phase 3 validation.

**PR checklist line** (add to each PR description): "☑ CMake build green ☑ ctest green
☑ qmake6 build green".

---

## 2. Phase 0 — Safety Net First (tests before any refactor) — done ✅

**Landed**: 2026-07-20 — `test(shp): pin shapelib 1.6.3 behavior over corpus fixtures` —
24 test cases / 1790 assertions green over the 31-fixture corpus.

**Goal**: lock shapelib's observable behavior in a Catch2 test *before* touching it, so the
C→C++17 conversion in Phase 1 is provably behavior-preserving. Also brings the corpus into git.

### Steps
1. `git add test_data/shp test_data/shp_inventory.json` (currently untracked).
2. New `librecad/src/lib/filters/tests/shp_shapelib_tests.cpp` (Catch2 v3, mirrors the
   include/header conventions of `dwg_smoke_tests.cpp`), using
   `LIBRECAD_SOURCE_DIR "/test_data/shp/..."` for paths. Test cases:
   - **Happy path per type**: for each of `points/polylines/polygons/multipoints/pointz` +
     all four `z_m_types/*`: `SHPOpen` succeeds; `SHPGetInfo` reports the expected
     `nSHPType` and record count (expected values hardcoded from `shp_inventory.json`);
     first record's `nVertices`/`nParts` match inventory; `padfZ` populated for `pointz`.
   - **Multi-part**: `multi_part_polyline` — `nParts > 1`, `panPartStart` monotonically
     increasing, all `< nVertices`.
   - **DBF**: `points.dbf` field count/names; `real_world/utf8-property` and
     `latin1-property` raw bytes round-trip; `boolean/date/number-property` via
     `DBFReadStringAttribute`/`DBFReadIntegerAttribute`/`DBFReadDoubleAttribute`;
     `DBFIsAttributeNULL` on `number-null-property`.
   - **Edge**: `null_shape` (record reads as `SHPT_NULL`, no crash); `real_world/empty.shp`
     (0 records); `missing_shx` (opens, linear-scan reads succeed).
   - **Hostile (CVE-class)**: `malformed_dbf`, `malformed_truncated` — every
     `SHPReadObject`/`DBFRead*` call either returns null/empty or valid data; **no crash**
     (the assertion is process survival + no garbage vertex counts `> nPoints` cap).
3. Register in top `CMakeLists.txt` `librecad_tests` source list (after
   `dwg_tarch_tests.cpp`, ~:2434). shapelib sources are *temporarily* compiled into the
   test target from `plugins/importshp/shapelib/` (3 files + include dir) — this
   temporary wiring is replaced in Phase 1 when shapelib joins `librecad_lib` properly.

### Validation
```bash
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build-test --target librecad_tests -j12
ctest --test-dir build-test -R librecad_tests --output-on-failure   # all green
```

### Commit
```
test(shp): add shapelib corpus tests and track test_data/shp fixtures

Locks shapelib 1.6.3 behavior over 31 sample shapefiles (all geometry
types, Z/M variants, multi-part, codepages, null/empty/missing-shx,
malformed CVE-2023-30259-class inputs) before the C++17 conversion.
```

### Discovered during PR 0

Several **pre-existing** master rot issues surfaced when first attempting
`cmake --build build-test --target librecad_tests -j12`. None are related to
the SHP plan; each was addressed as follows, so the Phase-0 safety net can
actually run.

1. **Compile-time API drift in test files (fixed inline)**:
   - `librecad/src/lib/engine/document/entities/tests/lc_parabola_tests.cpp`:
     capitalised `d.GetFocus()` / `d.GetDirectrix()` — the actual accessors on
     `LC_ParabolaData` are `getFocus()` / `getDirectrix()`.  Renamed.
   - `librecad/src/lib/engine/document/entities/tests/rs_ellipse_tests.cpp`:
     called `ellipse.getNearestEndpoint(pt, &dist)` — the current signature is
     `getNearestEndpoint(const RS_Vector&, RS_Entity** entity=nullptr,
     double* dist=nullptr)`.  Added the explicit `nullptr` for `entity`.
2. **Removed-API references in test files (excluded from build; deferred)**:
   - `librecad/src/lib/filters/tests/rs_graphic_layouts_tests.cpp` references
     `RS_Graphic::setMargins/getMarginLeft/…` which have moved to
     `LC_PlotSettings::setMarginsInMm` / `getMarginLeftMm` etc.
   - `librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp` references
     `RS_Graphic::newDoc()` which no longer exists.
   Both files are commented out of the `librecad_tests` source list with a
   `TODO(pre-existing master rot)` note; porting them to the current API is
   out of scope for the SHP plan and should be a separate `fix(tests)` commit.
3. **Environment-sensitive test failure (documented, not fixed)**:
   `dxf_corpus_tests.cpp` `[corpus][dwgdxf]` iterates over `~/doc/dwg{,2}/*.dwg`
   local samples and one of those DWGs triggers a SIGSEGV during DXF round-trip
   — again pre-existing and unrelated to SHP.  Excluded from validation via
   `~[corpus]` tag filter (`librecad_tests "~[corpus]"` → 630/630 test cases,
   11 860 assertions green).
4. **DBF field-type surprise (pinned as-is)**: `points.dbf` has field `FID`
   with width=11, decimals=0.  Because shapelib returns `FTDouble` for numeric
   fields with width > 10 (regardless of decimals), the test asserts
   `ft == FTDouble` — that classification rule must survive the Phase 1
   `.c → .cpp` rename.
5. **Fixture guardrail (added to running rules)**: fixtures under
   `test_data/shp/` are pinned reference data.  Do not "fix", regenerate, or
   run any writing shapelib API against them (in particular:
   `SHPOpenLLEx(..., bRestoreSHX=1)` and `SHPRestoreSHX` both silently write
   a fresh `.shx` — never call either on a corpus path).  Tests use
   read-only `SHPOpen` throughout; fixtures with truncated/missing `.shx`
   are pinned as "SHPOpen returns null, no crash".

---

## 3. Phase 1 — Relocate shapelib to `libraries/` and convert to C++17

**Goal**: satisfy "all implementation in C++17" with upstream-diffable structure; make
shapelib linkable from `librecad_lib` (the filter needs it) while the plugin (until Phase 5)
keeps building.

### Sub-plan 1a — move (own commit, so git tracks the rename cleanly) — done ✅

**Landed**: 2026-07-21 — `refactor(shapelib): move vendored shapelib to libraries/shapelib`.
Rename is inert (Phase-0 tests still 24/24 pass, 1790 assertions).

1. `git mv plugins/importshp/shapelib libraries/shapelib/src` (keep `LICENSE-MIT`,
   `LICENSE-LGPL`, `ChangeLog` with it); add `libraries/shapelib/README.librecad` noting
   version 1.6.3, source URL `https://download.osgeo.org/shapelib/shapelib-1.6.3.tar.gz`,
   dual MIT/LGPL license, and the local-delta list (kept minimal on purpose).
2. Re-point consumers:
   - `plugins/importshp/CMakeLists.txt`: include dir + the 3 source paths →
     `../../libraries/shapelib/src` (interim; dir deleted in Phase 5).
   - `plugins/importshp/importshp.pro`: `INCLUDEPATH`/`SOURCES`/`HEADERS` likewise.
   - Phase-0 test wiring in top `CMakeLists.txt` → new path.
3. Build both systems + rerun Phase-0 tests (green = move is inert).

### Sub-plan 1b — rename `.c`→`.cpp`, compile as C++17 (own commit) — done ✅

**Landed**: 2026-07-21 — `build(shapelib): compile shapelib as C++17 (.c -> .cpp)`.
Phase-0 corpus tests still 24/24 (1790 assertions) — behavior invariance under
the rename is proven.  Both build systems (CMake `librecad_tests` + qmake6 full
app w/ new `libshapelib.a` static lib) green.  Optional `common.pri`
`QMAKE_CFLAGS = -std=c++17` cleanup deferred: repo-wide audit shows no
remaining `.c` sources, so the line is dead but the revert has cross-platform
Windows/MinGW risk not worth taking in this commit — flagged as follow-up.

1. `git mv` `shpopen.c→shpopen.cpp`, `dbfopen.c→dbfopen.cpp`, `safileio.c→safileio.cpp`.
   **Zero source edits required** — empirically verified clean under
   `clang++ -std=c++17 -Wall -Wextra -Wpedantic` (cast macros already emit C++ casts under
   `__cplusplus`; all 94 heap allocations already routed through them).
2. Permitted minimal deltas (each listed in `README.librecad`):
   - dedupe the duplicated `#define CPLsprintf/CPLsnprintf` (`dbfopen`, lines 25-26 vs 48-49);
   - convert the 16 C-style casts in `safileio.cpp` to `reinterpret_cast`/`static_cast`
     (only file not already old-style-cast-clean).
3. Build integration for C++ sources:
   - Update filename references: `plugins/importshp/CMakeLists.txt`, `importshp.pro`,
     Phase-0 test source list.
   - **New `libraries/shapelib/shapelib.pro`** (static lib, mirror `libraries/jwwlib`) and
     add `shapelib` to `libraries/libraries.pro` `SUBDIRS` (:14 region). Wire
     `librecad/src/src.pro` exactly like jwwlib: PRE_TARGETDEPS (:45/:48 pattern),
     `-lshapelib` (:110 region), `INCLUDEPATH += ../../libraries/shapelib/src` (:115 region).
   - **CMake**: follow the jwwlib pattern — add `libraries/shapelib/src` to the include
     list (~:157) and the two `.cpp`+headers to the `librecad_lib` source lists (~:391
     region). (`safileio.cpp` + `shpopen.cpp` + `dbfopen.cpp`.)
   - `CMAKE_CXX_STANDARD 17` (top `CMakeLists.txt:4-5`) and `CONFIG += c++17`
     (`common.pri:77`) already guarantee the standard — no flag changes.
4. Optional cleanup, **gated on a repo-wide audit**: `common.pri:30` sets
   `QMAKE_CFLAGS = -std=c++17` (a hack to force C files through C++ semantics). After the
   rename, `grep` for remaining `.c` sources in the qmake build; if none rely on it, revert
   that line in this commit; otherwise leave with a comment.
5. Revert the stray uncommitted `CONFIG += debug` in `plugins/importshp/importshp.pro`
   (present in the working tree; not wanted in either outcome).

### Validation
```bash
# per-file standalone proof stays true after rename
cd libraries/shapelib/src && for f in shpopen dbfopen safileio; do
  clang++ -std=c++17 -fsyntax-only -Wall -Wextra -Wpedantic -I. $f.cpp && echo "$f OK"; done
# both build systems
cmake --build build-test --target librecad_tests -j12 && ctest --test-dir build-test -R librecad_tests --output-on-failure
cd plugins/importshp && qmake6 importshp.pro && make -j12          # plugin still builds
cd ../.. && qmake6 librecad.pro && make -j12                    # full qmake app
# behavior invariance: Phase-0 tests green with .cpp sources = conversion proven inert
```

### Commits
```
refactor(shapelib): move vendored shapelib 1.6.3 to libraries/shapelib

build(shapelib): compile shapelib as C++17 (.c -> .cpp)

Renames the three translation units and switches both build systems to
C++ compilation. Zero functional edits: upstream's __cplusplus cast-macro
branches make the sources valid C++17 as-is. Local deltas (documented in
README.librecad): deduped CPLsprintf defines, modern casts in safileio.
Corpus tests from the previous commit prove behavior is unchanged.
```

---

## 4. Phase 2 — `RS_FilterSHP`: the native import filter — done ✅

**Landed**: 2026-07-21 — `feat(filters): native ESRI Shapefile import filter (RS_FilterSHP)`.
Sub-plans 2a (enum + skeleton + registration), 2b (geometry mapping via RAII
shapelib wrappers, ARC/POLYGON/MULTIPOINT/POINT + Z-preserved, MULTIPATCH ring
vs strip simplification), 2c (DBF-driven layers + per-entity RS_Pen, codepage-
aware string decoding via QStringDecoder, LC_ShpImportOptions options seam),
and 2d (smoke test) all in one commit per the section-9 ladder.  25/25 [shp]
cases green, 631/631 total cases green (`~[corpus]`).


**Goal**: first-class SHP import producing native entities with full styling fidelity.

### Sub-plan 2a — enum + skeleton + registration
1. `librecad/src/lib/engine/rs.h:109` — add `FormatSHP, /**< ESRI Shapefile (import only). */`
   to `FormatType` (append near `FormatJWC` to avoid renumbering concerns; then check every
   `switch` over `FormatType` compiles — `grep -rn "FormatJWW" librecad/src` finds the
   switch sites to mirror).
2. New `librecad/src/lib/filters/rs_filtershp.{h,cpp}`:
   ```cpp
   class RS_FilterSHP : public RS_FilterInterface {
   public:
       RS_FilterSHP();
       static RS_FilterInterface* createFilter() { return new RS_FilterSHP(); }
       bool canImport(const QString& fileName, RS2::FormatType t) const override; // t == RS2::FormatSHP
       bool canExport(const QString&, RS2::FormatType) const override { return false; }
       bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType type) override;
       bool fileExport(RS_Graphic&, const QString&, RS2::FormatType) override { return false; }
   };
   ```
3. Register: `rs_fileio.cpp` `getFilters()` — append `RS_FilterSHP::createFilter`;
   `detectFormat()` — add `{"shp", RS2::FormatSHP}` to the lookup map (read path only).
4. Build wiring: top `CMakeLists.txt` source lists (with the other `rs_filter*` entries)
   and `librecad/src/src.pro` (`HEADERS` ~:538-543, `SOURCES` ~:1176-1180).

### Sub-plan 2b — geometry mapping (`fileImport`)
RAII wrappers (filter-local, in `rs_filtershp.cpp`): `ScopedSHP`/`ScopedDBF` closing via
`SHPClose`/`DBFClose` (port of the pattern already proven in `importshp.h:42-59`).

| SHP type | Native entities | Rules |
|---|---|---|
| `SHPT_POINT/Z/M` | `RS_Point` | Z from `padfZ[0]` **preserved** in `RS_Vector` (2D views ignore it; data survives round-trip) |
| `SHPT_MULTIPOINT/Z/M` | one `RS_Point` per vertex | — |
| `SHPT_ARC/Z/M` | one **open** `RS_Polyline` per part | parts split by `panPartStart`; parts with < 2 vertices skipped |
| `SHPT_POLYGON/Z/M` | one **closed** `RS_Polyline` per ring | rings with < 3 distinct vertices skipped; duplicated closing vertex (SHP convention: first==last) dropped before `setClosed(true)` |
| `SHPT_MULTIPATCH` | ring part-types → closed polylines; TRIANGLE_STRIP/FAN → open polyline wireframe | documented 2.5D simplification; `panPartType` consulted |
| `SHPT_NULL` / unknown | skipped, counted | import continues |

Skipped/failed record counts logged via `RS_DEBUG->print(RS_Debug::D_WARNING, …)`;
`fileImport` returns `false` only if the `.shp` itself cannot be opened or yields zero
readable records *and* had `numEntities > 0`.

### Sub-plan 2c — DBF attributes, layers, styling (the fidelity win)
1. **Codepage**: `DBFGetCodePage()` (shapelib reads `.cpg`/LDID itself) → map to
   `QStringDecoder` (UTF-8, LDID/87→Latin-1, common LDID table); fallback: try UTF-8, on
   invalid sequences re-decode Latin-1. Fixtures `utf8-property`/`latin1-property` pin this.
2. **Field auto-detection** (case-insensitive, first match):
   - layer: `LAYER`, `LEVEL`, `LYR` — a layer per value; a character DXF cannot hold in
     a symbol name becomes `_`
   - color: `COLOR`, `COLOUR` — value `0-255` treated as AutoCAD color index (reuse the
     ACI→`RS_Color` mapping from `rs_filterdxfrw.cpp`, `numberToColor`-equivalent), larger
     values as 24-bit RGB
   - linetype: `LINETYPE`, `LTYPE` — kept as the pen's linetype name, as from DXF, when
     DXF can hold it as a symbol name (the first 256 distinct names of an import); any
     other value → `RS2::LineType` via the mapping `rs_filterdxfrw` uses (`CONTINUOUS`,
     `DASHED`, …; unknown → `SolidLine`)
   - width: `WIDTH`, `LWEIGHT`, `LINEWT` — numeric → nearest `RS2::LineWidth`
   - label: `NAME`, `LABEL`, `TEXT` — for point shapes, adds an `RS_MText` next to the point
     (height = 2.0 drawing units default)
3. **Layers**: distinct layer-field values → `RS_Layer` created once
   (`graphic->findLayer()` first), default pen; entities assigned to their layer; records
   without a layer value land on layer `"0"`. Pattern: `rs_filterjww.cpp:190-212`.
4. **Per-entity pen**: `RS_Pen(color, width, linetype)` set via `entity->setPen()` when any
   styling field resolved; otherwise ByLayer defaults. **This is the capability the plugin
   could never deliver.**
5. **Missing `.dbf`**: geometry-only import (all on current layer, ByLayer pen) — do not fail.
6. **Options plumbing** (no UI this phase): `struct LC_ShpImportOptions { QString layerField,
   colorField, ltypeField, widthField, labelField; double labelHeight; bool importLabels; }`
   populated from auto-detection, overridable via `RS_Settings` group `"/ShpImport"` — the
   seam a future options dialog drives without touching the filter again.

### Sub-plan 2d — smoke test (in this commit)
Minimal `TEST_CASE` in `shp_import_filter_tests.cpp`: `RS_Graphic` +
`RS_FilterSHP().fileImport(g, points.shp, RS2::FormatSHP)` → returns true, entity count
matches inventory, first point coordinates approx-equal. (Full matrix lands in Phase 4.)

### Validation
```bash
cmake --build build-test --target librecad_tests -j12
ctest --test-dir build-test -R librecad_tests --output-on-failure
qmake6 librecad.pro && make -j12       # dual-build discipline
```

### Commit
```
feat(filters): native ESRI Shapefile import filter (RS_FilterSHP)

Adds RS2::FormatSHP and an import-only RS_FilterInterface implementation
on shapelib 1.6.3 (C++17). Full-fidelity mapping: per-entity pen
(color/linetype/width) and DBF-driven layers — beyond the retired
plugin's layer-only ceiling; Z preserved; codepage-aware DBF decoding.
```

---

## 5. Phase 3 — UI wiring (File→Open surfaces SHP) — done ✅

**Landed**: 2026-07-21 — `feat(ui): surface Shapefile import in File→Open and format detection`.
`fShp` filter added to `getOpenFile` only (import-only contract);
`getSaveFile` list left untouched.  Both build systems green after the edit;
librecad_tests still 25/25 [shp] cases, 631/631 total.
Optional Phase 3b (File→Import→Shapefile action) and 3c (Linux mime files)
deferred; can be follow-up commits without touching the filter.


**Goal**: user-visible: `*.shp` appears in the Open dialog, extension auto-detected, drawing
opens with imported content.

### Steps — all in `librecad/src/ui/dialogs/file/qg_filedialog.cpp`
1. `getExtension()` (~:42): `case RS2::FormatSHP: return QString(".shp");`
2. `hasExtension()` supported lists (:73 DWG-enabled and :75 fallback): append `".shp"`.
3. `getType()` (~:83): map filter string containing `"shp"` → `RS2::FormatSHP`.
4. Name filter (~:140): `fShp = tr("ESRI Shapefile %1").arg("(*.shp)");` — appended in
   `getOpenFile()` (:156) **only**; NOT in `getSaveFile`/`getSaveFileName` (import-only).
5. Audit remaining `FormatType` consumers so `FormatSHP` can't reach a save path:
   `grep -rn "FormatJWW" librecad/src --include=*.cpp` and mirror each *read-side* site;
   ensure save-as format pickers never offer SHP.
6. Optional (3b, separate small commit if wanted): a `File→Import→Shapefile…` action
   inserting into the *current* drawing, modeled on `BlocksImport`
   (`lc_actionfactory.cpp:677`, `slotImportBlock`) — nice-to-have; File→Open is the
   contract for this plan.
7. Optional (3c): add `*.shp` to the Linux desktop/mime association files if present
   (`desktop/` dir) — cosmetic, skip if noisy.

### Validation (manual, end-to-end — the check the plugin never had)
```bash
qmake6 librecad.pro && make -j12
open ./LibreCAD.app
# 1. File→Open → filter dropdown shows "ESRI Shapefile (*.shp)"
# 2. Open test_data/shp/polygons.shp → closed polylines render; layer list populated
# 3. Open test_data/shp/real_world/TM_WORLD_BORDERS-0.2.shp → 246 country polygons render
# 4. Open test_data/shp/multi_part/bc_hospitals.shp → 44 points + labels (NAME field)
# 5. Open test_data/shp/malformed_truncated.shp → error/partial-import, NO crash
# 6. Save-As dialog: no SHP entry offered
```

### Commit
```
feat(ui): surface Shapefile import in File→Open and format detection
```

---

## 6. Phase 4 — Full validation matrix, corpus gaps, hardening proof — done ✅

**Sub-plan 4b landed** (2026-07-21, commit `04b749474`): filter-level test
matrix over the pre-existing corpus.  40 [shp] cases / 1841 assertions.

**Sub-plan 4a landed** — done ✅ (2026-07-21):
`test(shp): generated Z/MULTIPATCH/DoS fixtures + filter coverage`.
`scripts/make_shp_fixtures.py` (pure Python stdlib `struct`-packing, no
GDAL) writes six new fixtures under `test_data/shp/` (all genuinely new
filenames — never touches existing pinned fixtures): `polygonz.shp`,
`polylinez.shp`, `multipointz.shp`, `multipatch.shp` (OUTER_RING +
INNER_RING + TRIANGLE_STRIP), `dos_npoints.shp` (nPoints=60M),
`dos_nparts.shp` (nParts=15M).  Each has a companion `.shx` (and `.dbf`
for the four happy-path Z types).  Refuses to overwrite existing files
without `--force`.  Inventory extended with `generated_z_types` and
`generated_hostile` buckets in `test_data/shp_inventory.json`.

New test coverage after 4a: `[shp]` grew to 52 cases / 1897 assertions
(6 shapelib-level cases pinning the raw record structure, 6 filter-level
cases pinning the RS_FilterSHP emission — including MULTIPATCH: 2 closed
polylines from the rings + 1 open polyline from the strip; and the two
DoS crafts: zero entities emitted, no allocation blow-up).  Full
librecad_tests suite 658 cases / 11 967 assertions with `~[corpus]`.
Skipped optional Natural Earth download (step 2) — network dependency
not worth it; documented as skippable.

**Sub-plan 4c landed** — done ✅ (2026-07-21): fresh `build-asan` Debug
tree configured exactly per this section's command block
(`-fsanitize=address,undefined -fno-omit-frame-pointer`, no additional
flags). `[shp]` tag: 52 cases / 1897 assertions, zero ASan/UBSan reports
(includes the 4a hostile DoS fixtures — `dos_nparts.shp` correctly hits
shapelib's own `nParts` corruption guard and is skipped without a
crash). Full suite with `~[corpus]`: 658 cases / 11 967 assertions,
zero ASan/UBSan reports.
`ASAN_OPTIONS=detect_leaks=1` was tried first and is a dead end on
macOS ("detect_leaks is not supported on this platform") — omit it
there; leak checking isn't available on this platform's ASan.
Discovered in passing (unrelated to SHP, not fixed here): an earlier
build-asan configuration that added `-fno-sanitize-recover=all` (beyond
what this plan specifies) hit one UBSan finding in unrelated DWG code —
`libdxfrw.cpp:6966`, an invalid `DRW::Version` enum load — which aborted
that run after 120/658 cases. It did not reproduce under this section's
own (recoverable) flags in the same full-suite run, so it looks
uninitialized-read-flavored and order/layout-dependent rather than
deterministic. Worth a follow-up issue against libdxfrw, out of scope
for this plan.


### Sub-plan 4a — complete the corpus (the "downloaded SHP sample files" item)
1. Generate missing Z/MULTIPATCH fixtures with a small stdlib-only script
   `scripts/make_shp_fixtures.py` (pure `struct`-packing, no GDAL dependency):
   `polygonz.shp`, `polylinez.shp`, `multipointz.shp`, `multipatch.shp` (one
   TRIANGLE_STRIP + OUTER_RING/INNER_RING record), plus two crafted hostile files:
   `dos_npoints.shp` (header claims `nPoints = 60,000,000` — shapelib must reject, cap is
   50M) and `dos_nparts.shp` (`nParts > 10M`).
2. Real-world download (one more independent source, license-friendly):
   Natural Earth `ne_110m_admin_0_countries` (public domain,
   `https://naciscdn.org/naturalearth/110m/cultural/ne_110m_admin_0_countries.zip`) →
   `test_data/shp/real_world/`. (Skip if offline; corpus is already rich.)
3. Regenerate/extend `test_data/shp_inventory.json` (record counts/types per file) —
   tests read expectations from it or hardcode from it, one source of truth.

### Sub-plan 4b — filter-level test matrix (`shp_import_filter_tests.cpp`)
| Test | Asserts |
|---|---|
| every basic + Z/M fixture | `fileImport` true; entity count & types match inventory; polylines open (ARC) vs closed (POLYGON); Z present on pointz entities |
| `multi_part_polyline` | one polyline per part; vertex counts per part |
| `bc_hospitals` + label field | `RS_MText` count == point count when labels enabled; text content matches DBF |
| layer field fixture (`mixed-properties`) | distinct `RS_Layer`s created; entities on correct layers |
| `utf8-property` / `latin1-property` | decoded strings byte-exact vs expected |
| `null_shape`, `empty`, `singleton` | no crash; correct (possibly zero) entity counts; import result semantics |
| `missing_shx` | full import succeeds |
| `malformed_dbf`, `malformed_truncated` | no crash; graceful degradation; `fileImport` result documented |
| `dos_npoints`, `dos_nparts` | `SHPReadObject` returns null; import completes without allocation blow-up (assert RSS-safe by construction: shapelib cap) |
| MULTIPATCH fixture | rings → closed polylines; strips → open wireframe |

### Sub-plan 4c — sanitizer proof (CVE-class hardening evidence)
```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \
  -DCMAKE_PREFIX_PATH=/opt/homebrew \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan --target librecad_tests -j12
ctest --test-dir build-asan -R librecad_tests --output-on-failure   # zero ASan/UBSan reports
```
Record the clean run's summary in the commit message (this is the auditable CVE-2023-30259
regression evidence).

### Commit
```
test(shp): full import validation matrix, fixture generator, ASan/UBSan pass

Covers all 14 SHP types (generated POLYGONZ/ARCZ/MULTIPOINTZ/MULTIPATCH
fixtures), codepages, multi-part splitting, null/empty/missing-shx, and
crafted DoS headers. Clean AddressSanitizer+UBSan run over the hostile
corpus documents CVE-2023-30259-class hardening.
```

---

## 7. Phase 5 — Retire the importshp plugin — done ✅

**Landed**: 2026-07-21 — `refactor(plugins)!: remove importshp plugin,
superseded by native SHP filter`.  `plugins/importshp/` git-rm'd; both build
systems deregistered (`plugins/CMakeLists.txt` line 109, `plugins/plugins.pro`
line 23); stale `LibreCAD.app/Contents/Resources/plugins/libimportshp.dylib`
deleted from the local bundle; superseded historical planning doc
`docs/plan_importshp_refactor.md` moved to `docs/attic/` with a "SUPERSEDED"
header note.  Untracked planning drafts
`docs/plan_importshp_fix_all.md` and `docs/plan_importshp_port_back.md`
left as-is on the working tree — they're not in git so they don't confuse
future readers via git history.  Post-commit validation: full-clean qmake6
build has zero `importshp` mentions in the log; librecad_tests unchanged.


**Gate**: Phases 2-4 fully validated (all tests green incl. sanitizers, manual E2E done).
The user decision was *native filter*, not *both* — the plugin (whose import has never
functioned: `execComm` bug) is superseded, and single-sourcing avoids maintaining two SHP
readers.

### Steps
1. `git rm -r plugins/importshp` (shapelib already relocated in Phase 1).
2. Deregister: `plugins/CMakeLists.txt:109` (`add_subdirectory(importshp)`),
   `plugins/plugins.pro:23` (`importshp \`).
3. Remove the stale artifact `LibreCAD.app/Contents/Resources/plugins/libimportshp.dylib`
   from the local bundle (it predates all fixes and would shadow-confuse manual testing);
   verify no packaging script references importshp
   (`grep -rn importshp scripts/ desktop/ CI 2>/dev/null`).
4. Move the three superseded plan docs under `docs/attic/` (or annotate their headers as
   superseded by this plan) so future readers don't implement against them.

### Validation
```bash
cmake -S . -B build-test -DBUILD_TESTS=ON ... && cmake --build build-test -j12   # full build, no references break
qmake6 librecad.pro && make -j12
open ./LibreCAD.app   # Plugins menu: no ESRI entry; File→Open: SHP works
```

### Commit
```
refactor(plugins)!: remove importshp plugin, superseded by native SHP filter

The plugin's Document_Interface path was layer-only/2D and its import was
never reachable (execComm never invoked procesFile). Native RS_FilterSHP
provides File→Open integration with per-entity styling and Z preservation.
```

---

## 8. Phase 6 — Docs, changelog, translations — done ✅

**Landed**: 2026-07-21 — `docs(shp): changelog, shapelib vendoring/resync notes, translations`.
CHANGELOG.md updated (Added: native ESRI Shapefile import; Removed: importshp
plugin).  `libraries/shapelib/README.librecad` already contained the full
vendoring notes + resync procedure from Phase 1b (no further edits needed).
Ran `lupdate librecad/src/src.pro -no-obsolete` — three new source strings
picked up by every locale (`ESRI Shapefile %1`, `Cannot open shapefile %1
(missing or corrupt .shx?)`, `Shapefile %1 contained %2 records but none
were readable`) — translations left `type="unfinished"` for translators to
fill in per the repo's normal workflow.


1. `CHANGELOG.md`: `Added: native ESRI Shapefile (.shp) import — File→Open, per-entity
   color/linetype/width from DBF attributes, shapelib 1.6.3 (C++17, CVE-2023-30259-hardened)`.
2. New `tr()` strings (dialog filter label, any warnings): run `lupdate` per the repo's
   translation workflow so `librecad_*.ts` pick them up (the repo has active ts tooling —
   `scripts/ts_review.py` exists; follow its conventions).
3. `libraries/shapelib/README.librecad`: version, upstream URL, license, local-delta list,
   and the **resync procedure** (download upstream tarball → overwrite `src/` C files →
   re-apply the 2 documented deltas → rename to `.cpp` → run Phase-0 corpus tests).
4. Update any user documentation listing supported formats.

### Commit
```
docs(shp): changelog, shapelib vendoring/resync notes, translations
```

---

## 9. Commit Ladder (summary)

| # | Phase | Commit (conventional, matches repo style) |
|---|---|---|
| 1 | 0 | `test(shp): add shapelib corpus tests and track test_data/shp fixtures` |
| 2 | 1a | `refactor(shapelib): move vendored shapelib 1.6.3 to libraries/shapelib` |
| 3 | 1b | `build(shapelib): compile shapelib as C++17 (.c -> .cpp)` |
| 4 | 2 | `feat(filters): native ESRI Shapefile import filter (RS_FilterSHP)` |
| 5 | 3 | `feat(ui): surface Shapefile import in File→Open and format detection` |
| 6 | 4 | `test(shp): full import validation matrix, fixture generator, ASan/UBSan pass` |
| 7 | 5 | `refactor(plugins)!: remove importshp plugin, superseded by native SHP filter` |
| 8 | 6 | `docs(shp): changelog, shapelib vendoring/resync notes, translations` |

Every commit builds green under **both** build systems and keeps `ctest` green — the ladder
is bisect-safe. (Optional 3b `File→Import→Shapefile…` action would slot between 5 and 6.)

---

## 10. Risks & Mitigations

| Risk | Severity | Mitigation |
|---|---|---|
| Dual build systems drift (CMake vs qmake edits missed) | High | Every phase's validation runs *both*; commit checklist includes the src.pro/CMakeLists pair |
| `FormatSHP` leaks into a save/export path | Medium | Phase 3 step 5 audits every `FormatType` switch; Phase 3 manual check "Save-As offers no SHP"; `canExport` returns false |
| shapelib upstream resync friction after local deltas | Medium | Deltas capped at 2 (CPL dedupe, safileio casts), each listed in `README.librecad` with the resync procedure; core `shpopen/dbfopen` byte-identical to upstream apart from filename |
| Windows build of `safileio.cpp` (`SHPAPI_WINDOWS` wchar block guarded off on macOS) | Medium | CI on Windows must compile it as C++ before release; flag in PR description; the block's casts are part of the 16 fixed in 1b |
| `common.pri:30` `QMAKE_CFLAGS=-std=c++17` revert breaks another `.c` consumer | Low | Gated on repo-wide grep; revert only if zero remaining `.c` in qmake build |
| MULTIPATCH wireframe simplification surprises GIS users | Low | Documented in CHANGELOG + debug-log notice per simplified record |
| Very large shapefiles (100k+ records) freeze UI on open | Low | Import is batch (no per-entity redraw); if profiling shows pain, note follow-up for a progress callback — out of scope here |
| No `.prj` reprojection (raw coordinates imported) | Accepted | Same as retired plugin; documented limitation; future enhancement hook is `LC_ShpImportOptions` |
| Locale decimal parsing of DBF numerics | Low | shapelib uses `psDBF->sHooks` atof paths — corpus `number-property` test pins behavior |

## 11. Explicit Non-Goals

- **SHP export** (`canExport == false`) — write support is a separate project.
- **`.prj`/CRS reprojection** — would drag in PROJ/GDAL.
- **Options dialog** for field mapping — the `LC_ShpImportOptions`/`RS_Settings` seam is
  built in Phase 2c; the dialog itself is follow-up work.
- **True polygon holes** — inner rings import as separate closed polylines (LibreCAD has no
  polygon-with-holes entity).
