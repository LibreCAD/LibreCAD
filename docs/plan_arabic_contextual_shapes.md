<!--
********************************************************************************
This file is part of the LibreCAD project, a 2D CAD program

Copyright (C) 2026 LibreCAD.org
Copyright (C) 2026 Dongxu Li (github.com/dxli)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
USA.
********************************************************************************
-->

# Arabic Contextual Shaping Implementation Plan

Arabic text needs contextual glyph selection and font-defined positioning, not
just right-to-left ordering. Introduce an OpenType backend using HarfBuzz and
FreeType, shared by TEXT and MTEXT, while retaining the existing LFF backend for
unchanged legacy drawings. Keep editable Unicode in logical order and generate
positioned native CAD outlines from the shaped glyphs.

## Status and scope

| Field | Value |
| --- | --- |
| Architectural review baseline | `628a1ba845931a37b8d745a8ad08d4731615812c` |
| Workflow review checkout | `ea5696771e74c4f22ae73fc7b031149a1b69a305` |
| Review date | 2026-10-11 |
| State | Implementation in progress; baseline/probe and headless services under validation; entity rendering and release gates pending |
| Execution requirement | Intermediate scoped commits, explicit validation, plan updates, draft PR creation and revision-bound deep PR refinement are required; implementation is authorized by the active goal |
| Primary delivery | Opt-in, versioned OpenType font profile with correct ordinary Arabic shaping and common diacritics |
| Compatibility rule | No automatic reinterpretation of existing LFF styles, text order or RTL markers |
| Dependency decision | Recommend explicit HarfBuzz and FreeType core dependencies; confirm distribution support in P0 before committing to the backend |
| Validation completed | Pristine headless bidi baseline and durable pinned-font probe, including independent cmap and CoreText placement comparisons; not application rendering or export acceptance |

The first production delivery includes isolated, initial, medial and final
forms; required substitutions; lam–alef variants; ordinary vowel marks and
shadda combinations; cursive and mark attachment; tatweel; ZWJ/ZWNJ; mixed
Arabic, Latin, numbers and punctuation; and TEXT/MTEXT parity. Correct mark
placement is part of this delivery, not an optional later enhancement.

Do not advertise exhaustive support for every Arabic-script language. Arabic
with explicit language `ar` and the pinned Amiri face is the initial certified
profile. Extended Arabic letters, Persian and Urdu samples belong in coverage
tests, but language-specific typography requires its own acceptance corpus.
Defer automatic kashida justification, discretionary-feature UI, variable/color
fonts, vertical Arabic, advanced Quranic composition and Nastaliq certification.
Ordinary tatweel and combining marks are not deferred with those features.

Automatic wrapping and a full MTEXT paragraph editor remain separate work.
Preserve explicit line breaks and reference-width data; expose shaped advances
and safe break information so future wrapping can reshape at actual boundaries.
Coordinate shared metrics with the [MTEXT indentation plan](/Users/dli/dev/LibreCAD/docs/plan_mtext_indentation_3032.md)
without absorbing that entire project into Arabic support.

## Corrections to the earlier approach

| Earlier assumption | Revised requirement |
| --- | --- |
| Four presentation forms plus lam–alef provide complete Arabic support | They are a limited fallback, not complete Amiri shaping; use the font's GSUB, GPOS and GDEF tables |
| Existing Amiri LFF outlines can be supplemented with advances | Conversion discarded horizontal origins and excluded uncoded glyphs; use the original pinned OpenType bytes |
| Combining marks can stay at a grapheme origin initially | Common marks need font-defined offsets and mark-to-mark positioning from the first production release |
| Visual bidi clusters can be passed directly to a shaper | Shape logical directional runs, then perform visual run placement once |
| A glyph can still be identified by a Unicode string | OpenType output uses font-scoped glyph IDs, many of which have no Unicode encoding |
| The existing export and explode paths will inherit correctness | Explicit persistence, embedded ATTRIB and both explode-command integrations are required |
| Switching the existing Amiri name enables an upgrade safely | New geometry and metrics need an explicit profile; old drawings retain their old backend |

## Verified repository constraints

Function names are the durable anchors below; line numbers describe the reviewed
architectural baseline. The workflow review also accounts for indentation commit
`567799327` and its progress record `ea5696771`: settings isolation, whitespace
and parts of persistence now have adjacent implementation. P0 must inherit and
revalidate those changes rather than reimplement them or restore the old behavior.
The earlier architectural anchors are not a claim that every issue remains
unfixed at the workflow review checkout.

| Area | Evidence | Design consequence |
| --- | --- | --- |
| Bidi is already separate from shaping | [qtbidi documentation](/Users/dli/dev/LibreCAD/libraries/qtbidi/README.librecad:38), [paragraph levels API](/Users/dli/dev/LibreCAD/libraries/qtbidi/qt_bidi.h:35) | Reuse the resolver; expose shaping levels/control boundaries separately from final display levels |
| TEXT emits Unicode-keyed inserts and advances from ink bounds | [RS_Text::update](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/rs_text.cpp:276) | The new backend needs positioned glyph records and authoritative advances |
| MTEXT emits visual grapheme segments with spacing heuristics | [flushBidiLine](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/rs_mtext.cpp:468), [addLetter](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/rs_mtext.cpp:641) | Replace only the new profile's emission path; retain legacy layout unchanged |
| MTEXT parser lacks raw-source ranges and consumes UTF-16 units | [RS_MText::update](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/rs_mtext.cpp:310) | Introduce token/span and scalar-to-UTF-16 mappings before shaping |
| LFF glyphs are Unicode-keyed geometry without per-glyph layout metrics | [RS_Font::readLFF](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/fonts/rs_font.cpp:346), [RS_FontChar](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/fonts/rs_fontchar.h:37) | Do not put OpenType GIDs into LFF character keys or PUA text |
| Converter enumerates cmap and removes each glyph's x-min | [cmap traversal](/Users/dli/dev/LibreCAD/tools/ttf2lff/main.cpp:1500), [outline normalization](/Users/dli/dev/LibreCAD/tools/ttf2lff/main.cpp:509) | Regenerating the current LFF format cannot preserve complete shaping data |
| Unknown fonts silently fall back to standard | [RS_FontList::requestFont](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/fonts/rs_fontlist.cpp:95) | The OpenType resolver must return explicit capability and substitution diagnostics |
| Exported bidi levels already include L1 and BN adjustments | [QBidiAlgorithm::process](/Users/dli/dev/LibreCAD/libraries/qtbidi/qt_bidi.cpp:946) | The existing final level array alone is not a correct shaping-run API |
| Arabic locale currently selects legacy Amiri | [getDefaultFont](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/fonts/rs_fontlist.cpp:133) | Do not change existing styles or use the system locale as the shaping language |
| MTEXT RTL persistence has two existing modes | [import](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/rs_filterdxfrw.cpp:8119), [writeMText](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/rs_filterdxfrw.cpp:30874) | Preserve `LibreCad` 1071 values 1 and 2 with their current meanings |
| TEXT forced direction is session-only | [RS_TextData](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/rs_text.h:70) | New-profile direction overrides require explicit persistence; legacy behavior stays unchanged |
| Generic MTEXT decoding flattens scoped properties | [toNativeString](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/rs_filterdxfrw.cpp:32833) | Capture and tokenize supported source formatting before destructive flattening |
| Embedded MTEXT has its own writer and sidecar replay | [writeInsert](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/rs_filterdxfrw.cpp:30598) | Standalone MTEXT tests cannot establish attribute edit/save correctness |
| Explode into letters reconstructs text from insert names | [explodeTextIntoLetters](/Users/dli/dev/LibreCAD/librecad/src/lib/modification/rs_modification.cpp:2864) | Freeze placed glyph/cluster geometry instead of reshaping opaque glyph keys |
| Current supported Qt baseline is 6.4 | [README](/Users/dli/dev/LibreCAD/README.md:18) | Do not depend silently on newer public glyph-to-string APIs |

## Font evidence and backend decision

### Pinned font probe

The [official Amiri 1.003 archive](https://github.com/aliftype/amiri/releases/download/1.003/Amiri-1.003.zip)
contains the candidate `Amiri-Regular.ttf`, face index 0, UPEM 1000. Its SHA-256
is `cd2550c0f4c05eb341bf97958211aaa39382bca96577ba3a67d4a3b4912c43c0`.
The archive SHA-256 is
`81af0aff7d2086d8af24cea7202f7546130997982534691373485cd96744d05e`.
The selected Unicode cmap reaches 1,622 of the face's 6,710 glyphs.

A standalone probe used HarfBuzz 14.6.0 with its `ot` shaper and FreeType 2.14.3.
It shaped all 21,952 ordered triples of this 28-letter set, in logical order,
with RTL direction, Arabic script, language `ar`, default features and the
default grapheme-monotone cluster policy:

```text
0627 0628 062A 062B 062C 062D 062E 062F 0630 0631 0632 0633 0634 0635
0636 0637 0638 0639 063A 0641 0642 0643 0644 0645 0646 0647 0648 064A
```

| Observation | Number of triples |
| --- | ---: |
| At least one output glyph has no entry in the selected Unicode cmap | 20,944 |
| At least one nonzero positioning offset | 671 |
| Output changes when only `curs` is disabled | 418 |
| Output changes when only `rlig` is disabled | 8,393 |

Feature comparisons checked glyph IDs, clusters, both advances and both offsets.
Across these triples, the default output used 415 distinct glyph IDs, including
387 not reachable through that cmap. This is a controlled font experiment, not
a frequency estimate for Arabic documents.

The reproducible probe corrected a temporary-audit lookup bug on 2026-10-11:
printing sample glyphs used `std::map::operator[]`, inserting 25 uncoded keys
before the triple loop. Eighteen occur in that corpus, causing 125 triples to
be misclassified. An independent SFNT cmap reader plus `hb-shape` reproduced
the corrected counts and the old erroneous 20,819/369 counts. Font bytes,
placements and feature-sensitivity counts are unchanged.

| Logical input | Output GIDs in HarfBuzz order | X advances in font units | Significant positioning |
| --- | --- | --- | --- |
| `ببب` | 1589, 3999, 3958 | 883, 244, 233 | All three GIDs are uncoded |
| `لا` | 2518, 2513 | 340, 302 | Two uncoded components, not the U+FEFB cmap glyph 1059 |
| `لَا` | 2518, 94, 2513 | 340, 0, 302 | Fatha offset is (-76, 0) |
| U+0628 U+064E U+0651, or U+0628 U+0651 U+064E | 1439, 97, 56 | 0, 0, 926 | Mark offsets are (453, 51) and (253, 0); GID 1439 is uncoded |

The CoreText backend comparison on the audit machine matched output GIDs,
absolute glyph origins and total run advance for all triples and 15 additional
mark/control/ligature examples. Per-glyph advance allocation can differ between
engines, so independent comparisons must use absolute placements and total
advance, not demand identical internal bookkeeping. These results do not test
FreeType outline conversion or LibreCAD's geometry, bidi integration or exports.
The reproducible [audit probe](/Users/dli/dev/LibreCAD/tools/arabic-font-probe/README.md)
is now retained in the repository. Its checks are not application/backend
acceptance tests; P0 still needs the remaining durable acceptance fixtures.

Amiri's [versioned release history](https://github.com/aliftype/amiri/blob/1.003/NEWS.md)
also documents required contextual substitutions and positioning fixes. The
evidence rules out presenting a four-form lookup as complete Amiri support.

### Primary backend

Use HarfBuzz's OpenType font functions for design-unit shaping and FreeType for
outlines from the same immutable font bytes and face index. Set the shaping
scale explicitly to UPEM; do not obtain positions through hinted screen-size
metrics. Check shaping failure instead of accepting a fallback shaper silently.
Keep the existing LFF renderer as a separate backend behind the same layout
interface. Do not extend the LFF file format as a prerequisite for this route.

Public `QTextLayout` plus `QGlyphRun` and `QRawFont::pathForGlyph()` is a valid
alternative if dependency policy rejects the proposed backend. It reduces new
library deployment, but needs GUI/font-database facilities and controlled font
fallback. Public `QGlyphRun::stringIndexes()` is available only from Qt 6.5,
above the documented baseline. `QRawFont::glyphIndexesForString()` alone is not
full shaping. Revisit this alternative in P0 only with an explicit baseline or
source-mapping decision; do not use Qt private APIs.
[Qt glyph source indexes](https://doc.qt.io/qt-6.5/qglyphrun.html#stringIndexes),
[Qt raw-font limitations](https://doc.qt.io/qt-6/qrawfont.html)

## Layout and geometry contracts

### Shared model

Introduce a shared layout service, provisionally `LC_TextShaper` with an
OpenType font-resource provider and a native-outline cache. Keep parsing,
shaping and entity transforms separable and testable without a GUI application.
The exact class split may change; the following data contracts must not.

| Record | Required contents |
| --- | --- |
| Logical paragraph | Original payload identity; parsed logical text; UTF-16 source ranges; formatting spans; inline objects; effective base direction and language |
| Font resource | Profile/version; font checksum; face index; UPEM; fixed design-height and baseline metrics; capability/error status |
| Logical run | Source interval; resolved bidi level; script; language; effective font and scale; bounded pre/post shaping context |
| Positioned glyph | Font-scoped GID; shaping cluster; mapped source range or ranges; x/y advance; x/y offset; drawable/empty status |
| Layout result | Visual run placement; independent logical advances and ink bounds; baseline/line metrics; source map; diagnostics |

Do not treat a Unicode scalar, UTF-16 unit, grapheme, shaping cluster and glyph
as interchangeable. A ligature may span characters, and one character can
produce multiple glyphs. Cluster mapping is many-to-many provenance, not exact
per-character ownership or a complete set of editor caret positions.

### Processing order

1. Parse TEXT or supported MTEXT syntax into logical paragraphs and attribute
   spans while retaining mappings to original source. Decode scalars without
   splitting surrogate pairs. Represent stacks as inline objects, not letters.
2. Resolve paragraph bidi levels using the existing resolver and projection
   policy. Expose post-I2, pre-L1 levels for retained characters plus X9/control
   and isolate-boundary metadata. Keep the current final display-level API for
   legacy callers and retain the Chinese explicit-RTL projection's source map.
3. Itemize logical runs from that shaping information by level, script,
   effective font, scale and language. X9-excluded BN characters such as ZWJ/ZWNJ
   retain their logical adjacency for shaping; their artificial display levels
   must not create a joining boundary. Resolve Common/Inherited characters
   deliberately and keep Arabic marks associated with their base.
4. Shape each logical run with explicit direction, script and language. Supply
   appropriate context without crossing paragraph, bidi-level or isolate
   boundaries. Do not pass the output of `visualClusters()` as shaping input.
5. Apply L1 and visual reordering to each actual line, separately from shaping
   itemization. Determine run placement with source mappings for removed or
   zero-ink controls. Consume each HarfBuzz buffer in its returned order; do not
   reverse RTL glyphs a second time. Handle mirroring once in the shaping/backend
   path, not also through the existing scalar-mirroring loop. Keep the legacy
   path's behavior isolated.
6. Accumulate advances, add each glyph's offset, obtain its outline, then apply
   font normalization, entity width/height, alignment, rotation and insertion.
   Calculate transformed ink bounds from actual geometry separately.

This follows shaping after bidi level resolution and before final display
placement, rather than reshaping visually reordered characters. New line
wrapping must also apply line-level bidi rules and reshape at breaks, not cut
an already-shaped glyph array. Keep joining context inside the applicable level
run, including the special boundary effects of isolates.
[Unicode 17 UAX 9 shaping rules](https://www.unicode.org/reports/tr9/tr9-51.html#Shaping)

Do not obtain the new shaping runs by grouping `resolve().levels`: the current
implementation applies L1 and rewrites BN levels for display before exporting
that array. A pre-L1 snapshot alone also does not define the treatment of
X9-excluded joining controls. Include an LTR-paragraph regression containing
Arabic beh followed by trailing ZWJ: the ZWJ may receive the base display level,
but must still affect the beh's contextual form. Isolates, including empty
isolates, terminate joining context even where adjacent text levels match.

Use `hb_buffer_add_utf16()` or equivalent explicit cluster assignments, with a
documented UTF-16 index convention and real pre/post context. Set beginning/end
flags only at real applicable boundaries, not at every formatting token. Use
`HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS` for the production source map;
derive logical source intervals from cluster boundaries and retain a separate
grapheme map for editing/fallback. Never infer cluster length from adjacent
visual glyphs in descending RTL order.
[HarfBuzz contextual input](https://harfbuzz.github.io/adding-text-to-the-buffer.html),
[HarfBuzz cluster semantics](https://harfbuzz.github.io/working-with-harfbuzz-clusters.html)

### Controls and formatting

Keep ZWJ, ZWNJ and bidi controls in canonical text. Let the resolver and shaper
process their respective effects before suppressing zero-ink output. Do not
reuse the blanket `Other_Format`/`Other_Control` filter on shaping input. Keep
NBSP distinct from ordinary space and give spaces the selected backend's real
advance even when there is no outline. Treat Tab as a layout field boundary
measured using shaped advances; coordinate its placement with indentation work.
Explicit newlines and MTEXT paragraph breaks end joining context.

Do not rewrite Arabic into presentation-form code points or globally normalize
saved documents. Shaper-internal canonical processing is ephemeral and retains
source provenance. Existing presentation-form input remains unchanged in the
backing store; any compatibility conversion requires an explicit, undoable
action. Ill-formed UTF-16 must produce a deterministic replacement in layout
with a mapping to the original unit, not a crash or silent source edit.
[Unicode Arabic encoding and rendering](https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-9/)

Paint-only spans must not unnecessarily break shaping. For a ligature crossing
paint spans, initially paint the resulting indivisible glyph with the first
logical component's effective pen and retain all source ranges; document and
test this policy. Do not disable required substitutions to simulate per-letter
color. Font or size changes are real shaping boundaries: provide surrounding
joining context, but do not form a ligature across different font faces or
incompatible scales. Each stack child shapes recursively with its effective
inline font; the parent treats the stack as an object boundary.

Keep default Arabic OpenType behavior, including required substitutions and
font-supplied cursive, mark and mark-to-mark positioning. Do not independently
implement lam–alef formation or guess ligature-component anchors in this backend.
Use explicit `ar`, not the machine's UI locale; future language overrides must
be persisted. Preserve optional-feature defaults in the versioned profile.
[Arabic OpenType substitutions and positioning](https://learn.microsoft.com/en-us/typography/script-development/arabic)

### Metrics and native outlines

HarfBuzz advances and offsets are authoritative. Never add legacy Amiri
`LetterSpacing: 3` between joined glyphs, use ink width as advance, apply MTEXT's
width-modulo heuristic, or replace space advances with legacy `WordSpacing`.
Tracking remains zero for the initial OpenType profile; any later control needs
a cursive-safe policy rather than arbitrary per-glyph gaps.

Preserve the original font coordinate origin, negative bearings and overhangs.
Use one fixed design-height per profile to map into the existing nominal
9-unit CAD cell, then scale by entity height/9. For the Amiri profile, use the
pinned face's U+0041 outline maximum Y of 647, giving an exact normalization
factor of 9/647. This matches the converter's height convention, not the A's
650-unit ink height: its exact outline bounds are (-14, -3, 618, 647).
The face ascender, descender and height are 1124, -634 and 1758 font units.
Freeze these measured values and their distinct meanings in P0 fixtures.
Do not normalize each glyph or word to its bounding box. This does not promise
metric compatibility with the unversioned 2015 LFF conversion.

Keep baseline, ascender/descender, nominal cell, line pitch, logical advance and
ink bounds separate. Define Exact line spacing independently from mark height;
AtLeast spacing must account for the profile's metrics and actual line extents.
Freeze top/middle/bottom/baseline behavior in the alignment fixtures before
reusing existing transforms. For the new backend, ordinary left/center/right
horizontal attachment uses the logical advance interval, including whitespace;
visual-middle modes use an explicit ink-center rule. Aligned/Fit modes measure
their two anchors against logical width and guard zero-width/no-ink cases.
Legacy alignment remains unchanged. Do not redefine `getUsedTextWidth()`
globally: expose new metrics separately and audit its dimension/tolerance and
export consumers before changing any public meaning.

Load outlines by GID from the exact face, without hinting or bitmap substitution,
and allow FreeType to resolve composite glyphs. Decompose all contours, including
quadratic/cubic segments and contour closure; copy data out of FreeType's mutable
glyph slot. Convert to supported native curves or adaptively flatten with a
stated error bound in document units. The converter's fixed four samples are
not a geometry-accuracy contract. Test the approximation after extreme entity
scales, not only at nominal font size.
[FreeType glyph loading](https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html),
[FreeType outline decomposition](https://freetype.org/freetype2/docs/reference/ft2-outline_processing.html)

The initial backend produces stroked contour geometry, consistent with the
existing outline-derived Amiri LFF appearance; it does not introduce filled
glyph rendering. Holes and contour direction still need faithful outlines.
Use native entities for screen display, selection, bounds, print/PDF, geometry
export and explosion. A painter-only raster or Qt-text overlay is insufficient.
Whitespace-only and controls-only text may have advance but no ink bounds;
preserve the engine's no-extent sentinel and guard all bounds consumers.

### Caches and failure behavior

Cache immutable source-font resources and exact design-space contour commands
by checksum, face index, GID and variation coordinates. Flatten after the
effective entity transform, or include the derived design-space error tolerance
and effective transform scale in the tessellation cache key. A cached nominal
polyline does not guarantee a document-unit error bound at arbitrary scale.
Cache shaped layouts separately using logical text, source-span attributes,
context, direction, script, language,
features, profile version and shaping-engine identity. Never key by GID alone
or mutate shared contours when transforming an entity. Keep device-independent
design-unit data; DPI changes must not reshape geometry.

Invalidate affected layouts on text/font/profile/format changes and undo/redo.
Insertion and rotation may reuse design-space results. Bound cache memory and
input/glyph/contour complexity; use checked arithmetic and deterministic error
results rather than truncating saved text. Use RAII ownership and either
thread-confined FreeType faces or explicit synchronization. Do not share mutable
font slots or buffers across threads. Font upgrade invalidates both caches.

Start with the bundled static Amiri face. Resolve missing coverage explicitly:
choose an approved fallback for a complete joining segment and base-plus-mark
cluster, then reshape with context; never substitute arbitrary per-codepoint
outlines into an already-shaped Arabic run. If no safe candidate exists, render
a deterministic missing-glyph indication and report the unsupported range.
Do not silently switch a missing OpenType profile to the `standard` LFF font.
Loading arbitrary installed, variable or color fonts is not needed for the
initial certified profile and must not create an uncontrolled fallback path.

## Profiles and saved-file compatibility

### Activation and migration

Introduce an explicit profile such as `lc-amiri-1.003-ot-v1`, resolving to the
pinned bundled face. Keep `amiri-regular` mapped to its current LFF. A style name
is a display identity, not permission to change every old entity using it.
Select the backend from versioned profile data and a deterministic registry;
do not infer it from Arabic characters, locale or an ambiguous font basename.

Offer the new profile in font selection with an engine-rendered preview. Change
the default for newly created Arabic-locale text only after release and PR
refinement gates pass and an explicit rollout decision is recorded;
never rewrite document styles on open. An explicit conversion command can
preview changed width/height and offer undo, but must not guess whether legacy
text is logically or visually ordered. Preserve marker 1 legacy reversal and
marker 2 logical RTL; never combine legacy reversal with modern shaping.
Changing font/backend and migrating source order are separate actions.

Convert selected entities using an entity override or a cloned STYLE; do not
attach a new profile to a shared style and change unselected references. If
marker 1 and an OpenType profile conflict, preserve both source and metadata,
diagnose the conflict, and retain legacy rendering when its resources are
available. Do not shape reversed source as modern logical text until the user
explicitly migrates order. For nonconflicting modern text, an explicit entity
direction override wins; otherwise marker 2 supplies RTL, followed by the
effective style/profile default and then Auto. Preserve this precedence in
clone, import and conversion tests.

### Persistence design and proof gate

Keep standard STYLE family/file information usable by other CAD applications.
Prototype compact, namespaced STYLE XDATA for the default rendering profile,
with separately versioned entity overrides where needed. Persist profile/schema
version, backend identity, source-font checksum/face, metric convention and
shaping policy/engine compatibility identity; persist effective language and
base-direction overrides at the appropriate style/entity level. Store no
absolute font paths, font binaries, transient GIDs or shaped glyph arrays.
Recompute geometry from canonical text.

Use a registered application name, provisionally `LibreCADText`, with 1001 APPID,
1000 string fields and 1070 version/enum fields. Respect DXF string/EED limits
and keep unknown versions opaque and recoverable. Do not copy MTEXT's 1071
marker scheme to STYLE: [DRW_Textstyle::parseCode](/Users/dli/dev/LibreCAD/libraries/libdxfrw/src/drw_objects.cpp:4300)
currently consumes 1071 as `fontFamily`. Fixing that parser is a separate
option, not an assumed capability of this schema.

STYLE XDATA is a plausible transport, not yet proof of this profile. The DXF
writer emits STYLE `extData`; the DWG table path also transports EED. The current
custom group-102 application-data rejection does not reject ordinary XDATA.
Start from existing [DXF XDATA tests](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/tests/dxf_object_tests.cpp:9222)
and [DWG table EED fixtures](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/tests/dwg_write_smoke_tests.cpp:25197).
P0 must prove filter-level DXF and DWG transport before freezing this schema.

Establish these rules before enabling saved new-profile drawings:

- Preserve unrelated STYLE fields, application data and source style names.
  Enumerate effective inline fonts as well as parent text styles on export.
- Retained raw metadata is authoritative only for unmodeled fields. Current
  edited text/font/height/direction/profile fields override their stale sidecars.
  No-op saves retain untouched metadata; edits cannot replay old text.
- Cover standalone TEXT/MTEXT and embedded ATTRIB independently. Preserve outer
  plain attribute fallback, embedded MTEXT authority, hidden state, tags,
  ownership and ordering. A no-op retains the original outer fallback; an
  edited embedded value regenerates its plain fallback unless an independently
  modeled outer override applies. Refresh embedded direction/profile XDATA
  from current data after sidecar replay. ATTDEF semantic restoration is
  separate work.
- Preserve raw MTEXT spans before the generic decoder removes supported scopes.
  Layout never mutates canonical text. Save/edit handling must retain unsupported
  properties without leaking their state into supported spans.
- Unknown profile versions, absent fonts and checksum mismatches preserve source
  data and expose a visible diagnostic. Offer an explicit substitute or geometry
  export only where appropriate: appearance-preserving outlines require exact
  resources or verified retained native geometry. If neither exists, preserve
  source and report that appearance cannot be reproduced. Do not present a
  substitute's geometry as equivalent.
- New-profile forced TEXT direction must round-trip through its override schema.
  Do not imply that the existing session-only direction behavior was already
  persistent. Resolve ByStyle from the effective style, not a hard-coded LTR.
- Test older LibreCAD and an independent CAD reader. Standard Unicode plus the
  matching installed font is best-effort interchange, not guaranteed identical
  shaping. Explicit outline export is the appearance-preserving alternative.

Test DXF→DXF, DWG→DWG and DXF↔DWG for all supported output families, including
new styles, imported styles, multiple profiles, blocks, attributes and dimensions.
Check APPID resolution, decoded values and standard STYLE fields, not merely a
self-round-trip screenshot. If compact XDATA proves insufficient, decide on a
document registry/extension record in P0 and update the plan before proceeding.

## Entity and editor integration

TEXT and MTEXT must consume one layout contract. Preserve their canonical data,
user-defined anchors, width factors, rotation and pens. MTEXT's effective
inline-font state must reach each shaping run and stack child; correct the
current stack-child use of parent style where it discards an active inline font.
Repeated `update()`
must be idempotent and must not change raw text or source order. Preserve both
user-defined Aligned/Fit anchors; in ordinary TEXT modes, recomputed derived
second anchors must stabilize rather than being treated as immutable input.

Integrate dimensions, arc dimensions, tolerances, TABLE fallback and
single/multiline attributes as consumers, not new shaping implementations.
Propagate the owning annotation/cell's effective STYLE and profile explicitly:
[arc-dimension labels](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/lc_dimarc.cpp:227)
currently hard-code `unicode`, while [TABLE fallback](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/rs_filterdxfrw.cpp:7968)
uses the drawing default rather than resolving cell style handles. Define
override/inheritance authority and retain dedicated symbol fonts where needed.
If these propagation paths remain unsupported, narrow the certified consumer
list explicitly; they do not inherit support automatically. Test modified text
width effects on leaders and dimension placement. Characterize imported
backward/upside-down and combined text-generation flags separately; glyph
joining direction is not geometric reflection. Do not claim previously missing
flag or ATTDEF semantics are fixed merely by adding Arabic shaping.

MLeader requires particular handling: [drawTextContent](/Users/dli/dev/LibreCAD/librecad/src/lib/engine/document/entities/lc_mleader.cpp:145)
currently creates transient MTEXT per paint, and its bounds omit the label.
Retain privately owned cached layout/native geometry: MLeader is an atomic
entity, not an entity container. Do not register a document sibling or export
duplicate text. Define clone/copy/undo ownership, parent-font context and
geometry-export access. Include label extents in selection, clipping and print
bounds for the new backend; test context/rotation/style/resource changes and
invalidate correctly.

Normal explode should clone the already-positioned native contours. For
Explode into letters, use appearance-preserving positioned geometry groups.
Coarsen shaping-cluster provenance with Unicode grapheme/base–mark membership
and relevant ligature/attachment ownership, so marks and ligature components
freeze together. `MONOTONE_CHARACTERS` clusters alone are not letter boundaries
and can separate a mark from its base. The initial implementation should choose
these grouped outlines and explain that ligatures are not independently
editable letters. Do not reconstruct TEXT from GIDs, emit private glyph keys
as Unicode, or reshape an individual contextual glyph in isolation. Keep source text in undo/history or
optional provenance metadata, not as the visible group/block name.

Keep native Qt editing for ordinary Arabic in `LC_TextEdit` and the single-line
editor. Do not insert shaped characters into their documents or reuse the
Chinese projection where it is unnecessary. Test cursor/selection around
ligatures and marks, Arabic IME preedit, clipboard, undo/redo and invisible-control
edits in both dialogs and property panels. Qt's editor font is not an exact
preview of CAD contours; provide a preview generated by the shared backend when
showing actual profile appearance. Preserve Chinese RTL regression coverage.

## Build and distribution requirements

Wire sources and dependencies into both [CMake](/Users/dli/dev/LibreCAD/CMakeLists.txt)
and [qmake](/Users/dli/dev/LibreCAD/librecad/src/src.pro) in the same change.
Qt's internal HarfBuzz is not a supported public link target. Existing optional
FreeType detection for `ttf2lff` does not supply a core runtime dependency.
Choose tested minimum versions and static/dynamic policy in P0; the audit's
HarfBuzz 14.6.0 and FreeType 2.14.3 are reference versions, not automatically
the required deployment minimums.

Ship the exact static font, its copyright notices and [OFL license](https://github.com/aliftype/amiri/blob/1.003/OFL.txt)
with an upstream/version/checksum manifest. Do not relabel the existing 2015 LFF
as a conversion of this face. Treat font, shaping-engine and shaping-configuration
upgrades as potential layout changes. Geometry-changing updates require a new
profile version and explicit activation/migration; old profile behavior must
remain reproducible or raise an unavailable-profile diagnostic. A newer engine
may serve an older profile only under an explicit tested compatibility policy.
Updating fixtures alone must not silently redefine saved `v1` drawings.
Include dependency licenses and check all distributed configurations against
project licensing policy.

Resource installation must be verified in the final package, not only a source
checkout. CMake copies font directories, but current postprocessors select LFF
files in [Windows](/Users/dli/dev/LibreCAD/scripts/postprocess-win.bat:24),
[Unix](/Users/dli/dev/LibreCAD/scripts/postprocess-unix.sh:28) and
[macOS](/Users/dli/dev/LibreCAD/scripts/postprocess-osx.sh:44);
the [NSIS installer](/Users/dli/dev/LibreCAD/scripts/postprocess-windows/nsis-msvc.nsi:166)
also needs inspection/update. Cover actual Windows toolchains/architectures,
macOS universal binaries and Linux packages/AppImages used by CI. Check library
discovery, runtime loader paths, font lookup without a developer prefix, license
files and installed application smoke tests. The converter's minimal FreeType
build script alone is not a core runtime deployment solution.

Production builds should require the chosen backend dependencies when the
feature is enabled. An explicitly configured legacy-only build may remain
available, but must preserve new-profile metadata and clearly diagnose unsupported
rendering. Do not silently compile out shaping in a normal release package.

## Implementation workflow

### Validate and commit each bounded slice

Use intermediate commits throughout implementation, not one final commit for
the entire backend. Each slice must be independently reviewable, buildable and
covered by its regression tests. Keep unfinished functionality behind the
opt-in backend gate. Do not commit a failing-test-only change to the target
branch or call feature-gated code release-complete.

1. Inspect HEAD, branch, working-tree status and the staged diff before starting.
   Record the implementation baseline and existing failures. Preserve unrelated
   tracked/untracked files, staged changes and submodule state. If edits overlap,
   isolate the work or resolve ownership before proceeding; never reset, discard
   or include another task's work implicitly.
2. Select the next commit-sized slice and its acceptance checks from the ladder
   below. Set it to In progress and identify dependencies. Split a large slice
   further when implementation reveals another independently testable boundary.
3. Implement the slice with its tests. Add source/dependency/resource registration
   to both build systems in the same commit where applicable. Reuse configured
   out-of-source builds; do not overwrite another task's build or generate
   project files in the source checkout.
4. Run the required checks against the candidate contents and record evidence.
   Fix newly introduced failures before committing. Distinguish pre-existing
   failures from regressions with evidence; an uncertain attribution is not a
   passing gate. If unrelated source edits affect the tested build, use isolated
   inputs or label the result mixed-tree validation, not proof of the scoped commit.
5. Update this plan with results, decisions, remaining gates and the next slice.
   Stage only owned paths/hunks, including the plan update. Inspect the staged
   diff and run `git diff --cached --check`; also check unstaged owned edits and
   new files. Do not use blanket staging or commit pre-existing staged work.
6. Create the focused intermediate commit, verify its contents and resolve its
   hash. Report the checkpoint and next action. During an authorized PR workflow,
   push only the owned implementation branch as described in P7. This plan-editing
   request alone did not authorize publication; resolve publication authority
   from the active implementation request before P7a. History rewriting and
   unrelated cleanup require separate authority.

If a required check cannot run, record why and keep that gate Pending or Blocked;
do not substitute compilation for rendering, a self-round trip for independent
format proof, or skipped tests for a pass. Independently validated work may be
checkpointed with the limitation recorded, but its milestone remains incomplete.
After a failed check, preserve the last passing checkpoint, diagnose the failure
and rerun affected checks after the fix. Do not weaken expectations, silently
regenerate golden files or reinterpret a saved profile to obtain green tests.

### Keep the plan current

The implementing agent must update this file after each slice, validation
failure, dependency/schema decision, PR refinement round and material scope
change, and before every handoff. This is an in-task requirement, not a scheduled
automation. Include the update with the corresponding implementation commit
whenever possible.

Use Planned, In progress, Validated, Blocked and Complete consistently.
Validated means the recorded slice checks passed on the stated candidate;
Complete additionally requires a scoped commit and all required acceptance
gates. A phase is complete only when all its slices and phase gates are complete.
Compilation alone, opt-in code or a commit hash alone cannot establish completion.
The release checklist remains unchecked until its own evidence exists.

Maintain the progress and validation records below as implementation proceeds:

| Date | Phase and slice | State | Commit reference | Validation evidence | Remaining gate and next action |
| --- | --- | --- | --- | --- | --- |
| 2026-10-11 | P0a.1 | Validated | Candidate subject `Pin Amiri font and reproduce Arabic shaping audit`; baseline `ea5696771` | Pristine HEAD headless bidi: 1,688 assertions / 16 cases; durable probe: 21,952 triples + 15 CoreText samples, maximum placement error 0; independent cmap audit passes | Create scoped checkpoint; P0a CAD/performance fixtures and P0b transport/decision gates remain pending |
| 2026-10-11 | P1a, P1b, P2a, P2b, P3a, P3b | In progress | Uncommitted owned service slices | Dependency/resource, font/shaper, parser and shaping-bidi work; individual results recorded before checkpoints | Validate and checkpoint bounded slices; adapters/persistence/P6/P7 remain pending |

For every executed check, record its exact command and filters, exit status,
result/test count, tested revision and candidate-tree state, build configuration,
platform/toolchain/Qt/font/shaper versions where relevant, corpus identity and
durable log or fixture reference. Record exclusions and unavailable lanes
explicitly. Keep generated build output and large logs out of Git unless they
are intentionally selected fixtures; retain concise results in the plan.

### Initial baseline and font evidence

P0a.1 validates only the baseline and pinned resource experiment, not the entire
P0 phase. The font bytes, license and manifest are checkpointed with the probe
so it is reproducible without temporary audit files. P1a still owns dependency
wiring, installation and runtime lookup. The reference environment is macOS
arm64, Apple Clang 21.0.0, Qt 6.11.2, Catch2 3.16.1, HarfBuzz 14.6.0 and
FreeType 2.14.3. Minimum supported versions and other platforms are not validated
by these runs.

The pristine baseline was exported from
`ea5696771e74c4f22ae73fc7b031149a1b69a305` into a temporary snapshot, compiled
with only that snapshot's qtbidi/textbidi sources and installed QtCore/Catch2,
and run without GUI initialization. All 1,688 assertions in 16 cases passed,
exit 0. This is distinct from service tests compiled from the changing worktree.
The reproducible command pattern is:

```sh
ARABIC_BASELINE_DIR=$(mktemp -d)
git archive ea5696771e74c4f22ae73fc7b031149a1b69a305 libraries/qtbidi/qt_bidi.cpp libraries/qtbidi/qt_bidi.h librecad/src/lib/engine/document/entities/lc_textbidi.cpp librecad/src/lib/engine/document/entities/lc_textbidi.h librecad/src/lib/engine/document/entities/tests/lc_textbidi_tests.cpp | tar -xf - -C "$ARABIC_BASELINE_DIR"
c++ -std=c++17 "$ARABIC_BASELINE_DIR/libraries/qtbidi/qt_bidi.cpp" "$ARABIC_BASELINE_DIR/librecad/src/lib/engine/document/entities/lc_textbidi.cpp" "$ARABIC_BASELINE_DIR/librecad/src/lib/engine/document/entities/tests/lc_textbidi_tests.cpp" -I"$ARABIC_BASELINE_DIR/libraries/qtbidi" -I"$ARABIC_BASELINE_DIR/librecad/src/lib/engine/document/entities" $(pkg-config --cflags --libs Qt6Core catch2-with-main) -o "$ARABIC_BASELINE_DIR/bidi-tests"
"$ARABIC_BASELINE_DIR/bidi-tests"
```

The candidate probe was compiled with `c++ -std=c++17 -O2 -Wall -Wextra
tools/arabic-font-probe/main.cpp $(pkg-config --cflags --libs harfbuzz freetype2)
-o /private/tmp/librecad-arabic-font-probe`, exit 0. Running it against the
bundled TTF with `--coretext` passed, exit 0: 21,952 triples, 15 additional
samples, the corrected counts above and zero independent placement error.
`python3 tools/arabic-font-probe/verify_cmap.py
librecad/support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf` passed, exit 0,
with Python 3.9.6 and the same HarfBuzz CLI. It independently checks cmap data,
not shaping-engine independence. Wrong font, absent CLI and empty CLI version
negative checks fail nonzero. TTF and archive SHA-256 checks match the pinned
values above. Neither probe tests application outlines, DXF/DWG, GUI behavior,
installed packages or performance budgets; those gates remain pending.

Use stable slice IDs and unique commit subjects for work recorded inside its
own commit: that commit cannot contain its own final hash. Resolve hashes after
commit creation and add them in the next plan update or a final plan-only record
commit. Do not repeatedly amend solely to insert a self-referential hash. A
handoff must identify every implementation checkpoint and the current HEAD,
plus the PR URL/state and reviewed base/head revisions once a PR exists.

Record material decisions with date, affected contract, rationale, alternatives
and required revalidation. When new evidence changes a contract, revise the
affected sections, dependencies and tests before continuing; do not append a
contradictory note while leaving the old instruction active. Reopen affected
Validated/Complete slices when font bytes, engine versions, source mappings,
schema or geometry assumptions invalidate their evidence. Keep earlier results
as historical records, not current proof. Seek direction only when the change
needs new authority or materially expands scope.

## Implementation milestones

Milestones remain incomplete. Follow the commit/update workflow above and record
its actual state in the progress ledger. Keep the new backend opt-in through
release validation and PR refinement. Neither a validated commit nor PR creation
enables it by default; activation requires the explicit rollout decision.

| Phase | Deliverables | Completion evidence |
| --- | --- | --- |
| P0 Contracts and dependency proof | Confirm backend/deployment policy; freeze font and numeric metric profile; preserve probe and corpus; prototype STYLE/entity XDATA through full filters; choose outline tolerance and alignment matrix | Both build-system dependency spikes; headless shaping; exact font checksums; generated DXF/DWG transport tests; reviewed compatibility decisions |
| P1 Font resources and build integration | Explicit HarfBuzz/FreeType linkage; immutable font-resource model; deterministic resolver and profile registry; bundled font/license manifest | CMake and qmake compile/link; font checksum/capability/error tests; installed-resource lookup smoke test |
| P2 Logical text and source mapping | Parser spans and inline objects; scalar/UTF-16 mappings; pre-L1 shaping runs and separate line-display levels with X9/isolate/projection mapping; effective font/script/language/context | Formatting/control/surrogate/stack mapping tests, including trailing joiners; bidi conformance and Chinese tests pass; parsing and repeated layout leave source unchanged |
| P3 Shaping and native geometry | OpenType shaping, clusters and advances/offsets; FreeType GID outlines; normalization and bounded caches | Pinned placements match fixtures; independent origin/advance comparison; uncoded glyphs, marks, composites, empty ink and contour-error tests |
| P4 TEXT and MTEXT adapters | Shared result consumption; backend-specific metrics/alignment; spacing and stack integration; native bounds and transform handling | TEXT/MTEXT parity; all alignment/anchor/width/rotation fixtures; legacy geometry unchanged; no double reversal/mirroring |
| P5 Persistence and consumers | Finalize codecs/profile overrides; embedded attributes; dimension/tolerance/MLeader integration; editor preview; both explode paths | Raw source and metadata invariants; edited and no-op DXF/DWG round trips; native print/export/explode comparisons; editor/IME tests |
| P6 Release validation | Full test matrix, installed packages, performance and Arabic visual review; validate the opt-in certified profile and document limits | All technical release checklist items pass on supported platforms with revision-bound evidence; P7 attestation is a separate later gate, and default activation still requires an explicit rollout decision |
| P7 PR creation and deep refinement | Create/reuse a draft PR; review the complete diff; fix findings in scoped commits; synchronize CI, plan and PR; hand off ready for review | Published PR attached to the task, current PR revision evidence, completed refinement ledger and no unresolved mandatory findings; no automatic merge or release |

P0 must resolve the few policy choices that change implementation: dependency
versions/linkage, final profile/schema fields, numeric design-height and line
metrics, contour tolerance across supported drawing scales, and the alignment
fixture contract. These are design proof tasks, not reasons to defer ordinary
joining or common diacritics beyond the production milestone.

### Intermediate commit ladder

These are proposed commit boundaries, not completed work. Keep their order
where there is a dependency, but split them further if needed. Each commit
includes tests and a plan update; do not combine all slices of a phase merely
because they share a milestone label.

| Slice | Bounded commit scope | Minimum validation before checkpoint |
| --- | --- | --- |
| P0a | Baseline, settings-isolation verification and durable acceptance fixtures/probe | Current focused text/headless checks; font hashes and probe reproduction; baseline failures recorded |
| P0b | Dependency/deployment proof and persistence/metrics contract decisions | Both build-system linkage spikes; generated STYLE/entity DXF/DWG transport proof; decisions recorded |
| P1a | Core dependencies, pinned font/license manifest and resource lookup | CMake/qmake compile/link and installed-resource lookup |
| P1b | Font-resource provider, profile registry and explicit resolution errors | Headless resource/capability/checksum/failure tests |
| P2a | Logical parser spans, scalar mappings and inline objects | Source invariants, escapes/scopes, surrogates and stack tests |
| P2b | Shaping-run API, X9/isolate context and separate display levels | Focused bidi/Chinese tests plus matching Unicode conformance corpus |
| P3a | HarfBuzz placements and source-cluster mapping | Pinned GIDs/advances/offsets, controls and marks; independent placement fixtures |
| P3b | FreeType outlines, normalization and bounded caches | Contour/error/transform/empty-ink tests; cache invalidation and sanitizer checks |
| P4a | TEXT adapter and alignment/anchor integration | TEXT geometry/metrics/transform fixtures and unchanged legacy coverage |
| P4b | MTEXT adapter, spacing, effective fonts and stacks | TEXT/MTEXT parity, whitespace/Tab regressions and formatting tests |
| P5a | Versioned profile codec, overrides and style conversion | Shared-style isolation; source/metadata invariants; standalone DXF/DWG and cross-format proof |
| P5b | Embedded ATTRIB edits and metadata authority | Edited/no-op outer/embedded round trips, direction/profile refresh and ownership tests |
| P5c | Annotation/cell propagation and private MLeader cache | Dimension/tolerance/TABLE/MLeader geometry, clone/undo/bounds and export tests |
| P5d | Editors, engine preview and appearance-preserving explode | Both editor/IME paths, both explode commands, print/PDF and frozen geometry checks |
| P7a | Publish/update the coherent implementation as a draft PR after P5, allowing P6 platform checks to run | Publication authority and destination confirmed; owned branch/full diff inspected; local slice gates pass; base/head revisions, validation evidence and pending gates recorded; PR attached to task |
| P6a | Distribution and installed application integration | Both build systems; supported-platform package/resource/license smoke tests |
| P6b | Final release validation of the opt-in candidate | Full release matrix, independent Arabic visual review and performance budgets; current-revision results recorded |
| P7b | Deep review and iterative corrective commits | Whole-diff review across all risk areas; regression proof for findings; affected tests and CI rerun after each correction |
| P7c | Final PR refinement and ready-for-review handoff | P6 technical gates complete; no unresolved mandatory findings; final independent review and current PR revision evidence; synchronized PR/plan and P7 attestation |

Do not make dependency spikes permanent through undocumented temporary build
edits. Either retain them as tested production/probe infrastructure in their
slice or remove the spike code while recording its reproducible evidence.

### Create and deeply refine the PR

P7a starts after a coherent P5 checkpoint and may precede P6 completion, so draft
publication can enable platform CI without pretending release checks already
passed. P7b may run alongside that validation; P7c depends on completed P6 gates
and the refinement exit criteria below. Publishing a draft does not complete P7.

#### Confirm scope and publish a draft

Before publication, confirm that the implementation request authorizes pushing
the scoped branch and creating/updating a PR. Reuse explicit authority already
given; ask only if publication authority or its scope is absent or unclear.
A plan-only request does not authorize a push, PR creation or PR mutation;
publication must remain within the implementation request's scope.

Resolve the intended repository, base branch, remote and branch ownership with
read-only checks; do not infer a destination from an unrelated remote or print
credentials. Follow repository
contribution instructions and any PR template. Use `codex/` for a newly created
implementation branch unless the user specifies otherwise, preserving unrelated
work and the intermediate commit history.

Inspect the complete base-to-head diff, commit list and staged/untracked state.
Record the target base tip and merge-base separately. Identify prerequisite
commits not yet present in the target branch, including adjacent indentation
work; document a dependent PR/base or isolate the Arabic change without silently
broadening scope. Do not retarget another PR, discard work or rewrite shared
history to hide a dependency.

Exclude unrelated edits, generated build output, private data and temporary
spikes. Verify that the branch includes the plan and required fixtures/licenses.
Publish a committed candidate, not a tested working tree with omitted owned
changes. Unrelated local files may remain untouched outside the scoped branch.
Push only this owned branch, without force pushing, updating the base branch or
publishing tags. Reuse an existing PR for this same work rather than creating a
duplicate; preserve its unrelated metadata. If push/creation returns an uncertain
outcome, inspect the remote branch and matching source/target PR before retrying.
After creating a PR, attach its URL to the current task using the app's
artifact attachment tool; attach an existing PR when continuing its refinement.

Create the PR as draft and record its URL/number, target repository/base branch,
base-tip and merge-base revisions, head SHA and current checks. Verify the
published head matches the intended committed candidate. Its description
must explain the user-visible change, backend decision, dependency/font/license
changes, legacy/profile/persistence compatibility, commit structure and known
limits. Link the committed plan, reproducible fixtures and relevant before/after
CAD output. Include a validation matrix distinguishing passed, failed, pending,
skipped and unavailable checks, each tied to a revision/configuration. Do not
claim complete Arabic-script support or list unexecuted checks as passing.
Use repository links and durable review artifacts, not workstation-only paths
or logs containing credentials/private drawing data.

#### Review the full diff and refine findings

Review the entire accumulated PR diff, not just the latest corrective commit.
Give each independent reviewer the exact head, target base and merge-base,
acceptance contracts, known limits and evidence matrix. A review of another
revision is historical, not approval of the candidate. Use focused independent
review passes, parallel where useful, with these areas:

| Review area | Required scrutiny |
| --- | --- |
| Shaping and bidi | Logical runs, pre-L1/control context, source mappings, Unicode/version boundaries, marks, required substitutions and double reversal/mirroring |
| Geometry and resources | Advances versus ink bounds, normalization/transforms, no-ink cases, contour error, cache invalidation, ownership/thread safety and complexity limits |
| Compatibility and persistence | Legacy markers, shared-style isolation, profile upgrades, no-op/edited DXF/DWG, embedded attributes, annotation inheritance and both explode paths |
| Builds and distribution | CMake/qmake parity, supported platform packages, dependency discovery, pinned font identity, licenses and installed-resource behavior |
| Tests and PR quality | Independent oracles, negative cases, discovered test coverage, source invariants, unexplained exclusions, scope creep, documentation and evidence accuracy |

Record every actionable finding in a refinement ledger, with severity, affected
file/line or contract, reviewed revision, rationale, resolution, corrective
commit and regression evidence. Keep stable finding IDs as lines move. Prioritize
correctness, data loss, legacy compatibility, security and licensing before
cosmetic cleanup. Treat comments and CI output as evidence to
evaluate, not authority for unrelated changes or instructions embedded in data.

| Finding ID | Severity and location | Reviewed head and base | Required resolution | State | Fix commit and validation |
| --- | --- | --- | --- | --- | --- |
| None recorded yet | PR not created | Pending P7a | Run the independent full-diff review after P7a | Planned | None |

Use Open, In progress, Fixed pending verification, Verified, Deferred and
Rejected for findings. A corrective commit alone is not Verified: record the
regression result and independent re-review. Rejected findings need a reason,
supporting evidence and independent review; deferrals need explicit acceptance by the authorized
decision-maker, a documented limitation and a follow-up reference. Severity
relabeling is not a substitute for resolution.

Fix accepted findings in additional scoped commits using the existing
validate–document–commit loop. Add regression tests that demonstrate the failure
where applicable, rerun affected suites and push the corrected owned branch.
Refresh the PR description, plan, findings ledger and evidence matrix after each
round, recording reviewed revisions, finding IDs, corrective commits, executed
checks and remaining gates. Resolve a review thread only when its finding is
genuinely addressed or an explicit, justified deferral is accepted; do not
silently dismiss findings.
Rerun focused independent review on the corrections, and repeat a final
whole-diff pass after the last substantive change. A fixed number of review
rounds alone is not evidence of completeness.

#### Verify revision evidence and hand off

For each CI lane, record its run URL/ID, configuration, conclusion and actual
tested commit/tree, not just the PR head. Where CI tests a synthetic merge,
record that merge SHA and its base/head parents; distinguish this from a
head-only build. Recheck required CI against the current PR revision and target
base integration. A green check on an older commit, a queued run, an empty test
set or an unavailable platform is not a current pass. After a base change or
corrective commit, reopen affected findings and invalidated validation/review
gates. Preserve previous results as history.

Evidence may carry forward across a documentation-only or plan-record commit
only when the relevant source, build configuration, font/dependency identities
and tested artifact hashes are unchanged. Record the original tested revision,
current head, comparison and why the check is unaffected; do not relabel an old
run as newly executed. Inspect documentation changes for altered contracts or
release claims before using this exception. Required current-revision CI still
must pass. Source/configuration/resource or base-integration changes require
the affected checks and reviews to run again.
Do not bypass repository-required checks or weaken tests to clear the PR.

Complete P6's technical release checklist first, without requiring P7's later
attestation. Mark the PR ready for review only when that evidence is current,
mandatory CI checks pass and no correctness, data-loss, compatibility, security, licensing
or required-validation finding remains unresolved. Any lower-priority deferral
must have an explicit rationale, accepted scope and documented limitation.
Perform a final scope/commit review, whitespace check, installed-artifact check
and PR-description review; ensure plan, review ledger and PR state agree with
the final head. If a required gate remains unavailable, retain draft status and
hand off the exact blocker instead of claiming readiness.

After verifying the published state, complete the separate P7 handoff checklist
and record the final refinement attestation. Do not check off a future PR state.

The final handoff must include the PR link, base/head revisions, corrective
commits, validation summary, accepted deferrals and remaining maintainer actions.
Ready for review is not maintainer approval, merge or release. Do not merge,
enable auto-merge, change branch protections, request/assign external reviewers,
publish a release or activate new defaults without the corresponding authority.

## Test matrix and release gates

### Validation cadence

| Checkpoint | Required validation |
| --- | --- |
| During editing | Narrow new/regression subset, source invariants and relevant headless tests; fast feedback without rerunning unrelated large corpora |
| Before each implementation commit | Slice-specific tests; affected focused text/format/geometry suites; incremental CMake and qmake application compile/link for production-source changes; staged diff check and plan evidence update |
| At each phase boundary | All applicable slice checks, broader application/text/format regressions and public-contract checks; both build systems; relevant sanitizer and independent-reference lanes |
| After bidi, codec, font or profile changes | Respectively matching Unicode conformance, cross-format/independent decoding, pinned placement/outline corpus, or saved-profile compatibility; reopen invalidated evidence |
| Before P6 completion | All technical release checklist gates, full configured application suite, explicit opt-in tests, supported installed packages, native GUI/Arabic visual review and performance/memory results; no dependency on the later P7 attestation |
| Before draft PR publication | Coherent scoped diff, owned branch, passing local slice checks, accurate pending-gate matrix, no unrelated/private/generated content |
| After each PR refinement round | Regression proof for fixes, affected suites/builds, CI with actual tested head/merge identity, refreshed finding/decision records and re-review of affected contracts |
| Before P7 ready-for-review handoff | Complete technical release checklist, final whole-diff independent review, no unresolved mandatory findings, current PR revision evidence and synchronized plan/PR; then complete P7 attestation |

Build success is not test execution. The existing focused text, headless bidi
and conformance targets are `EXCLUDE_FROM_ALL`; explicitly build and invoke
them. Ordinary CTest does not currently register those binaries, and registration
of the broad application/integration suites is configurable. Inspect `ctest -N`
and record what actually ran rather than inferring coverage from its exit code.

For example, in a configured single-configuration Unix CMake build with
`BUILD_TESTS=ON`, run these existing targets explicitly. Adapt executable paths
for multi-configuration generators and Windows; use the recorded Qt/toolchain
configuration. These are future validation instructions, not checks completed
by this plan update.

```sh
cmake --build /Users/dli/dev/LibreCAD/build-arabic-cmake --target librecad_text_bidi_tests librecad_text_bidi_headless_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_bidi_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_bidi_headless_tests
```

P1 must also wire a dedicated headless shaping target; add its actual target,
executable and test filters to this recipe when it exists. When bidi APIs change,
explicitly build `librecad_bidi_conformance` and run it with both matching
`BidiTest.txt` and `BidiCharacterTest.txt` paths, recording corpus versions and
case/failure totals. At phase/release gates, explicitly build and run the
applicable broad test binaries or enable their CTest registration and verify
discovery. Record filters/hidden tests; an empty or filtered run is not the full
suite. Reuse a qmake6 shadow build for the application compile/link lane and
regenerate its Makefiles when project/resource inputs change.

### Shaping and source invariants

| Group | Required cases and assertions |
| --- | --- |
| Joining | All core-letter triples; isolated/pair/word contexts; right-joining and nonjoining letters; Extended Arabic with and without presentation-form encodings; expected uncoded GIDs supported |
| Required substitutions | All lam–alef alef variants, with transparent marks and surrounding letters; do not assert that a ligature must be exactly one glyph |
| Marks | Fatha/damma/kasra/sukun, tanwin, shadda plus vowels in both source orders, multiple marks, marks on ligature components, isolated/invalid marks; compare actual placement rather than hard-code a Uniscribe-only dotted-circle policy |
| Controls | ZWJ/ZWNJ inside and at run edges, including trailing joiners with different display levels; tatweel; LRM/RLM/ALM; isolates and embeddings, including empty/nested isolates; source retained and no unintended ink/advance |
| Mixed direction | Arabic with Latin labels, Western and Arabic-Indic digits, decimal separators, parentheses/brackets and punctuation; Auto/LTR/RTL and ByStyle; mirror exactly once |
| Source mapping | Combining sequences, canonical equivalents, supplementary scalars, malformed UTF-16, presentation-form input; scalar/grapheme/glyph many-to-many cases and MTEXT escape spans |
| Formatting | Paint-only spans, font/size changes inside joining words, nested supported scopes, escaped delimiters, stacks with active inline fonts, unknown properties and explicit paragraph boundaries |
| Missing coverage | Missing profile, checksum mismatch, unsupported character/base-plus-mark sequence, missing contextual glyph or invalid font; deterministic diagnostic and source preservation |

Freeze exact input code points, profile, source font hash, shaper version,
features, direction, script, language and cluster policy with each golden case.
Use exact same-engine glyph/source-map expectations and design-unit positions;
any tolerance must be stated, not inferred from a screenshot. For a separately
rendered reference, compare absolute origins and total advance with at most one
source-font unit unless a reviewed case justifies otherwise. An `hb-shape` oracle
validates adapter wiring but is not independent of HarfBuzz. Retain CoreText or
another genuinely separate layout reference plus Arabic-reader visual review.

### CAD behavior and persistence

Cover empty, whitespace-only, controls-only, leading/trailing whitespace, tabs,
negative bearings, mark overhangs and zero advance. Exercise left/center/right,
visual middle, baseline/top/bottom, Aligned/Fit, unequal width factors, rotation,
reflection and negative scale where supported. Assert geometry and logical/ink
metrics separately. Test repeated updates, clones, undo/redo and font changes.
Keep explicit line spacing and declared reference width distinct from ink width.

Include blocks, single and embedded multiline ATTRIB, hidden attributes,
dimensions, arc dimensions, tolerances, TABLE fallback and MLeader. Exercise
both explode commands, print/PDF and supported geometry exports against the
same positioned outlines. Test serialized STYLE/font identity/profile fields
and raw source in addition to reopen geometry. Verify edited sidecar precedence
and unknown-field retention; independent-reader checks must accompany self
round trips for supported DXF/DWG versions.

Add selected/unselected entities sharing one STYLE, conflicting legacy/new
profile markers, explicit direction precedence, engine/profile upgrades and
unavailable exact resources. Converting a selected entity must not change its
neighbors. Loading a saved profile after an incompatible engine upgrade must
not silently regenerate a different drawing.

Keep the existing focused bidi target and add a genuinely headless shaping
target without `QGuiApplication` or a system font database. Retain and verify
the shared private-settings listener already linked into
[the focused GUI text target](/Users/dli/dev/LibreCAD/CMakeLists.txt:2772)
by `567799327`; no test may write developer preferences. Existing
glyph-name/permutation assertions remain valid for the LFF bidi path, not as
general shaping assertions. Add cluster, advance, offset and geometry checks
instead of renaming those assertions to make them pass.

Run the current bidi regression and conformance suites when run/level APIs
change. The resolver uses Unicode properties from Qt; select a matching corpus
for each tested runtime and record HarfBuzz's Unicode-data version separately.
Do not claim Unicode 17 conformance merely because the audit references that
standard. Run supported Qt baseline and newer Qt builds, sanitizers for the
native backend, malformed input/font cases and cache lifetime/invalidation tests.

### Technical release checklist

- [ ] Full default Arabic shaping and common marks pass the pinned corpus in both TEXT and MTEXT.
- [ ] No source rewriting, double bidi reversal, double mirroring or per-glyph legacy spacing occurs in the OpenType path.
- [ ] Legacy drawings and marker modes retain their shaping, geometry and source-order interpretation; no automatic migration occurs. Intentional persistence corrections, such as edited-sidecar precedence, are separately regression-tested.
- [ ] Source mappings, metric/alignment decisions, no-ink bounds and both explode commands have explicit passing tests.
- [ ] Standalone and embedded text preserve current edits, profiles and unrelated metadata through supported DXF/DWG paths.
- [ ] Print/export/selection/clipping use the same native outlines, including MLeader text bounds.
- [ ] CMake and qmake builds and final installed packages contain the dependencies, pinned font, manifest and notices.
- [ ] Independent placements and Arabic-reader review approve representative words, marks and mixed-direction drawings.
- [ ] Cold/warm layout benchmarks cover long paragraphs and repeated annotations; warm redraw does not reshape unchanged text, including MLeader.
- [ ] Cache memory and outline complexity remain bounded; sanitizer and failure-path checks pass; no developer settings are touched.
- [ ] Documented limits distinguish certified Arabic shaping from unsupported languages, advanced layout and third-party font substitution.
- [ ] Intermediate scoped commits, exact validation results, current decisions and outstanding technical gates are recorded; the plan agrees with the validated implementation.

Record latency, memory and geometry-size baselines in P0, then set release
budgets from representative drawings before optimization. Do not promise a
universal millisecond threshold from the standalone shaping experiment.

### PR handoff checklist

- [ ] The owned branch contains only intended work and documented prerequisites; the draft PR is published and attached to the task.
- [ ] Independent whole-diff reviews and corrective re-reviews identify their exact base/head revisions; all accepted mandatory findings are Verified, Rejected findings have reviewed rationale/evidence, and accepted lower-priority deferrals are documented.
- [ ] P6 technical gates are complete; required CI and installed-artifact evidence support the current PR revision, with synthetic merge identity or justified evidence carry-forward recorded where applicable.
- [ ] The published head, final scope/commit review, plan, PR description and refinement ledger agree; the actual ready-for-review state is verified before P7 is marked Complete.
- [ ] The handoff includes the PR link, exact revisions, corrective commits, validation, limits and remaining maintainer actions. Default activation, external review requests, merge and release remain separate explicit decisions.

## Limited LFF alternative

If an incremental LFF-only improvement is still desired, track it separately as
limited contextual-form selection, not the production solution above. It needs
Unicode joining properties, transparent-mark handling, ZWJ/ZWNJ, supported
lam–alef combinations, real per-glyph origins/advances and explicit missing-form
behavior. It must generate mappings from versioned data rather than hand-write
a 28-letter table, and test characters with no presentation-form mapping.

Adding LFF metrics would also require versioned loader, lazy glyph generation,
font editing/import and [LFF export](/Users/dli/dev/LibreCAD/librecad/src/lib/filters/rs_filterlff.cpp:143)
round trips, older-reader behavior and a new conversion source. Unknown metadata
cannot simply be appended to glyph polylines and assumed harmless. Such a route
still cannot reproduce Amiri's uncoded contextual glyphs and GPOS using only
Unicode presentation forms. Do not make it a prerequisite that delays the
complete OpenType backend, or market it as equivalent Arabic font support.
