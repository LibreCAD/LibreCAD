# DWG/DXF Code Review — Fix Plan

Derived from `dwg-dxf-review-findings.md`. Findings are grouped into
independently-shippable batches, ordered by **severity × independence**. Each
fix names a proving test (extend the existing Catch2 suite under
`librecad/src/lib/filters/tests/`; round-trip pattern uses the
`DrwEntityEncodeTestAccess` friend harness). Build/run with
`cmake -B build-review -DBUILD_TESTS=ON && cmake --build build-review -j && ./build-review/librecad_tests`.

Each correctness fix should land with a test that **fails before, passes after**.

---

## B0 — Tooling & hygiene (no behavior change; land first)
*Independent, low-risk, unblocks the rest.*

- **Lint harness** — `scripts/dwg-review-lint.sh` (done). Wire it into CI as a
  non-gating report initially; capture today's counts (959 warnings / 6 analyzer
  / 1309 cppcheck) as the baseline so regressions are visible.
- **Remove the 20 `.orig` backups** from the tree (or add `*.orig` to
  `.gitignore`) so they stop polluting greps/tooling.
- **C1 — unify the C++ standard to 17:** `libraries/libdxfrw/CMakeLists.txt:23`
  → `CMAKE_CXX_STANDARD 17`; line 107 → `cxx_std_17` (or drop the feature line).
  Verify the standalone lib + `dwg2dxf` + libdxfrw `tests/` still build.
- *Proof:* harness baseline committed; `cmake` configure+build of both the
  standalone lib and the in-tree target succeed at C++17.

## B1 — Correctness: crash / data-loss (highest priority)
*Each is a real defect with user-visible impact; mostly small, independent diffs.*
**Status: the no-decision items are APPLIED (2026-05-31)** — all compile-verified
(engine standalone + filter via compile DB; `librecad_lib` rebuilt clean).
F04/F06 held for a decision — see "F04 / F06 decision report" below.

| Finding | Status | Fix | Proving test |
|---------|--------|-----|--------------|
| **F01, F02** | ✅ applied | Counter → `dint32`/`duint32` **and `&& buf->isGood()`** guard (a corrupt 32-bit count would still spin a wide counter to ~4B reads; the buffer-good guard is what actually stops the runaway). drw_entities.cpp:3285,7651. | Crafted INSERT-≥256-attribs / VIEWPORT-≥256-frozen (or unit-drive `parseDwg`); assert termination. |
| **F09** | ✅ applied | Added `case RS2::EntityDimArc:` to the `writeEntity` dimension group (rs_filterdxfrw.cpp:8410) → routes to the existing (previously dead) DimArc arm in `writeDimension`. | Round-trip an `LC_DimArc` export→import; assert it survives. |
| **F07, F08** | ✅ applied | `if (data == nullptr) return;` after the metadata block in `addImage`/`linkImage`. | Invoke the callback with `nullptr` — no crash. |
| **F03** | ✅ applied | Appended `DRW_TableEntry::reset();` to `DRW_AppId::reset()` (drw_objects.h:1098). | Two APPID records w/ distinct EED; assert no bleed/leak (ASAN). |
| **F05** | ✅ applied | `readInt16` now `static_cast<dint16>((uchar)hi<<8 \| (uchar)lo)` — unsigned bytes, signed 16-bit result. | Binary-DXF int16 with bit 7 set. |
| **F18** | ✅ applied | `if (loop->objlist.empty()) continue;` before `at(0)` (rs_filterdxfrw.cpp:4417) — kills the `std::out_of_range` thrown out of the callback. (Type-confusion downcast left as a lower-severity follow-up.) | `addHatch` polyline-flagged loop w/ empty objlist — no throw. |
| **F04** | ⏸ held | Emit the R2007+ header string stream in `encodeDwg`, or (interim) `return false` for `>AC1018`. | New header round-trip test at AC1024/27/32. |
| **F06** | ⏸ held | Finish R2007 string-stream separation **or** reject AC1021 on read. | AC1021 entity-string read test, or assert `BAD_VERSION`. |

### F04 / F06 decision report

Both currently **silently corrupt** data and both gate on the same question:
*finish R2007/R2010+ support now, or fail honestly until it's finished?*

**F06 — R2007 (AC1021) READ.** Confirmed: `dwgRW::openFile` sets `BAD_VERSION`
only when the reader factory returns null (`libdwgr.cpp:917`), but
`createReaderForVersion` returns a live `dwgReader21` for AC1021 (`:866`). So
R2007 files **import without error**, yet every entity/object string is read
from the wrong offset because R2007's data/string/handle stream separation is
done only for CLASSES + the header, not for entities/objects (`drw_entities.cpp:6464`
carries a `TODO: AC1021+ strBuf routing`). Result: garbage layer/text strings +
cursor desync. (The `libdwgr.cpp:205` rejection some notes cite is the *write*
guard and is correct — there is no `dwgWriter21`.)
- *Option A (small, safe now):* reject AC1021 on read — add a version check in
  `openFile`/`createReaderForVersion` so AC1021 returns `BAD_VERSION`. Converts
  silent corruption into an honest "R2007 not supported." ~5 lines + 1 test.
- *Option B (large):* thread a string-stream `strBuf` (positioned via record
  bit-size) through the entity/object `parseDwg` chain for AC1021, mirroring the
  header/CLASSES handling. Substantial; needs R2007 fixtures.

**F04 — R2010+ (AC1024/27/32) header WRITE.** Confirmed: `DRW_Header::encodeDwg`
admits AC1024/27/32 (`drw_header.cpp:2576`) and `dwgWriter24/27/32::writeDwgHeader`
calls it (`dwgwriter24.cpp:57`) — but its signature has **no string-buffer
parameter** and it emits the header strings inline only for `version < AC1021`.
For R2010+ the ~12 header strings the reader expects in a separate string stream
are simply absent → corrupt header + bit-cursor desync. Only AC1015/AC1018
header round-trips are tested.
- *Option A (small, safe now):* make `encodeDwg` `return false` for `>AC1018`
  (and have the DWG export UI offer only ≤R2004) so R2010+ export fails cleanly
  instead of writing a corrupt file. A few lines.
- *Option B (large):* add a string-stream path to `encodeDwg` + `dwgWriter24`
  (emit the UCS-16 strings + the stream-present bit/length so the reader's
  backward scan finds them). Mirrors `parseDwg` 2353-2372.

**Recommendation:** take **Option A for both now** (honest failure beats silent
corruption, and they're each a few lines + a test), and schedule Option B as
dedicated R2007/R2010+ feature work with real fixtures. This is a product call
on how much R2007/R2010+ LibreCAD intends to support — hence held for you.

> **RESOLVED (deep validation + implementation — see `dwg-dxf-f04-f06-solution.md`):**
> - **F06 — VALIDATED NON-ISSUE, no fix.** R2007+ string read *and* write are
>   already fully wired (offset version-correct; string-bearing types route
>   through `sBuf`/`strBuf`; writer threads `m_objectStrings`→`finishObject`).
>   Every `NULL`-strBuf arm was confirmed string-free; XRecord is self-consistent.
>   Removed the stale `drw_entities.cpp:6527` TODO. AC1021 stays accepted on read.
> - **F04 — IMPLEMENTED & TESTED.** `DRW_Header::encodeDwg` gained a `strBuf`
>   param routing the R2007+ header strings; `dwgWriter24::writeDwgHeader` appends
>   the `[strings][7 zero bits + RS(strBytes*8+7) + present bit]` footer
>   (`bitSize = 32 + dataBuf.size()*8`, now inclusive). New test
>   `R2010 header string vars round-trip through the string stream` passes;
>   full suite 451 cases / 7048 assertions, 0 failures (135 AC1024/27/32 write
>   tests still green after the layout change).

## B2 — Ownership / lifetime (memory safety)
**Status: IMPLEMENTED & TESTED (2026-05-31)** — full suite 451 cases / 7048
assertions, 0 failures (the file round-trip tests exercise the F16/F17 import
paths). Done: F13 (UAF — `operator=` resets `curr`), F24 (dup-key leak —
`storeVar` delete-before-overwrite), F22 (copy-ctor copies `oType`/`objSize`),
F10/F36 (`hdir` init + copy `hdir`/`flipArrow1`/`flipArrow2`), F11 (LWPolyline
deep-copy `operator=`), F16 (`m_dummyContainer` RAII guard frees on all returns),
F17 (`endBlock` erases the `m_blockHash` entry + per-import caches cleared at
`fileImport` start — XREF uses a separate child filter so this is safe),
F38 (45 encode-path uninitialized members initialized in drw_entities.h +
drw_classes.h). **Deferred:** F30 (move ops — Advisory/perf-only; correct fix is
migrating `DRW_TableEntry::extData` to `unique_ptr`, a Rule-of-0 refactor) and the
28 reader-internal uninit members (scratch state, never written to files).
Recommended follow-up: an ASAN run for defense-in-depth.

*Run B1 + B2 under ASAN to confirm.*

| Finding | Fix |
|---------|-----|
| **F13** | `DRW_Header::operator=` — reset `curr = nullptr` after `clearVars()`. |
| **F16** | `m_dummyContainer` → `unique_ptr` (or delete on both `fileImport` failure returns). |
| **F17** | Erase the `m_blockHash` entry in `endBlock`; clear per-import caches (`m_blockHash`, `m_mlineStyleCache`, `m_underlayDefMap`, `m_xrefBlockNames`) at the top of `fileImport`. |
| **F24** | Header `vars`/`customVars` → delete-before-overwrite, or migrate to `unique_ptr<DRW_Variant>`. |
| **F11** | `DRW_LWPolyline` — add deep-copy `operator=` + `=default` moves (or drop the custom copy ctor). |
| **F22, F30** | `DRW_TableEntry` — add `oType`/`objSize` to copy-ctor; add `noexcept` move ops (Rule-of-5). |
| **F10, F36, F38** | Initialize `DRW_Dimension::hdir`/`flipArrow1/2`; then sweep the **73 `uninitMemberVar`** sites, prioritizing members read by encode paths. |

*Proof:* the existing round-trip + metadata suites run green under
`-fsanitize=address,undefined`; add a copy/move-semantics test for
`DRW_LWPolyline` and `DRW_TableEntry`.

## B3 — Inverse-property & robustness (lower-severity correctness)

| Finding | Fix |
|---------|-----|
| **F12** | XRECORD — store one ordered (code, value-or-handle) sequence; replay in order. |
| **F14** | Low-level getters (`get2Bits`/`getRawChar8`/`getRawShort16`/`getRawDouble`) — add `isGood()` guard / honor `read()` result. |
| **F19, F20, F21, F23** | DimArc leader-point symmetry; EED chunk size → unsigned; clamp `objSize` underflow; validate `replayRawObject` bit-size + version gate. |
| **F26, F27, F29** | Writer: reject out-of-MC-range `putModularChar`; guard oversized `putUCSText`/`putCP8Text`; harden `patchRawLong32AtBit`. |
| **F37** | rscodec `RSgen_poly` — bound-check the upper end of `gg[j]`. |
| **F39** | Resolve the **45 `duplInheritedMember`** shadows (remove/rename); several may be real round-trip bugs — verify each against parse/encode. |
| **F25** | Locale-independent double parse — `std::from_chars` / `imbue(classic())`. |

## B4 — Static-analysis backlog & modernization (advisory, mechanical)

- **F28** version-string map → `std::string` key or scanned array.
- **F31, F32, F33, F34, F35** — pad-bit assert; dead `||` term; named constants
  for hatch/dim bit-flags; ellipse exact-π boundary; null-guard
  `addSpline`/`convDimensionData`.
- **C2** — replace ~26 byte-buffer `reinterpret_cast` derefs with `std::memcpy`
  (start with the 5 `-Wcast-align` sites in `dxfreader.cpp`).
- **C3** — `NULL`→`nullptr` (clang-tidy `modernize-use-nullptr`, 30 sites).
- cppcheck `missingOverride` (36) → add `override`; `passedByValue`/`constParam*`
  → const-correctness sweep. Drop the dead `patchRLL`.
- *Disposition:* baseline `-Wsign-conversion` (723) — do **not** churn.

## B5 — Test coverage (close the gaps the review exposed)

- Add round-trip/import tests for the parse↔encode pairs flagged above (DimArc,
  XRECORD interleave, AC1024+ header strings, INSERT/VIEWPORT large counts).
- Add import-fidelity tests for the **sparse types**: 3DFace, Trace, Solid, Ray,
  Xline, ModelerGeometry, Light, Viewport, and the untested OBJECTS
  (Group, Sun, RasterVariables, GeoData, SpatialFilter, the Phase-2b raw-replay
  objects — verify byte-exact round trip).
- Add a copy/move-semantics test fixture for the `DRW_*` classes with custom
  copy ops (F11, F22, F30).

---

## Sequencing & open questions

- **Order:** B0 → B1 → B2 (gate on ASAN green) → then B3/B4/B5 in parallel.
- **F04 / F06 need a product decision before implementation:**
  1. **R2007 read (F06)** — finish R2007 string-stream support, or hard-reject
     AC1021 on read? (Finishing is substantial; rejecting is a few lines and
     stops silent corruption now.)
  2. **2010+ header write (F04)** — implement the string stream, or restrict DWG
     export to ≤R2004 until it exists?
  Both currently *silently* corrupt; the interim "reject/return false" options
  convert silent corruption into an honest, testable limitation.
- **Effort sketch:** B0 ~0.5d · B1 ~2-3d (F04/F06 excluded — those depend on the
  decision) · B2 ~2d + the 73-member uninit sweep ~1-2d · B3 ~2-3d ·
  B4 mechanical ~1-2d · B5 ongoing.
