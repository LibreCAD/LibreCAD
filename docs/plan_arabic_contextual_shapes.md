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
| Execution requirement | Intermediate scoped commits, explicit validation, plan updates, draft PR creation and revision-bound deep PR refinement are required; implementation is authorized by the active goal, with publication authority resolved at P7a |
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
   boundaries for retained characters. Adjacent X9-only joiner context may be
   borrowed at a level boundary without borrowing a retained different-level
   scalar: `A + ZWJ + beh` must still expose the joiner to the Arabic item.
   Keep ownership/source emission disjoint even where shaping context overlaps.
   Do not pass the output of `visualClusters()` as shaping input.
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
an already-shaped glyph array. Keep joining context inside the applicable
retained-character level run, with the adjacent X9 joiner exception above and
the special boundary effects of isolates.
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

Retain the effective inline style for empty paragraphs and empty stack children,
even though they have no glyph spans. The parser records that state directly;
layout must not reconstruct it from raw tokens or silently use another font.
Manually constructed paragraph records may omit it only when the layout caller
supplies an explicit fallback style.

The initial opt-in `stack-v1` metric policy uses child scale 4/9, parent-local
axis 4.5 times the inline height factor and clearance 1 times that factor.
The child scale matches the existing 4-unit child in a 9-unit cell, not a
promise of legacy placement: shared layout centers horizontal children by
logical width and uses their profile/ink extents to avoid overlap. `^` has no
separator, `/` a horizontal rule and `#` a diagonal separator, with object
source/paint provenance retained. Tabs use a default 36-local-unit interval
(four initial cells), independent of inline font changes, and final whole-line
bidi fields are placed from the resolved paragraph flow edge. Explicit Exact
line pitch ignores actual mark height; AtLeast also respects profile and
adjacent ink extents. These constants and full-affine child composition need
fixtures before profile activation; later geometry changes follow the versioned
profile compatibility rule.

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
| 2026-10-11 | P0a.1 | Complete | `c2d3b01e1` | Pristine HEAD headless bidi: 1,688 assertions / 16 cases; durable probe: 21,952 triples + 15 CoreText samples, maximum placement error 0; independent cmap audit passes | P0a CAD/performance fixtures and P0b transport/decision gates remain pending |
| 2026-10-11 | P2b | Complete | `644b146c5` | Headless: 1,850 assertions / 21 cases; Unicode 17: 770,241 type/direction + 91,707 character cases, zero failures; legacy display parity checked | Font/script itemization and renderer consumption remain pending; P2 overall is incomplete |
| 2026-10-11 | P1a, P1b, P2a, P3a, P3b | In progress | Uncommitted owned service slices | Dependency/resource, font/shaper, parser and outline work; individual results recorded before checkpoints | Validate and checkpoint bounded slices; adapters/persistence/P6/P7 remain pending |
| 2026-10-11 | P1core.1 | Complete | `7996e9b5a` — `Add headless OpenType font and shaping core` | Isolated ON: 639,014 assertions / 7 cases; OFF without FreeType: 4 / 1; ASan/UBSan: 639,014 / 7; CMake and qmake full application builds exit 0 | Bounded core checkpoint only; registry, installation, certified deployment and entity adapters remain pending |
| 2026-10-11 | P1 registry | Complete | `b25777bb0` and `be1d6fb69` — `Reject broken priority font links without substitution` | Corrected isolated ON/ASan: 128 assertions / 7 cases; OFF: 31 / 3; CMake/qmake application builds pass. Independent before/after review verifies broken-link precedence, hot-cache identity rejection and immutable resource lifetime | Bounded registry checkpoint only; package/certification and entity/profile integration remain pending |
| 2026-10-11 | P2a logical parser | Complete | `d2e9ac848` — `Add source-preserving logical text document parser` | Isolated CMake ON, OFF and ASan/UBSan each pass 452,158 assertions / 22 cases; complete incremental CMake/qmake application builds exit 0 | Bounded parser checkpoint only; font/script itemizer, shared layout and entity adapter integration remain pending |
| 2026-10-11 | P2c logical itemizer | Complete | `19e2f7793` — `Itemize source-mapped logical font and script runs` | Isolated ON/OFF/ASan: 208,140 assertions / 16 cases; ON/ASan shaping handoff: 441 / 3, OFF: 1 / 1; CMake/qmake application builds pass; independent source/context review finds no itemizer defect | Bounded itemizer checkpoint only; projection complexity correction below, blank-paragraph styles and shared layout remain separate gates |
| 2026-10-11 | P2b.1 bounded projection | Complete | `95549793f` — `Bound explicit bidi scope lookup in Chinese projection` | Isolated focused application: 541,335 assertions / 70 cases; headless ON/OFF/ASan: 526,521 / 23; itemizer/handoff regressions and Unicode 17 conformance pass; CMake/qmake application builds pass | Bounded projection checkpoint only; empty-paragraph styles, shared layout and entity integration remain pending |
| 2026-10-11 | P2a.1 empty paragraph styles | Complete | `a477b68ef` — `Preserve effective styles on empty logical paragraphs` | Independent style/scope review; strict private build and isolated ON/OFF/ASan each pass 452,371 assertions / 24 cases; itemizer/handoff and CMake/qmake application builds pass | Consume it in recursive layout and validate empty-line/stack metrics; overall layout/entity gates remain pending |
| 2026-10-11 | P0b transport proof | In progress | Uncommitted filter candidate; provisional codec is a separate service | Earlier STYLE binding failure repaired; new STYLE-unit and LTYPE-control prerequisites pass focused/direct native JSON checks; converted DXF still fails at DIMSTYLE color fields | Isolate/checkpoint prerequisites; codec/schema freeze and public activation require separate proof, not a self-round trip |
| 2026-10-11 | P3b.1 native outline adapter | Validated | Candidate subject: `Add precision-bounded native text contours` | Both numeric regressions fail before correction (exit 42); isolated ON/ASan: 12,326 assertions / 8 cases; OFF: 11,460 / 7; enabled/disabled CMake applications and qmake application build pass; independent numeric review approves | Checkpoint bounded native service; complete-affine entity consumption, layout caches and release geometry gates remain pending |
| 2026-10-11 | P7 execution | Planned | `55cbfc54f`; plan-only follow-up subject: `Preserve prepublication findings through PR refinement` | Draft baseline, creation checkpoint, acceptance map, deep-review cadence and CI history defined; prepublication findings retain provenance and require local-contract resolution before publication. Independent plan review and scoped whitespace checks pass; no publication or PR acceptance claimed | Complete the coherent P5 candidate, reconcile existing review findings, then execute P7a and keep the records below current |

For every executed check, record its exact command and filters, exit status,
result/test count, tested revision and candidate-tree state, build configuration,
platform/toolchain/Qt/font/shaper versions where relevant, corpus identity and
durable log or fixture reference. Record exclusions and unavailable lanes
explicitly. Keep generated build output and large logs out of Git unless they
are intentionally selected fixtures; retain concise results in the plan.

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

### Shaping bidi API evidence

P2b is independent of the font backend: existing callers retain final display
levels, while the new API exposes pre-L1 levels, X9 exclusions and source-mapped
context boundaries. On the same reference environment, the owned bidi candidate
at parent `c2d3b01e1`, subsequently checkpointed as `644b146c5`, passed these
explicit commands, exit 0:

```sh
cmake --build /Users/dli/dev/LibreCAD/build-phase-c --target librecad_text_bidi_headless_tests librecad_bidi_conformance -j 6
/Users/dli/dev/LibreCAD/build-phase-c/librecad_text_bidi_headless_tests
/Users/dli/dev/LibreCAD/build-phase-c/librecad_bidi_conformance /private/tmp/librecad-bidi17.rcEM8M/BidiTest.txt /private/tmp/librecad-bidi17.rcEM8M/BidiCharacterTest.txt
```

The headless suite passed 1,850 assertions in 21 cases. The official matching
Unicode 17.0.0 corpora passed 770,241 type/direction cases and 91,707 character
cases with zero failures; the runner checks old/new display parity and X9
metadata for every record. Corpus files are external test inputs from
`https://www.unicode.org/Public/17.0.0/ucd/`. Their SHA-256 values are
`888bdfc8090652272d1f859cdb00ae659e2dc6c26740be61ef1d03998a687620`
for BidiTest and
`a3e6e905ab5afbe318a96df5401d0372a04cd73ef139ab5e3cf0ae241c255488`
for BidiCharacterTest. These checks do not validate font
itemization, glyph placement or CAD geometry. The changing-worktree application
suite additionally passed 16,664 assertions in 68 cases; it is mixed-tree
evidence, not proof of an isolated backend commit.

### Historical mixed tree validation gates

The following 2026-10-11 runs used HEAD `644b146c5` plus uncommitted build,
service and filter edits, on the reference macOS arm64 environment above with
LibreDWG 0.14. CMake used Unix Makefiles, native architecture and no explicit
build type. They are diagnostic evidence, not isolated-commit or release proof.
The exact commands were:

```sh
cmake -S /Users/dli/dev/LibreCAD -B /Users/dli/dev/LibreCAD/build -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=ON
cmake --build /Users/dli/dev/LibreCAD/build --target librecad_textprofile_transport_tests librecad_dwg_write_fast_tests librecad_text_outline_tests librecad -j4
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build/librecad_dwg_write_fast_tests '[vport]'
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build/librecad_textprofile_transport_tests
QT_QPA_PLATFORM=offscreen DWG2DXF=/opt/homebrew/bin/dwg2dxf /Users/dli/dev/LibreCAD/build/librecad_textprofile_transport_tests '[external-reader]'
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build/librecad_text_outline_tests
```

| Check | Exit status and result | Remaining gate |
| --- | --- | --- |
| Configure and listed build targets | 0; application links | Full qmake application linkage, isolated checkpoint and platform/package validation remain pending |
| Focused VPORT regression | 0; 364 assertions / 10 cases pass | Checkpoint the narrow reader-provenance prerequisite without weakening duplicate/reserved-identity guards |
| Generated transport matrix | 42; 5,070/5,078 assertions pass; one case fails across DXF-first and DWG-first branches | Eight failures decode DWG TEXT/MTEXT STYLE binding as STANDARD; passing profile payload checks alone do not prove interchange |
| Explicit hidden independent-reader case | 42; 526/529 assertions pass; one case fails | Two initial STYLE mismatches plus rejected converted DXF; LibreDWG returns 0 but reports buffer/handle diagnostics; writer-versus-decoder cause is not isolated |
| Native outline adapter | 42; 12,239/12,240 assertions pass; 4/5 cases pass | Empty container bounds retain valid inverted reset values; fix and verify no-ink behavior and supported coordinate range |

Source/binary hashes and raw logs are retained in the local diagnostic snapshot
`/private/tmp/librecad-arabic-p0b-validation.md`. This temporary snapshot is not
a durable acceptance artifact; rerun and record isolated-candidate evidence in
the relevant checkpoint. Do not infer malformed native DWG from the independent
decoder's diagnostics alone or claim a green cross-format gate. These failures
predate the subsequent repairs in the progress ledger; they remain historical
evidence, not the current candidate's acceptance results.

### Isolated headless core evidence

P1core.1 contains the exact-byte font provider, design-unit HarfBuzz shaper,
immutable bounded outline cache, headless tests and dependency/source wiring in
both build systems. It does not activate an entity backend or complete P1/P3.
The staged candidate at parent `f8921f6e116482e839e823791a427fc05fbeabe3`
was exported with `git checkout-index --all` into
`/private/tmp/librecad-arabic-core-index.TOns5t`; each staged source/build file
was compared byte-for-byte with that snapshot before the final test rerun.
Parser, itemizer, registry, native-entity outline and filter edits were excluded.
The later plan-only edits do not change these tested implementation inputs.

On macOS arm64 with Apple Clang 21.0.0, Qt 6.11.2, Catch2 3.16.1,
HarfBuzz 14.6.0 and FreeType 2.14.3, these commands all exited 0:

```sh
cmake -S /private/tmp/librecad-arabic-core-index.TOns5t -B /private/tmp/librecad-arabic-core-index.TOns5t/build-on -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=ON
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-on --target librecad_text_shaper_tests -j6
/private/tmp/librecad-arabic-core-index.TOns5t/build-on/librecad_text_shaper_tests
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-on --target librecad -j4
cmake -S /private/tmp/librecad-arabic-core-index.TOns5t -B /private/tmp/librecad-arabic-core-index.TOns5t/build-off -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-off --target librecad_text_shaper_tests -j6
/private/tmp/librecad-arabic-core-index.TOns5t/build-off/librecad_text_shaper_tests
cmake -S /private/tmp/librecad-arabic-core-index.TOns5t -B /private/tmp/librecad-arabic-core-index.TOns5t/build-asan -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=ON -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-asan --target librecad_text_shaper_tests -j4
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /private/tmp/librecad-arabic-core-index.TOns5t/build-asan/librecad_text_shaper_tests
```

Enabled ordinary and sanitizer runs each passed 639,014 assertions in 7 cases;
the disabled run passed 4 assertions in 1 case. Sanitizer instrumentation covers
the font/shaper/tests, not the external Qt/HarfBuzz/FreeType/Catch2 libraries;
LeakSanitizer is unsupported on this host and was explicitly disabled.

The complete qmake application shadow build also exited 0. From the snapshot's
`build-qmake` directory, run `qmake6 -r -nodepend ../librecad.pro
LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true -o Makefile`, then
`make -j4 sub-librecad-make_first-ordered`. An earlier invocation before top-level
qmake completed failed with an absent make target; the ordered retry above
completed without source changes. The local SDK/deployment-target warnings
do not establish compatibility with older macOS or universal packaging.

The build API minima are HarfBuzz 2.6 and FreeType 2.10; only the reference
versions above are validated here. The proposed saved-profile registry accepts
`HarfBuzz-OT/14.6.0` only, not every API-compatible runtime. Normal dependency
discovery and certified rendering availability are distinct contracts. P1/P6
must supply the pinned engine in release packages or explicitly certify another
engine without changing saved geometry. CI dependency installation, universal
libraries, package resources/licenses, supported Qt baseline and other platform
builds are still pending; no release/PR checklist item is implied by this slice.

### Isolated font registry evidence

The registry candidate at parent `7996e9b5a96c57fcebe48ad96da937d4c87b05e9`
adds immutable profile identity, explicit certified-engine checks, deterministic
installed roots, resource/cache diagnostics and a headless CMake test target.
The service sources are registered in both build systems. Unknown profiles, policy/checksum/face mismatches,
incompatible engines and disabled backends do not become LFF substitutions.
Tests cover every supported installed layout, explicit-root precedence, corrupt
higher-priority resources and concurrent resolve/clear with retained resources.

The five staged implementation/build files were exported over the preceding
isolated snapshot at `/private/tmp/librecad-arabic-core-index.TOns5t`; it now
represents this registry candidate, not an immutable copy of the earlier core
checkpoint. Parser, itemizer, native-entity outlines, filter changes and the
pending dependency-prefix/package integration remain excluded. The plan and PR
workflow edits do not alter the tested source/build inputs.

On the same macOS arm64/toolchain/library versions as the core evidence, all
four validation processes exited 0. No tests were skipped in these runs:

| Configuration | Executed checks | Result |
| --- | --- | --- |
| CMake ON | Configure; build registry target and full application; run registry binary | 97 assertions / 6 cases; full application compile/link succeeds |
| CMake OFF | Configure with FreeType discovery disabled; build/run registry target | 31 assertions / 3 cases; disabled-backend diagnostics pass without HarfBuzz/FreeType |
| CMake ASan/UBSan ON | Configure reused sanitizer build; build/run registry target | 97 assertions / 6 cases; no sanitizer diagnostics |
| qmake ON | Recursive top-level configure; complete ordered application target | Incremental application compile/link and postprocessing succeed |

Reproduce with the isolated source at `$registry_source` and the corresponding
out-of-source build directories; this task-specific variable is an absolute
snapshot path, not a system environment override:

```sh
registry_source=/private/tmp/librecad-arabic-core-index.TOns5t
cmake -S "$registry_source" -B "$registry_source/build-on" -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=ON
cmake --build "$registry_source/build-on" --target librecad_text_font_registry_tests librecad -j4
"$registry_source/build-on/librecad_text_font_registry_tests"
cmake -S "$registry_source" -B "$registry_source/build-off" -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON
cmake --build "$registry_source/build-off" --target librecad_text_font_registry_tests -j4
"$registry_source/build-off/librecad_text_font_registry_tests"
cmake -S "$registry_source" -B "$registry_source/build-asan" -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE=ON -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build "$registry_source/build-asan" --target librecad_text_font_registry_tests -j4
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$registry_source/build-asan/librecad_text_font_registry_tests"
cd "$registry_source/build-qmake"
qmake6 -r -nodepend ../librecad.pro LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true -o Makefile
make -j4 sub-librecad-make_first-ordered
```

Sanitizer instrumentation covers the font, registry and tests, not external
dependencies or application adapters. LeakSanitizer remains disabled on this
host. qmake's postprocessor ran in the isolated snapshot despite the supplied
`DISABLE_POSTSCRIPT` argument; its success is not installed-package acceptance.
The existing newer-SDK/deployment-target linker warnings remain. Logs are local
`/private/tmp/librecad-arabic-registry-{on,off,asan}-{configure,build,test}.log`
and `/private/tmp/librecad-arabic-registry-qmake-{configure,build}.log`;
reproducible source/tests and this concise record are committed, not build output.

#### Registry correction and independent verification

Independent review reproduced a priority violation: `QFileInfo::exists()`
follows symlinks, so a dangling font link in the first configured root was
silently skipped in favor of a valid later root. Treat an existing link as a
resource candidate even when its target is absent; exact loading now returns
`FontFailure` at that path instead of substituting another installation.
The added Unix regression failed before the correction (7 of 8 assertions pass,
exit 42) and passes afterward. The independently rebuilt original probe changes
from `resolution=1, chose-secondary=1` to `resolution=0, error=7, chose-secondary=0`;
its intentional pre-fix-success exit convention changes from 0 to 1.

Additional tests reject all six mismatched saved identity/engine fields with a
hot cache. Loaded resources retain immutable bytes and outlines after the file
is modified or deleted; cache clearing exposes corrupt/missing installations,
and restoring the exact file allows a fresh resource load. Independent strict
ON/OFF builds pass 128 assertions / 7 cases and 31 / 3 respectively, without
skips; the reviewer made no source changes.

Only the two staged registry implementation/test files were exported into the
same isolated source, now containing the committed parser as well. Using the
commands above, corrected ON and ASan/UBSan targets each pass 128 assertions /
7 cases, OFF without FreeType discovery passes 31 / 3, and complete incremental
CMake/qmake application builds succeed. Process IDs ON 6552, OFF 88874,
sanitizer 49727 and qmake 78491 all terminate with exit 0. Logs are
`/private/tmp/librecad-arabic-registry-fix-{on,off,asan}-{configure,build,test}.log`
and `/private/tmp/librecad-arabic-registry-fix-qmake-{configure,build}.log`.
The before-fix regression log is
`/private/tmp/librecad-arabic-registry-symlink-before-test.log`.
Instrumentation/deployment limitations remain those stated above. These results
do not establish package, entity-adapter or full profile integration acceptance.

### Isolated logical parser evidence

The P2a candidate at parent `b25777bb0c077adf68a1ec6463509359549f2bda`
adds logical paragraphs, scalar/grapheme maps, raw UTF-16 source intervals,
formatting spans, recursive stack children and bounded malformed-input handling.
Parsing never rewrites backing text. The numeric lexer accepts immediate
optional semicolons without consuming later text/scopes/commands; incomplete
exponents preserve their spelling, including before active native caret breaks.
Native caret policy and RGB-overflow compatibility were not redefined.

Only its header, implementation, tests and CMake/qmake registration hunks were
exported into the isolated snapshot used above. Itemizer, native-entity outline,
filter and dependency/package edits remain excluded. All five staged files were
compared with their exported inputs. On the same macOS arm64 toolchain,
registered ON, OFF without FreeType discovery and ASan/UBSan runs each passed
452,158 assertions in 22 cases, with no skips. Full incremental CMake and
ordered qmake application builds also exited 0. The independent review's
registry finding does not affect this parser-only acceptance evidence.

Use the registry commands above with `librecad_text_document_tests` replacing
`librecad_text_font_registry_tests`; retain `librecad` in the ON build command,
the explicit OFF flag/discovery exclusion, sanitizer flags/options and the
same qmake commands. Executed process IDs were ON 34459, OFF 83563, sanitizer
55444 and qmake 43129; every process completed with exit 0. Exact command
outputs are in `/private/tmp/librecad-arabic-parser-{on,off,asan}-{configure,build,test}.log`
and `/private/tmp/librecad-arabic-parser-qmake-{configure,build}.log`.
Instrumentation is limited to the parser and its tests; external libraries,
native lifecycle and entity rendering remain separate gates. This checkpoint
does not claim integration or release acceptance.

### Isolated logical itemizer evidence

The P2c candidate at parent `be1d6fb69` adds QtCore logical font/script/language
items, disjoint UTF-16 source ownership and bounded shaping contexts. Paint-only
spans remain separate without splitting shaping. Retained pre-L1 levels and
isolate/object/tab boundaries bound context; adjacent X9-only controls may be
borrowed without duplicate ownership or other-level letters. Generated Chinese
projection controls have no original-source ranges. Script/grapheme policy uses
the public Qt properties and is not a claim of Script_Extensions conformance.

Four frozen service/test files and only their CMake/qmake registration hunks
were exported from the index into the same isolated source. Other working-tree
changes, including the newly discovered bidi complexity correction, blank-line
style extension, layout, native outlines, filters and dependency packaging,
were excluded. Independent review of the itemizer and tests found no concrete
defect; its same-face shaping fixtures prove context/ownership handoff, not
actual font substitution or final CAD scaling.

Using the same macOS arm64 toolchain and sanitizer limits as above, ON, OFF and
ASan/UBSan each pass 208,140 assertions / 16 itemizer cases. The shaping handoff
passes 441 / 3 in ON and sanitizer builds; OFF executes its disabled-path case
(1 / 1) without FreeType discovery. Complete incremental CMake and ordered
qmake application builds pass. Processes ON 41782, OFF 19410, sanitizer 18356
and qmake 38597 all terminate with exit 0, with no skipped tests.

Use the registry configure/build recipes with both targets
`librecad_text_runs_tests librecad_text_runs_shaping_tests`, retaining `librecad`
in the ON build; invoke both binaries separately, with the documented sanitizer
environment for each. The qmake recipe is unchanged. Exact output logs are
`/private/tmp/librecad-arabic-runs-{on,off,asan}-{configure,build,test,shaping-test}.log`
and `/private/tmp/librecad-arabic-runs-qmake-{configure,build}.log`.
The existing projection's unmatched-PDI quadratic lookup remains a separate
robustness gate until its correction is validated and checkpointed; passing
these narrower tests does not waive it or establish entity/rendering acceptance.

### Bounded bidi projection correction

The correction at parent `19e2f7793` removes a proven quadratic lookup in
`directionalText`: N embeddings followed by N unmatched PDI controls repeatedly
scanned the unchanged scope stack. A separate isolate-depth stack makes each
lookup constant-time and each scope push/removal amortized constant-time,
without changing PDF, PDI, paragraph-reset or Chinese projection semantics.
Exact projection/source-map tests cover nested controls and the adversarial
65,536-embedding / 65,536-unmatched-PDI case without machine-speed assertions.
Independent review found no defect and compared the old/new scope models over
2,441,406 event prefixes through length 9 with identical results.

Only its two staged files were exported over the isolated itemizer checkpoint;
other working-tree edits remain excluded. The first ON/OFF/sanitizer builds
failed with exit 2 before executing tests: a test's `auto` concatenation becomes
`QStringBuilder` under the repository's `QT_USE_QSTRINGBUILDER`. Declaring that
test input as `QString` fixes the mismatch; no flag or expectation was weakened.
The original private test build additionally documented two pre-existing test
warning exceptions, not a fully warning-free suite.

All corrected isolated processes pass: ON 36456, OFF 39183, sanitizer 20162
and qmake 58922 terminate with exit 0. Focused application text tests execute
541,335 assertions / 70 cases; headless ON/OFF/ASan each execute 526,521 / 23.
The itemizer remains 208,140 / 16 in every configuration and its shaping handoff
441 / 3 in ON/ASan. Matching Unicode 17 conformance executes 770,241 type/direction
and 91,707 character cases with zero failures. Complete incremental CMake/qmake
application builds pass. Sanitizer exclusions remain as previously stated.

Reproduce using the existing configured builds: ON builds/runs
`librecad_text_bidi_headless_tests librecad_text_bidi_tests librecad_text_runs_tests
librecad_text_runs_shaping_tests librecad_bidi_conformance librecad`; the focused
GUI test binary uses `QT_QPA_PLATFORM=offscreen`. OFF builds/runs headless bidi
and itemizer; sanitizer builds/runs headless bidi, itemizer and shaping handoff
with the documented environment. Conformance takes both corpus paths from
`/private/tmp/librecad-bidi17.rcEM8M`. qmake uses the unchanged ordered application
target. Exact logs are `/private/tmp/librecad-arabic-projection-{on,off,asan}-build-retry.log`,
the corresponding `*-{headless,focused,runs,shaping}-test.log`,
`/private/tmp/librecad-arabic-projection-on-conformance.log` and
`/private/tmp/librecad-arabic-projection-qmake-build.log`; failed initial logs
retain `-build.log`. Private proof/corpus hashes and warning details are in
`/private/tmp/librecad-projection-scope.P1mP7S/EVIDENCE.md`.
Passing this correction does not certify final Arabic entity rendering.

### Empty paragraph style evidence

The parser/model correction at parent `95549793f` retains an optional effective
end style for every parsed paragraph. Empty lines and empty stack children have
no spans, so the previous record could not carry their active inline font,
metrics or paint into layout. The optional value is absent only on manually
constructed public records; consumers must validate any supplied style before
using it. No raw text, token, scalar or source-range behavior is changed.

Two new regressions independently assert exact styles across scoped blank lines,
scope restoration and empty stack-child inheritance. Independent read-only
review also checks contiguous CR/LF merging and EOF with unclosed scopes; no
defect was found. A strict private build with the repository's QStringBuilder
definition executes 452,371 assertions / 24 cases with exit 0 (process 31739,
`/private/tmp/librecad-arabic-empty-style.KR1zSx/tests.log`).

Only the three staged parser/model/test files were exported over the isolated
projection checkpoint. Existing configured ON, OFF and ASan/UBSan builds each
execute 452,371 / 24 parser assertions/cases, 208,140 / 16 itemizer checks and
the enabled 441 / 3 or disabled 1 / 1 shaping handoff. Complete incremental
CMake and ordered qmake application builds pass. Processes ON 33736, OFF 72471,
sanitizer 64414 and qmake 59623 all terminate with exit 0, without skipped tests.
Build the three corresponding targets (plus `librecad` in ON), then invoke
each binary separately with the previously documented sanitizer environment.
Exact logs are `/private/tmp/librecad-arabic-empty-{on,off,asan}-build.log`,
`/private/tmp/librecad-arabic-empty-{on,off,asan}-{parser,runs,shaping}-test.log`
and `/private/tmp/librecad-arabic-empty-qmake-build.log`.
Recursive-layout consumption, final entity geometry and release acceptance
remain separate, unfinished gates.

### Native outline precision evidence

P3b's native adapter creates inherited-pen closed polylines only after the full
caller-supplied affine. It preserves bearings, holes, exact closing edges and
the no-extent sentinel, and rejects range, precision and complexity failures
without partial geometry. Default error tolerance is `1e-4` final document
units, with a caller budget of 100,000 segments and hard limits of 1,000,000
segments, 100,000 commands and 32 subdivision levels. This service does not
activate TEXT/MTEXT or prove that their eventual adapters compose every affine.

The five staged service/test/build files at parent `55cbfc54f` were exported into
the existing isolated snapshot; uncommitted shared layout, profiles, filters and deployment
changes were excluded. The `[tiny]` quadratic reproducer, with a nonzero
`1e-170` chord and `1e-180` tolerance, exits 42 before the correction because
squaring the chord underflows and falsely exhausts subdivision depth. The
distance calculation now uses `hypot`, a unit chord and a clamped physical
projection. The corrected enabled suite passes 12,322 assertions / 7 cases;
its full CMake application build and ordered qmake application build exit 0.
Logs are `/private/tmp/librecad-arabic-outline-before-{configure,build,test}.log`,
`/private/tmp/librecad-arabic-outline-on-{build,test}.log` and
`/private/tmp/librecad-arabic-outline-qmake-{configure,build}.log`.

Independent review also found that the relative rounding allowance can
underflow to zero. An affine-only triangle with scale `denorm_min()` then
accepts a tolerance smaller than its actual rounding error. The `[subnormal]`
regression exits 42 before the guard correction, observing success instead of
`PrecisionLoss` (`/private/tmp/librecad-arabic-outline-subnormal-before-{build,test}.log`).
An absolute `256 * denorm_min()` allowance now complements the relative term.
This is a conservative precision floor, not a claim that every rejected
axis-aligned case is individually unrepresentable. The two numeric regressions
are independently reviewed against analytically derived expectations.

All five staged input files were compared byte-for-byte with the snapshot
before the final runs. On the same macOS arm64/Apple Clang 21/Qt 6.11.2/
HarfBuzz 14.6.0/FreeType 2.14.3/Catch2 3.16.1 configuration, execute:

```sh
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-on --target librecad_text_outline_tests librecad -j4
QT_QPA_PLATFORM=offscreen /private/tmp/librecad-arabic-core-index.TOns5t/build-on/librecad_text_outline_tests
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-off --target librecad_text_outline_tests librecad -j4
QT_QPA_PLATFORM=offscreen /private/tmp/librecad-arabic-core-index.TOns5t/build-off/librecad_text_outline_tests
cmake --build /private/tmp/librecad-arabic-core-index.TOns5t/build-asan --target librecad_text_outline_tests -j4
QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 /private/tmp/librecad-arabic-core-index.TOns5t/build-asan/librecad_text_outline_tests
```

Final ON and ASan/UBSan each pass 12,326 assertions / 8 cases; OFF without
FreeType passes 11,460 / 7, excluding only the explicit font-dependent case.
Complete enabled/disabled CMake application builds and the ordered qmake
application build pass. All four final processes (78343, 3693, 78841 and 48455)
terminate with exit 0. Sanitizers instrument the native application library
and adapter/tests, not the external Qt/HarfBuzz/FreeType/Catch2 libraries;
unsupported LeakSanitizer is disabled. Final logs are
`/private/tmp/librecad-arabic-outline-{on,off,asan}-final-{build,test}.log` and
`/private/tmp/librecad-arabic-outline-qmake-final-build.log`.
Reviewed source/test SHA-256 values are
`11118e8fa6887422e04fba6af4302aed599a4c91e44dc496e428fd3a92b58213` and
`72a7eb13ef56e6bfaa5691b1ff317d03c49f5c03e1123e126836328bf79d90b9`.
Earlier mixed-tree and intermediate passing results remain historical, not
proof of the final candidate. Installed packages, actual entity geometry and
all-platform/Arabic visual acceptance remain unfinished.

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
| P7a | Create/reuse and verify a draft PR for the coherent implementation after P5, allowing P6 platform checks to run | Publication authority and destination confirmed; committed candidate/full diff inspected; local slice gates pass; creation checkpoint below complete, including attached PR and verified published head |
| P6a | CI coverage, distribution and installed application integration | Explicit shaping/format test execution; both build systems; supported-platform package/resource/license smoke tests |
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
P7a becomes In progress when publication preparation starts, even while no PR
exists. P7b and P7c remain Planned until their work begins. Keep the following
record current and reference it from the progress ledger; this plan update does
not itself create or change a PR.

A coherent P5 candidate has an executable opt-in path through TEXT/MTEXT,
editing, save/reopen, the required consumers and both explode commands, with
every mandatory P0–P5 implementation and local acceptance gate mapped to that
committed candidate and passing. Unfinished implementations, stubs and known
mandatory local regressions are not merely pending P6 checks. Outstanding
platform, installed-package, final performance and independent visual lanes may
remain Pending with an explicit configuration, owner or required maintainer
action and next check. P7a publishes that candidate; P7b refines it;
P7c verifies the final candidate and hands it off. None of these steps enables
the backend by default or authorizes merge or release.

Carry findings from implementation and prepublication reviews into the PR
refinement ledger, retaining their original reviewed revisions, evidence and
dispositions. Before P7a publication, resolve known mandatory P0–P5 local-contract
findings through a validated correction and independent verification, or an
independently reviewed, evidence-backed rejection. Pending P6 platform and final
acceptance lanes remain distinct. Creating the draft must not reset the finding
count, erase failed validation history or turn an unverified fix into a pass.

| PR tracking field | Current value |
| --- | --- |
| Phase states | P7a Planned; P7b Planned; P7c Planned |
| PR URL and published state | Not created |
| Verified destination | Repository, remote, source branch and target base to be confirmed at P7a |
| Frozen candidate | Head/tree, base tip, merge-base and submodule revisions not yet recorded for publication |
| Last reviewed candidate | None; reviewer coverage and refinement round pending P7b |
| Integration status | Not evaluated for publication; current base compatibility and mergeability pending |
| Validation and findings | No PR acceptance evidence; populate revision-bound evidence and finding references when executed |
| Next action | Complete P5 dependencies, validate a coherent committed candidate and confirm publication authority before P7a publication |

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

Freeze a committed candidate before each publication or review round. Record
its head/tree, target base tip, merge-base and submodule revisions; use an
isolated checkout or snapshot for tests and review when concurrent edits exist.
Do not let an unrelated dirty tree, a moving agent worktree or a stale build
stand in for the published candidate. Record dependency/lockfile and pinned-font
identities with the candidate so later comparisons can establish what changed.

Inspect the complete base-to-head diff, commit list and staged/untracked state.
Record the target base tip and merge-base separately. Identify prerequisite
commits not yet present in the target branch, including adjacent indentation
work; document a dependent PR/base or isolate the Arabic change without silently
broadening scope. Do not retarget another PR, discard work or rewrite shared
history to hide a dependency.

Prepare a compact acceptance map covering the P0–P6 contracts, not just changed
files. For each contract, name its implementation entry points, regression or
fixture, CI/manual validation lane, review area and current evidence/state.
Include failure paths and compatibility promises as well as successful Arabic
rendering. Use the map to expose missing implementation, tests and review
coverage; a commit count or green workflow cannot substitute for coverage.
At P7a, P6 lanes may be Pending and P7b reviews Planned with a named next action.
At P7c, every mandatory contract needs passing evidence and completed review.
Changing a mandatory contract requires explicit authority for that scope change
and a consistent update to the P6 milestones/checklist, tests and PR; an ordinary
finding deferral cannot waive it or relabel failed/unavailable evidence.
Otherwise keep the gate Pending and the PR draft.

Use the following acceptance-map columns, with stable contract IDs and links to
the detailed contracts above. Cover every mandatory P0–P6 contract; group rows
only when their implementation, tests and review coverage genuinely coincide.
An empty field is a gap, not an implied pass. Keep the map in this plan or a
linked committed review record and summarize it in the PR description.

| Contract ID and requirement | Implementation entry points | Regression or independent fixture | Validation lane and revision evidence | Independent review reference | State and next action |
| --- | --- | --- | --- | --- | --- |
| Pending P7a | Map P0–P6 contracts | Link reproducible checks | Record actual tested inputs and outcomes | Record reviewer coverage | Planned; populate before publication |

Exclude unrelated edits, generated build output, private data and temporary
spikes. Verify that the branch includes the plan and required fixtures/licenses.
Publish a committed candidate, not a tested working tree with omitted owned
changes. Unrelated local files may remain untouched outside the scoped branch.
Push only this owned branch, without force pushing, updating the base branch or
publishing tags. Reuse an open PR only when its repository, source branch, target
and scope match this work; preserve its unrelated metadata. Do not repurpose a
closed, merged or unrelated PR. If push/creation returns an uncertain
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

Include a short reviewer walkthrough: enable the exact opt-in profile, create
TEXT and MTEXT with the same logical Arabic/mark/mixed-direction sample, inspect
metrics and native geometry, edit/save/reopen, then exercise both explode paths.
State expected results and link the fixtures and automated checks; screenshots
alone cannot prove preserved source, positioning or persistence. Explain how
legacy behavior remains available and what happens when the exact font/engine
is unavailable, without suggesting automatic migration or loss of metadata.

Before relying on PR CI, map every release gate to an actual workflow/job or
explicit manual lane, including trigger/path filters, draft/fork restrictions,
test registration and dependency installation. At `644b146c5`, the tracked
`build-all.yml` restricts PR triggers to its workflow/macOS packaging paths and
its Linux and Windows jobs to push events; `dwg-dxf-verify.yml` runs bounded format targets,
not the shaping suites; `run_pixi.yml` checks installation, not Arabic behavior.
These workflows are not proof of full PR acceptance. Reinspect them at P7a,
record the gaps, and add the scoped CI coverage in P6a. Explicitly build and
invoke the new headless/parser/outline and profile-transport targets, and verify
nonzero executed test counts. Include both enabled and explicit legacy-only
configurations where required. Do not count a skipped or absent lane as green.
If required acceptance jobs skip drafts, provide a safe draft-compatible or
manual validation lane in P6a before the readiness gate. Do not mark the PR
ready merely to trigger checks that this plan requires before readiness. If
repository policy prevents such a lane, retain draft status and identify the
maintainer action needed; do not bypass that policy or describe a skipped job
as passing.
Keep validation separate from release publication or deployment actions. Do
not enable secrets, privileged fork execution or protected-environment approvals
merely to obtain a check; record any maintainer action that is needed.

#### PR creation checkpoint

Complete P7a only after all of the following are verified and recorded:

- [ ] Publication authority, repository, owned source branch and intended base are resolved; any prerequisite branch/PR dependency is explicit.
- [ ] The coherent committed candidate and complete diff are inspected; mandatory P0–P5 implementation/local gates pass, and the acceptance map distinguishes remaining P6 checks from completed work.
- [ ] Prepublication findings and validation history are carried into the review records; no known mandatory P0–P5 local-contract finding remains unresolved or fixed without verification.
- [ ] The draft PR exists, is open, targets the intended base and publishes the intended head; its URL is attached to this task. Uncertain push/creation outcomes are reconciled before retrying.
- [ ] The description links the committed plan, fixtures, reviewer walkthrough and revision-bound validation matrix, including known failures and unsupported cases.
- [ ] CI triggers, test discovery, package lanes and draft/fork restrictions are audited; each missing or pending gate has a concrete next action.
- [ ] The PR tracking record and progress ledger identify the published candidate and the next P6/P7b work. P7b/P7c and the overall P7 milestone remain incomplete.

PR creation is a delivery step, not evidence that Arabic rendering is accepted.
Do not mark these boxes from intended future actions.

#### Review the full diff and refine findings

Review the entire accumulated PR diff, not just the latest corrective commit.
Give each independent reviewer the exact head, target base and merge-base,
acceptance contracts, known limits and evidence matrix. A review of another
revision is historical, not approval of the candidate. Use focused independent
review passes, parallel where useful, with these areas:

| Review area | Required scrutiny |
| --- | --- |
| Shaping and bidi | Logical versus visual order; pre-L1 shaping versus final L1 placement; X9, isolate and ZWJ/ZWNJ context across style boundaries; surrogate/grapheme/cluster mappings and Unicode/version boundaries; required substitutions and common marks; no double reversal/mirroring |
| Geometry and layout | Advances versus ink bounds; glyph origins, mark/cursive offsets and no-ink cases; height/width/oblique plus entity transforms; alignment/anchors, tabs, stacks and line spacing; contour tolerance and native coordinate limits; repeated-update stability |
| Resources and robustness | Exact saved font/face/engine policy; missing/corrupt resources and complete-segment fallback; bounded cache keys/lifetime; thread ownership; malformed Unicode/format/font input and complexity limits |
| Compatibility and persistence | Unchanged legacy modes and shared-style isolation; profile upgrades and unknown metadata; no-op/edited DXF/DWG and embedded ATTRIB precedence; annotation/TABLE inheritance and MLeader cache; both explode paths and print/export parity |
| Editing and user experience | Both native text dialogs and property panels; logical Unicode retained through IME preedit/commit, cursor/selection, clipboard, invisible-control edits and undo/redo; engine-rendered preview versus Qt editing font; explicit profile selection/conversion, undo and actionable resource errors; Chinese editing regressions |
| Builds and distribution | CMake/qmake parity and explicit legacy-only builds; supported Qt/platform/architecture matrix; clean installed lookup without developer fonts/prefixes; dependency linkage, pinned font, notices/licenses and actual package contents |
| Tests and PR quality | Independent placement/format oracles, not self-round trips alone; negative and boundary cases; actual nonzero test execution; source invariants; unexplained exclusions; prerequisite scope, documentation and evidence accuracy |

Use these cross-layer cases to challenge the implementation, not just inspect
individual helpers:

- Verify final placements when L1 splits a pre-L1 shaping item: trailing joiners,
  borrowed X9 context, empty isolates and Chinese projection. Clusters remain
  atomic, HarfBuzz order is consumed once, and generated controls acquire no
  original-source ownership.
- Check joining and required substitutions across paint-only spans. An
  indivisible glyph uses its first logical component's effective pen and retains
  all source ranges. Approved fallback reshapes the whole joining/base-plus-mark
  segment; unsafe partial replacement is explicitly rejected.
- Measure contour error after the complete glyph/entity affine, including
  supported anisotropy, reflection and large origins. Check tiny contours,
  no-ink extents, exact coordinate boundaries and deterministic precision,
  range or budget errors without clamping or partial geometry.
- Trace the same revision-bound TEXT/MTEXT cases through shared layout, native
  geometry and representative PDF/explode output. Record the exact artifacts
  and cases approved by an Arabic reader; the standalone CoreText probe cannot
  substitute for application acceptance.
- Exercise repeated updates, cloning, undo/redo, cache eviction, resource
  failure and destruction under sanitizers. Record instrumented sources and
  exclusions; a font/shaper-only run is not native-adapter lifecycle coverage.
- Commit Arabic IME composition, edit a combining sequence and an invisible
  control, then undo/redo and save/reopen from each editor entry point. Verify
  logical source and profile metadata independently of the preview, and confirm
  an explicit conversion leaves unselected references to a shared style unchanged.

Review public headers, build/package inputs, new and modified tests, fixtures,
licenses and prerequisite commits as well as runtime code. Reproduce selected
checks from the committed candidate instead of relying only on the author's
summary. For behavioral fixes, demonstrate the regression on the parent or an
isolated reproducer where feasible, then its success on the fix; record when
such a before/after check is unavailable. Keep deliberately failing reproducers
off the target branch. Compare geometry/output with the pinned reference and
legacy baseline, not just with a newly regenerated golden file.

Record each reviewer's identity or agent role, independence from the reviewed
implementation, review date, exact revisions, covered areas and review artifact
or comment links. Every area in the table needs an explicit coverage record,
including reviews with no findings; a quiet comment thread is not evidence of
review. Keep private reviewer details out of public artifacts.

Record coverage separately from findings so a no-finding review remains
auditable. Authors may explain their implementation, but their own inspection
does not satisfy independent review of that implementation or their fixes.

| Review area | Reviewer and independence | Head, base tip and merge-base | Coverage and reproduced checks | Result or artifact reference |
| --- | --- | --- | --- | --- |
| Pending P7b | Assign an independent reviewer per area | Freeze the candidate | List contracts and checks actually examined | Planned; no review claimed |

Record every actionable finding in a refinement ledger, with severity, affected
file/line or contract, reviewed revision, rationale, resolution, corrective
commit and regression evidence. Keep stable finding IDs as lines move. Prioritize
correctness, data loss, legacy compatibility, security and licensing before
cosmetic cleanup. Treat comments and CI output as evidence to
evaluate, not authority for unrelated changes or instructions embedded in data.

| Finding ID | Severity and location | Reviewed head and base | Required resolution | State | Fix commit and validation |
| --- | --- | --- | --- | --- | --- |
| Populate before P7a | Import prepublication findings; add PR review findings as discovered | Retain original revisions | Apply the resolution and verification rules below | Record actual state | Link existing or new evidence |

This template is not a claim that no findings exist. Reconcile it with slice
reviews and failed-check records before publication. Previously verified fixes
remain part of the accumulated diff and the final whole-diff review; retain
their historical proof and reopen them when later changes invalidate it.

Use Open, In progress, Fixed pending verification, Verified, Deferred and
Rejected for findings. A corrective commit alone is not Verified: record the
regression result and independent re-review. Rejected findings need a reason,
supporting evidence and independent review; deferrals need explicit acceptance by the authorized
decision-maker, a documented limitation and a follow-up reference. Severity
relabeling is not a substitute for resolution.

A finding is mandatory when it violates an accepted P0–P6 contract, prevents a
required validation gate, or is introduced or worsened by this PR and creates
a correctness, data-loss, legacy-compatibility, security or licensing failure,
regardless of its numerical severity. Fix it or reject it with independently
reviewed evidence; do not defer it as cosmetic cleanup.
Optional improvements may be deferred under the rules above. Put unrelated
refactoring or new feature requests in a separate proposal unless separately
authorized; keep necessary in-scope corrections in this PR.

Run each refinement round in this order:

1. Freeze the review candidate. The initial round covers the entire accumulated
   diff, every review area and the acceptance map. Intermediate rounds examine
   the corrective diff, any base delta and all affected contracts and consumers,
   with the complete diff available to reviewers. Identify which prior coverage
   is invalidated rather than claiming another complete review.
   Record the round ID, exact revisions, coverage and review artifacts,
   including explicit no-finding results. A final whole-diff review remains
   required after the last substantive change.
2. Triage findings by impact and contract. Record whether each is introduced by
   the PR, an in-scope prerequisite or unrelated baseline behavior. Baseline
   attribution does not waive an in-scope gate; track unrelated issues with
   evidence and apply the deferral/rejection rules above.
3. Reproduce accepted behavioral findings, add a regression where applicable
   and implement the correction. Record unavailable before/after proof rather
   than claiming it. Triage related findings together, but keep unrelated fixes
   in separate scoped commits. Apply the existing validate–document–commit loop;
   do not push an unvalidated correction or a failing-test-only checkpoint.
   For a shared-service or public-contract fix, trace every affected consumer
   and failure path through the acceptance map. Record which checks must rerun
   and why any consumer is unaffected; a passing reproducer in one caller does
   not establish TEXT/MTEXT, annotation, persistence or export parity.
   Algorithm, cache or geometry-budget changes also require the affected
   worst-case, cold/warm and memory checks against the recorded P0/P6 budgets;
   a small correctness fixture alone cannot establish bounded performance.
4. Push the corrected owned branch, reconcile the published head and checks,
   and refresh the PR description, plan, acceptance map, findings ledger and
   evidence matrix. Record corrective commits, executed checks and invalidated
   gates using the revision-evidence rules below.
5. Have an independent reviewer verify corrections and their regressions before
   marking findings Verified. Resolve threads only after genuine resolution or
   an explicitly accepted deferral or independently reviewed, evidence-backed
   rejection; preserve the rationale and evidence. Repeat a whole-diff review
   after the last substantive change.

Keep a concise record of each round: reviewed candidate, reviewer coverage,
finding IDs/states, corrective commits, validation references, remaining gates
and disposition. Preserve earlier rounds as history rather than overwriting
them with the latest green result. A fixed number of rounds, a lack of comments
or author-only approval is not evidence of completeness.

| Round ID | Reviewed head, base tip and merge-base | Coverage and finding IDs | Corrective commits and validation | Independent verification | Disposition and next action |
| --- | --- | --- | --- | --- | --- |
| Not started | Pending P7a | No review claimed | None | Pending P7b | Planned; start after draft publication |

Each round ends with an explicit disposition: continue refinement with the next
finding/check, retain draft with the exact outstanding gate and required action,
or proceed to P7c when all exit criteria hold. Missing credentials, unavailable
platforms, failed CI and unresolved mandatory feedback do not justify bypassing a
gate or claiming completion. Accepted optional deferrals and independently
evidenced unrelated baseline issues need not expand this PR; they cannot waive
a required acceptance gate. Reconcile comments and CI after every push; do not
assume previously resolved threads or superseded runs still certify the result.
On a subsequent authorized continuation, feed new maintainer feedback and
post-handoff CI regressions through this same ledger and corrective loop.
Reopen affected P7 gates, restore draft status for mandatory failures, and
replace the stale handoff with a new revision-bound summary after verification.

#### Verify revision evidence and hand off

For each CI lane, record its run URL/ID, configuration, conclusion and actual
tested commit/tree, not just the PR head. Where CI tests a synthetic merge,
record that merge SHA and its base/head parents; distinguish this from a
head-only build. Recheck required CI against the current PR revision and target
base integration. A green check on an older commit, a queued run, an empty test
set, a cancelled/timed-out run or an unavailable platform is not a current pass.
Retain failed, cancelled, timed-out and superseded attempts with their run IDs,
reasons and replacement-run links; record whether the candidate or configuration
changed, and distinguish intentional cancellation from a test failure. Repeated
retries until one succeeds do not resolve an unexplained intermittent failure.
Classify the failure with evidence and keep an affected mandatory gate Pending
until its cause or safe
resolution is verified. A verified successful replacement can satisfy the gate
without erasing the earlier history. After a base change or corrective commit,
reopen affected findings and invalidated validation/review gates. Preserve
previous results as history.

For packaged validation, record the artifact name/checksum, producer run and
tested revision, platform/architecture, dependency/font identities and smoke-test
result. Verify the installed artifact rather than a source-tree executable;
retain the small manifests/reports needed to audit results before CI artifacts
expire. Redact private drawings and metadata before publishing review examples.
Record whether a run rebuilt the candidate or reused a cache/artifact, and
verify that reuse matches the intended inputs. A base advance requires renewed
integration evidence; it does not authorize an automatic rebase, force push or
base-branch update.

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
mandatory CI checks pass and no mandatory finding remains unresolved under the
rules above. Any lower-priority deferral must have an explicit rationale,
accepted scope and documented limitation.
Perform a final scope/commit review, whitespace check, installed-artifact check
and PR-description review; ensure plan, review ledger and PR state agree with
the final head. If a required gate remains unavailable, retain draft status and
hand off the exact blocker instead of claiming readiness.

Verify the PR is open, its base/head match the recorded candidate, it has no
merge conflicts and current base-integration evidence is passing. Conflicting
or unknown integration status keeps the PR draft; do not resolve it by rewriting
shared history or updating the base without authority. Inspect required
check/review status and ensure no newly arrived mandatory finding is untriaged.
Repository-required maintainer approval may remain a named handoff action;
ready for review must not be represented as ready
to merge. If a substantive fix is needed after the ready transition, return to
draft and repeat the affected validation/review gates before restoring readiness.

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
| Before draft PR publication | Frozen committed candidate, coherent scoped diff, owned branch, passing local slice checks, workflow/test-discovery audit, accurate pending-gate matrix, no unrelated/private/generated content |
| After each PR refinement round | Regression proof for fixes, affected suites/builds and performance budgets, CI with actual tested head/merge identity and retained failure history, refreshed finding/decision records and re-review of affected contracts |
| Before P7 ready-for-review handoff | Complete technical release checklist, final whole-diff independent review, no unresolved mandatory findings, current PR revision evidence and synchronized plan/PR; then complete P7 attestation |

Build success is not test execution. The existing focused text, headless bidi,
parser, shaper, registry, native-outline and conformance targets are `EXCLUDE_FROM_ALL`;
explicitly build and invoke them. Ordinary CTest does not currently register
those binaries, and registration of the broad application/integration suites
is configurable. Inspect `ctest -N`
and record what actually ran rather than inferring coverage from its exit code.

For example, in a configured single-configuration Unix CMake build with
`BUILD_TESTS=ON` and `LIBRECAD_ENABLE_OPENTYPE=ON`, run these existing targets
explicitly. Adapt executable paths for multi-configuration generators and
Windows; use the recorded Qt/toolchain configuration. These are future
validation instructions, not checks completed
by this plan update.

```sh
cmake --build /Users/dli/dev/LibreCAD/build-arabic-cmake --target \
  librecad_text_bidi_tests librecad_text_bidi_headless_tests \
  librecad_text_document_tests librecad_text_shaper_tests librecad_text_font_registry_tests \
  librecad_text_runs_tests librecad_text_runs_shaping_tests librecad_text_outline_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_bidi_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_bidi_headless_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_document_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_shaper_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_font_registry_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_runs_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_runs_shaping_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_outline_tests
```

Run the parser, shaper, registry, itemizer and native-outline targets again in a separate explicitly
configured `LIBRECAD_ENABLE_OPENTYPE=OFF` build; confirm the shaper/registry/integration
disabled-path cases actually execute rather than treating their smaller counts
as enabled coverage. The parser's backend-independent cases must still pass.
Extend this recipe as new headless targets are committed. When bidi APIs change,
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
- [ ] PR workflow coverage and actual test execution are verified; missing/skipped lanes are not counted as passing, and package evidence identifies the tested artifact and producer revision.
- [ ] Independent whole-diff reviews and corrective re-reviews record reviewer/coverage provenance and exact base/head revisions; all accepted mandatory findings are Verified, Rejected findings have reviewed rationale/evidence, and accepted lower-priority deferrals are documented.
- [ ] The P0–P6 acceptance map has no unexplained implementation, test or review gaps; refinement rounds and the PR tracking record identify the final reviewed candidate and outstanding maintainer actions.
- [ ] P6 technical gates are complete; required CI and installed-artifact evidence support the current PR revision, with synthetic merge identity or justified evidence carry-forward recorded where applicable.
- [ ] The current base/head have no merge conflicts and passing integration evidence; conflicting or unknown integration status is not represented as ready for review.
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
