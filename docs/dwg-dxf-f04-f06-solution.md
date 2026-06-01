# F04 / F06 Solution Research — R2007+ DWG String Stream

Deep research (ODA spec `~/doc/dwg/dwg.pdf` → `/tmp/dwgspec.txt`; libdxfrw code at
`libraries/libdxfrw/src`; LibreDWG `master` reference) into how to *properly*
fix F04 (R2010+ header write) and F06 (R2007 entity/object read). All three
sources corroborate the algorithm below.

## STATUS (2026-05-31): validated & resolved

Deep validation against the actual code changed the picture and both are now closed:

- **F06 — VALIDATED NON-ISSUE, no fix needed.** Direct reading proved the R2007+
  string stream is fully wired on **both** read and write: `DRW_Entity::parseDwg`
  computes the offset version-correctly (RL for AC1021, MC for AC1024+),
  string-bearing entities/tables/dimensions route through `sBuf` on read, and
  the writer threads `m_objectStrings` → `finishObject` footer on write. Every
  `NULL`-strBuf arm (Point/Line/MLine/Underlay/Insert/Ole2Frame) was confirmed to
  read **no** inline string-stream strings; XRecord is self-consistent inline.
  The `drw_entities.cpp:6527` "TODO: AC1021+ strBuf routing" was **stale** (the
  routing is done at :6538) — removed it. No behavior change; AC1021 stays
  accepted on read (it works).
- **F04 — IMPLEMENTED & TESTED.** The header was the one section not threaded
  into the existing string-stream machinery. Fix landed:
  `DRW_Header::encodeDwg` gained a `strBuf` param and routes the R2007+ header
  strings (4 unknowns, MENU, DIMPOST, DIMAPOST, [DIMALTMZS/DIMMZS], HYPERLINKBASE,
  STYLESHEET, FINGERPRINTGUID, VERSIONGUID, PROJECTNAME) to it;
  `dwgWriter24::writeDwgHeader` appends `[strings][7-zero-bits + RS(strBytes*8+7)
  + present-bit]` and keeps `bitSize = 32 + dataBuf.size()*8` (now inclusive).
  New test `R2010 header string vars round-trip through the string stream` passes
  (MENU/PROJECTNAME survive an AC1024 write→read; empty before). Full suite:
  **451 cases / 7048 assertions, 0 failures** — the 135 AC1024/27/32 write tests
  still pass after the header-layout change.

The design rationale below remains accurate; the implementation matched it.

## Headline: both findings are NARROWER than originally filed

The review's single-pass findings overstated scope. Direct code reading shows the
**R2007+ string-stream machinery already exists in libdxfrw**:

- **Read offset computation is version-correct and already present.** In
  `DRW_Entity::parseDwg` ([drw_entities.cpp:1045-1080](../libraries/libdxfrw/src/drw_entities.cpp#L1045))
  and `DRW_TableEntry::parseDwg` ([drw_objects.cpp:805-838](../libraries/libdxfrw/src/drw_objects.cpp#L805)):
  - `version > AC1014 && version < AC1024` (i.e. R2000/R2004/**R2007**) → `objSize = getRawLong32()` (the RL bitsize). ✅ correct for AC1021.
  - `version > AC1021` (R2010+) → `objSize = size*8 − bs` (the MC handle-stream size). ✅
  - For `strBuf != NULL && version > AC1018` it runs the exact endbit walk
    (`moveBitPos(objSize-1)` → present bit → `moveBitPos(-17)` → `getRawShort16` →
    `0x8000`/`hiSize` continuation → `moveBitPos(-strDataSize-16)`).
  - `readDwgEntity` ([dwgreader.cpp:1099-1102](../libraries/libdxfrw/src/intern/dwgreader.cpp#L1099)) reads the MC `bs` **only** for `> AC1021`. ✅
- **String-bearing entities/tables already pass `strBuf`** via the idiom
  `dwgBuffer sBuff=*buf; dwgBuffer *sBuf=buf; if (version>AC1018) sBuf=&sBuff;`
  (Block, Text, Attrib, MText, Hatch, Dimension, all `DRW_TableEntry` subclasses).
- **The write side already emits string streams** for objects
  (`dwgWriter24::finishObject` [dwgwriter24.cpp:191](../libraries/libdxfrw/src/intern/dwgwriter24.cpp#L191))
  and CLASSES (`writeDwgClasses` [:81](../libraries/libdxfrw/src/intern/dwgwriter24.cpp#L81)).

So R2007 reading is **not** fundamentally broken — the smoke suite reads
`line_2007.dwg` (AC1021) fine. **F06 should be re-scoped** (and AC1021 should
**not** be rejected — that would remove working functionality). The only write
gap is the **header**, which is F04.

## The R2007+ string-stream mechanism (authoritative)

Each object/entity/header record splits its bit-stream into **data | string |
handle** sub-streams. TU (UTF-16LE) strings appear *inline in field order* but
are physically read from the string stream. Spec §20.1 (`/tmp/dwgspec.txt:4687-4700`):

**Read** (let `endbit` = handle-stream offset in bits; from the RL bitsize for
R2007, from `size*8 − MC` for R2010+):
1. present bit `B` = bit at `endbit−1`. If 0 → no strings, done.
2. `strDataSize` = RS at `endbit−1−16`. If `0x8000` set: read `hiSize` = RS 16
   bits lower, `strDataSize = (strDataSize & 0x7FFF) | (hiSize << 15)`.
3. string stream starts at `endbit − strDataSize`. `strDataSize` is in **bits**.

**Write** = inverse: emit data bits, append string bits, append the size short(s)
+ present bit so a reader's backward scan recovers them; set the bitsize field to
the handle-stream offset. LibreDWG's clearest reference is the header path
`encode.c:2028-2116` (layout `[main][string + RS(size) + B(endbit)][handle]`,
`bitsize = 32 + main_bits + str_bits + 16 + 1`).

**libdxfrw has two footer conventions — do not mix them:**
- **Per-object** (`finishObject`): after `alignToByte`, append string bytes, then
  `7 zero bits + RS(strDataSize) + 1 flag bit` (24 bits), with
  `strDataSize = strBytes*8 + 7`. The `+7` is tuned so the *reader's*
  `moveBitPos(-strDataSize-16)` lands byte-aligned (the reader uses **relative**
  `moveBitPos`).
- **Header** (`DRW_Header::parseDwg` [drw_header.cpp:2328-2372](../libraries/libdxfrw/src/drw_header.cpp#L2328))
  uses **absolute** `setPosition`/`setBitPos` with `strStartPos -= strDataSize`,
  so its `strDataSize` is the spec-literal bit count. **F04 must invert the
  header reader, not copy `finishObject` verbatim.**

## F06 — R2007/R2010+ entity/object string read (re-scoped)

**What already works (verified):** the offset computation (version-correct for
AC1021 vs AC1024+) and all entity/table types that route through `sBuf`.

**Residual gap:** `parseDwg` arms that pass `NULL`/`nullptr` for `strBuf` **but
contain inline TV/TU fields** — those strings get read from the data cursor at
the wrong offset under R2007+. The NULL-passing arms are enumerated in
`drw_entities.cpp` (Point/Line/Arc/Circle/Ellipse/… — most are genuinely
string-free, so NULL is correct). **Audit these specifically for hidden string
fields:** `DRW_XRecord` ([drw_objects.cpp:2975](../libraries/libdxfrw/src/drw_objects.cpp#L2975)),
`DRW_Underlay`, `DRW_MLine`, `DRW_Insert`, `DRW_Ole2Frame`, plus the explicit
`TODO: AC1021+ strBuf routing` at `drw_entities.cpp:6464`.

**Fix (per affected arm):** adopt the existing idiom —
```cpp
dwgBuffer sBuff = *buf;
dwgBuffer *sBuf = buf;
if (version > DRW::AC1018) sBuf = &sBuff;   // separate string cursor
... DRW_Entity::parseDwg(version, buf, sBuf, bs);   // base positions sBuf
... str = sBuf->getVariableText(version, false);    // read strings from sBuf
```
No new offset-computation code is needed; the base already positions `sBuf`.

**Severity reclassification:** from "High: R2007 read fundamentally mis-decodes"
→ **Medium: specific string fields in a few record types route to the wrong
stream under R2007+.** Do **not** reject AC1021 on read.

**Verify:** there are AC1021 fixtures (`line_2007.dwg`, `point*_2007.dwg`, … in
the smoke corpus; larger ones under `~/doc/dwg2/`). Add a read test that asserts
the *string* fields of an audited type (e.g. an XRECORD's group-1 strings, or an
entity carrying EED app names) decode correctly from an AC1021 file — failing
before, passing after.

## F04 — R2010+ header string write (the real, well-scoped gap)

**Current behavior:** `DRW_Header::encodeDwg`
([drw_header.cpp:2575](../libraries/libdxfrw/src/drw_header.cpp#L2575)) has
signature `(version, dwgBufferW *buf, dwgBufferW *hBbuf)` — **no string buffer** —
and emits MENU/DIMPOST/DIMAPOST/HYPERLINKBASE/STYLESHEET/FINGERPRINTGUID/
VERSIONGUID/PROJECTNAME inline only under `version < AC1021` (`putCP8Text`). For
AC1024/27/32 those strings are silently dropped, and `dwgWriter24::writeDwgHeader`
([dwgwriter24.cpp:37-67](../libraries/libdxfrw/src/intern/dwgwriter24.cpp#L37))
sets `bitSize = 32 + dataBuf.size()*8` with no string-stream footer → the reader's
backward scan lands in the handle bytes → corrupt header.

**Fix — two functions:**

1. **`DRW_Header::encodeDwg`** — add a string buffer parameter:
   `encodeDwg(version, dwgBufferW *buf, dwgBufferW *hBbuf, dwgBufferW *strBuf)`.
   For `version > AC1018`, route the string vars to `strBuf->putUCSText(...)` in
   the **exact order the reader reads them** (`drw_header.cpp:2353-2372`): the 4
   "unknown text" fields, MENU, DIMPOST, DIMAPOST, (DIMALTMZS, DIMMZS for R2010+),
   HYPERLINKBASE, STYLESHEET, FINGERPRINTGUID, VERSIONGUID, PROJECTNAME. Keep the
   `< AC1021` inline `putCP8Text` path unchanged. Update the decl
   ([drw_header.h:128](../libraries/libdxfrw/src/drw_header.h#L128)).

2. **`dwgWriter24::writeDwgHeader`** — allocate `dwgBufferW strBuf`, pass it to
   `encodeDwg`, then assemble `[dataBuf][string footer]` and recompute `bitSize`.
   The footer **must invert `DRW_Header::parseDwg`** (absolute positioning):
   - after `dataBuf.alignToByte()` append `strBuf` bytes, then write the present
     bit at `endBitPos−1`, `RS(strDataSize)` at `endBitPos−17`, where
     `strDataSize` (bits) = the value such that `endBitPos − strDataSize` lands at
     the first string bit — i.e. `strDataSize = strBufBits + (gap to present bit)`.
   - set the leading `RL bitSize` so the reader's
     `endBitPos = 160 (+32 if hSize) + bitSize` puts the present bit at the bit
     just written. Concretely `bitSize` must equal the bit offset of the
     handle-stream start measured from the section-size field.
   - empty case: no strings → present bit 0, no size short (mirror `finishObject`
     / the reader's `getBit()==0` early-out).

   **Recommended methodology:** treat the *existing header reader* as the spec to
   invert and drive the writer with a round-trip test (encode→decode→compare),
   exactly as the entity encoders were built. Cross-check the bit accounting
   against both `DRW_Header::parseDwg` (the inverse target) and `finishObject`
   (the proven sibling), and against LibreDWG `encode.c:2028`.

**Header strings are small** (< a few KB total) so `strDataSize < 0x8000` always
— the `hiSize`/`0x8000` continuation can be omitted on write (but assert it).

**Verify:** extend `dwg_header_encode_round_trip_tests.cpp` (currently AC1015/
AC1018 only) with AC1024/AC1027/AC1032 cases carrying non-empty MENU/PROJECTNAME/
GUIDs; assert they survive encode→decode. Then a full-file write→read of a 2010
file confirming header strings. (Interim honest-failure option — `return false`
for `>AC1018` — is still the right stopgap until this lands.)

## Gotchas (from LibreDWG, apply to both)

1. **UCS-2/UTF-16LE**, BS length is a **code-unit** count (×2 for bytes). Verify
   libdxfrw's `putUCSText`/`getUCSText` length convention matches on both ends.
2. **Trailing NUL**: LibreDWG counts the terminating 0 in the BS length. Confirm
   libdxfrw's `getUCSText(false)` (nullTerm=false) vs `putUCSText` agree, or every
   subsequent string in the stream shifts.
3. **Empty string → present bit 0, no size short.** Don't unconditionally
   write/read the 16-bit size.
4. **`0x8000` two-short order** (only if >32KB): hi-short first, lo-short carries
   the flag; reader walks backward. N/A for the header; relevant if ever applied
   to large object strings.
5. **`strDataSize` is a BIT count**, despite a misleading "in byte" comment in
   LibreDWG's `obj_string_stream`.
6. **R2010+ vs R2007**: identical string algorithm; differs only in how `endbit`
   is sourced (RL vs MC) and the header's extra 32-bit hSize (+191 vs +159 bit
   offset). libdxfrw already handles both.
7. The standalone `~/dev/libdxfrw` checkout is an **older copy without writers** —
   ignore it; the authoritative tree is `LibreCAD/libraries/libdxfrw`.

## Effort & sequencing

- **F06:** ~0.5–1d — audit the ~5 candidate arms, route the ones with real string
  fields, add an AC1021 string-read test. Low risk (follows an established idiom).
- **F04:** ~1–2d — signature change + `writeDwgHeader` assembly + bit-accounting,
  driven by a new AC1024+ header round-trip test. Medium risk (bit-exact footer).
- Both are **feature work**, separable from the B1–B5 batches. Until they land,
  the interim honest-failure stopgaps (reject `>AC1018` header *write*; the F06
  audit means no AC1021 *read* rejection is needed) avoid silent corruption.
