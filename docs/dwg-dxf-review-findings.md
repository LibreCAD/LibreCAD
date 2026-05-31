# DWG/DXF Code Review — Findings Register

Scope: the `libdxfrw` engine (`libraries/libdxfrw/src` + `intern/`) and the
LibreCAD filter bridge (`librecad/src/lib/filters/rs_filterdxfrw.{cpp,h}`).
Method: strict-warning + analyzer + cppcheck passes (Dim 1), C++17/build-wiring
audit (Dim 2), and per-module adversarial correctness review (Dim 3 function
scope, Dim 4 class scope). Severity = Critical / High / Medium / Low / Advisory.
The three High filter/engine bugs marked ✓verified were read-confirmed against
source during the review.

## High

| ID | Dim | File:line | Summary | Fix |
|----|-----|-----------|---------|-----|
| F01 ✓ | 3 | drw_entities.cpp:3222 | INSERT/MINSERT attrib loop `for (duint8 i=0; i<objCount; ++i)` over a 32-bit `objCount` (getBitLong, 3196). ≥256 attribs → counter wraps → **infinite loop / buffer over-read** on large or malformed files. | Use `dint32 i` (as `DRW_Table::parseDwg:3283` does) and bounds-check `objCount` before the loop. |
| F02 | 3 | drw_entities.cpp:7588 | VIEWPORT `for (duint8 i=0; i<frozenLyCount; ++i)` over 32-bit `frozenLyCount` (getBitLong, 7554). Same wrap → infinite loop. | Widen counter to `duint32`; validate `frozenLyCount`. |
| F03 | 4 | drw_objects.h:1047 | `DRW_AppId::reset()` does not chain `DRW_TableEntry::reset()`. AppId inherits the EED-accumulating base `parseCode`, and `dxfRW::processAppId` reuses one object with `reset()` between records → EED **leaks and cross-contaminates** between APPID records. | Append `DRW_TableEntry::reset();` to `DRW_AppId::reset()`. |
| F04 ✅ FIXED | 3 | drw_header.cpp:2575 + dwgwriter24.cpp:57 | `encodeDwg` emitted **no R2007+ string stream**, dropping header strings (MENU/DIMPOST/PROJECTNAME/GUIDs) for AC1024/27/32. **Fixed 2026-05-31**: `encodeDwg` gained a `strBuf` param routing the R2007+ header strings; `writeDwgHeader` appends the string footer. New AC1024 round-trip test passes; full suite green (451 cases). See `dwg-dxf-f04-f06-solution.md`. | (done) |
| F05 | 3 | dxfreader.cpp:160 | Binary-DXF `readInt16`: `(int)((buffer[1]<<8)|buffer[0])` with signed `char` → low/high byte **sign-extends**; any int16 with bit 7 set is misread. `readInt32`/`readDouble` use unsigned ptrs; this one doesn't. | Mask each byte: `((unsigned char)buffer[1]<<8)|(unsigned char)buffer[0]` via a `dint16`/`duint16` intermediate. |
| F06 | 4 | drw_objects.cpp:2975 + audit list | **REVISED → Medium** (deep research, see `dwg-dxf-f04-f06-solution.md`). R2007 read is *not* fundamentally broken: `DRW_Entity::parseDwg`/`DRW_TableEntry::parseDwg` compute the string-stream offset version-correctly for AC1021 (RL) and AC1024+ (MC), and string-bearing types already pass `strBuf`. Residual gap: specific `parseDwg` arms that pass `NULL` but contain inline TV/TU fields — audit `DRW_XRecord`, `DRW_Underlay`, `DRW_MLine`, `DRW_Insert`, `DRW_Ole2Frame` + the `TODO` at drw_entities.cpp:6464. | Route those arms through the existing `sBuf` idiom. **Do NOT reject AC1021** (it largely works). |
| F07 ✓ | 3 | rs_filterdxfrw.cpp:4540 | `addImage` null-guards only the metadata call (4540) then unconditionally derefs `data->basePoint`/`secPoint`/`vVector`/… (4543-4551). Every sibling pointer callback early-returns on null; this one crashes. | Add `if (data == nullptr) return;` after the metadata block. |
| F08 | 3 | rs_filterdxfrw.cpp:5005 | `linkImage` — identical pattern: guards `addImageDefinition(*data)` (5005) then derefs `data->handle`/`data->name` (5008-5009). | Add `if (data == nullptr) return;` after the metadata block. |
| F09 ✓ | 3 | rs_filterdxfrw.cpp:8405 | `writeEntity` dispatch routes the six dimension RTTIs to `writeDimension` but **omits `RS2::EntityDimArc`** → arc-length dimensions hit `default` and are **dropped on export**; the correct `case EntityDimArc` arm in `writeDimension` (9260-9286) is dead/unreachable. | Add `case RS2::EntityDimArc:` to the dimension group at 8405-8410. |

## Medium

| ID | Dim | File:line | Summary | Fix |
|----|-----|-----------|---------|-----|
| F10 | 4 | drw_entities.h:1788 | `DRW_Dimension::hdir` has no initializer; default + copy ctors omit it (`//RLZ ... hdir = ???`, 1713). DXF-sourced or copy-constructed dimensions write **garbage** via `encodeDwgDimBase` putBitDouble (6477). | `double hdir = 0.0;` + copy it in the copy ctor (also `flipArrow1/2`). |
| F11 | 4 | drw_entities.h:937 | `DRW_LWPolyline` has a deep-copy copy-ctor but **implicit shallow copy-assign** (Rule of 3/5 hole, `//TODO rule of 5` at 949) → assigned copies alias each other's vertices. | Add a matching deep-copy `operator=` (+ `=default` moves) or drop the custom ctor. |
| F12 | 3 | drw_objects.cpp:2856 vs 2964 | XRECORD parse splits values/handles into two vectors, losing their **interleave order**; encoder emits all values then all handles. Breaks the ordered-group-code contract for interleaved XRECORDs. | Store one ordered (code, value-or-handle) sequence so the encoder replays original order. |
| F13 | 4 | drw_header.h:92 | `DRW_Header::operator=` calls `clearVars()` (deletes all variants) but never resets `curr` (copy-ctor does). Reparse after assignment → **use-after-free**. | Add `curr = nullptr;` inside the self-check block. |
| F14 | 3 | dwgbuffer.cpp:248 | `getBit` guards `!isGood()`; `get2Bits`/`getRawChar8`/`getRawShort16`/`getRawDouble` do **not**, and ignore `read()`'s return → decoding **silently continues on stale/zero bytes** past EOF, defeating callers' `isGood()` checks. | Add the `isGood()` guard (or check `read()` result and stop advancing) to the low-level getters. |
| F15 | 4 | rs_filterdxfrw.cpp:1172 | Orphan-XREF warning is gated on `m_xrefStack.empty()`, but the RAII guard inserts self at 1000 and erases only on return → condition **always false**, warning is dead code. | Capture `isOuter = (m_xrefStack.size()==1)` after self-insert and gate on that. |
| F16 | 4 | rs_filterdxfrw.cpp:991 | `m_dummyContainer = new …` (991) is deleted only at 1155, past both failure returns (1115 DWG, 1146 DXF) → **leak on every failed import**. | `unique_ptr` or delete on both failure paths (RAII guard). |
| F17 | 4 | rs_filterdxfrw.cpp:1700 | `endBlock` calls `removeBlock(bk)` (deletes `*D` anon block) but never erases its `m_blockHash` entry → **dangling pointer**; later `setBlock(handle)` returns freed memory. Compounded by caches never cleared at import entry. | Erase the `m_blockHash` entry in `endBlock`; clear per-import caches at top of `fileImport`. |
| F18 | 3 | rs_filterdxfrw.cpp:4417 | Hatch polyline-loop does `loop->objlist.at(0)` + unchecked `static_cast<DRW_LWPolyline*>` with no empty/type check. Malformed hatch → `std::out_of_range` **thrown out of a non-exception-safe callback** (abort), or UB downcast. | `if (loop->objlist.empty()) continue;` + verify `eType == LWPOLYLINE`. |

## Low

| ID | Dim | File:line | Summary | Fix |
|----|-----|-----------|---------|-----|
| F19 | 3 | drw_entities.cpp:6688 | `DRW_DimArc::parseDwg` reads leader pts unconditionally; `encodeDwg` substitutes extension-line pts when `hasLeader==false` → **lossy round-trip** of Pt6/leaderPt2. | Always write the stored `getPt6()`/`leaderPt2`. |
| F20 | 3 | drw_entities.cpp:1092 | EED outer chunk size stored in signed `dint16` (`getBitShort`); a 32768-65535 chunk goes negative → loop exits early, EED dropped. | Use `duint16`/`dint32`. |
| F21 | 3 | drw_entities.cpp:1057 | R2010+ `objSize = ms*8 - bs` (all unsigned); `bs>ms*8` underflows to ~UINT32_MAX, then drives `setPosition(objSize>>3)` → out-of-range seek. | Clamp: `(bs<=ms*8)?ms*8-bs:0` and bail on inconsistency. |
| F22 | 4 | drw_objects.h:122 | `DRW_TableEntry` copy-ctor omits `oType`/`objSize`; copy-assign (162-163) copies them → divergent copy semantics. (Contained today: both are parse-transient.) | Add `oType`/`objSize` to the copy-ctor init list. |
| F23 | 3 | dwgwriter15.cpp:1660 | `replayRawObject` doesn't validate `m_bodyBitSize` vs `m_rawBytes.size()*8`, and re-checks no source-version gate (only the filter layer does). | Reject when `m_bodyBitSize > m_rawBytes.size()*8`; assert source==target version. |
| F24 | 3 | drw_header.cpp:49 | Header `vars[name]=curr` / `add*` overwrite existing map entries without `delete` → **leak on duplicate keys**. | Delete existing before assign, or use `unique_ptr<DRW_Variant>`. |
| F25 | 3 | dxfreader.cpp:274 | Non-Apple `readDouble` uses `istringstream >> d`, which honors the global C++ locale (unaffected by `setlocale(LC_NUMERIC,"C")`). Comma-decimal locale → "1.5" parses as 1. | `sd.imbue(std::locale::classic())` or `std::from_chars`. |
| F26 | 3 | dwgbufferw.cpp:277 | `putModularChar` drops the sign bit when magnitude fills the 4th chunk's 0x40 → large positive offset deltas (~>64 MB files) decode as negative → corrupt object map. | Reject out-of-MC-range values on write (mirror reader's representable range). |
| F27 | 3 | dwgbufferw.cpp:391,355 | `putUCSText`/`putCP8Text` emit a `duint16` length but the full payload for strings >65535 units → truncated count + full bytes = **stream desync**. | Guard/clamp oversized strings (fail or emit consistent prefix). |
| F28 | 4 | drw_base.h:92 | `dwgVersionStrings` is `unordered_map<const char*, …>` — hashes by pointer identity; works only because the sole lookup uses `strncmp` linearly. `.find()`/`[]` would silently miss. | Key by `std::string`, or use a plain scanned array. |
| F29 | 3 | dwgbufferw.cpp:543 | `patchRawLong32AtBit` `>=` size guard can leave `objSize` unpatched (0) for minimal bodies; benign for R2000/R2004 (informational) but a latent conformance gap. | Assert body ≥5 bytes after OT+RL, or harden the guard. |

## Advisory

| ID | Dim | File:line | Summary |
|----|-----|-----------|---------|
| F30 | 4 | drw_objects.h:114 | `DRW_TableEntry` declares dtor+copy ops but **no move ops** (Rule-of-5 gap) → every "move" deep-copies `extData`. Add `noexcept` move ctor/assign. |
| F31 | 3 | dwgbufferw.cpp:63 | `alignToByte` relies on the undocumented invariant that all `put*` leave pad bits zero (true today; fragile for future primitives). Optionally assert. |
| F32 | 1 | rs_filterdxfrw.cpp:2647 | `if (alignV!=0 || alignH!=0 || alignH==HMiddle)` — third disjunct (HMiddle=4) is always implied by the second → dead term. |
| F33 | 1 | rs_filterdxfrw.cpp:4408 | Recurring unnamed magic bit-flags: hatch `type & 32`/`& 2`, dim `type >= 128`/`& 64`, `1+32`…`6+64`. Lift to named constants on both import & export sides. |
| F34 | 3 | rs_filterdxfrw.cpp:4485 | Hatch ellipse-edge angle→param quadrant fixup uses strict `> M_PI`; the exact-π / 0-2π seam recovers the wrong half-period. Use `>=` or compute param from the point. |
| F35 | 3 | rs_filterdxfrw.cpp:2493 | `addSpline`/`convDimensionData` deref their `const DRW_*` arg with no null guard (latent; libdxfrw never passes null today). Add `if (!data) return …;` for callback uniformity. |
| F36 | 4 | drw_entities.h:1788 | `DRW_Dimension` copy ctor also drops `flipArrow1`/`flipArrow2` (default to false, so lower-impact than `hdir`); fix alongside F10. |

## Dimension 1 — static-analysis tool output

Engine is already clean at `-Wall -Wextra -Wpedantic`. Full `scripts/dwg-review-lint.sh`
run (26 engine TUs + filter):

**Strict-warning pass — 959 total:**

| Category | Count | Disposition |
|----------|-------|-------------|
| -Wsign-conversion | 723 | Mostly intentional truncation in bit code → **baseline**, do not churn. |
| -Wold-style-cast | 65 | Modernize incrementally (Dim 2, C-casts). |
| -Wimplicit-int-conversion | 57 | **Review** — can hide real truncation; cross-check vs F01/F05/F20. |
| -Wshadow | 30 | **Highest value** — triage each (clusters in drw_entities.cpp 5459-5608). |
| -Wfloat-equal | 27 | Geometry `==` on doubles; review tolerance-sensitive ones. |
| -Wcast-align | 5 | **dxfreader.cpp:118/124/170/181/192** — alignment/UB in binary-DXF reads (ties to F05, C2). |
| -Wunused-function / -but-set | 2 | `patchRLL` (dwgwriter18.cpp:72) dead; `cb` (dwgbuffer.cpp:807). |

**clang static analyzer — 6:** the notable one is the rscodec OOB below (F37);
the rest are 2 dead stores (drw_header.cpp:2417 `sz`, dwgbuffer.cpp:827 `cb`)
and an uninitialized assign (dwgreader.h:86).

**cppcheck 2.20.0 — 1309 issues** (`build-review-lint/cppcheck.log`). Actionable
clusters:

| Category | Count | Disposition |
|----------|-------|-------------|
| uninitMemberVar | 73 | **Systemic — see F38.** Same bug class as F10 (`hdir`); each can leak garbage into DWG output. |
| duplInheritedMember | 45 | **See F39** — derived member shadows a base member; possible real aliasing bugs. |
| missingOverride | 36 | Add `override`; catches silent signature mismatches in the parseDwg/encodeDwg vtables. |
| knownConditionTrueFalse | 31 | **Review** — dead/always-true conditions (e.g. F32); some may hide intended logic. |
| shadowVariable (29), variableScope (32) | 61 | Overlaps -Wshadow; triage with F-series. |
| passedByValue / constParam* / stlcstrParam / useInitializationList … | ~250 | Perf/const-correctness/modernize → batch B4 (advisory). |

clang-tidy requires Homebrew LLVM (not installed) + `.qtc_clangd/compile_commands.json`;
the harness auto-runs it once `clang-tidy` is on PATH.

New findings promoted from tool output:

| ID | Dim | File:line | Summary | Fix |
|----|-----|-----------|---------|-----|
| F37 | 3 | rscodec.cpp:102,108 | `RSgen_poly` guards only `gg[j] < 0` before `index_of[gg[j]]`; the **upper bound is unchecked** so an out-of-range `gg[j]` indexes `index_of[]`/`alpha_to[]` OOB (analyzer-flagged). Low reachability (GF params are fixed, not per-file), but the guard is asymmetric. | Bound-check both ends: `if (gg[j] < 0 \|\| gg[j] > nn) { isOk=false; return; }`. |
| F38 | 4 | (73 sites, cppcheck `uninitMemberVar`) | Systemic uninitialized data members across `DRW_*` classes — the generalization of F10. Any member read by `encodeDwg`/`write*` but left unset by a constructor emits garbage. | Add in-class default initializers (`= 0` / `= 0.0` / `{}`); enable `cppcheck --enable=warning` in CI to hold the line. Triage the 73 against the encode paths first. |
| F39 | 4 | (45 sites, cppcheck `duplInheritedMember`) | A derived `DRW_*` class declares a member with the same name as a base member → the two shadow each other; reads/writes may hit the wrong one. | Audit each: remove the duplicate (use the base member) or rename; several are likely real round-trip bugs. |

## Dimension 2 — C++17 validity & standard unification

The subsystem is **valid and well-defined under C++17**: it compiles clean at
`-std=c++17 -Wall -Wextra -Wpedantic`, and is free of `auto_ptr`, dynamic
exception specifications, `register`, and unsafe C string functions. Findings:

| ID | File:line | Summary | Fix |
|----|-----------|---------|-----|
| C1 | CMakeLists.txt:4 + libraries/libdxfrw/CMakeLists.txt:23,107 | **Three-way standard split:** root `CMAKE_CXX_STANDARD 17`, libdxfrw dir `CMAKE_CXX_STANDARD 11`, and `target_compile_features(dxfrw PUBLIC cxx_std_14)`. The library is built at a different standard than the tree that consumes it. | Unify to C++17: set `CMAKE_CXX_STANDARD 17` in libdxfrw and raise the feature floor to `cxx_std_17` (or drop it). Verify the standalone lib + `dwg2dxf`/`tests` still build. |
| C2 | dwgbuffer.cpp (×8), drw_objects.cpp (×6), … (~26 sites) | Byte-buffer `reinterpret_cast` to typed pointers (e.g. `double`) — `char`-aliasing is legal but the typed-pointer deref is a strict-aliasing/alignment risk. | Replace with `std::memcpy` into the typed value (zero-cost, well-defined). |
| C3 | 30 sites (.cpp) | `NULL` vs `nullptr` inconsistency. | Mechanical `NULL`→`nullptr` (clang-tidy `modernize-use-nullptr`). |
| C4 | dxfreader.cpp:274 | (= F25) Locale double-parse → use C++17 `std::from_chars`. | See F25. |
