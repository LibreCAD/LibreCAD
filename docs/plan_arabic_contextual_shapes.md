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
| PR workflow refinement date | 2026-10-13; #3099 remains an early draft, with P7a incomplete and P7b/P7c pending |
| State | Implementation in progress; baseline/probe and headless services under validation; entity rendering and release gates pending |
| Execution requirement | During authorized implementation, require intermediate scoped commits, explicit validation, plan updates, draft PR creation and revision-bound deep PR refinement; a plan-only update does not authorize implementation or publication |
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
Coordinate shared metrics with the [MTEXT indentation plan](plan_mtext_indentation_3032.md)
without absorbing that entire project into Arabic support.

## Corrections to the earlier approach

### Current-master integration and bounded repairs: 2026-10-13

This update starts at published head `648bbfcb4` and merges master `f94ca6eab`
without rewriting history. It incorporates #3090, #3091, #3098 and #3102;
private adapters and the primary checkout remain untouched. Concurrent merge
`d7457620e` is retained with its reviewed master export/reader semantics.
#3099 stays draft.

- Master's attribute lineage, edit/move/Undo export, pre-2018 flattening,
  angle/X-axis precedence and safe-save veto supersede the older attribute
  implementation in this branch. The veto runs before the optional staged-file
  verifier; failed traversals neither verify nor replace an existing file.
- Canonical-source state, output-width validation, scoped writer lifetime and explicit
  verified-write APIs remain. Legacy group-100 trailing XDATA is stored on its
  outer ATTRIB, as on master; marker provenance survives value operations.
  Both marker forms retain MTEXT continuation chunks, including on ATTDEF.
  Embedded objects have no independent DXF XDATA frame: as on master, the writer
  emits only the host's XDATA. Inert model capture/transport is still pending.
- CMake and qmake now default to OpenType **OFF** until application integration
  and release dependencies are ready. Explicit ON remains available and is
  tested separately; the exact certified-engine policy is not weakened.
- Informational parser notices are bounded without becoming fatal. Malformed
  input and actual resource limits still fail. Argumentless `\\N` and `\\X`
  breaks no longer consume subsequent text through a semicolon. Column and
  dimension-specific positioning are not implemented by this paragraph model.
- `librecad_text_native_tests` groups the six formerly uncovered native programs;
  the integration option registers their `text-native` CTest label. Linux and
  both Windows format lanes now build/run them. The isolated allocator program
  is never linked into Catch or the application. The two low-level format tests
  are no longer registered in default trees that do not build their targets.
- New project source/test files carry both LibreCAD.org and Dongxu Li copyright
  notices. Third-party font/license notices are unchanged.

Fresh default-OFF Release foundations pass 16/16 on macOS; default qmake
configuration and workflow lint pass. The enabled application builds; the
registered CTest run passes 31/31, including all sixteen foundations, six native
programs and both low-level format programs. The application test executable
passes 3,488 cases with 22 skips, using the repository's registered exclusions
(5,399,593 assertions). The separate DWG writer lane passes 417 cases / 16,514
assertions. Parser/codec ASan+UBSan checks pass (2/2, leak
detection disabled; external Qt/Catch libraries are not instrumented). Both new
parser regressions fail against `d7457620e`'s parser. This checkpoint does not close remaining
P5/P7, parser parity/control-layout, outline tolerance/precision, mutation,
certified-engine, packaging/license or independent Arabic visual-review gates.

### PR #3099 refinement: 2026-10-13

This round updates the published foundations at `c40da2c8c`, not the private
TEXT/MTEXT integration. The primary dirty checkout remains untouched. Current
master `7a8d24ddf` is merged without rewriting published history. Its refined
#3061 indentation, paragraph-command, tab and width contracts supersede the
older prerequisite implementation and tests retained in the initial draft.
Canonical-source APIs, empty-line border guards and scoped writer ownership
remain; staged DWG cleanup from master is retained alongside those guards.

The opt-in `librecad_text_foundation_tests` aggregate builds sixteen headless
executables without the application library. Register them with
`LIBRECAD_REGISTER_TEXT_FOUNDATION_TESTS=ON`, then run the `text-foundation`
CTest label. The separate allocation-failure executable stays isolated from
Catch. Native rendering, external Unicode conformance, GUI, package and slow
outline suites are not silently included in this bounded target.

Reproduction from a clean checkout with Qt6, Catch2 v3 and the enabled backend's
public development dependencies:

```sh
set -e
for backend in ON OFF; do
    build="build-text-$backend"
    cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_TESTS=ON -DLIBRECAD_ENABLE_OPENTYPE="$backend" \
        -DLIBRECAD_REGISTER_TEXT_FOUNDATION_TESTS=ON
    cmake --build "$build" --target librecad_text_foundation_tests --parallel 2
    ctest --test-dir "$build" -L text-foundation --no-tests=error --verbose
done
```

Fresh macOS/AppleClang 21, Qt 6.11.2, HarfBuzz 14.6.0 and FreeType 2.14.3:
ON passes all sixteen foundation executables and both registered format
executables (18/18); OFF passes all sixteen foundation executables (16/16).
The format suites retain 710 assertions/6 cases and 279/3 respectively.
The ON layout suite passes 715,036/16, shaper 639,014/7 and registry 128/7.
No certified-engine registry cases skip in that local ON run. OFF checks
backend-unavailable contracts, not equivalent shaping coverage. Workflow lint
passes. The fresh coherent native library and regression binaries build.
Native bidi passes 535,371 assertions/84 cases, source state 1,065/14, writer
lifetime 290/4, outlines 12,326/8, pen inheritance 39/4 and child replacement
55/4. The separate child-replacement allocator injector passes all six
precommit failures with zero postcommit allocations. qmake6 ON/OFF configure
and focused font/shaper compiles pass; ON also compiles merged MTEXT and the
corrected filter. This is not whole-PR, application-package or sanitizer
acceptance.

Commit `ba1719dcc` records the merge and CI target/discovery fixes. Its first
native run fails one master regression (535,287/535,288 assertions): the stale
draft guard rejects an invalid *unused* reference width. The corrective slice
validates the final serialized width instead and extends the unchanged master
case through same-filter DXF/DWG export and reopen. The complete native bidi
replay passes as recorded above; the initial failure is retained here rather
than erased. No private adapter or release-packaging work is included.

The new Ubuntu ON/OFF workflow uses system dependencies and records actual
engine versions plus verbose test/skip logs as artifacts. A different HarfBuzz
version intentionally skips certified installed-resource cases: such a green
job is system-engine regression coverage, not certified-profile acceptance.
DWG/DXF CI explicitly selects the legacy backend and builds both newly
registered format binaries before unfiltered CTest. Release-build defaults are
unchanged; missing release dependency and package coverage remains a blocker.

Outstanding gates are still the native adapters/consumers and persistence,
STYLE/ligature/paint-order findings, coherent fault/sanitizer gates, certified
platform dependencies, packaged resources/licenses and independent Arabic
visual review. Private work and historical failures are not promoted to passes.

### CI portability repair: 2026-10-13

The first hosted run of `e8521de01` exposes two compile failures before tests:
[Headless ON](https://github.com/LibreCAD/LibreCAD/actions/runs/38449735668/job/115477246318)
uses Qt 6.4's deferred `QStringBuilder`, whereas local Qt 6.11 accepted the
stored `auto` concatenations; calls to `.size()` fail. Materialize all stored
concatenations in the parser/bidi suites as `QString`, also avoiding references
to expired temporary operands. Do not disable `QT_USE_QSTRINGBUILDER` or remove
assertions. The separate
[Linux format job](https://github.com/LibreCAD/LibreCAD/actions/runs/38449735224/job/115477245064)
rejects the enum/integer conditional in the public layout-handle lookup under
`-Wextra -Werror`; cast the existing `NoHandle` sentinel to the declared return
type without changing its value or warning policy.

Both original errors reproduce from immutable `e8521de01` sources in disposable
Ubuntu 24.04/ARM64 with GCC 13.3, Qt 6.4.2 and the exact Catch2 3.8.1 gitlink.
The corrected public header passes `-Wall -Wextra -Werror -pedantic` there.
Incremental macOS ON18/18 and OFF16/16 pass with every assertion retained.
Linux ON16/16 and OFF16/16 pass. With system HarfBuzz 8.3, the four certified
installed-resource cases explicitly skip as designed; those skips are not
certified-profile acceptance. Both previously failing format test translation
units compile with their actual `-Wall -Wextra -Werror -pedantic` target flags.
Linux format validation is compile-only; both format runtime suites pass on
macOS. The unchanged Linux format core/full application is not rebuilt for this
bounded correction. Hosted retries remain pending. No package or sanitizer
acceptance is claimed; the primary dirty checkout is untouched.

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
650-unit ink height. FreeType's integer outline bounding box is
(-14, -3, 618, 647); the continuous quadratic contour reaches X=618.2.
Use analytic contour extrema, not the rounded integer box, for independent
ink-width fixtures; the fixed height normalization remains 9/647.
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

The initial native adapter's fixture contract uses left/center/right offsets
of 0, minus half the logical advance, and minus the full advance per explicit
line. Top and bottom use the block's profile extents; visual middle uses its
ink center, falling back to the profile box when no ink exists. TEXT HAMiddle
also uses ink-center X, falling back to logical center. Ordinary TEXT scales
by `(height * widthRel / 9, height / 9)` and MTEXT by `(height / 9, height / 9)`;
reference width does not become a glyph-width scale. Aligned TEXT uses the
anchor distance divided by logical advance uniformly; Fit uses that X scale
and `height / 9` in Y. These two modes supersede, but retain, stored widthRel;
preserve both anchors and input height, deriving angle and effective metrics
without mutating canonical data. Zero advance or coincident anchors fail
explicitly; whitespace with a positive advance is not an error. Explicit
multiline TEXT uses the shared block contract without automatic wrapping.
MTEXT's spacing factor multiplies profile pitch, with Exact/AtLeast passed to
shared layout. These are opt-in policy choices, not claims of legacy/CAD
metric equivalence. Characterize generation flags, anisotropic transforms and
reflection before P4 completion; do not silently ignore them. A temporary
explicit activation environment may exercise the integrated adapter while
imported-profile activation remains gated by P0 proof; that guard is not the
final production opt-in UI or completion of P4/P5.

For explicit multiline TEXT in Aligned/Fit, let `A` be the maximum logical
advance over all lines, including whitespace. The anchors bound `[0, A]` on
the first baseline, not necessarily a glyph endpoint or the longest line's
baseline. All lines use `sx = distance / A`; Aligned also uses `sy = sx`, while
Fit uses `sy = height / 9`. Keep each shared-layout origin `(0, baseline_i)`;
do not fit individual lines or rescale pitch again. Use baseline attachment
regardless of stored vertical alignment, preserving canonical anchors, height,
widthRel, angle and alignment. Positive X follows the first anchor to the
second without swapping RTL anchors. An empty first line is valid when another
line has positive advance. Zero block advance, coincident anchors or a
nonfinite/unrepresentable affine fail atomically, without an arbitrary epsilon
or ink-width fallback. This is an explicit opt-in multiline policy, not a claim
of single-line CAD equivalence. The independent analytic fixture uses advances
`[10, 20]`, baselines `[0, -12]` and anchors `(3, 4)` and `(33, 44)`:
Aligned line endpoints are `(3, 4)` to `(18, 24)` and `(27, -14)` to `(57, 26)`;
Fit at height 18 places the second line from `(22.2, -10.4)` to `(52.2, 29.6)`.
Also cover empty/whitespace lines, reversed anchors, overhangs and idempotence.

Initial decoration-v1 fixtures place underline at -1, overline at 9 and
strike-through at 4.5 nominal-cell Y, scaled by inline height and the complete
entity affine. Draw each painted cluster's advance once, including whitespace,
with first-logical-component paint/provenance; do not duplicate strokes per
ligature glyph. These explicit policy positions are not font-table decoration
metrics or a promise of mark avoidance. Stack-child transforms apply once.

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
- Native DWG embedded MTEXT has no independent common/EED frame. Use an outer
  ATTRIB extension-dictionary entry, provisionally `LibreCADEmbeddedText`, with
  a versioned [XRECORD full-model carrier](https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-24668FAF-AE03-41AE-AFA4-276C3692827F.htm); any compact registered XDATA locator
  is separate. Never activate embedded `LibreCADText` or RTL metadata as outer
  fields. Canonical embedded DXF uses [group 101 for the embedded-object marker](https://help.autodesk.com/cloudhelp/2017/ENU/OARXMAC-RefGuide/files/OREFMAC-__MEMBERTYPE_Methods_AcDbDxfFiler.html).
  Its native body precedes the encapsulating ATTRIB/ATTDEF XDATA; the independent
  [ezdxf reader](https://raw.githubusercontent.com/mozman/ezdxf/master/src/ezdxf/lldxf/extendedtags.py)
  ends embedded fields at the first 1001 and assigns the following application
  groups to the outer entity. Do not concatenate an independent embedded XDATA
  namespace into that stream or move outer XDATA before the native body.
  Retain embedded metadata in the explicit outer carrier even for 2018 DXF;
  legacy LibreCAD group-100 files require characterized, non-destructive read
  compatibility, not continued noncanonical output. Raw XDATA retention alone is
  insufficient: pre-2018 DXF/DWG writers cannot emit the native embedded body,
  so lossless downgrade also requires its complete canonical source, STYLE
  association, geometry, spacing, columns and retained unmodeled fields.
  Keep these output gates Pending with explicit failure until full-model
  transport is implemented and proven; do not silently flatten or omit it.
  Resolve native data versus retained-envelope precedence explicitly; edited
  fields cannot replay a stale capsule. Preserve unknown/duplicate envelopes
  opaque with a diagnostic, distinct from absence. Retain the 64 MiB in-memory
  budget without confusing it with a wire allowance: Autodesk documents a
  [16K total XDATA limit per entity](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-LT-AutoLISP/files/GUID-A94BC605-5517-437F-A6FE-D3EB8116A01A.htm)
  and [one group per APPID with 127-byte binary chunks](https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-A2A628B0-3699-4740-A215-C560E7242F63.htm).
  Larger EED frames or repeated APPIDs cannot bypass those interoperability
  limits. Frame the XRECORD payload explicitly and validate complete lengths,
  ordering, ownership and digest before publishing a decoded model. Rebind
  transient runtime owner IDs to the actual current attribute sequence; retain
  embedded STYLE/source-convention authority explicitly. A retained baseline
  must detect external changes to the outer fallback/native fields so stale
  embedded text or geometry cannot silently override them. Prove dictionary
  ownership, supported-version XRECORD transport, full-model restoration and
  independent DXF/DWG cross-format behavior before schema freeze. R12 lacks
  this carrier; its lossless downgrade remains a separate unfinished gate.
  This decision authorizes implementation, not profile activation.
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
and can separate a mark from its base. The pinned Amiri lam–alef `rlig`
substitution retains two GIDs and two HB clusters, with the second logical
cluster marked UNSAFE_TO_BREAK. Therefore two GIDs/clusters do not authorize two
independently editable policy units. Conservatively coarsen actual unsafe
logical-neighbor boundaries within the continuous shaping context, in addition
to grapheme/base–mark unions; this may freeze an entire joining word. Preserve
paint and source provenance inside the owned unit, and never merge independent
paragraphs/inline objects merely because visual positions overlap. Unsafe flags
are boundary safety information, not dedicated ligature-component identities;
do not infer grouping from glyph count or Arabic codepoint heuristics. The
conversion must also preserve the original leaf paint/emission order across
units, including safe RTL clusters. Source-logical order is not paint order,
and safe boundaries do not prove nonoverlapping ink. Keep components in their
original first-emission order. If grapheme/base–mark coarsening would join
noncontiguous emitted groups across unrelated paint, refuse letters conversion
before publication rather than silently moving that intervening paint. A native
unsafe boundary with an unproved predecessor similarly remains renderable but
must carry explicit unresolved coupling that refuses letters conversion, even
inside a whole inline object. Independent paragraphs and proved object/tab
boundaries remain separate. The initial implementation should choose
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
7. After the coherent P5 candidate passes its local gates, execute P7a as a
   separate delivery checkpoint: create or reuse the matching draft PR, attach
   it to this task and verify the published candidate against the PR creation
   checkpoint below. Keep P7b/P7c incomplete; draft publication alone does not
   complete delivery.
8. Execute P7b as a separate deep-refinement step. Independently review the
   entire PR diff and its acceptance contracts, reproduce actionable defects,
   and make scoped corrective commits with regressions and affected-consumer
   validation. Update this plan, the finding ledger and PR description after
   every round; obtain independent verification and repeat until the refinement
   checkpoint passes. P6 validation may proceed alongside this work, but pending
   release gates keep the PR in draft.
9. Execute P7c only after P6 and the refinement gates pass. Reconcile the exact
   published revisions, current feedback and validation evidence, verify the
   ready-for-review transition, and hand off the PR link and remaining maintainer
   actions. This step does not authorize merge, release or default activation.

If a required check cannot run, record why and keep that gate Pending or Blocked;
do not substitute compilation for rendering, a self-round trip for independent
format proof, or skipped tests for a pass. Independently validated work may be
checkpointed with the limitation recorded, but its milestone remains incomplete.
After a failed check, preserve the last passing checkpoint, diagnose the failure
and rerun affected checks after the fix. Do not weaken expectations, silently
regenerate golden files or reinterpret a saved profile to obtain green tests.

### Keep the plan current

The implementing agent must update this file after each slice, validation
failure, dependency/schema decision, PR refinement round, newly received or
reopened mandatory finding and material scope change, and before every handoff.
This is an in-task requirement, not a scheduled automation. Include the update
with the corresponding implementation commit whenever possible.

Before each checkpoint, push or handoff, reconcile the current candidate with
the last tested and reviewed inputs. Update affected contract states, findings,
validation records and the next action together; do not append a passing result
while leaving contradictory instructions or stale completion claims active.
Once a PR exists, refresh its tracking record and description after verifying
the published state. Preserve earlier evidence as history and explicitly mark
invalidated coverage; a local commit, attempted push or planned review is not
proof that the corresponding remote action succeeded.

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
| 2026-10-11 | P2a.2 consecutive CR and CRLF | Complete | `dd9fe4392` — `Preserve consecutive CR and CRLF paragraph boundaries` | Before-fix direct regression fails 2 != 3 paragraphs; final registered ON/OFF/ASan parser 453,636 / 25, itemizer/handoff and recursive-layout checks pass; enabled/disabled CMake applications and qmake pass; independent source/provenance review approves | Bounded parser prerequisite only; syntax-aware caret codec and full cross-format acceptance remain pending |
| 2026-10-11 | P0b transport proof | In progress | Uncommitted filter candidate; provisional codec is a separate service | Earlier STYLE binding failure repaired; new STYLE-unit and LTYPE-control prerequisites pass focused/direct native JSON checks; converted DXF still fails at DIMSTYLE color fields | Isolate/checkpoint prerequisites; codec/schema freeze and public activation require separate proof, not a self-round trip |
| 2026-10-11 | P3b.1 native outline adapter | Complete | `1ed63892a` — `Add precision-bounded native text contours` | Both numeric regressions fail before correction (exit 42); isolated ON/ASan: 12,326 assertions / 8 cases; OFF: 11,460 / 7; enabled/disabled CMake applications and qmake application build pass; independent numeric review approves | Bounded native service only; complete-affine entity consumption, layout caches and release geometry gates remain pending |
| 2026-10-11 | P5a.1 provisional profile codec | Complete | `ade6fee6f` — `Add inert versioned text profile codec` | Isolated QtCore-only ON/OFF/ASan each pass 4,544 assertions / 12 cases; enabled/disabled CMake applications and qmake application pass; strict QStringBuilder failure corrected and independently reviewed | Bounded inert codec only; owning bindings, STYLE/entity/filter integration, transport proof, schema freeze and activation remain pending |
| 2026-10-11 | P3c.1 shared explicit layout | Complete | `7440ffc3c` — `Add source-mapped recursive OpenType text layout` | Source-provenance regression fails before correction; isolated ON/ASan: 715,036 assertions / 16 cases; OFF: 26 / 1. Registered include failure corrected; enabled/disabled CMake applications and qmake application pass; independent source/registration review approves | Bounded recursive service only; native entity adapters, caches, complete-segment fallback and independent application acceptance remain pending |
| 2026-10-11 | P5a.2 owning profile bindings | Complete | `a8ade2677` — `Add owning text profile edit bindings` | Move, retained-buffer alias and copy-allocation regressions reproduced and repaired; registered ON/OFF/ASan each pass 8,150 / 22 plus both controlled allocation-failure cases; enabled/disabled CMake applications and qmake application pass; independent ownership/registration review approves | Bounded inert binding checkpoint only; transport/entity/style integration and schema/activation gates remain pending |
| 2026-10-11 | P5a.3 entity and transport authority | In progress | Uncommitted owned entity/style/filter slice | Source/profile host APIs and guarded unavailable-import state under implementation; whole-slice registered validation and independent review pending | Complete source/STYLE/namespace ownership and lifecycle tests; syntax-aware lossless cross-format caret conversion remains required, not waived by a guarded intermediate export error |
| 2026-10-11 | P5a.3a entity source and profile state | Complete | `2db2a26fb` — `Preserve canonical text and inert profile state on entities` | Final registered ON/OFF/ASan each pass state 322 / 7, native entities 912 / 12 and focused legacy text/bidi 541,335 / 70; enabled/disabled CMake applications and qmake application pass; independent source/registration and pre-alignment API/test review approve | Bounded host API checkpoint only; filter/STYLE transport, native renderer activation and full P5 acceptance remain pending |
| 2026-10-11 | P5a.3b syntax-aware source conversion | Complete | `1636de0ce` — `Add lossless text source convention conversion` | Private strict ON/OFF/O1 sanitizer and final registered isolated ON/OFF/ASan each pass 25,398 / 15; parser/itemizer/handoff/layout regressions and CMake ON/OFF/qmake application builds pass; independent source/registration review approves after test-expression ownership repair | Bounded service checkpoint only; filter integration, Qt 6.4/platform lanes and native cross-format acceptance remain pending |
| 2026-10-11 | P4 shared native entity integration | In progress | Uncommitted owned adapter slice | Alignment/spacing fixture policy specified; actual TEXT/MTEXT update dispatch, recursive native geometry and guarded resource/profile resolution under implementation | Validate integrated updates, transforms, metrics, lifetime and all required modes; helper tests alone cannot complete P4 or enable public imported profiles |
| 2026-10-11 | P7 execution | Planned | `55cbfc54f` and `537839cfe` — plan-only workflow checkpoints | Draft baseline, creation checkpoint, acceptance map, deep-review cadence and CI history defined; prepublication findings retain provenance and require local-contract resolution before publication. Independent plan review and scoped whitespace checks pass; no publication or PR acceptance claimed | Complete the coherent P5 candidate, reconcile existing review findings, then execute P7a and keep the records below current |
| 2026-10-11 | P7 workflow refinement | Complete | `c956d8ca2` — `Link PR feedback to verified refinement` | Independent read-only plan review and scoped whitespace check pass; explicit P7a/P7b/P7c steps, source-linked feedback and final state reconciliation defined | Plan-only checkpoint; P7 execution remains Planned, with no push, PR creation, review completion or readiness claimed |
| 2026-10-11 | P7 execution clarification | Complete | `9e50184ff` — workflow record in capsule checkpoint | Independent read-only review and scoped whitespace checks pass; explicit creation/refinement/handoff sequence, refinement completion checkpoint and cross-format lifetime finding recorded; Pending P6 keeps handoff incomplete and the PR draft | Plan workflow only; P7a/P7b/P7c remain Planned; resolve mandatory P5 findings before authorized publication; no PR action performed |
| 2026-10-11 | P5b.1 raw embedded metadata capsule | Complete | `9e50184ff` — `Preserve exact embedded text metadata capsules` | Independent source review approves; isolated registered ON/OFF/ASan each pass 21,535 assertions / 5 cases; enabled/disabled CMake applications, coherent sanitizer application library and qmake application build exit 0 | Bounded in-memory retention checkpoint only; full-model wire transport, filter integration and activation remain pending |
| 2026-10-11 | P5 filter writer lifetime | Complete | `37d2c614d` — `Clear export writer observers on every exit` | Coherent baseline observer regression fails 5 / 108 assertions; final registered ON/OFF/ASan each pass 290 / 4; enabled/disabled CMake applications, coherent sanitizer library and qmake application build exit 0; independent production/test/registration review approves; committed source/test identities match the validated candidate | Bounded writer ownership/version/retry checkpoint only; raw-aware STYLE/filter and embedded transport acceptance remain pending |
| 2026-10-11 | P4 atomic geometry prerequisite | Complete | `883aec664` — `Publish native child geometry atomically` | Coherent registered ON/OFF/ASan each pass 55 / 4, six injected C++ allocation failures, pen 39 / 4 and bidi 541,335 / 70; enabled/disabled applications, coherent sanitizer library and fresh qmake application build exit 0. Independent source/test/registration re-review approves | Bounded atomic replacement checkpoint only; native adapter, durable explode transaction and full P4/P5 remain incomplete |
| 2026-10-11 | P7 review gate maintenance | Complete | `10858ab25` — `Refine Arabic PR review gates and validation recipe` | Independent complete-diff plan review, readback, shell-recipe syntax and scoped whitespace checks pass; lifetime commit identities verified; explicit-run recipe and final PR state corrected; transactional scene-integrity and matching-header review gates defined | Plan-only checkpoint; P7a/P7b/P7c remain Planned, with no push, PR creation or readiness claimed |
| 2026-10-11 | P4 pen inheritance prerequisite | Complete | `d006e580d` — `Resolve native text pens through live ancestors` | Exact-fixture baseline fails 30 / 39 assertions; coherent registered ON/OFF/ASan each pass 39 / 4 and focused text/bidi 541,335 / 70; CMake ON/OFF applications, coherent sanitizer library and qmake application build exit 0; independent source/test review approves | Bounded live pen inheritance checkpoint only; native adapter, atomic replacement and full P4 remain incomplete |
| 2026-10-11 | P7 publication and refinement plan | Complete | `883aec664` — plan record in atomic geometry checkpoint | Independent workflow/native reviews approve the plan diff; transport review finds no mandatory gap. Plan readback, scoped whitespace check and shell-recipe syntax check pass; private finding counts and frozen source identities reconciled | Historical plan review only; later findings below still govern publication. P7a/P7b/P7c remain Planned. No push, PR creation, readiness or whole-feature acceptance claimed |
| 2026-10-11 | P7 persistence review coverage refinement | Complete | `de670d842` — plan record in inert-model checkpoint | Independent final-diff review approves; scoped `git diff --check` exits 0; changed ledger table structure, frozen source/patch SHA256 and resolved atomic commit identity verified | Plan-workflow checkpoint only, not PR acceptance. Findings were Open at the original plan review; follow their current dispositions below. P7a/P7b/P7c remain Planned; no push or PR action performed |
| 2026-10-12 | P5b.2 inert retained-model codec | Complete | `de670d842` — `Preserve inert embedded text model fields` | Root registered ON/OFF/ASan each pass model 102,708 / 13, raw capsule 21,535 / 5, profile 4,544 / 12 and binding 8,150 / 22; CMake ON/OFF applications, coherent sanitizer library and qmake application exit 0. Independent four-file and build-registration reviews approve; staged and committed file identities match the tested snapshot | Bounded inert-service checkpoint only; native capture/restoration, full wire transport, STYLE graph authority, schema freeze and activation remain required |
| 2026-10-12 | P5a.3c STYLE profile authority and ownership | Complete | `c8b5a193c` — `Own text STYLE provenance and profile authority` | Root coherent current-HEAD-plus-STYLE ON/OFF/ASan each pass STYLE 286 / 7, profile 4,544 / 12, binding 8,150 / 22, entity-state 322 / 7, model 102,708 / 13, raw capsule 21,535 / 5, bidi 541,335 / 70 and writer lifetime 290 / 4. CMake ON/OFF applications, coherent sanitizer library and qmake application exit 0. Independent source/test/registration review approves; five existing-API baseline checks fail before and pass after. Staged and committed source/build inputs byte-match the tested candidate | Bounded STYLE/list authority checkpoint only; native STYLE transport, activation and overall P5 acceptance remain pending |
| 2026-10-12 | P7 PR creation and deep-refinement protocol | Complete | `c8b5a193c` — plan record in STYLE checkpoint | Independent workflow review; premature certification wording corrected; scoped whitespace, shell-recipe syntax and readback checks pass | Plan-only protocol checkpoint: draft creation, clean-candidate rehearsal, durable review evidence, PR description, merge-base scope audit and revision-bound refinement closure defined. P7a/P7b/P7c remain Planned; no push, PR creation or readiness action performed |
| 2026-10-12 | P4a/P4b canonical affine integration | In progress | Uncommitted guarded native/entity/test candidate plus owned build registrations; revised failure-test identity below | Guarded ON/OFF/ASan applications and 23 preceding executables finish; native 135,009 / 28 passes. OFF failure checks pass; ON exits 53 at the insufficient test ceiling, not a publication invariant failure. Original ASan standalone likewise has only four completed update contexts, without a retained terminal exit. Guarded qmake no-op completion check exits 0. Independently reviewed calibrated exhaustive test is integrated; its matching-header private ON matrix is running | Obtain complete calibrated fault results and coherent current-source acceptance before the bounded checkpoint. Native transport and overall P4 remain incomplete |
| 2026-10-12 | P7 publication identity and PR reuse clarification | Complete | Included in this source-state checkpoint; historical plan-only update | Independent workflow review approves the exact source/target identity and authority clarifications; draft-reuse ambiguity corrected and independently re-reviewed. Readback, changed-table structure, scoped whitespace and all eight shell-recipe syntax checks pass | P7a/P7b/P7c remain Planned; no push, PR creation or readiness action performed. Execute publication only under an authorized implementation/PR request |
| 2026-10-12 | P7 PR presentation and refinement sign-off | Complete | Included in this source-state checkpoint; historical plan-only update | Independent final workflow review approves explicit scoped dispositions, candidate-accurate presentation, stale-remote reconciliation and the distinct qualified Arabic-reading lane. Readback, all 22 table structures, balanced fences, all eight shell-recipe syntax checks and scoped whitespace checks pass | P7a/P7b/P7c remain Planned; implementation and PR acceptance are not claimed. No push, PR creation or readiness action performed |
| 2026-10-12 | P7 execution linkage and draft-checkpoint consistency | Complete | Included in this source-state checkpoint; historical plan-only update | Independent review approves explicit P5-to-P7 delivery, adversarial public admission checks, separate P6 execution gates and the reused-PR draft-checkpoint correction. Readback, all 23 table structures, balanced fences, all eight shell-recipe syntax checks and scoped whitespace checks pass. Current unaccepted block integration and source-evidenced `P5-BLOCK-OWNER-001` are reconciled | P7a/P7b/P7c remain Planned; the ownership finding was Open at this plan-only checkpoint; its current disposition is recorded below. No implementation, push, PR creation or readiness action performed by this plan-only update |
| 2026-10-12 | P7 explicit creation and refinement steps | Complete | Included in this source-state checkpoint; historical plan-only update | Main workflow separates P7a draft creation, P7b deep refinement and P7c verified handoff. Independent delta review approves; readback, all 23 table structures, balanced fences, all eight shell-recipe syntax checks and scoped whitespace checks pass | Existing gates, intermediate commits and per-round plan updates remain required; P7a/P7b/P7c remain Planned. No PR action authorized or performed by this update |
| 2026-10-12 | P5a.3b current compatibility input — bounded checkpoint | Complete | `4f8c8de61` — `Preserve current text compatibility input`; parent `c8b5a193c` | Existing-API before regression fails 4 / 18. Exact three-file candidate passes private normal/O1 ASan and fresh strict disabled-flag state-only suites, each 420 / 9. Coherent ON/OFF/ASan applications plus all nine suites and source-manifest checks pass in each lane: 680,333 assertions / 156 cases. Fresh qmake application build and dependency recheck also exit 0. Independent source/test and bounded-checkpoint review approve; committed source identities match the tested snapshot | Bounded source ownership/no-op checkpoint only. Current-source restoration, carriers, native/filter integration and activation remain unfinished; this does not complete P5 |
| 2026-10-12 | P5 durable block owner admission | In progress | Two-file repair integrated into the uncommitted prepared-block slice | Actual coherent before selector fails 2 / 53 across 2 cases; repaired selector 53 / 2, focused 336 / 22 and standalone block fault matrix pass. Combined ON/OFF/ASan applications and all 22 non-fault suites now pass; qmake application/dependency recheck passes. Independent peer/root source review approves | Sole exhaustive native allocation matrix still running; scoped combined checkpoint remains pending. Native explode/save/reopen and overall P5 remain unfinished |
| 2026-10-12 | P5b.3 retained native bridge | In progress | Private reviewed candidate only; not integrated or committed | Final private normal and full-native-source ASan/UBSan harnesses each pass 540 / 9 after correcting the fixture's default-layer handle oracle. Independent complete native mapping review approves the bounded capture/restoration delta | Integrate only with actual filter consumption and coherent application/build-system validation; current source/profile/STYLE, owned carrier and observed target baseline remain separate unfinished gates |
| 2026-10-12 | P5 current source materialization | In progress | Frozen private proposal `librecad-current-source.gsR4Tn`; not integrated or committed | Strict forced normal, disabled-flag and O1 ASan/UBSan builds/runtimes all exit 0; each configuration passes mapper 331 / 10 and existing model 102,708 / 13. Root source/API/test review and all 21 frozen-input checks approve the bounded proposal | Actual filter/host consumption, immutable original native/raw history, unique STYLE association, full root/carrier/graph and observed target-baseline validation remain required. No application, wire transport, activation or P5 acceptance |
| 2026-10-12 | P5 observed target baseline | In progress | Frozen private proposal `librecad-observed-native.yoKlRf`; not integrated or committed | Strict normal and whole-native-source O1 ASan/UBSan unfiltered runs each pass 109,348 / 29; actual typed DWG2013 outer-only and DWG2018 embedded rereads, local aggregate bounds and invalid-version before/after regressions included. Root source/test/hash review and final source-manifest verification pass | Local capture/validation only. Actual filter consumption, target provenance, full root, carrier graph, STYLE authority, current-header application validation and activation remain required; retained failed sanitizer invocations are recorded below |
| 2026-10-12 | P5 owned carrier framing and local graph checks | In progress | Frozen private proposal `librecad-embedded-carrier.FQfVV6`; not integrated or committed | Strict normal and whole-native-source O1 ASan/UBSan unfiltered suites each pass 147,724 / 38; typed DWG2013/2018 and ASCII/binary DXF2013/2018 transport included. Independent source review finds two local graph defects; literal before regression fails 26 / 2,819, both repaired. Final 421-file manifest and private dependency checks pass | Bounded carrier/value-stream and controlled-fixture links only. Complete inventory is a caller precondition; arbitrary drawing/raw-reference authority, actual filter consumption, target agreement, full root/current/STYLE restoration, atomic save and activation remain open |
| 2026-10-12 | P5 carrier and observed-target admission | In progress | Frozen private proposal `librecad-carrier-admission.Sh5lLK`; not integrated or committed | Strict normal and whole-native-source O1 ASan/UBSan unfiltered suites each pass 166,588 / 44. Exact Top6, intended complete-envelope, absent/body/version/format and byte-budget negatives pass; six actual typed two-pass fixtures retain stable links and baseline. Complete source and additive test reviews approve; 423-file manifest checks pass | Known model access requires every local admission check. Fixture callback inventories deliberately remain Incomplete; actual whole-drawing inventory, filter consumption, STYLE/current/history restoration and atomic destination publication remain open |
| 2026-10-12 | P5 immutable fresh embedded import history | In progress | Frozen private proposal `librecad-history-bounded.1y6GN1`; not integrated or committed | Strict normal, disabled-helper and whole-native-source O1 ASan/UBSan lanes each pass History 143 / 9, State 420 / 9, Current 331 / 10, model 102,708 / 13 and retained native 540 / 9. STYLE budget regression fails before and passes after preflight repair; root source review and frozen input checks approve | Fresh embedded ATTRIB only. Actual selective filter integration is separate; no loaded-carrier factory, STYLE catalog proof, saved-profile activation or application acceptance is established |
| 2026-10-12 | P5 verified native destination publication seam | In progress | Reviewed native API subset integrated additively but uncommitted; private merged canonical101 closure `librecad-verified-canonical.juU83Q` | Original fZwPVN strict normal/whole-native ASan each pass 175,314 / 56. Exact canonical101 merge also passes normal complete-manifest recheck and fresh ASan, 175,314 / 56 each; all 424 source identities verify. Independent shared integration review approves | Utility checkpoint below is separate from native APIs. Actual complete inventory, frozen current/STYLE/topology, both rereads, full intended payload, coherent application integration and activation remain required |
| 2026-10-12 | P5 verified owned-stage utility — bounded checkpoint | Complete | `83f0f4b27` — `Verify owned output stages before publication`; exact helper h/cpp, utility test, 14 CMake target lines and plan only | Fresh archive of parent `293670bca` plus only this subset passes CMake ON/OFF/O1 ASan, each 93 / 9; qmake native-library build/dependency recheck and all 5,829-source checks pass. Both strict private TUs and sanitizer link audited; independent bounded checkpoint review approves exact source/test/registration and limits | Owned-stage seam only. No native API, canonical101, block/filter, full application, Windows, stream exception-mask or P5 acceptance implied; no activation |
| 2026-10-12 | P5 actual DWG read-session inventory | In progress | Final private proposal `librecad-read-inventory-receipt.6NZC4J`; not integrated or committed | Original compile exit 2 and next full runtime exit 42 are preserved. Independently reviewed reference-binding/receipt-authority repairs now pass fresh strict normal and whole-native-source O1 ASan, each 167,943 / 52; all 426 source identities verify. Actual controlled outer-only DWG2013/2018 fixtures reach receipt-backed Carrier/Admission Known | DWG-only bounded common-link/fixed-dictionary inventory, not a universal body-reference graph or all embedded bodies. DXF collector/finalized inventory, filter consumption, detached loaded restoration and save publication remain unfinished |
| 2026-10-12 | P5 fresh embedded filter consumer | In progress | Original revisions retained; reviewed owner freeze cgivVy and coherent source `librecad-history-consumer-final.TpAEe7` | Original17464 builds application/all31 targets0, then exits42 on unset selection-fixture flag. SSJCiX93271 fixture-only compile exits2 at protected API. Correct public flag/list GgaMI2/94753 passes fresh fixture743 / 13 and three unchanged helper suites, then exits42 at unchanged scopeguard4,984 / 16 with12 failures/8 cases; listed source/input-after checks0, later26 suites unrun | Old5,833-file manifest omits17 hidden non-C++ files. Six failures are missing fixture reads; six modern exports expose the STYLE gap and missing actual STYLE records. Preserve success expectations and add exact assets/catalog. No coherent runtime, full carrier, loaded restoration or activation acceptance |
| 2026-10-12 | P5 native letters and normal explode | In progress | Private nJ11IJ/failed rPuxGU preserved; narrow Csh2Qp repair and separate continuations, not integrated or committed | Original fixture compile exits 2; initial six-suite replay has 1,300,193 / 95 with two failures. Incremental40131 exits 2 at missing external-header receipt;214BnJ/34434 exits 2 at protected test API. Fixture-only G6nym3/29629 terminal 0:1,385,240 / 96, separate mark34,883 / 1; all25 fresh objects/six links and10,571-input-after checks pass | The initial single-group expectation was wrongly dismissed from two-GID evidence alone: actual rlig/unsafe-boundary trace confirms a separate mandatory grouping defect. Current two-group diagnostic does not close letters acceptance. Fresh application/OFF/ASan/fault/serialized and current-authority gates remain required |
| 2026-10-12 | P5 independent canonical embedded DXF boundary | In progress | Private normal e2Dv8i and sanitizer p9dA3K; exact matching reviewed native archives reused, one fresh strict test TU per lane | Sessions90905/78893 both terminal 0:710 / 6 each; actual instrumentation/link audited, identical paired output; ezdxf semantic checks and paired-output audit errors0/fixes0. Upstream424/own11 before/after checks pass; independent bounded reviews approve | Bounded ASCII DXF2018 group101/body/outer-XDATA check only, not fresh production/application build. Full-model carrier, whole-filter save/reopen, DWG/pre2018 and broader `P5-EMBEDDED-DXF-001` acceptance remain open |
| 2026-10-13 | P5 canonical native DXF checkpoint | Complete | `54d5adb7f` — `Preserve canonical embedded DXF scope`; exact parent737f plus eight tested source/build paths and plan | Fresh ON40185/OFF9829/whole-native ASan29388 all exit0:710 / 6 and CTest each;38 fresh project TUs/87 private dependencies/exact36 native members audited. Owned qmake14514 native build/recheck and all5,833-source/direct-input checks0. Existing-API probe fails6 / 11 before, passes17 / 17 after; paired ezdxf audit errors0/fixes0. Independent review approves; all committed blobs match tested inputs | Bounded checkpoint only; verified APIs and unrelated WIP excluded. Original shallow qmake56680 retained, not owned-shadow acceptance. No application, full filter/carrier, DWG/pre2018, activation or P5 completion |
| 2026-10-13 | P5 native verified writer checkpoint | Complete | `996440cb4` — `Verify native DXF and DWG saves before publication` (`P5-NATIVE-VERIFIED-CP-001`); seven source/build paths plus plan | Fresh ON22725/OFF97138/whole-native ASan42247 each exit0:279 / 3 plus CTest. Exact38 fresh TUs/88 private dependencies/native36-member archive/link checks pass. Owned qmake74039 builds36 native TUs and passes no-op recheck; complete5,834-source/six-direct-input before/after checks0. Root/peer approve bounded source and corrected evidence scripts; all eight committed blobs and parent54d match the reviewed checkpoint | Per-call native LINE oracle only, not full model/STYLE/carrier fidelity. Callback sees flushed/closed owned stage; rejection/throw preserves prior destination. Application, two-pass filter save, Windows, activation and P5 completion remain open |
| 2026-10-12 | P5 native DXF source observer | In progress | Private `librecad-dxf-observer.G6LIgu`; reviewed nine-path seam, not integrated or committed | Original fresh normal29839 and whole-native ASan/UBSan16714 both terminal0:168,404 / 63 each, including11 observer cases. Actual48-TU/dependency/archive/link audits and428-source/446-normal or447-sanitizer-input before/after checks pass; root independently rechecks both audits | Native warning policy unchanged/helper12strict; external libraries uninstrumented, leaks disabled only. Observer events do not bound prior native extraction allocation or establish collector grammar, Complete/whole-graph, filter, application/qmake or activation acceptance |
| 2026-10-13 | P5 neutral DXF inventory collector | In progress | Frozen private `librecad-dxf-inventory.AccI1m`; four-path source-only proposal | Root reads the complete1,021-line delta and nine authored cases. Tiny modern source grammar accounts for callback-less SEQEND; DXF rows stay separate from DWG rows. Source review requires temporary-alias byte preflight before allocation | Separate successor requested; unbuilt/unrun. Complete inventory must not imply unique carrier ownership or DXF Carrier Known. Full tables/blocks/embedded bodies and Carrier adaptation remain pending |
| 2026-10-13 | P5 DXF inventory allocation preflight repair | In progress | Frozen private `librecad-dxf-inventory-preflight.zWvzRh`; two-path successor, source only | Root and independent peer approve direct borrowed-value sizing before owning copies and native appData alias allocation. Complete429-source identities verify; ten authored cases include actual2,048-value/reactor N and N−1 limits | Unbuilt/unrun; fresh normal/sanitizer driver under preparation. Existing DWG helpers are unchanged. Neutral DXF Complete is not Carrier Known or unique ownership; footer-alias rejection remains a future Carrier-adaptation gate |
| 2026-10-13 | P5 DXF inventory first fresh validation | In progress | Exact zW source in `librecad-dxf-collector-driver.Bxtz0l`; original normal36473/ASan58176 retained | Both configure/build steps exit0 with49 fresh TUs. Both drivers exit2 before runtime: actual dependency audit refuses selected Qt's resolved `qtbase/6.11.2_1` headers outside the declared Qt meta-package root. All429-source/26-direct-input before/after checks0 | No runtime acceptance or successful complete build audit. Independently verify the selected Qt module relationship and use a distinct narrowly corrected driver; do not broaden the allowlist to all Homebrew or overwrite these runs |
| 2026-10-13 | P5 DXF inventory corrected-provider validation | In progress | Source-identical `librecad-dxf-collector-qt.4exo5A`; original normal61763/ASan31332 retained | Normal terminal0:168,934 / 73 unfiltered and530 / 10 expanded. Both lanes freshly compile49 TUs and pass actual dependency/archive/link audits; ASan unfiltered also exits0:168,934 / 73, but expanded run aborts−6 during Catch teardown after its printed530 / 10 footer. All429-source/50-direct/104-artifact after-checks0 | Sanitizer lane terminal2, not accepted. Isolate the framework-only failure without disabling container checks; a printed assertion footer is not process success. Neutral inventory only: no unique carrier ownership, whole graph, filter or activation |
| 2026-10-13 | P5 isolated test-framework sanitizer diagnostic | In progress | Frozen `librecad-catch-compatibility.FXQ12O`; original51412 retained, no production source linked | Fresh strict instrumented C++ test compile/link0; unfiltered0:3 / 1. Expanded `[compatibility] --success` aborts134 with ASan container-overflow during Catch configuration, before test execution. Clean environment; ten input and six built-artifact before/after checks0; lane terminal1 | A framework-only reproduction, not proof that the original teardown failure is identical or harmless. Exact installed Catch3.16.1 archives remain uninstrumented. Rebuild the exact version coherently with instrumentation and rerun without container-detector suppression; collector sanitizer acceptance remains open |
| 2026-10-13 | P7a user-requested early draft publication | In progress | [PR #3099](https://github.com/LibreCAD/LibreCAD/pull/3099), created from committed79d1c297a; only committed work published | Explicit user publication request; owned origin branch push0. Browser creation, open/draft state, exact source/target/head, task attachment and all100 published filenames verified independently through GitHub reads. Initial40-commit scope and target/merge-base recorded below | Sequencing exception only, not a coherent P5 candidate or P7a acceptance. P5 implementation/findings, master conflicts, CI discovery/dependencies, portable evidence and final validation remain open; no reviewer request, readiness, merge or release |
| 2026-10-12 | P5 current STYLE catalog materialization | In progress | Frozen private `librecad-style-materialize.7PRrg9`; ten-path source-only proposal, not integrated or built | Complete root/peer source review requires changes for old-name fallback resurrection and independent header/legacy/dimension references. Sealed original catalog follows the live style list; one detached current emission plan resolves body and outer references independently | Separate source successor required. Header ABI changed: no old Tp archive acceptance. Add matching registrations, exact fixture assets/actual STYLE records and reviewed unsafe/paint repair before fresh coherent builds. No full STYLE history wire graph, carrier/loaded restoration or activation |
| 2026-10-13 | P5 current STYLE consumer successor | In progress | Frozen private `librecad-style-successor.HcEKWL`; fourteen cumulative paths, source only | Root and independent peer approve the bounded source successor; all5,856 source paths/hashes verify. Pre-open legacy/dimension/header checks prevent stale aliases; literal regressions retain DXF/DWG actual STYLE references and prior-file preservation | Unbuilt/unrun. Combine full inherited letters prerequisites and reviewed unsafe/paint repair, register catalog sources/tests, and rebuild all matching ABI consumers. Outer-only opaque-profile unchanged-save policy and full carrier remain pending |
| 2026-10-13 | P4/P5 coherent STYLE and letters assembly | In progress | Frozen private `librecad-style-letters-combined.GYQ5Hq`; nineteen-path delta from Hc, source only | Root/peer approve complete2,674-line source delta and closure; all5,862 source identities verify. Full seventeen-path letters prerequisite union, catalog registrations and committed canonical scope test are included; all fourteen Hc-owned files remain exact | Unbuilt/unrun. Application-plus35-target driver held for a distinct reviewed host-common successor; both after-checks must run independently on failure. No stale ABI archive, allocator duplication, activation or completion claim |
| 2026-10-13 | P5 unchanged outer-profile import-baseline prototype | In progress | Frozen private `librecad-outer-unchanged.LNyJ6L`; eight-path source-only delta from Hc | Root reads the complete814-line delta and actual import/common/export/STYLE/native-writer contexts; all5,856 identities verify. Private owned common baseline and both unchanged STYLE associations narrow the proposed exception. Review finds a writer-capability admission gap and three test-only compatibility-input/oracle corrections | Unbuilt/unrun and held; no integration approval. A deep original DTO copy does not prove wire fidelity. Preserve this freeze and require a distinct supported-domain repair, literal refusal cases and full-native target readback; no saved activation |

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

### Consecutive paragraph boundary evidence

Caret-codec testing exposed a parser defect: physical `A\r\r\nB` produced two
paragraphs instead of three, unlike its equivalent `A^M^M^JB` spelling. Eager
physical CRLF folding fed LF into the existing decoded CR/LF coalescer, allowing
that pair to consume a preceding independent CR. The narrow repair sends each
physical control through the same coalescer exactly once. It changes no public
header, saved source or legacy renderer.

The direct before-fix fixture fails at 2 != 3 paragraphs (1 failure / 2
assertions). Seventeen independent fixtures now check physical, caret and
Unicode-escape mixtures, exact break/token/source ranges, intervening empty
paragraphs and complete provenance. Independent source review confirms the
contiguous-source and immediately preceding break-token conditions prevent
merging across intervening content. Private strict and O1 ASan/UBSan parser
runs pass 453,636 assertions / 25 cases. Logs remain
`/private/tmp/librecad-arabic-parser-crcrlf-before-tests.log`,
`/private/tmp/librecad-arabic-parser-crcrlf-after-{build,tests,focused-tests}.log`
and `/private/tmp/librecad-arabic-parser-crcrlf-sanitized-{build,no-leaks-tests}.log`.
A separate `detect_leaks=1` attempt aborted as unsupported on macOS before
tests; it is not a code regression or a leak pass.

Only the two staged parser/test files were exported and compared against
`/private/tmp/librecad-arabic-core-index.TOns5t` after the entity checkpoint.
Registered ON, OFF without FreeType and ASan/UBSan each pass parser 453,636 /
25 and itemizer 208,140 / 16. Enabled ON/ASan handoff and layout pass 441 / 3
and 715,036 / 16; OFF passes the explicit disabled paths, 1 / 1 and 26 / 1.
The source changes do not alter bidi resolution, profile codecs or native
outline interfaces. Complete enabled/disabled CMake applications and qmake
also build; the coherent ASan application library is rebuilt, without claiming
a new sanitizer GUI application run. Processes 77553, 29434, 24902 and 38955
terminate with exit 0. Build the parser, itemizer, shaping-handoff and layout
targets explicitly and invoke their unfiltered binaries. Root logs are
`/private/tmp/librecad-arabic-parser-crcrlf-root-{on,off,asan}-{build,parser-tests,itemizer-tests,handoff-tests,layout-tests}.log`
and `/private/tmp/librecad-arabic-parser-crcrlf-root-qmake-build.log`.
The entity evidence's macOS toolchain, isolated configurations and
sanitizer environment/exclusions apply. Syntax-aware conversion, native entity
consumption and independent saved-file acceptance remain separate gates.

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

### Provisional profile codec evidence

P5a.1 adds a QtCore-only typed profile, strict isolated-namespace codec and
explicit inheritance resolution. Unknown schema/backend/fields and invalid raw
records remain inactive and retained; entity omission, explicit legacy opt-out,
ByStyle and marker-1 conflict have distinct authority. This is an inert
prerequisite, not a frozen public schema, file import activation or font/engine
certification. Its low-level raw-retention API requires caller storage to remain
alive and unmodified through aliases; entity bindings must own their snapshots.

Strict compilation with the repository's `QT_USE_QSTRINGBUILDER` flag initially
exits 1 with 13 diagnostics: deferred concatenation cannot implicitly enter
QVariant or mixed initializer lists. Materializing QString values corrects the
production encoder and test setup without changing expectations. The corrected
strict build passes 4,544 assertions / 12 cases. Initial and corrected logs are
`/private/tmp/librecad-arabic-profile-root.vKQ3JG/{build,build-retry,tests}.log`.

At parent `537839cfe`, only the five codec/test/build inputs were staged and
exported to `/private/tmp/librecad-arabic-core-index.TOns5t`; byte comparisons
exclude pending layout, bindings, filter and prefix/package changes. Existing
macOS arm64/Apple Clang 21/Qt 6.11.2/Catch2 3.16.1 builds were reconfigured with
`BUILD_TESTS=ON`: ON, OFF with `CMAKE_DISABLE_FIND_PACKAGE_Freetype=ON`, and ON
with cached address/undefined sanitizers plus frame pointers. Each explicitly
builds and invokes `librecad_text_profile_tests`, passing 4,544 / 12 with exit 0
and no excluded cases. The sanitizer target instruments codec/tests, not
external Qt/Catch2; unsupported LeakSanitizer is disabled. Full incremental
enabled/disabled CMake `librecad` builds and ordered qmake application build
also exit 0. Final process handles are 98677, 86201, 12015 and 69561.

Use `cmake --build` on each configured directory with
`--target librecad_text_profile_tests` (plus `librecad` in ON/OFF), then run
that directory's binary; the ASan invocation uses
`ASAN_OPTIONS=detect_leaks=0`. The qmake invocation, from `build-qmake`, is
`qmake6 ../librecad.pro LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true`
followed by `make -j4`. Full configure/build/test logs are
`/private/tmp/librecad-arabic-profile-{on,off,asan}-{configure,build,tests}.log`
and `/private/tmp/librecad-arabic-profile-qmake-{configure,build}.log`.
Independent codec/build-registration review passes. Owning snapshot repair,
real entity/style integration and cross-format persistence acceptance remain
unfinished; green codec checks do not establish those contracts.

### Shared recursive layout evidence

P3c.1 places source-mapped shaping clusters and graphemes through explicit
paragraphs, whole-line bidi tab fields, recursive stacks and Exact/AtLeast line
spacing. Paint-only spans do not break joining; glyph order is consumed once.
Logical advance, profile extents and analytic transformed ink remain separate.
It uses explicit resource resolution, no system-font substitution or GUI font
database. Missing coverage retains the exact font's visible .notdef and source
diagnostics; approved complete-segment fallback remains a separate required slice.
No native entity adapter, persistent layout cache or automatic wrapping is added.

Independent review reproduced a recursive provenance gap: two children could
agree on an altered whole original while disagreeing with their root. The
before-fix case exits 42 with 1 failed / 21 assertions
(`/private/tmp/librecad-arabic-layout-provenance-before.log`). Every recursive
block now compares original-source values against the document root; the
paragraph-only API explicitly lacks that root and binds its children to their
first original. Equal detached strings remain valid. Failure returns no partial
successful result; earlier valid resolver callbacks may already have executed.
Frozen source/header/test hashes match the independently approved candidate.

Five staged service/test/build inputs at parent `ade6fee6f` were exported and
compared with `/private/tmp/librecad-arabic-core-index.TOns5t`, excluding pending
binding/entity/filter/prefix work. Initial registered ON/OFF/ASan builds each
exit 2 because both new sources lack `qt_bidi.h`; private source compilation had
not tested this registration. The scoped source include property now covers
layout and its tests in the application and standalone target. Failed logs
remain `/private/tmp/librecad-arabic-layout-{on,off,asan}-build.log`.

On the codec evidence's toolchain/configurations, explicitly build
`librecad_text_layout_tests` in each directory, plus `librecad` in ON/OFF, then
invoke that directory's layout binary. Final ON and ASan/UBSan pass 715,036
assertions / 16 cases; OFF without FreeType passes 26 / 1 (backend-unavailable
contracts only, not enabled shaping). Full enabled/disabled CMake applications
and the ordered qmake application pass. Processes 26453, 75246, 26698 and 6236
terminate with exit 0. ASan uses `detect_leaks=0:halt_on_error=1` and UBSan
`halt_on_error=1`; layout and its compiled local dependencies are instrumented,
not external Qt/HarfBuzz/FreeType/Catch2. Final configure/build/test logs are
`/private/tmp/librecad-arabic-layout-{on,off,asan}-final-{configure,build,tests}.log`;
qmake logs are `/private/tmp/librecad-arabic-layout-qmake-{configure,build}.log`.
Independent Arabic-reader/native-entity/recursive-layout output approval remains
a release gate; these service checks do not establish it.

### Owning profile binding evidence

P5a.2 separates immutable imported provenance from Untouched, Replace and Remove
edit authority. It owns supported Qt payload buffers, including externally
backed strings/bytes, while preserving their exact types, nullness and fixed
scalar storage. Unsupported QVariant metatypes require a nonthrowing preflight
and raw retention outside the binding; their constructor rejection must never
escape untrusted file import. No registry, host entity, font or activation policy
is introduced by this QtCore-only service.

Three defects were demonstrated before repair. Default moves left the source
with Replace authority over moved payloads (16 failed / 53 assertions, exit 42).
Retained mutable QString/QByteArray pointers changed accepted edits/imported raw
(10 failed / 42 assertions in 2 cases, exit 42). Default copy assignment changed
destination authority/value before the pending-encoding vector allocation
failed: both Untouched and Replace destinations lost preservation/coherence
(private controlled-allocation probe, exit 2). The repairs deep-own inputs,
retain imported provenance in moved-from values and prepare a complete copy
before a nonthrowing swap. Before/fix evidence is retained in
`/private/tmp/librecad-textprofilebinding-{moves.AWaJPX,alias.u7abi1,copy.rtN90T}/validation.md`.

The allocation regression is now durable in the dedicated
`librecad_text_profile_binding_copy_failure_tests` executable, isolated from
Catch and application targets. Its thread-local global-new injector is armed
only around assignment after all snapshots are prepared. Both destination
states throw at the checked 560-byte vector allocation on this toolchain, retain
the complete source/destination state and pending pointer, and remain coherent.
It exits 0 in registered ON, OFF and ASan/UBSan runs; these are two exercised
failure cases per run, not a Catch assertion count or all-platform STL proof.

The six staged binding/test/build inputs were exported and compared with
`/private/tmp/librecad-arabic-core-index.TOns5t`, excluding uncommitted entity,
style/filter and prefix/package work. On the codec evidence's macOS toolchain,
registered ON, OFF without FreeType and ASan/UBSan each pass 8,150 assertions /
22 cases. Final processes 62245, 18164 and 4341 exit 0; explicitly build both
binding targets with `cmake --build ... --target` and invoke both binaries.
Configure/build/test logs are
`/private/tmp/librecad-arabic-binding-fault-{on,off,asan}-{configure,build,tests}.log`.
Sanitizers instrument local codec/binding/test sources, not external Qt/Catch2;
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1` applies,
with no leak acceptance claimed. Independent source and registration reviews
pass, including isolation of allocator overrides from production.

Full enabled/disabled CMake applications and the ordered qmake application
passed for the same production binding sources and registrations (processes
42945, 94176 and 27673, exit 0). The later change only adds the isolated failure
test/CMake target; those application results are retained evidence, not new
executions. Logs remain `/private/tmp/librecad-arabic-binding-{on,off}-build.log`
and `/private/tmp/librecad-arabic-binding-qmake-{configure,build}.log`.
The intervening `c956d8ca2` changes only this plan. Host setters, STYLE lifecycle,
source serialization, namespace transport/schema freeze and runtime activation
remain separate unfinished gates.

### Entity source state validation

P5a.3a adds immutable imported source/provenance and copy-on-write edit state to
TEXT/MTEXT data without selecting a renderer. Canonical source is distinct from
legacy decoded text and its initial alignment stripping. Imported exact UTF-8,
original STYLE identity, marker/payload origin and inert profile authority remain
owned through edits and copies. Duplicate or unrepresentable namespaces expose
an unavailable current profile rather than silently inheriting STYLE defaults;
explicit Replace/Remove may supersede that state, and reset restores it.

The initial staged candidate passed registered state tests (304 assertions /
7 cases) and native entity tests (874 / 11) in ON, OFF without FreeType and
ASan/UBSan configurations. Complete enabled/disabled CMake application builds
and the qmake application also passed. These builds used the coherent source
snapshot `/private/tmp/librecad-arabic-core-index.TOns5t`, not a new entity ABI
linked to an old application archive. Earlier private mixed-ABI native runs are
diagnostic only and cannot establish sanitizer or native lifecycle acceptance.
Initial configure/build/test logs remain
`/private/tmp/librecad-arabic-entity-{on,off,asan}-{configure,build,state-tests,native-tests}.log`
and `/private/tmp/librecad-arabic-entity-qmake-{configure,build}.log`.

The raw-aware edit contract explicitly takes the generic legacy interpretation
before stripping one leading `\A0;`, `\A1;` or `\A2;`. A final `getText()` that
still begins with another alignment directive is not interchangeable. Independent
review approved this clarification and the added state/TEXT/MTEXT no-op and
raw-only-edit regressions. All nine final staged source/test/build inputs were
compared with the isolated snapshot before starting new registered tests and
affected CMake/qmake builds. Those runs pass state 320 / 7 and native entities
910 / 12 in each ON/OFF/ASan lane; both CMake applications and qmake exit 0.
The final test-only cleanup supplies pre-alignment input in two older helpers
and adds a moved-state nested-baseline assertion. Independent re-review approves;
the final registered reruns pass state 322 / 7 and native entities 912 / 12 in
all three lanes. Each lane also passes the existing unfiltered native text/bidi
suite: 541,335 assertions / 70 cases. Processes 7829, 11719, 63542, 64274, 63726
and 94562 exit 0. These tests link a header-consistent application library.

Explicitly build `librecad_text_source_state_tests`, `librecad_text_source_tests`
and `librecad_text_bidi_tests` with `cmake --build ... --target ... -j3`, then
invoke their unfiltered binaries; native targets use `QT_QPA_PLATFORM=offscreen`.
Final test/build logs are
`/private/tmp/librecad-arabic-entity-accepted-{on,off,asan}-{build,state-tests,native-tests,legacy-build,legacy-tests}.log`.
The sanitizer cache applies `-fsanitize=address,undefined -fno-omit-frame-pointer`
to local library/test sources and sanitizer linker flags. Runs use
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1`;
external Qt/HarfBuzz/FreeType/Catch2 are not instrumented and leak acceptance is
not claimed. The codec evidence's macOS arm64 toolchain and Qt versions apply.

The final production-source CMake application builds (ON and OFF) and qmake
build exit 0, processes 87888, 8417 and 40950. Logs remain
`/private/tmp/librecad-arabic-entity-final-{on,off,qmake}-build.log`.
Only the test source changed afterward: production headers, sources, build
registrations and dependencies are identical, so application build evidence is
carried forward explicitly, not reported as newly executed. Native adapter
geometry, UI/undo, persistence and schema/activation acceptance remain separate
gates.

### Current compatibility input preservation

P5a.3b retains the current generic-decoder input before one leading alignment
strip, separately from final legacy text and immutable import history. A source
edit changing `\A1;legacy` to `\A2;legacy` must not disappear merely because both
produce the same final text. The owned getter supplies CurrentSource13 directly;
copies, moved-from values, profile edits and constructor reconciliation retain
the current buffer without replaying the historical baseline.

The existing-API before regression in
`/private/tmp/librecad-state-input.VFSRsx/before-run.log` exits 42: four failed
assertions of 18 across one case, exercising both source syntaxes. Normal and
O1 ASan/UBSan private state-only after runs each pass 420 assertions / 9 cases.
The independent review approves this bounded repair; state-only checks exclude
entity construction and cannot establish host acceptance.

The coherent application candidate
`/private/tmp/librecad-state-input-root.41laoi` contains committed `c8b5a193c`
plus exactly these three source/test files, without pending native, block,
filter or package edits:

| File under `librecad/src/lib/engine/document/entities` | SHA256 |
| --- | --- |
| `lc_textentitystate.h` | `efb895448d72353d83c394a774007c10463e7e07bacc262a330c273d769a5222` |
| `lc_textentitystate.cpp` | `36f939c5139a869739dbd6a024c101b8d621af5327b61023acd158bf41a849c1` |
| `tests/rs_text_source_tests.cpp` | `2353a4078b8b892a627b5558dea12440dbbcef690f45a4b863bc1a6696ed9340` |

The full-source manifest SHA256 is
`bdb78b43f4eaf9fa02cd92f647dfe1f44aba888e4b2f031b2a23376381951f0b`.
The enabled application and all nine requested binaries build; recorded ON
runtime footers pass source 1,065 / 14, state 420 / 9, STYLE 286 / 7,
profile 4,544 / 12, binding 8,150 / 22, inert model 102,708 / 13,
raw metadata 21,535 / 5, bidi 541,335 / 70 and writer lifetime 290 / 4.
Original session 91903 is no longer registered. Fresh dependency/runtime
confirmation preserves the original logs and records build, all nine runtimes
and manifest verification exiting 0 in `review-on-recheck-status.log`; the
dependency build has no compilation/link step. The fresh coherent OFF lane
also exits 0 for configure, application/all-nine-target build, every unfiltered
runtime and both 5,829-file manifest checks. It passes the same 680,333 assertions
/ 156 cases; exact commands/statuses are in `review-off-status.log` and matching
`review-off-librecad_*_tests.log` files. Its cache, compile-command manifest and
application SHA256 values are respectively
`b56faa2508e55e6f8f3bc5269b55aa41e73b8fa6942081358380783c316a021e`,
`cf7fbe612f9a229de89a6b16a7de8c6c56b46179bb66066ef438ae26457dd907`
and `674e5e1a2cb0b9d3323841e6bd20884876321001cabb122421bea2dbfbd1e068`.
The whole-private-source O1 ASan/UBSan lane also completes configure, full
application/all-nine-target build, every unfiltered runtime and both manifest
checks with exit 0, again passing 680,333 assertions / 156 cases. Exact statuses
and footers are retained in `review-asan-status.log` and matching runtime logs;
the cache, compile-command manifest and application SHA256 values are
`ea918c02d39fd76cbb1587dcaa424d1992998dc27901f91ad078717d1b964b8c`,
`b7318451f0b2de53cb67648b5ca046cd2c2c20e27eabfbcde4da6ffa5a8ac09b`
and `b5cdbf8e76c238e2aa4c1ca2a1aacc469e57eca22af315bbfbd135ad9ca45279`.
The fresh qmake shadow application now also completes configure, bundled-library
and application build, dependency recheck and both source-manifest checks with
exit 0. Exact commands/statuses are in `review-qmake-status.log`; application
SHA256 is `3dd2aed3d4f3aa1be51c2156c2c37d05e2e171f5086418c989b9775ebf28bd6d`.
The dependency recheck has no compile/link step. This is build acceptance, not
startup, packaging or deployment acceptance.
External dependencies are not instrumented;
leaks are not claimed. This prerequisite does not restore a saved carrier,
activate an imported profile or complete P5.

The intermediate checkpoint is deliberately split at the source-state boundary,
using the workflow's allowance for independently validated work with limitations
recorded. The added buffer remains inside CPP-private immutable `Data`; the
nonvirtual getter introduces no new public TEXT/MTEXT Data member, no backend
conditional and no source-registration change. Independent review finds no
concrete dependency/consumer issue with this bounded checkpoint. Its ON, OFF,
ASan/UBSan and qmake application gates are now complete for these exact inputs;
transport and milestone acceptance remain separate. The final evidence manifest
`review-final-artifacts-sha256.txt` has SHA256
`d3bb2ba9db61bc4bbbfc7b7719b6dbc63aa8723d39f35f0699214449f236662d` and
records all 27 CMake suite footers, binaries, commands and retained logs.

A fresh strict disabled-flag state-only build in
`/private/tmp/librecad-state-input-off.VUgS9R` compiles all four project test/
State/profile/binding translation units against the same frozen candidate,
with `LC_TEXT_STATE_ONLY=1`, `LC_WITH_OPENTYPE=0`, `QT_USE_QSTRINGBUILDER`,
`-O1 -g -Wall -Wextra -Werror`. Configure, build and runtime exit 0; the suite
passes 420 assertions / 9 cases. This pure QtCore check executes no native
backend and does not substitute for the separate disabled application lane.
Commands configure that directory's `CMakeLists.txt` with
`-DCMAKE_PREFIX_PATH=/opt/homebrew -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS='-O1 -g' -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`,
then run `cmake --build .../build --target text_state_input_off_tests -j1` and
`.../build/text_state_input_off_tests` unfiltered. Exact configure/build/runtime
logs are `configure.log`, `build.log` and `run.log`; the driver CMake SHA256 is
`41ecde01b70241f93f50f6750ce86f4ea08672822340617b42ada7249443e5df`,
compile-command manifest SHA256 is
`982b24ce265aa07f16c00de37bb443731af11bad50a190aea14bd943afc3eaf2`,
and binary SHA256 is
`5ad984fe947f18537a74101ea7fa13d476a0a0aef41737d4c711800467205c81`.
Root byte-compares all three staged source/test candidates to the frozen inputs.

### Private current source materialization

The private candidate `/private/tmp/librecad-current-source.gsR4Tn` maps actual
current source state, compatibility input and final text, convention, STYLE
association/profile, payload/marker provenance and explicit deformation into
the existing CurrentSource fields. Optional UTF8 comes from explicit owned
current bytes, never from imported history or a guessed re-encoding. A restored
current profile is an inert Untouched baseline, not Replace authority or an
opt-in activation. Historical ImportedSource remains separately owned and does
not replace the fresh current state's reset/replay authority. A loaded model
with no historical record retains authoritative absence through subsequent
saves. Current12 and Current15 remain passive captured history through unrelated
scalar/profile/STYLE changes while source and compatibility buffers are exact.

Actual native import consumers must supply the original raw capsule for each
non-Absent historical profile. A Qt binding alone cannot establish original
DRW string flags, raw layer-reference provenance or source version. Supplied
capsules must exactly match the owning binding's modeled values and status;
unavailable namespaces retain their original stream without being treated as
Absent. Wider QVariant types, typed-null values and lone UTF16 surrogates refuse
instead of coercing. Present empty binary values stay non-null. Current and
historical records share byte/item ceilings before restoration copies; the final
full root must separately enforce its aggregate framing/depth and graph budget.

`HANDOFF.md`, `README.md`, `input-sha256.txt` and the nine `final-*` logs retain
the recipes and frozen inputs. Their input manifest SHA256 is
`75ca04701e3bdc0101aef2fb236582f6073f419d8dae29f8eba107c8a443dfe5`;
all 21 entries verify unchanged after the final sanitizer runtime and root
readback. Mapper header/CPP/test SHA256 values are respectively
`030cf873c133c554deedab50da1899c56f3dcd2f7b660df3c3503eff5c92a8dd`,
`59c62e4940f70a37dd104a1b1975d7709817aba0983217a70b35abc22c695bf2`
and `4bf5c19e5abc9d60f7be8a18ac86fd53343b761c4a1c2ce1086ea2b6fd109e54`.
The private model delta adds only CurrentSource and ImportedSource subrecord
validators using the existing validator, without fabricating a full root.

Forced `make -j1 -B -f .../Makefile on model-on`, `off model-off` and
`asan model-asan` builds, followed by the corresponding current mapper
`'[embedded-current]' --reporter compact` and unfiltered model executables,
all exit 0. Each configuration passes 331 assertions / 10 mapper cases and
102,708 / 13 model cases. AppleClang 21.0.0 arm64, Qt 6.11.2, C++17,
`-O1 -Wall -Wextra -Werror` and QStringBuilder are used; all project dependencies
and headers are private copies and dependency files contain no shared-worktree
paths. ASan/UBSan instruments every project translation unit in this boundary;
external Qt/Catch are uninstrumented, leaks are disabled and no allocator
override is linked. ON/OFF are pure-service configurations, not native-backend
or application checks. The expanded ninth fixture initially used 32 raw hash
bytes instead of the required 64 lowercase hexadecimal bytes; its failed run
is retained and the test-only correction precedes the final ten-case matrix.

Root review accepts this bounded proposal, not production transport. Actual
ATTRIB/ATTDEF capture must retain an immutable per-host original native/history
bundle before working copies resolve STYLE or compatibility decoding. Export
must consume that bundle rather than synthesize a partial original MTEXT. Actual
current host Data/State remains edit authority; unique live STYLE association,
full root/carrier ownership and actual typed target reread remain acceptance
gates. The proposal stays private until that consumption is implemented and
coherently validated. An efficiency-only follow-up may remove an extra raw-tag
vector copy without changing these frozen tested bytes.

### Syntax aware source convention evidence

`LC_TextSourceCodec` converts caret conventions using parser-owned raw ranges,
not flattened logical text. Same-convention QString and explicit-length UTF-8
values retain exact bytes/nullness, including malformed unaffected input; output
owns its storage. Cross-convention edits retain scopes, format spelling, stack
separators/padding and paragraph boundaries. Literal quoting prevents decoded
carets from activating escapes or extending numeric commands. Sensitive opaque
arguments fail atomically rather than guessing; checked source/output, traversal
and recursive budgets prevent partial success.

Independent expected spellings cover every active ASCII follower, controls,
Arabic marks, scopes/stacks, BOM/NUL, invalid UTF-8, malformed UTF-16 and bounds.
Numeric fixtures distinguish true H/W/Q exponent bridges from malformed signed
exponents, integer C/c commands and completed relative H/W x/X suffixes. The
last two cases exposed unnecessary optional-delimiter rewrites, not semantic
data loss: retained before-fix proofs report 16 and 32 raw-output failures,
respectively; the relative case's semantic/style assertions already passed.
Private strict ON/OFF and O1 ASan/UBSan each pass 25,398 assertions / 15 cases.
Exact private commands and logs are in
`/private/tmp/librecad-arabic-source-codec-relative.1vTt9U/commands.txt`,
`{on,off,sanitized}-{build,tests}.log` and `before-tests.log`; the earlier integer
proof is `/private/tmp/librecad-arabic-source-codec-integer-before-tests.log`.

The same three frozen files and only their CMake/qmake registration hunks were
added to `/private/tmp/librecad-arabic-core-index.TOns5t`, after the parser
checkpoint. Registered ON, OFF without FreeType and ASan/UBSan each pass the
unfiltered codec 25,398 / 15, parser 453,636 / 25 and itemizer 208,140 / 16.
ON/ASan shaping handoff and layout pass 441 / 3 and 715,036 / 16; OFF executes
their explicit disabled paths, 1 / 1 and 26 / 1. Complete CMake ON/OFF and qmake
applications build; the coherent sanitizer application library also rebuilds.
The entity evidence's toolchain/configurations apply, including unspecified
CMake build type, prebuilt Qt/Catch exclusions and no leak acceptance. Root
sanitizer tests use `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`.

Root commands configure the existing snapshot builds with `BUILD_TESTS=ON`,
`LIBRECAD_ENABLE_OPENTYPE=ON/OFF` and, for OFF,
`CMAKE_DISABLE_FIND_PACKAGE_Freetype=ON`; then explicitly build
`librecad_text_source_codec_tests`, `librecad_text_document_tests`,
`librecad_text_runs_tests`, `librecad_text_runs_shaping_tests`,
`librecad_text_layout_tests` and `librecad` (`librecad_lib` in the sanitizer lane)
with `cmake --build <build> --target <targets> -j3`, and invoke each listed test
binary without filters. Qmake uses `qmake6 <snapshot>/librecad.pro
LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true` then `make -j3` in
`build-qmake`; the project's postprocess override is not package acceptance.
All four lanes exit 0. Logs are
`/private/tmp/librecad-arabic-source-codec-root-{on,off,asan}-{configure,build,codec-tests,parser-tests,itemizer-tests,handoff-tests,layout-tests}.log`
and `/private/tmp/librecad-arabic-source-codec-root-qmake-{configure,build}.log`.
Independent review found delayed `auto` concatenation expressions in tests that
could retain temporary operands on older Qt with `QT_USE_QSTRINGBUILDER`.
Materializing the nine persistent locals as QString/QByteArray repairs their
ownership without changing expectations or production code. Final registered
ON/OFF/ASan codec runs again pass 25,398 / 15, with logs
`/private/tmp/librecad-arabic-source-codec-accepted-{on,off,asan}-{build,tests}.log`.
The final test SHA256 is
`9215c5595c33489b4363928530636912ecccc8c6493b79153b9d9cb3ff002a62`;
production header/source are unchanged from the application builds. Carry
forward those build and unaffected parser/itemizer/layout results explicitly,
not as newly executed checks. Independent source/registration review approves
the repaired candidate; Qt 6.4 execution remains unverified.
This is a bounded source-conversion service, not native-filter, minimum-Qt,
saved-profile activation or independent CAD interoperability acceptance.

### Live ancestor pen inheritance evidence

Native line/source containers use invalid pens for transparent inheritance.
`RS_Entity::getPenResolved` now walks to the nearest valid ancestor, resolves
each ByBlock component through the actual successive ancestors and stops at the
Graphic boundary. A supplied ByLayer component uses that supplying ancestor's
live layer, not an unrelated leaf layer. Explicit RGBA, named colors and custom
linetype identities remain intact; parent/layer edits remain live rather than
being copied onto native glyphs during layout.

The isolated candidate contains only `rs_entity.cpp`, the dedicated
`rs_entity_pen_inheritance_tests.cpp` and its four-line CMake registration on
top of the committed capsule/lifetime source. Source and test SHA256 are
`69e2cb92e61659ffb88554b1f4e41d9b569ab6ea61db4a787f012c0410baa917` and
`3515446b2596161077a888d11e8fb88444c136fe65b029b110a56e20944e9551`.
Independent production/test review approves these exact inputs.

The final exact-fixture baseline at `37d2c614d` exits 42: 39 assertions,
9 passing and 30 failing in four failing cases. Restoring the isolated fix
gives unfiltered registered ON, OFF without FreeType and coherent ASan/UBSan
results of 39 assertions / 4 cases, all exit 0. The focused text/bidi suite
also exits 0 with 541,335 assertions / 70 cases in each configuration.
Complete CMake ON/OFF applications, coherent sanitizer application library and
qmake application build exit 0. The capsule's toolchain, sanitizer settings,
snapshot and qmake postprocessing exclusion apply; this is not package or
native adapter acceptance.

Commands configure the same isolated snapshot lanes, build
`librecad_entity_pen_inheritance_tests` and `librecad_text_bidi_tests` with
`-j2`, invoke both without filters, then build `librecad` in ON/OFF or
`librecad_lib` in ASan. Final logs are
`/private/tmp/librecad-arabic-pen-root-accepted-{on,off,asan}-{configure,tests-build,tests,bidi-tests,application-build}.log`
and `root-accepted-qmake-{configure,application-build}.log` with the same prefix.
Exact baseline logs use `root-final-before-{build,tests}.log`.

Earlier runs retain two fixture corrections, not product workarounds: the
initial test lacked `RS_Settings` initialization and exited 139; a later
parent-layer fixture incorrectly used `RS_Pen()`, whose color is explicit
black despite the header comment. Explicit ByLayer color fixes that fixture.
The intermediate ON/OFF/ASan results (35 passing, 4 failing of 39 assertions)
are superseded, not counted as passing acceptance. Logs retain the
`root-before`, `root-before-fixed-fixture` and `root-{on,off,asan}` histories.

### Raw embedded metadata capsule evidence

`LC_EmbeddedTextXData` is an inert, lossless in-memory retention service, not a
native XDATA schema or compatible wire carrier. Its version-2 capsule retains
ordered signed codes and exact types, null versus INVALID slots, raw strings
including NUL and invalid UTF-8, binary values, integer64 and binary64/coordinate
bits. Active unresolved DWG layer-reference provenance survives only while
owned storage, type, code and baseline spelling still match; stale or rebound
storage cannot revive replay authority. Aggregate item/byte limits include all
framing and provenance overhead. Malformed/truncated/version/count/flag/trailing
input fails without partial publication. The one-million-item and 64 MiB
ceilings are memory budgets, not native-wire allowances.

The independently reviewed candidate is HEAD `1636de0ce` plus only the capsule
header/source/tests, the read-only `DRW_Variant::activeDwgLayerReference` getter
and their CMake/qmake registration. In the coherent snapshot
`/private/tmp/librecad-arabic-core-index.TOns5t`, enabled, disabled without
FreeType and ASan/UBSan runs of `librecad_embedded_text_xdata_tests` each exit 0
with 21,535 assertions / 5 cases; the same binaries were rerun before staging.
The unfiltered tests include a hand-authored literal version-2 fixture and
4,097 distinct alternating raw values compared slot by slot, not merely a
self-round-trip count. Complete incremental CMake ON/OFF applications and the
coherent sanitizer application library build; qmake compiles the capsule and
relinks the application, with its existing `-nodepend` submake behavior.
All four build lanes exit 0. No unrelated STYLE/filter/native-writer candidate
is included. The existing recorded AppleClang 21.0.0, Qt 6.11.2 and Catch2
3.16.1 toolchain applies; enabled builds use HarfBuzz 14.6.0 and FreeType 2.14.3.
CMake build type is unspecified.

Exact commands configure `<snapshot>/build-{on,off,asan}` with `BUILD_TESTS=ON`
and `LIBRECAD_ENABLE_OPENTYPE=ON/OFF`; OFF additionally sets
`CMAKE_DISABLE_FIND_PACKAGE_Freetype=ON`. Build the registered capsule target
with `cmake --build <build> --target librecad_embedded_text_xdata_tests -j2`
and invoke `<build>/librecad_embedded_text_xdata_tests` without filters. Build
`librecad` in ON/OFF and `librecad_lib` in ASan with the same command form.
ASan uses `-fsanitize=address,undefined -fno-omit-frame-pointer`, linker
`-fsanitize=address,undefined`, runtime
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`; prebuilt Qt/font/Catch libraries are not
instrumented and leak acceptance is not claimed. Qmake uses
`qmake6 <snapshot>/librecad.pro LIBRECAD_ENABLE_OPENTYPE=true
DISABLE_POSTSCRIPT=true`, then `make -j2` in `build-qmake`.
Logs are `/private/tmp/librecad-arabic-capsule-root-{on,off,asan}-{configure,tests-build,tests,application-build}.log`
and `/private/tmp/librecad-arabic-capsule-root-qmake-{configure,application-build}.log`.

Frozen SHA256 identities are header
`466c9620ec708d766bf14d7b43b76702fe7eca3e7b19c9d2f134ca3bcd51a172`, source
`94ae1bd7c19708ff42309039a0f9ca552d1d6be2de84bfe7da8e4e7719cf21fd`, tests
`0cc788078bb89b44edfbee0e5abca4791dce2df4f7b4b96b0f01d0aab03e1eb0` and
`drw_base.h` `3b13eb09bd4149419f7bfa5055eb6e552d8f6eda32946f214bc392d2f7f3f70c`.
This bounded checkpoint does not close `P5-SIDECAR-001`, establish full-model
transport, prove Qt 6.4/platform/package acceptance or activate saved profiles.

### Embedded metadata retention finding

Finding `P5-SIDECAR-001` is Open. Independent baseline review at `a8ade2677`
confirmed that the embedded ATTRIB XDATA sidecar can omit streams over 4,096
items or containing null/INVALID slots and truncate string data at NUL. The
codec also omits active raw DWG 1003 replay provenance; this is a confirmed
representational gap, not a reproduced native embedded-DWG import failure.
Its decoder rejects nonfinite raw values
and original codes outside 1000–1071; failed extraction can then omit the entire
attribute on export. This is an in-scope data-preservation gate, not an optional
PR cleanup or a reason to weaken raw retention.

Retain schema-1 reading and implement a versioned lossless capsule with explicit
lengths, exact scalar bits/types, slot identity and active replay provenance.
Encoding and decoding require checked aggregate byte/count budgets and
transactional publication. Distinguish absent, malformed and unknown sidecars;
never turn a decoding failure into permission to skip an attribute. A genuinely
unrepresentable output must fail explicitly while preserving in-memory source
and metadata. Raw retention alone does not make malformed native wire writable.
Regression, supported-output and independent transport proof remain Pending.
The in-memory raw-tag capsule alone does not solve native DWG transport or
pre-2018 embedded-body retention; the full-model outer-ATTRIB envelope specified
above remains required. The current DXF writer/parser use a noncanonical
group-100 embedded marker and assign trailing outer XDATA to MTEXT. Moving the
outer XDATA ahead of the embedded marker is not a valid correction; the
canonical marker/scope and independent carrier finding below must be resolved
before transport acceptance.
R12 currently needs an
interim explicit failure for raw-aware formatted/profile/embedded MTEXT that
the legacy TEXT-line downgrade cannot retain faithfully. Ordinary plaintext
fallback remains characterized legacy behavior. That guard does not complete
the required supported-output round trips or authorize narrowing their scope;
lossless downgrade authority/source retention remains an unfinished gate.

### Cross format filter lifetime finding

Finding `P5-FILTER-LIFETIME-001` is Verified at `37d2c614d`. Its committed
production and test identities match the independently reviewed, coherently
validated candidate below. A final mixed-tree diagnostic on
HEAD `1636de0ce` plus the uncommitted STYLE/filter candidate crashes when one
`RS_FilterDXFRW` exports DXF and then DWG. The `[text-filter-authority]` run
exits 139 with 241 passing assertions and one fatal assertion across six
started cases; an earlier 14,180 / 10 pass does not close this gate. The DXF
export path deletes `m_dxfW` without clearing it, while
`prepareDRWDimStyleText` can subsequently dereference the non-null pointer.
The coherent baseline regression below establishes that stale observer lifetime
also fails without the uncommitted STYLE/filter candidate.

The retained diagnostic is
`/private/tmp/librecad-filter-authority-compile.IjGFH6/filter-after-final.log`;
filter/test SHA256 identities are
`8678bc6d1a6043e55a98e5ff12517954400dcdef33d64a1606e6260af6f2078d` and
`ff54e391340bae253623e6bcca340646bc9d41abbeb8dcde639773ce50182305`.
This is failure evidence, not isolated registered acceptance. The durable
regression and coherent normal/sanitizer verification below establish the
bounded repair; they do not complete the other P5 gates or PR-wide review.

The minimal repair owns each writer locally with `ExportWriterScope`, clears
the callback observer before deletion on all returns and exception unwinding,
and rejects nested export before mutating the active operation. DWG closes its
writer before staged-path finalization. Only this patch and a dedicated test
registration were added to the coherent capsule snapshot, excluding all other
pending filter/STYLE/native-writer edits. The production patch SHA256 is
`2f33acb221e5172f025130eef2e4e9a96e83d1a83a691e92654558f73dd8d971`.
The final isolated filter header/source hashes are
`0320825d8d8fb0648dab18167494ce4a84a425c18f58be827e506376fd3a6aae` and
`b8d3fea88ca13f3187509032a73b00b776d4ee29163e0071ba7cb699acd20420`;
test SHA256 is `ff1cdd712caac4e0a17e6db31944dba34407dcd4038fcc418bf9605930fa93ff`.

Before applying the production patch, the registered `[observer]` test on
`9e50184ff` exits 42 with 108 assertions, 103 passing and 5 failing; stale
non-UNKNOWN writer queries reproduce the defect. After the patch, unfiltered
registered ON, OFF without FreeType and ASan/UBSan tests each exit 0 with
290 assertions / 4 cases. Cases cover both format orders, repeated saves,
mixed 2004/2018 versions, preallocation and emission failure, opposite-format
nested rejection, and callback exception followed by cross-format and original
format retries. The actual active version and emitted signatures are checked,
and successful files are read back independently through fresh filter instances.
Independent source/test/registration review approves the final candidate.
Complete incremental CMake ON/OFF applications, coherent sanitizer library and
qmake application build all exit 0. Qmake recompiles the changed filter source
and relinks; this is not a package or GUI-runtime acceptance result.

The capsule's toolchain and sanitizer configuration apply. Commands configure
the same snapshot builds, then `cmake --build <build> --target
librecad_filter_writer_lifetime_tests -j2`, invoke that binary without filters,
and build `librecad` in ON/OFF or `librecad_lib` in ASan. Qmake uses the same
command and `make -j2` as the capsule. Root logs are
`/private/tmp/librecad-arabic-lifetime-root-{on,off,asan}-{configure,tests-build,tests,application-build}.log`
and `/private/tmp/librecad-arabic-lifetime-root-qmake-{configure,application-build}.log`;
before logs use `root-before-{configure,build,tests}.log`. Earlier private
178 / 3 results and the partial-instrumentation baseline diagnostic remain
historical in `/private/tmp/librecad-filter-lifetime.rfT90m/EVIDENCE.md`, not
coherent acceptance. This bounded fix does not accept embedded wire encoding,
STYLE/profile transport or the full P5 milestone.

The same isolated ON candidate also builds and runs the unfiltered existing
`librecad_dwg_write_fast_tests`: exit 0, 17,228 passing assertions, 419 passing
cases and 3 skipped cases out of 422. Missing `datatable_r2010.dwg`,
`visualstyle_r2007.dwg` and `blockvisibility.dwg` fixtures cause those skips;
their coverage is not counted as passing. Commands are `cmake --build
<snapshot>/build-on --target librecad_dwg_write_fast_tests -j2` and
`<snapshot>/build-on/librecad_dwg_write_fast_tests` without filters. Logs are
`/private/tmp/librecad-arabic-lifetime-root-dwg-{build,tests}.log`.

### Embedded DXF marker and namespace finding

Finding `P5-EMBEDDED-DXF-001` is Open. At `1636de0ce`,
`dxfRW::writeEmbeddedMText` and `DRW_Attrib::parseCode` use group 100 for
`Embedded Object`; the parser then routes trailing XDATA to embedded MTEXT.
The [Autodesk filer contract](https://help.autodesk.com/cloudhelp/2017/ENU/OARXMAC-RefGuide/files/OREFMAC-__MEMBERTYPE_Methods_AcDbDxfFiler.html)
requires group 101. Independent [ezdxf parsing](https://raw.githubusercontent.com/mozman/ezdxf/master/src/ezdxf/lldxf/extendedtags.py)
and [entity export](https://raw.githubusercontent.com/mozman/ezdxf/master/src/ezdxf/entities/dxfentity.py)
place outer XDATA after the native embedded fields. Its
[embedded MTEXT exporter](https://raw.githubusercontent.com/mozman/ezdxf/master/src/ezdxf/entities/attrib.py)
does not emit a separate XDATA frame.

The pending local test and writer-ordering change repeated the group-100/scope
assumption; their earlier 249 / 1 normal/sanitizer pass is not interoperability
acceptance. Replace that oracle with an external group-101 fixture, verify
native text and outer application ownership through an independent reader, and
retain independent embedded metadata through the compatible outer carrier.
Characterize legacy noncanonical inputs without silently reassigning ambiguous
metadata. A temporary explicit export failure preserves data but does not
complete the required lossless output gates.

### Embedded guard boundary diagnostics

The private corrected-header guard boundary in
`/private/tmp/librecad-canonical10858.dYDHPh` uses exact application headers from
the committed model snapshot and explicitly rebuilt native consumers/factory
objects. Earlier mixed-header crashes are ABI-confounded and invalid evidence,
not attributed guard defects. Replacement-object diagnostics are not a coherent
whole-tree build or full-source sanitizer acceptance.

Before the latest capability guards, normal and ASan/UBSan diagnostic runs each
execute 17,697 assertions / 10 cases with one failure: a live invisible modern
embedded ATTRIB exported to DWG2013. The separate owner/deleted-owner/marker
selector passes 716 assertions / 4 cases in both modes. Root reads these actual
logs: `boundary-guard-{normal,asan}.log` and
`boundary-guard-{normal,asan}-owner-marker.log`. These narrow passes do not prove
lossless body transport or final guard integration.

The private trace-only `boundary-trace-stage2.log` identifies ATTRIB encoding,
not PLOTSETTINGS, as the first failure. Root checks the exact native source:
`DRW_Attrib::encodeDwg` rejects modern embedded bodies/nonzero attribute versions
for AC1024 through pre-AC1032, before body encoding. The PLOTSETTINGS warning is
secondary to that failed transaction. Older output that emits only the outer
fallback can pass entity counts while dropping the embedded body; such counts
must not certify preservation. The full external-fixture failures also contain
unrelated imported native-metadata obstacles and are not deletion-only proof.

The frozen final private safety patch is
`canonical-filter-guard-safety-final-10858.patch`, SHA256
`d67ccb4931515753754cb09d24265c65efbc55339c616b9129a3997a27faf5c5`.
It adds plain-ATTRIB positive sequence/owner cases and pre-open refusal for
concretely unrepresentable modern-body/version states, preserving source, prior
destination and retry behavior. Capability checks follow live-host/owner
membership; proven deleted hosts are not emitted. Protected authored values
remain distinct from absent authoring defaults. R12 admission also refuses
current visibility inconsistent with retained ATTRIB bit 1, because this output
does not emit the later common visibility field; it does not rewrite flags.

Final matching-boundary normal and partial ASan/UBSan diagnostics both exit 0:
21,667 assertions / 16 cases without the independent-output fixture hook, and
21,679 / 16 with it. Actual logs are `boundary-final-{normal,asan}.log` and
`boundary-final-{normal,asan}-no-oracle.log`. Independent ezdxf/raw-tag checks
verify R12 visible/hidden flags and R14 common visibility; the preserved
before-R12 selector fails 12 / 252 assertions. The full command/input/evidence
record is `GUARD-BOUNDARY-EVIDENCE.md`, SHA256
`862ac9e9bc14f4ad8c59c66f2c9593380cfa99bc1d8bbe40259a3e36ec6ad0a6`.
Independent final whole-patch/source/test review finds no new proven defect.
This is still the old matching-header object-override boundary with partial
instrumentation and leak detection disabled, not coherent current-N4 root
acceptance. These temporary refusals do not waive required full-model capture,
carrier/restoration or lossless supported-format gates;
`P5-DELETED-ATTRIB-001` stays incomplete.

### Native adapter and carrier decisions under validation

The pen prerequisite is committed at `d006e580d`; its isolated checks above
cover only live inheritance. The atomic container/selection correction has
independent source approval and private runtime/fault-injection evidence.
Its coherent registered borrowed-child baseline exits 42: 3 assertions,
1 passing and 2 failing, across 1 case. This baseline intentionally restores
only the erroneous traversal into non-owning retired subcontainers, on the
otherwise corrected candidate; it is not a claim that the API existed at HEAD.
Registered ON acceptance exits 0 with 55 assertions / 4 cases, six injected
C++ allocation failures and zero observed postcommit C++ allocations, pen
39 / 4 and focused bidi 541,335 / 70. The complete ON application links.
Registered OFF and coherent ASan/UBSan acceptance now pass the same counts;
the complete OFF application and coherent sanitizer library link. The fresh
qmake application build also exits 0. Independent source/test and CMake
registration review approve the bounded checkpoint.

The atomic API prepares replacement children/bounds and per-document selection
removals before mutation, then swaps the prepared lists without allocation or
callbacks, reparents new children, resets iteration caches and retires exactly
the owned old subtree. Borrowed descendants survive and retain selection.
Caller metrics/status are published before the explicit single-consumption
notification; throwing or reentrant listeners are postcommit, not build failure.
The model thread must not mutate selection between prepare and commit.
The dedicated allocator executable is isolated from Catch2/application code;
it interposes C++ `new`, not Qt-private `malloc`. Qt list allocation/detach sites
are ordered before commit by source inspection, not claimed injected coverage.

The frozen production SHA256 identities are container header
`d72cc4d357c9758795ab7fcba6fcb32f58cfc6a3c49ebea75e053c4379cb1e0a`,
container source `119cf49991fd4013eac6ada6fb947d632298e9349e0a83a4dd303f2eeb7935f2`,
selection header `f0b46aba130d8df54d2c3d255306d3c993aba861dcf8b5466dd253f8a1cade01`
and selection source `4c33cc1dd6717ff59580f04d551900f881099ef96085b473dd3f958ecf804b6d`.
Normal and fault-test identities are respectively
`c8df957522aabbc3c1fdc80779d2c289831dd7e5d00e224f281f5e7c305425c7` and
`2b0b51fe8b2683a74fb7b0b0b83291136cc5d5280408ce048ae567d993aad97a`.
Root logs use `/private/tmp/librecad-arabic-atomic-root-before-{configure,build,tests}.log`
and `/private/tmp/librecad-arabic-atomic-root-accepted-{on,off,asan}-{configure,tests-build,tests,fault-tests,pen-tests,bidi-tests,application-build}.log`;
fresh qmake logs use `root-accepted-qmake-{configure,application-build}.log`.
The coherent source is committed `d006e580d` plus exactly these six files and
their two test registrations, excluding pending STYLE, native adapters,
filter/transport and packaging changes. Configure ON/OFF with `BUILD_TESTS=ON`
and explicit `LIBRECAD_ENABLE_OPENTYPE`; OFF also sets
`CMAKE_DISABLE_FIND_PACKAGE_Freetype=ON`. Build and invoke without filters
`librecad_entity_container_replace_tests`,
`librecad_entity_container_replace_failure_tests`,
`librecad_entity_pen_inheritance_tests` and `librecad_text_bidi_tests`, then build
`librecad` in ON/OFF or `librecad_lib` in ASan, all with `-j2`.
The capsule toolchain applies. ASan/UBSan uses
`-fsanitize=address,undefined -fno-omit-frame-pointer`, matching executable linker
flags, `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1`; Qt/font/Catch dependencies are not instrumented,
and this is not leak acceptance. A new shadow build at
`/private/tmp/librecad-arabic-atomic-qmake.UDlZpE` runs
`qmake6 <snapshot>/librecad.pro LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true`
and `make -j2`, rebuilding matching-header consumers instead of relying on
old qmake objects. SDK/Qt minimum-version linker warnings do not certify older
macOS deployment; supported packages, Qt 6.4 and GUI acceptance remain pending.
This remains a bounded prerequisite, not native adapter or full P4 acceptance.

Native adapter review identified `P4-DECORATION-001`: fuzzy `RS_Vector` equality
can discard a positive underline or stack separator even when it exceeds the
requested document tolerance. Retain a failing-before fixture and use exact
converted endpoint equality, with normal/sanitizer verification; do not relax
precision bounds to make an unrepresentable fixture pass. A separate direct
FreeType quadratic-extremum probe confirms the pinned A reaches X=618.2,
distinguishing continuous ink from its integer bounding box. Durable adapter
fixtures and coherent acceptance remain required.

Native transform review also identified `P4-AFFINE-001`: inherited shear changes
tessellated children without canonical Data/metric updates, and reparsing after
uniform height mutation cancels scaling of absolute inline `\Hn;` content.
The private matching-header failing-before test exits 42 with 6,201 assertions,
6,194 passing and 7 failing across 2 cases, including a nested stack. This is
diagnostic evidence, not coherent acceptance. Preserve it and add durable
virtual-dispatch and source/geometry reconstruction regressions.

The approved implementation direction stores a canonical four-double
deformation `N = {xx, xy, yx, yy}` in TEXT/MTEXT Data and requests, identity
`{1, 0, 0, 1}`. World transforms preserve local height, width factor, MTEXT
reference width and source, accumulating the complete affine instead; deliberate
property edits reflow local semantics. Compose insertion, rotation, `N`, scalar
scale, generation pivot and line/child transforms once before tessellation.
Aligned/Fit normalize `N` against its X basis without changing input anchors;
actual signed baselines remain separate from canonical reference anchors.
Both shear overloads need a single atomic native transaction, not inherited
multi-operation mutation; native contour-direction reversal is a no-op.
Nonfinite, singular or unrepresentable transforms fail without partial state.
Clone, getData/reconstruction, undo, consumers and P5 transport must retain `N`;
switching to legacy rendering cannot silently discard nonidentity deformation.
Independent mathematical review approves the affine identities. Root reproduces
the deterministic 12,000-point probe with Node v26.11.1: exit 0, maximum residual
`4.3220627292571254e-14` at small coordinates. The script is
`/private/tmp/librecad-affine-review.I4JO1g/affine_identity_probe.js`, SHA256
`7e6084f49d490fce47575893a96d98a3f636afb8e56107ebc7722fa3bbe88519`.
This is algebraic evidence only; native runtime/tolerance, durable fixtures and
coherent acceptance remain pending. Native draft-render selection must also use
final geometry size rather than canonical height: Aligned fitting and stored
deformation can make the actual contours much larger than the local height.

Generation is geometric, never bidi. The initial characterization uses
entity-authoritative TEXT flags, without live XOR with STYLE defaults; retain
valid STYLE flags as provenance/defaults. Autodesk's
[TEXT](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-62E5383D-8A14-47B4-BFC4-35824CAE8363.htm)
and [STYLE](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-DXF/files/GUID-EF68AF7C-13EF-45A1-8175-ED6CE66C8FC9.htm)
contracts define X/Y reflection bits 2/4, but do not establish a live combination
rule. The pinned [ezdxf renderer](https://github.com/mozman/ezdxf/blob/dd7f6cc34983fdbf0b2b910f2497e0720d06dd16/src/ezdxf/addons/drawing/text.py)
executes 48 literal placement cases: all 16 STYLE/entity flag pairs across
Left/Aligned/Fit. Root independently reproduces exit 0 using real entity,
placement and matrix code with mocked width/height metrics, not font shaping or
AutoCAD visual proof. Anchored X reflection uses the reference interval midpoint
and reverses its actual baseline while preserving both canonical anchors; extend
that rule to the explicit multiline block's maximum-advance interval. The
private oracle and retained failure fixture are under
`/private/tmp/librecad-arabic-transform-proposal.G8AYem`.

Current-profile mutation decisions must query current entity/STYLE authority,
not a cached rendering status. A default-disabled native request may classify
that authority without font access or layout. Direction-only edits must not
promote an untouched imported entity selection to explicit provisional opt-in
through whole-value replacement. Until saved-profile activation is proven,
reject that implicit conversion without changing canonical data or geometry;
explicit typed selection and inherited direction-only overrides remain distinct.
This temporary guard is not the final editing policy: known imported profiles
and direction edits remain required production P5 behavior.

The provisional inert full-model carrier codec is approved for implementation
with checked little-endian framing, a fixed field registry, exact raw/null/IEEE
retention, a body digest and aggregate 64 MiB / 1,000,000-item bounds. Current
source/convention and materialized entity profile, including direction-only
overrides, are separate from historical import/raw metadata. Current STYLE
identity and profile value preserve association, not permission to overwrite
shared document styles. Decode alone grants no graph, baseline or activation
authority. The bridge must verify unique dictionary/XRECORD ownership and an
exact observed target-wire baseline before initializing current state; retained
history cannot replace a current edit. Pure-codec tests do not complete native
transport, parser raw capture, R12 downgrade, independent interoperability,
schema freeze or activation gates.

For private two-pass output, plan the complete carrier topology before the
first write. The first transaction-owned staging file already contains the
final host → extension dictionary → XRECORD links, canonical entry, owners,
flags and reserved handles, with a bounded inert placeholder value stream.
Capture the actual typed target baseline from that file, checking its planned
associations. Freeze current source, STYLE/profile authority, emission order and
handle-allocation inputs across both passes. The second pass may fill only the
carrier record values; attaching a dictionary or reminting handles afterward
would invalidate the captured baseline, which includes the host's dictionary
link. Reread the final staging file and require exact baseline and graph identity
agreement. The final reserved carrier must decode Known and exactly match the
intended complete model/current-state payload; baseline and links alone cannot
detect a placeholder or stale record values. Perform all of these checks before
atomic destination publication. A remap or other native-field
change refuses; do not guess or patch the baseline to make it pass. The inert
placeholder never becomes a Known model or reaches the user destination on
success, retry or error. Inject failures between both writes/rereads and final
publication, and verify the destination and live source remain unchanged.

### Inert retained-model codec validation

The bounded service uses envelope 1/model schema 2. The reviewed
[field registry](development/embedded_text_model_registry.md) maps retained native
fields, current source/profile/deformation, STYLE association and history without
granting graph or renderer authority. Common fields 42–64, text field 117 and
embedded field 228 retain wide handles, owner/link/presence discriminants and
X-axis precedence. Nested ATTRIB/ATTDEF processing does not prove that precedence
flag finalized; retaining it avoids conflating conflicting angle/vector histories.
Shared STYLE import-name/UTF8 and unavailable-profile histories remain under
actual STYLE/document transport authority, not a reconstructed entity-carrier
STYLE clone. That future authority must be proven or capture must refuse safely.

Schema 1 is retained as exact bounded Opaque raw, never zero-filled into schema 2.
The original provisional registry's root 57,523 / 10 passes are historical
algorithm/framing evidence only; the field audit invalidated its completeness
claim. Required-field literal tests now cover the corrected registry, while
upstream parser-discarded native data and typed capture remain explicit gaps.

The root snapshot `/private/tmp/librecad-arabic-core-index.TOns5t` is based on
`883aec664` plus only the four files below and their owned CMake/qmake
registrations. It excludes pending STYLE, entity-adapter, native-format and
filter candidates. Staged and committed source/build identities were checked
against these inputs for checkpoint `de670d842`.

| Frozen file | SHA256 |
| --- | --- |
| `lc_embeddedtextmodel.h` | `c244cfa7ce704007d9889f9ee9a4c66ec88b8558898db75b37faefee2057c5f8` |
| `lc_embeddedtextmodel.cpp` | `00b3883c866d0fd96023ef71e7e1663ee050e4c8dfd580dfc6acb0d8d1b25422` |
| `lc_embeddedtextmodel_tests.cpp` | `1ed41c0b3358b70c8d3814637e68b6157968a3e653b5f2220f0c414bac2b3487` |
| `docs/development/embedded_text_model_registry.md` | `cd81a1346b0568e139da31cc7a8cb90e0e941d0920dcc382ae1b676e9da10d67` |

Root CMake lanes use `BUILD_TESTS=ON`, enabled and explicit disabled OpenType
configurations (OFF also disables FreeType discovery). The ASan/UBSan lane
instruments all candidate/application-library sources with
`-fsanitize=address,undefined -fno-omit-frame-pointer`; runtime uses
`ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`.
Qt/Catch/font dependencies are prebuilt, and this is not leak or installed-package
acceptance. Toolchain is AppleClang 21.0.0, arm64, Qt 6.11.2, Catch2 3.16.1,
HarfBuzz 14.6.0 and FreeType 2.14.3; CMake build type is unspecified.

For each `build-on`, `build-off` and `build-asan`, explicitly build and run
`librecad_embedded_text_model_tests`, `librecad_embedded_text_xdata_tests`,
`librecad_text_profile_tests` and `librecad_text_profile_binding_tests` without
filters: final counts are respectively 102,708 / 13, 21,535 / 5, 4,544 / 12 and
8,150 / 22, all exit 0. Root logs are
`/private/tmp/librecad-arabic-model-schema2-root-{on,off,asan}-{configure,tests-build,tests,raw-tests,profile-tests,binding-tests,application-build}.log`.
CMake builds `librecad` in ON/OFF and `librecad_lib` under sanitizers, all exit 0.
The existing matching-header qmake shadow
`/private/tmp/librecad-arabic-atomic-qmake.UDlZpE` regenerates with
`qmake6 /private/tmp/librecad-arabic-core-index.TOns5t/librecad.pro LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true`,
then `make qmake_all` and `make -j1`, all exit 0. The new model translation unit
is actually compiled. Its logs have the same prefix with `qmake-{configure,all,application-build}`;
SDK/deployment warnings remain, and disabled postprocessing is not package proof.

The supplementary independent-literal selector
`Full embedded model decodes independent field and literal fixtures exactly`
passes 820 / 1 using `LC_EMBEDDED_MODEL_LITERAL_OUTPUT=/private/tmp/librecad-arabic-model-schema2-root-literal.bin`.
System `shasum -a 256` independently confirms its 9,461-byte body:
`4f9f21576ca716113ae30227a79ae70704c203b31bc49508d10e158bd02a49ca`.
This proves retained DTO bits, not original-wire completeness, native ownership,
transport interoperability, schema freeze or saved-profile activation.

### Private retained native bridge validation

The private candidate `/private/tmp/librecad-retained-native.u2kyfN` captures
actual ATTRIB/ATTDEF plus actual embedded MTEXT into schema-2 native records,
then restores owning inert native objects. It does not fabricate a missing body,
source/profile/STYLE authority, canonical deformation or target-wire baseline.
The native parser prerequisite retains nested rectangle/extents/spacing,
presence/count/discriminant state and complete retained handles before
publication; its original failing-before fixture reports seven failures of 16
assertions. That proves a bounded parser-publication defect, not original-wire
completeness or independent interoperability.

Independent complete mapping review approves the native bridge, protected-state
accessors and parser-only group-101 provenance prerequisite. The final test
adds literal nested DWG coverage. Its initial normal and ASan runs each fail
two of 540 assertions because the fixture's default layer-0 handle was expected
as `0x10`, while the existing writer assigns `0x12`. Only that test expectation
and its explanatory comment change; code/size checks and production writer
bytes remain unchanged. Final normal and O1 ASan/UBSan runs each exit 0 with
540 assertions / 9 cases, followed by no-op dependency checks. The sanitizer
harness instruments all private native and bridge/model/raw/profile/test
sources; external Qt/Catch are not instrumented and leak acceptance is not
claimed.

`INDEPENDENT_REVIEW.md`, `review-current-owned-sha256.txt` and
`review-oracle-{before,after}-{normal,asan}.log` retain the exact scope and
results. Final bridge CPP SHA256 is
`e61c49cc37409e4b1824e6a1e069e4bc1f0245108c0e72d78ed7f269085a22f8`;
test SHA256 is
`30b66a2efd6277bdf52a78d3393b7b7a816a817cf7417a076d8a84bec7b16927`.
This remains a private proposal: application source/test registration, coherent
current-header consumers, actual filter capture/reconstruction, current-source
restoration, live STYLE authority, carrier graph and observed target-wire
reconciliation are not accepted by these pure native tests. No saved profile
is activated and no P5 transport gate is closed.

### Private observed target baseline validation

The private candidate `/private/tmp/librecad-observed-native.yoKlRf` extends the
retained native bridge with explicit input format, registered source-version
ordinal and source convention. It captures only the actual supplied typed
ATTRIB/ATTDEF and its actual body before compatibility decoding. A genuine
missing body produces Baseline6=false with no field8, even if historical Top4
exists. The caller must establish that the DTO came from the actual target
reread; a synthetic DTO is not that proof. Local Baseline framing, native
children and arrays share byte/item/depth budgets; final full-root validation
still charges the envelope, enclosing field and other records.

Root review found that the common Schema2 Baseline validator accepted version0,
future19 and wire-name1032. The independent full-root before regression exits42
with 21 failures of 3,257 assertions in one case. The repair requires registered
ordinals1..18 and the native bridge statically checks the enum endpoints.
Positive endpoint controls pass; the repaired selector passes 3,260 / 1.
Unknown numbers in this known schema remain retained Invalid, never activated.

Fresh strict normal and whole-native-source O1 ASan/UBSan unfiltered suites each
exit0 with 109,348 assertions / 29 cases. They include actual typed DWG2013
outer-only and DWG2018 embedded rereads, ATTDEF prompt and native raw-state
fixtures, exact aggregate bounds with N−1 refusals, class/metadata negatives,
and all original literal model/native tests. The sanitizer configuration
instruments all 42 project translation units; external Qt/Catch/system libraries
are not instrumented and leak detection is disabled. Source-manifest verification
exits0 after the final runtimes and again during root review.

The six-file final patch SHA256 is
`efa9f125a6fe736c2d87486ece44977a55c68b6149e61e1f0f473c0fdbf52587`;
the 418-file source/harness manifest SHA256 is
`40ee6164e186c4f815da5e58d7bd071c49dddfec0fbfa654a23f64c5cae3b4e2`.
`HANDOFF.md` records exact configure/build/runtime commands and retained logs;
its SHA256 is `2eee1fa843991aae3b112debf0f7e9b1348ad327577456d7509fc41f85c1166d`.
Current/Imported source validators from the separate current-source proposal
must merge additively rather than replacing either private snapshot wholesale.

Failed history remains distinct from this passing evidence. Initial fixtures
used ATTDEF object kind2 instead of3 and raw count8 instead of9; only those
expectations changed. An earlier sanitizer fixture constructed the out-of-range
C++ enum value1032 and exited134; the typed negative now uses legal
nonenumerator19, while record bytes still test1032. A final filtered sanitizer
invocation exits134 before test execution in Catch configuration/string copying
with a container-overflow report. Its attribution is unresolved. The unchanged
unfiltered binary passes all29 cases with detector options unchanged; no
container check was suppressed. These logs are not reported as passes.

This remains local helper/model evidence, not actual filter consumption,
arbitrary wire interoperability, native graph ownership, current-header
application acceptance or saved-profile activation. No allocator-failure
executable was run for this helper and no P5 transport gate is closed.

### Private owned carrier validation

The private carrier proposal `/private/tmp/librecad-embedded-carrier.FQfVV6`
extends the frozen observed-native closure with canonical entry name
`LibreCADEmbeddedText`, XRECORD magic `LCEM-CARRIER`, frame version1, complete
model-byte length and ordered code310 chunks of at most127 bytes. The enclosed
LCEM envelope retains its schema/length/digest checks. This is unreleased
framing, not an interoperable schema freeze. Prepare returns only a deep-owned
private XRECORD with caller-planned handles; it does not attach, replace or
publish dictionary entries.

Read requires exact native classes, nonzero unique handles, complete entry
streams, canonical reserved identity and unique host/dictionary/record ownership.
Complete inventory is explicitly a caller assertion, not provenance proved by
this helper. Uninventoried raw/unsupported references must leave it Incomplete.
The helper bounds all scanned graph nodes/entries and aggregate dictionary-name
bytes before allocating/copying names. Graph, value capsule and decoded model
each obey the configured ceiling; framing overhead can refuse a model whose
own bytes already reach that ceiling. Unknown frame versions retain their
bounded prefix/value stream without imposing version1 body structure.

Independent review finds two concrete initial defects: unrelated dictionary
entries/names bypassed work ceilings, and another entity/object could claim the
carrier record as an extension dictionary. Literal regressions exit42 before:
26 failed assertions of2,819 in2 cases. Both checks are repaired; exact graph
and transport N/N−1 limits and incoming/self-reference negatives pass. Invalid
or unknown bounded carriers remain inert. `rawCarrier` retains only the exact
selected value stream, not all native XRECORD common metadata or unsupported raw
and handle payloads; the caller must preserve the whole original native graph.

Actual typed DWG2013/2018 and ASCII/binary DXF2013/2018 transport checks verify
literal frame codes/chunk boundaries, enclosed bytes, binary signatures and
actual controlled-fixture host/dictionary/record links. DXF uses the existing
raw OBJECTS writer with explicit binary hex conversion followed by the typed
reader. These fixtures do not assert complete arbitrary drawing inventory or
independent external-reader interoperability. An initial strict compile exits2
only for a Catch ternary expression; the test-only parenthesization and failed
log remain recorded.

Fresh strict normal and whole-native-source O1 ASan/UBSan builds and final
unfiltered runtimes each exit0 with147,724 assertions /38 cases. All44 project
translation units have sanitizer flags and private source dependencies;
external Qt/Catch/system libraries are uninstrumented, leaks are disabled and
no detector is suppressed. The 421-file frozen source/harness manifest SHA256 is
`8b24a61ef6b5ed96ad43358f86f5f8babaaf75c14fcd2868318052924cdf468c`;
all entries verify after both final runtimes. Header/CPP SHA256 values are
`c1d3d71333b027f6b8b3abd6370c63ec305faebb50366f79cc067b62ec6baeab`
and `27269809b6384aaf4b68b2ffe56024bfde266287f5edaafb105762c9f24db8ed`.
`HANDOFF.md`, `build-{normal,asan}-header-confirmation.log`,
`run-{normal,asan}-full-final.log` and `manifest-after-final.log` retain the
exact scope, commands, failed history and final checks. No helper allocator
failure matrix was run while the separate native matrix remained active.

This proposal remains private until actual filter/host consumption and coherent
application validation. Known decoding does not establish target-baseline
agreement, live STYLE ownership, current-source restoration, atomic destination
publication or activation. Preserve all of those acceptance gates separately.

### Private carrier and observed target admission

The frozen proposal `/private/tmp/librecad-carrier-admission.Sh5lLK` adds one
admission wrapper to the carrier closure. It first requires the carrier's
complete-inventory/local-ownership checks, captures the actual typed target with
explicit format, version and convention, and compares exact packed Top6 bytes.
An optional intended complete envelope must also match byte-for-byte. Failed
admission never exposes a model for restoration, while bounded carrier/model
bytes remain inertly retained. It does not normalize IEEE values, remap handles
or synthesize a missing embedded body.

Fresh strict normal and whole-native-source O1 ASan/UBSan unfiltered runs each
exit 0 with 166,588 assertions / 44 cases. Literal closed graphs test Known
admission, stale baseline, full-envelope mismatch, absent/body/version/format
errors and exact N/N−1 budgets. Six actual typed DWG2013/2018 and ASCII/binary
DXF2013/2018 fixtures fill Top6 from pass1, change only record values for pass2,
then compare the actual reread baseline and complete bytes. Their callback
inventories intentionally remain Incomplete, so they do not prove Known
admission for arbitrary drawings, production placeholders or atomic save.

The complete 423-file source manifest has SHA256
`f84787dae16e6a3f86c4f9ea2088588bd7621aac5724c9149ae75a7ab2bf3c86`;
all entries verify after the final runtime. All 45 project translation units in
the sanitizer lane carry O1/debug and address/undefined instrumentation; external
libraries are uninstrumented, leaks are disabled and no detector is suppressed.
Independent complete source and additive test reviews approve this bounded
proposal. `HANDOFF.md`, `run-normal-full-limits.log`, `run-asan-full.log` and
`manifest-after-asan.log` retain exact scope and results. Actual inventory
provenance, filter consumption, STYLE/current/history restoration and verified
destination publication remain mandatory separate gates.

### Private verified native publication boundary

`/private/tmp/librecad-verified-stage.kWM8GZ` adds a synchronous read-only verifier
after successful flush/close of the owned adjacent stage and before the existing
platform replacement. A rejected or throwing verifier removes only the private
stage. Independent review finds nested null-verifier reentry could abort the
active outer stage; the actual before selector exits 42 with 2 / 8 failures.
The guard-before-abort repair passes fresh strict normal and O1 ASan/UBSan,
each 93 / 9. Its four-input manifest is
`5ad0d3412481832e45fbd0055cff50a407ac7e9bfc667ba58ff850e4b568614c`.
Original stream-exception-mask hardening and other platforms are not claimed.

The independently isolated utility checkpoint is
`/private/tmp/librecad-verified-stage-checkpoint.Weq5OX`: an archive of
`293670bcaf43b8159395d5a5473bebf9b5500e02` plus only the exact final helper
h/cpp, qualified utility test and 14-line standalone CMake target. It excludes
all dirty native API, canonical101, entity/block/filter and deployment changes.
Fresh CMake ON/OFF/O1 ASan sessions 56871/71037/77947 each finish 0, passing
93 / 9. Both project TUs are private and strict; sanitizer compile/link flags
are checked. Qmake library-only session 29726 finishes configure/build/dependency
recheck with 0. This is explicitly not a full application build. All lanes
verify the 5,829-file source manifest before/after; its SHA256 is
`61224ed1b0c9254a5083be38509f080919443afe491f7a654303fbb5721d7161`.
`run-checkpoint.sh` retains exact commands and guards reused build/status paths.
Helper h/cpp SHA256 values are
`f1666cc73f9ab40db72ec72b56c5dbfebfee01af02c368777a3c0b401498d247` and
`ccc599b6cc9aff4032ea43dcbac5be355bfcecafd1dde020fa61df4cd36fa473`;
the qualified utility test is
`0be5f79aa5d4a703f026d787ea9ed2db291ab26fec372cc444d108d1dca25798`.

The separate frozen native consumer
`/private/tmp/librecad-verified-writers-reviewed.fZwPVN` adds named
`dxfRW::writeVerified` and `dwgRW::writeVerified` entry points. The verifier must
be nonnull; its explicit context is per call. Neither is stored in the writer
or added to its interface vtable. Ordinary writes keep their existing API. Verification runs
after native finalization and writer destruction; invalid or failed writes do
not invoke it. Encoding may mutate DTOs, so a filter must emit private copies;
there is no live-document rollback or same-writer/interface reentrancy support.

Fresh strict normal session 5018 and whole-native-source O1 ASan/UBSan session
72404 each finish configure/build/unfiltered runtime/source checks with exit 0:
175,314 assertions / 56 cases. Actual typed DWG2013/2018 and ASCII/binary
DXF2013/2018 stages cover accept, reject, exception, prior-target preservation,
cleanup, same-object retry, context changes and subsequent ordinary writes.
Their callback inventories remain Incomplete: rereading alone does not admit
a Known model. Independent audit verifies all 46 private project translation
units and the sanitizer link, with external libraries uninstrumented, leaks
disabled, no detector suppression and no allocator override. All 424 input
hashes pass before/after both lanes.

The complete source manifest is
`2c7de760750d51ad8493e648d2a7223fe54bb990881c0293f55fe6849a0d283e`;
the lossless ten-path patch is
`b1704307c8a47941a0c8fd24fdbd2d9c60c53c6c7c5763496f86e3a629263eb7`.
The original unbuilt Xxd90x patch omitted a trailing context line; its failed
dry-run is an artifact-generation failure, not compiler evidence. Independent
review also corrected one qualified test include before any compilation.
`REVIEW_HANDOFF.md`, `run-lane.sh`, lane statuses and runtime logs retain these
identities and the final evidence. Integrate additively with canonical101 and
newer History/Current/filter revisions. Production two-pass freezing, complete
parser-backed inventories, exact Top6 and intended complete-model agreement,
coherent application acceptance and activation remain open.

The seven reviewed production files and utility test are now integrated
additively in the shared working tree, without checkpointing the native APIs.
Independent review verifies six production files byte-match fZwPVN; shared
libdxfrw.cpp differs only by the four reviewed canonical101 writer hunks.
The merged source-only closure `/private/tmp/librecad-verified-canonical.juU83Q`
byte-matches that shared writer. Its initial 419-entry rg manifest omitted five
hidden native metadata files; original normal session 10256 and that manifest
are preserved, not represented as complete checks. Corrected complete
424-source manifest SHA256 is
`b87b180657a40aa9a165437746814a413e5a5b8d3295af1a1dfdde0a21cc2057`.
Normal recheck session 39539 verifies that manifest and unchanged matching binary
before/after, with full 175,314 / 56 runtime. Fresh ASan session 78226 also
finishes configure/build/link/runtime and both complete checks with 0, passing
175,314 / 56. Root audits all 46 private TUs (36 native + 10 strict helper/test),
instrumentation and link; no compiler/runtime diagnostic or allocation override.
`ROOT_HANDOFF.md` preserves exact artifact hashes and incomplete-inventory limits.

The separate bounded wire check `/private/tmp/librecad-canonical-wire.e2Dv8i`
reuses that exact normal native archive without mutating it; only its strict
private test TU is freshly compiled. Session90905 finishes configure/build/link,
unfiltered runtime and before/after checks with0:710 assertions/6 cases. Native
external-fixture ATTRIB/ATTDEF, canonical101 before outer XDATA, continuation,
legacy100 ambiguity, copy/reset and embedded-body-XDATA refusal are covered.
The existing Python3.12.14/ezdxf1.4.3 reader separately verifies original and
native-written paired bodies, STYLEs, outer fallbacks and both outer namespaces;
an independent audit requires zero errors and zero fixes and reports both0.
Input manifest is
`d62305a6e7b20033a4d73a7aea68069e7394ee306843f091dc9c3299e3a0d44e`;
paired output is
`eb3210aef701d771be5bc665f1bd0d4faffd02e848f264c2619888089fdcf75e`.
`ROOT_HANDOFF.md` and `status.log` retain exact archive, driver, dependency and
result identities. The external Python package closure is not fully hashed or
instrumented. This is not fresh application acceptance, full-model carrier
transport, whole-filter save/reopen or complete closure of the broader finding.

Separate sanitizer test-only replay
`/private/tmp/librecad-canonical-wire-asan.p9dA3K`, original session78893,
finishes 0 with the same710 / 6 and byte-identical paired output. One fresh strict
O1/g/address/undefined/frame-pointer test TU links the exact reviewed instrumented
native archive; actual compile/dependency/link and upstream424/own11 before/after
checks pass. External Qt/Catch are uninstrumented, leaks disabled, no detector
suppression or allocation override. ezdxf paired semantic check/audit again
report errors0/fixes0. Input manifest is
`795dccf507eaa89cf22b3ee5d3c9b1f950bba19f01c2d0e9d0b503d7926d4604`;
`ROOT_HANDOFF.md` retains hashes and limits. Independent normal and sanitizer
evidence reviews approve this bounded replay, not full production acceptance.

The bounded checkpoint source
`/private/tmp/librecad-canonical-checkpoint.yDMeyb/source` is an actual archive
of parent737f plus only eight owned paths: canonical DRW_Attrib h/cpp,
four canonical writer hunks, ten CMake target lines, the exact six-case scope
TU and three external assets. Complete5,833-file manifest SHA256 is
`279440cd0c7ecc62f4d33608244545794939ffc84215dcbf5673406e392d27c1`.
An independent Git-blob whole-tree check verifies exactly four changed and
four added files; verified writer APIs and unrelated shared WIP are absent.
Root/peer approve the complete source, tests, registration and guarded driver.

Fresh CMake ON40185/OFF9829/ASan29388 all close0, each710 / 6 plus registered
CTest. All36 native and two strict test/helper TUs compile freshly; actual
flags,87 private dependencies, exact36-member archive and link checks pass.
ASan/UBSan covers all38 project TUs at O1/g with frame pointers; actual binary
imports ASan42/UBSan14. External libraries remain uninstrumented, leak detection
alone disabled, no diagnostic/suppression/allocator override. Complete source
and direct-input checks pass before/after. Three paired outputs match the
earlier independently checked bytes; fresh ezdxf semantic checks again pass,
and paired-output audit reports errors0/fixes0.

An existing-public-API probe compiles fresh once per exact matching archive/
header pair:baseline69694 exits42 with six failures among11 checks; corrected
46213 exits0 with17 / 17. The baseline reuses the completed Weq qmake native
archive only after all120 native/common.pri Git identities and exact36 members
match parent737f. Both124-input checks and actual matching-header MD closures
pass; no old-layout test object or new baseline production build is claimed.

Original shallow-shadow qmake56680 closes0 but resolves its relative output
outside the owned shadow, so it is not accepted as owned-shadow validation.
Retain those outputs/logs. Separately reviewed nested qmake14514 closes0 with
precompiler path/source audit,36 fresh native compilations, exact archive,
no-op dependency recheck and complete source/input checks0. It builds only the
native library, not the application. `ROOT_RESULTS.md` retains all lane and
artifact identities. The exact eight source/build paths plus this plan are
committed as `54d5adb7fe55cf32f26ab52851e68a81af096c77`. Root independently
checks the parent, exact nine committed paths, every tested source blob and the
reviewed plan identity; the index is empty and unrelated WIP remains unstaged.
Full filter/carrier/STYLE/history, activation,
DWG/pre2018, whole application and the broader finding remain open.

### Native verified writer checkpoint

The bounded source `/private/tmp/librecad-native-verified-checkpoint.JjaoZf`
is an actual archive of54d5 plus only five native API files, thirteen CMake
registration lines and an independent154-line LINE test. Its complete5,834-file
manifest is `254174ca4e7581da5b50732d8fa151ad3cd4eff9954123c0ce221922000a4e2b`.
Root and peer approve the full source/test/registration. Pre-run evidence review
tightens exact unique source sets, approved dependency paths and unconditional
terminal checks; the original unexecuted scripts remain preserved separately.

Original ON22725/OFF97138/ASan42247 all close0, each279 / 3 plus registered
CTest. All36 native and two strict test/helper TUs compile freshly; exact source
sets, flags,88 private compiler dependencies, native archive membership and
link checks pass. ASan/UBSan instruments all38 project TUs and link at O1/g
with frame pointers; actual binary imports42 ASan/13 UBSan symbols and links
the ASan runtime. External libraries remain uninstrumented; leak detection
alone is disabled, with no suppression or allocator override. Nested owned
qmake74039 closes0 with exact36 source/compile/archive checks and a no-op
dependency recheck. All complete source and six direct-input before/after
checks pass; no build or test lane is retried.

The actual native LINE callback rereads DWG2013/2018 and ASCII/binary
DXF2013/2018 before publication. Acceptance, rejection/throw cleanup and
prior-file preservation, same-writer rejected retry then ordinary write, null
verifier/interface, invalid version and throwing emission are covered.
Ordinary write remains unchecked; verifier context is not sticky. Callback
stage mutation/reentry is excluded, and DTO/interface side effects are not
rolled back. This checkpoint does not establish complete payload/graph/current
authority, application/Windows, filter two-pass save, Arabic activation or P5
completion. Exact lane commands, logs and archive identities remain in
`ROOT_RESULTS.md` under the checkpoint directory.

### Private parser-backed DWG inventory validation

The first freeze `/private/tmp/librecad-read-inventory.4jPCJ7` remains unchanged:
session 2826 exits 2 at two strict `typeid(*unique_ptr)` warning sites after all
36 native TUs build; no runtime. Reference-binding-only revision tZVYuc compiles
but session 96146 exits 42 (165,599 assertions / 51 cases, seven failures in
three cases). Two alias status expectations were incorrect; actual dictionary
positives stopped at an unpopulated typed-table common-tail marker.

Independent decoder/receipt review establishes that exact central read-session
receipts, not that unpopulated typed DICTIONARY/XRECORD marker, authorize those
table common fields. Final separate revision
`/private/tmp/librecad-read-inventory-receipt.6NZC4J` removes only that marker
requirement, retaining exact source/field equality, complete memberships,
raw-shadow checks, mandatory validated receipts and ATTRIB marker requirements.
No parser marker is fabricated. Literal missing/Unknown receipt negatives and
marker-absent authoritative positives pass; alias status remains conservatively
Unavailable with no model access.

Fresh strict normal session 49634 and whole-native-source O1 ASan session 9292
both exit 0: 167,943 / 52, configure/build/link/runtime and both full 426-source
checks. Manifest SHA256 is
`886ea19858ed5ea4f8bafebaca464d959f81d407c8bf91266062892bbbc14875`;
cumulative 12-path patch is
`264ac6562d5734bade223deb5cec537739b232153b6945e12e21fd40778f643a`.
The ASan lane audits 47 private TUs (36 native + 11 strict helper/test), fresh
36-object archive, instrumented link and no overrides or diagnostics. External
libraries remain uninstrumented, leaks disabled, no detector suppressed.

Controlled actual outer-only DWG2013/2018 fixtures now reach receipt-backed
Carrier Known and final Admission Known with exact intended model; aggregate
N/N−1 checks pass. Unmodeled typed/raw ACDBPLACEHOLDER and ignored-callback
inventories remain Incomplete. Embedded copying is source-reviewed, not actual
coverage of every embedded body/class. This certifies only bounded HANDLE/common
links and fixed dictionary carrier inventory, not universal body references,
STYLE/current/profile, DXF, filter or production two-pass publication. No shared
integration or saved-profile activation is implied.

The next source-only seam `/private/tmp/librecad-dxf-observer.G6LIgu` starts
from that exact426-file closure plus the explicit reviewed seven-production-file
verified/canonical overlay, not from dirty shared files. Its nine-path patch
`b983db4fd047495a5a04ecfd44c59a6fda05963f7bc9a402802fc1dae7c1e9be`
adds an optional per-call native observer, checked borrowed group views and
once-only noexcept completion, without changing DRW_Interface's vtable or
storing a sticky observer. Successful skipped999 comments are individually
observed; Unknown ASCII fallback stays Unknown. The fresh local result records
actual mode/version, refusal/overflow and the actual top-level EOF branch
separately from legacy parser success. Native exceptions still propagate after
observer detachment. ASCII spelling is the native value-line spelling after
trailing-CR removal, not original code-line bytes or line terminators.
The complete428-source manifest is
`1d5a703c9ca394321b0703154b327e4ca27008db80fc56d593373eb8fb97a659`.
Root and independent peer source/test readbacks approve the bounded seam.
Original guarded fresh normal29839 finishes 0:168,404 / 63, including eleven
observer cases. All48 fresh private TUs use O1/g/C++17,12 helper/test TUs are
strict,36 native TUs retain the established warning policy. Actual project
dependency, exact native archive and link/no-override audits pass, as do every
428-source/path and446-direct-input before/after check. No diagnostic appears.
Driver SHA256 is
`99fd854c411e433f47874282f21946f4cd96b74aeca8b5c40fc64507defd059f`;
`NORMAL-HANDOFF.md` retains exact result/artifact identities. The pre-run archive
audit correction and old four evidence files remain preserved separately;
source428 is unchanged.

The separately guarded whole-native ASan/UBSan lane16714 also finishes0:
168,404 assertions/63 cases. All48 TUs are freshly rebuilt with exact O1/g,
address/undefined and frame-pointer flags; the private archive contains the
exact36 expected native objects. Actual dependency/link/runtime-import audits
pass and root independently reruns the read-only provenance audit. Both complete
428-source/path checks and447-direct-input checks pass before and after. External
Qt/Catch/system libraries remain uninstrumented, leak detection alone is
disabled, and no diagnostics, detector suppression or allocation override are
present. Driver SHA256 is
`10fb1a10e554b29b8197013d4a28ea1c5a5f8fe32e1acea2e2443dd0afc512bf`;
`ASAN-HANDOFF.md` retains exact result/artifact identities. Identical normal and
sanitizer progress-log hashes reflect identical target-relative object paths,
not object reuse: actual instrumentation and archive/binary identities differ.
This validates the bounded observer seam only. No collector grammar, caller
Complete factory, full graph, filter, loaded restoration or application/qmake
acceptance is established.

### Private neutral DXF inventory validation

The reviewed allocation-preflight successor zWvzRh and its copied Bxtz0l
driver retain the complete429-source manifest
`5379fed7ca744bf4753a177d5602063a993ae7b539092877851a4e7298588fec`.
Original normal36473 and ASan58176 each compile/link0, then stop at audit2
before runtime: selected Qt6.11.2 headers resolve through the meta-package to
`qtbase/6.11.2_1`, outside the original declared root. Both429-source/26-input
after-checks0. No acceptance is inferred from these stopped runs.

The distinct source-identical successor
`/private/tmp/librecad-dxf-collector-qt.4exo5A` changes only the allowlist to the
exact resolved QtCore `Versions/A/Headers` directory. Root and peer verify the
module version, selected/resolved config and binary identities; no broad
Homebrew exemption is added. The unchanged full driver is
`11ab1c9921d88760504ce3480aa0d2d18cb29077fa5dca461d1c186ed3be666a`;
its50-direct-input manifest is
`2339ac2cd5da8aa1ac524ee813a3feab2f9106076cba0b93d4f6aa364a72d168`.

Original fresh normal61763 exits0. Unfiltered runtime passes168,934 assertions
in73 cases; separate `[embedded-dxf-inventory] --success` passes530 / 10.
Both normal and ASan31332 configure and freshly build49 project TUs:36 native
and13 strict helper/test. Root/peer actual compile/progress/own-TU dependency,
ordered native36-member archive and13-helper link audits pass. Normal has109
private/1,188 total actual dependencies; ASan has109/1,189. All49 ASan TUs and
the link use address/undefined/frame-pointer flags; actual imports42/14 and
dynamic runtime are present, with no allocation override.

ASan unfiltered runtime exits0:168,934 / 73. Its separate expanded selector
prints530 / 10 passed, then aborts−6 in Catch::Config/Session destruction with
a container-overflow report involving `vector<string>` and instrumented libc++
string destruction. The driver terminal is2. A mixed-instrumentation framework
boundary is a hypothesis to isolate, not an established false positive or a
passed sanitizer lane. Do not disable the detector or hide the final process
status. External Catch/Qt remain uninstrumented and leak detection alone was
disabled. Both lanes'429-source/50-direct/104-built-artifact after-checks pass.
Normal status SHA256 is
`6570208cc1a16872e76a98356dcba14476683a318f1f0f3a694662334b0b2bb2`;
failed ASan status is
`dee6ec7cce84e16bf288160c297d6006a0f8d88cdf03209824fe725941126d5d`;
its failure log is
`b77a1e68104e0e4ee850b0677304326d17f38d0bd8650b6a4554800d6ade5ce4`.
Keep all predecessor/current artifacts unchanged. No application/qmake,
unique-ownership/DXF Carrier Known, full drawing/model/STYLE, filter two-pass,
loaded restoration, saved activation or P5 completion is established.

#### Isolated framework-only sanitizer diagnostic

Root and an independent peer read the complete eleven-line standard-C++ test,
driver, README and ten-input manifest before the one original invocation:
`bash /private/tmp/librecad-catch-compatibility.FXQ12O/run-reviewed.sh --run-reviewed`.
Manifest SHA256 is
`611a5f0a93059a8db49a8bbe3cfff279154008503a6193313db629a681db0b65`.
The new test owns a `vector<string>` and checks its ordinary values. Its fresh
O1/g strict GNU++17 TU and link use address/undefined sanitizers and frame
pointers, the selected Xcode SDK, and only the exact installed Catch2 3.16.1
static archives. Actual link-map/runtime records contain no LibreCAD, Qt or
native library. The process uses an empty inherited environment; leak detection
alone is disabled. No container detector or compiler instrumentation is disabled.

Original session51412 closes1. Compile/link0; the unfiltered process exits0
with3 assertions / 1 case. The expanded `[compatibility] --success` process
aborts134 with ASan container-overflow in Catch configuration/string trimming
before test execution. Ten direct-input and six built-artifact after-checks0.
Status SHA256 is
`3a3be7ced989c5ccb734d35309d23371183e8520c6470702c13bf818de53049f`;
failure log is
`83223e8f27a5410b7b74affc7123916161350137f74860905fa3862d81d07c14`;
binary is
`2a5bd67db1a98a4d8c3aa4cdd11c171250d8a50d09c976395cb22b2b4c9cc3e5`.
This establishes a failure with no production code linked, not identity with
the collector's teardown failure, a proven false positive, or acceptance of
the collector. Retain both failures unchanged. The next check must use the
exact official Catch3.16.1 source, a freshly instrumented matching framework
and fresh matching test/helper objects, with actual dependency/link audits and
no detector suppression. Do not modify the user's existing Catch submodule or
global installation to obtain that diagnostic closure.

### Private immutable fresh embedded import history

The separately frozen proposal `/private/tmp/librecad-history-bounded.1y6GN1`
captures the actual original embedded ATTRIB before STYLE resolution, deep-owns
its native DTO and raw capsules, and seals it against exact source UTF8,
postconstructor scalar projection and resolved STYLE association. State holds
only immutable shared history in CPP-private Data; old two-argument import APIs
retain their semantics. The new host forwarding is compile-checked, not yet
accepted through an actual application import. There is no loaded-carrier
factory or implicit profile activation.

Normal, disabled-helper and whole-native-source O1 ASan/UBSan lanes each exit 0
for History 143 / 9, State 420 / 9, Current 331 / 10, model 102,708 / 13 and
retained native 540 / 9. The disabled lane checks the pure helper interface, not
the application's native backend. A literal oversized STYLE regression exits
42 before repair because UTF8 conversion precedes budget rejection; repaired
aggregate preflight runs before conversion and passes exact N/N−1 bounds and an
invented decoded-source negative. Root source review approves the bounded result.
The 21-input manifest SHA256 is
`96dd0ee481e7429781057d4e365f610c3b60be89dace29b1e6383313064e4b14`;
the complete copied-source manifest is
`ce8f7743a462e1ac0801cae843d8dfe93d58eb73a9551d50311f7f7062ed263d`.
The frozen subset contains no filter source. Selective `addInsert`/`writeInsert`
consumption, unique actual STYLE catalog proof, full carrier/current/Top6
restoration and application validation remain unfinished.

Fresh-consumer revisions remain separate and immutable. Pre-open profile-Replace
revision cdmKyj adds a prior-file preservation regression; target revision LznbWh
sets scratch STYLE ref/ref64 consistently. Both compile only. Owner revision
`/private/tmp/librecad-history-consumer-owner.cgivVy` then ties the captured
catalog to the live graphic pointer/nonzero getId without dereferencing a saved
pointer, and requires selected raw STYLE name/handle membership and unique folded
name/handle in current native metadata. This also rejects same-object metadata
reset, which does not change getId. Root/peer approve the three-path delta
`c526adacb68083ca6f8fa91ec0a9bfb1a8f77a30c434db89a2ab4fe12d1430b7`;
all 5,831 source checks and the filter/13-case test compilation finish 0.
Copied-host A→B, actual same-address reconstruction and cleared metadata negatives
are included in the first actual coherent suite below; the source-only checkpoint
did not itself run them. Destination/current/raw/history preservation and A retry
remain required and are not replaced by mere successful compilation.
Ordinary file I/O creates a separate export filter with no captured catalog;
this bounded guard therefore refuses unchanged imported embedded bodies as well.
Do not checkpoint that conservative refusal as ordinary-save acceptance or ship
it as complete filter integration. Current STYLE materialization with actual
graphic authority, including original duplicate/provenance checks, is mandatory.

The distinct source-only coherent closure
`/private/tmp/librecad-history-consumer-final.TpAEe7` includes the reviewed owner
repair with exact current native/header prerequisites and additive31-suite
registration. No configured object/archive was copied. Original5,833-listed-file manifest
is `a9c6e829a48b1eb22fdbd01cd59ad89375f16d8d4b9813eb6428b9d69d16b228`;
driver is `e89c1d32de66377c4cc3c7d016dc5e3725ae50bc1cb8580579c36eae15c840fc`.
Original session17464 finishes fresh application/all31-target compilation and
linkage with0, then exits42 at the first unfiltered History suite:739 / 13,
738 assertions pass and one fails at line->isSelected(). Full-source-after
check passes; later30 suites do not run. Root source review confirms the fixture
only calls LC_SelectedSet::add, which does not set the entity flag, so that flag
is false even before addInsert. The first separate fixture-only SSJCiX/93271
attempt exits2 because setSelected is protected; retain that compile failure.
The next GgaMI2 revision uses public setSelectionFlag plus exactly one list add,
with pre-import and post-rejection flag/membership checks. Root reads the entire
driver, Makefile, exact fixture diff and public header before authorizing one
test-only replay against the unchanged archive, headers and dependencies.
Session94753 compiles the one fresh fixture TU and checks its exact dependency
closure0. The corrected filter History suite passes743 / 13, then unchanged
History143 / 9, Current331 / 10 and Native540 / 9 pass. The next unchanged
scopeguard suite exits42:4,984 assertions/16 cases,12 failures in8 cases.
Six assertions fail while opening the absent external DXF fixture; those do
not exercise source semantics. Include the exact independently authored DXF,
README and generator in the next frozen closure without regenerating them.
The other six are modern-positive fileExport assertions exposing the
separate-export STYLE gap. Their minimal source fixtures also omit TABLES/STYLE,
so provide actual explicit STYLE records rather than guessing STANDARD authority.
Do not weaken ordinary-save expectations to accept those refusals.
The remaining26 suites do not run. Listed source and1,284-input checks pass before
and after; original failed sources/binaries remain intact. This is a bounded
ordinary-runtime diagnostic, not coherent application runtime acceptance.
OFF/ASan/qmake are not substituted or claimed. STYLE materialization must repair
the separate-export limitation before the next coherent acceptance lane.

A later independent complete-path audit finds5,850 regular files in that old
Tp source, not5,833: its rg-derived manifest omitted17 hidden metadata/config
files, including .github files, .gitignore/.gitmodules, formatting settings and
five native metadata files. No C++ TU/header is among those17. Preserve the
actual5,833 before/after results, but do not call them a complete source freeze
or manufacture retrospective checks for omitted files. The next STYLE freeze
includes all5,853 regular files (old5,850 plus its three new files); future
acceptance must independently check complete path identity as well as hashes.

### Private native letters repair and diagnostic history

The failed original nJ11IJ fixture compile and rPuxGU six-suite runtime remain
unchanged. The corrected source-only freeze
`/private/tmp/librecad-native-letters-repair.Csh2Qp` changes exactly three files,
with no header/ABI/registration change. Root and peer initially approve a
two-group pinned lam–alef diagnostic and the narrow recognized-native skip for
deleted/invisible/locked inputs before
clone/freshness work. Editable unavailable text still refuses. Cached Legacy
status with newly selected NoUpdate metadata is outside this narrow recognition
domain; full current-authority admission remains mandatory later.

The complete 5,840-source manifest is
`25478941c8878950b40be091b3a7b4a22d2bd01485e67dd9f76ec62ce32d3b49`.
Only an explicitly authorized incremental diagnostic may reuse the matching
950 unchanged production objects after full dependency/source/header checks,
compile the one changed cpp, construct a new archive with all 951 ordered
members, and freshly compile/link the six test targets. It is not a fresh full
application, OFF, sanitizer, allocator or serialized-output acceptance lane.
Prepare-only sessions15879/70096 preserve lexical dependency normalization and
exact leading macOS __.SYMDEF audit corrections; neither is a production test.
The final reviewed runner is
`61bd1ae257c5969760c19001404dc0317b11f652e7b8b42f7768468002ce6f38`.
Original diagnostic40131 exits2 after successful production compile/archive/
ranlib and two test compiles: its guard rejects the unrecorded external
QtCore QTemporaryDir header before further compilation, link or runtime.
All original source/input-after checks pass. Preserve this failed evidence
revision and require exact expanded dependency receipts/readback before a
separate test-only continuation; do not silently amend or rerun its frozen log.

The source-unchanged continuation214BnJ/34434 then finishes 2:letters compile/link
and three normal-explode support objects pass, but the fixture calls protected
countUndoCycles(). No runtime. Separate fixture-only G6nym3 changes only that
check to public const collectUndoState and asserts neither undo nor redo is
available; it does not weaken fresh-history, one-cycle or undo/redo behavior.
The exact unchanged40131 archive is reused, all25 test/support objects and six
links are fresh. Original29629 terminal 0:letters134,610 / 16, normal-explode
1,113,800 / 7, native135,009 / 28, prepared336 / 22, native-source1,065 / 14,
State420 / 9; aggregate1,385,240 / 96. Separate actual mark34,883 / 1 is not
double-counted. Full original/repair source-after and10,571-input-after checks
pass. Both prior failed revisions remain frozen. This is incremental diagnostic
evidence only, not fresh full-source acceptance.

Deeper semantic review corrects the earlier oracle diagnosis. Pinned HB14.6
OT rlig lookup140 substitutes two GIDs retaining two clusters; show-flags marks
the boundary before logical cluster1 UNSAFE_TO_BREAK. Shaper/layout retain this
flag, but NativeBuilder drops it and letters only unions overlapping grapheme
ranges. Two GIDs alone never disproved a one-policy-unit expectation: positioned
two-group geometry is a limited current-behavior diagnostic, not acceptance of
the plan's frozen ligature/attachment grouping. Track P5-LETTERS-UNSAFE-001 and
require conservative actual logical-neighbor unsafe-boundary coarsening, source/
paint retention and paragraph/inline-object controls before feature acceptance.
The source-only repair/design is separate; no frozen run or policy is silently
rewritten to make this diagnostic pass.

The separately frozen source-only successor
`/private/tmp/librecad-native-paint-order.pnYCJy` combines the reviewed unsafe
coupling repair with the EGC paint-order repair. Its complete8-path patch is
`c3d41f35346aeb3150c8d995bc3077580e2d3505e4aefc3f2ad5014d02934ab8`;
complete5,841-file manifest is
`44ced172251536796ceeb3ebaa2a64274e22e96e3192ca6f83462b7ff0453a3a`.
Root and an independent peer approve the new three-path delta and full helper.
Minimum-index union roots preserve first-emission order; noncontiguous EGC
components, including intervening controls and whole inline objects, refuse
without publishing partial output. Literal coincident-color RTL and A/B/C
regressions accompany an actual mixed-paint RTL ordered-leaf update/undo/redo
test. These tests are unbuilt/unrun; the pinned actual safe-boundary fixture
has separate engine-only calibration below, not native runtime acceptance.
Both letters findings remain Open. The inherited
public semantic Role change requires fresh matching consumers, not a stale
production archive with new headers.

Existing pinned HB14.6 tool diagnostics in
`/private/tmp/librecad-native-paint-calibration.WMBWsB` finish0 with explicit OT
font functions, upem scale, RTL/Arab/ar, BOT/EOT and cluster level1. Two alef
emit `[55=1+217|55=0+217]` without unsafe flags; lam–alef emits
`[2518=1+340#1|2513=0+302]`; three beh emit unsafe-before on logical1/2.
Captured results SHA256 is
`f8b557e68a900b1c087d8571f9935932a9d049a247ad29ea64e1468ce3781f33`.
Tool/direct-engine/pinned-font identities match before and after. Paint-only
formatting does not split the shaping run in reviewed source. This calibrates
the fixture's engine expectation only; native parsing, paint, geometry, block
update/history and full dependency/sanitizer acceptance remain pending.

### Current STYLE catalog materialization design

The next approved source-only STYLE design retains a sealed immutable original
catalog with the graphic's live LC_TextStyleList rather than a filter instance.
Capture every original row before map collapse, including duplicate provenance;
current list scalars/bindings remain current authority, while metadata consistency
checks compare original provenance to the catalog, not edited scalar values.
One detached pre-open emission plan must serve both embedded attributes and
writeTextstyles. Imported rows deep-copy original native state before explicit
current overlays; new typed rows use complete supplied current scalars/profile
and native scratch defaults/handle0, never guessed historical provenance.
Missing/removed/ambiguous associations refuse before destination publication.
The QtCore-only history holder must not allow callers to forge sealed catalog
authority. A private per-import-row identity must survive legitimate style/list
copies and current Replace/Remove edits, but not authorize a foreign same-name
row, even when its unavailable transport diagnostics happen to match. Preserve
the original transport and exact typed/raw checks as independent safeguards.
When an imported STYLE is renamed, resolve both references independently:
the embedded body's current reference follows current host authority; the
outer ATTRIB fallback follows its uniquely captured original catalog association
to the current row. Name/handle disagreement or a missing association must
refuse before opening the destination. Modify detached current scratch only,
never immutable outer/history state; do not emit a dangling old STYLE reference.
The later legacy-font fallback must not recreate suppressed original names.
Exclude only sealed embedded hosts whose detached current plan has already
passed preflight. Ordinary TEXT/MTEXT, plain ATTRIB, dimension overrides and
defaults, and header `$TEXTSTYLE` remain independent named consumers. If a
rename/removal leaves one of those consumers without a proven current row,
refuse before opening the destination rather than inventing a remap or guessed
font. Include a nonfirst imported STYLE selected by `$TEXTSTYLE` and mixed
legacy/modern rename/removal regressions with prior-file preservation.
Reset must clear both list and catalog:committed/frozen baseline
initForNewDocument omits the list clear, while an existing one-line shared WIP
already adds it and remains unvalidated. Import publication timing and separate
RS_FileIO import/export filters require actual tests; no full-import rollback,
arbitrary historical STYLE graph or saved activation is implied by this design.

The first concrete ten-path source-only freeze is
`/private/tmp/librecad-style-materialize.7PRrg9`. Its full patch is
`77f6a66e144208998d1b556f20db10cf89b64fb523df7987637dee5294c6f223` and
complete5,853-file manifest is
`e665aacb3843693a93eadb244e77dd174d776fadd266c9c52b8b24b560d96bd5`.
Root reads the entire production/test delta and the surrounding ownership and
public APIs. Independent complete review requires a separate successor for
the old-name fallback defect and named-consumer consistency above; no build is
attempted, and this initial source remains frozen.
LC_TextStyle and LC_TextStyleList ABI changes require fresh matching consumers,
not reused Tp objects. A separately frozen successor must add the missing exact
external fixture assets, actual STYLE records for modern-positive scope tests,
build registrations and the independently reviewed unsafe-grouping repair.

The separate successor `/private/tmp/librecad-style-successor.HcEKWL` has
fourteen cumulative paths versus Tp and seven paths versus7PR. Immediate patch
SHA256 is `93e17f5ab70098adfe01444c1ed735647f01ade3a28b769114bfb64f42502024`;
complete5,856-file manifest is
`2dbf26138afa7fa8dafb202d057ddc41b2127051f38fd2f830d97a1a82208158`.
Root and independent peer approve its bounded source changes; independent
complete path/hash checks pass. Only successfully prepared sealed modern hosts
and proved deleted-owner attributes bypass legacy font-name synthesis. Root/
block TEXT/MTEXT/plain ATTRIB, dimension overrides/definitions and explicit
`$TEXTSTYLE`/`$DIMTXSTY` consumers are checked before opening the destination;
unsupported old-name references refuse instead of guessing a remap. The final
fallback independently excludes catalog-owned original names.

The source-only tests cover actual DXF2018/DWG2018 body/outer STYLE references,
five mixed legacy-consumer refusals, nonfirst-header rename/removal refusals and
new typed STYLE handle consumption by ordinary TEXT in DWG2013/2018. Scope
fixtures now contain actual STANDARD STYLE records; all three external assets
match the committed checkpoint byte for byte. No compiler or runtime has
validated this ABI-changing successor. Its combined tree must explicitly add
the full inherited letters prerequisites, unsafe/paint repair and catalog
registrations, not only overlay the latest eight-path letters patch.

The independent external fixture also exposes a separate preservation policy
gap: an opaque profile on the outer ATTRIB with an absent body profile is
correctly Unavailable, but the blanket current-profile save guard rejects an
unchanged canonical DXF2018 copy. Do not strip the outer capsule or weaken the
positive regression. A narrowly scoped unchanged-only exception would need
sealed original history, exact namespace/body-absence checks and proof that
current common attributes match an owned import baseline. Existing history
does not retain that complete common-attribute baseline; ordinary import
normalizes `BYLAYER` to `ByLayer`, so exact whole-DTO equality conservatively
refuses typical unchanged input. An explicit baseline extension and target
reread proof remain pending. The separate source-only prototype
`/private/tmp/librecad-outer-unchanged.LNyJ6L` adds a private owning common baseline,
exact source-version/namespace/owner checks and unchanged body/outer STYLE
association checks. Its eight-path patch SHA256 is
`1da36ac984b4af42f1924fe70f44cf461a7549b2c7ca6de6b77dc70681808d8a`;
complete5,856-file manifest is
`944654c71afa756f848215b45084db70ccd3e9b005330a274b484b5251be0bfa`.
It remains frozen, unbuilt, unrun and unintegrated. Review identifies a further
admission gap: `writeEmbeddedMText` omits body common attributes and application
groups, while the canonical reader can retain them. For example, body group60
can set invisible, but output omits it and reread restores visible. An all-zero
group11 vector can likewise lose its retained presence. Original-copy ownership
and native writer preflight do not establish lossless serialization. Audit all
retained outer/body fields against actual writer/reader behavior; explicitly
refuse unsupported original states before publication, retain literal negatives,
and keep the complete native target comparison rather than weakening its oracle.
The complete retained-field audit additionally identifies outer extrusion
presence changing when the writer always emits210, including the existing
positive fixture that intentionally has no210. Preserve that input and
comparison; a narrow optional-extrusion writer repair needs its own actual
native before/after proof. The exception also needs a closed supported-domain
predicate covering omitted body common/app/proxy/DataStorage/text fields,
X-axis precedence and zero/signed-zero omission, outer secondary-point overwrite,
attribute-type normalization, raw typed/string/reference rewriting and exact
ASCII numeric roundtrip at the current16-digit writer precision. Do not infer
wire preservation from finite numbers, default-looking DTOs or private common
equality. Unsupported body fields are a current-emitter limit, not a declaration
that the source is invalid CAD. A distinct source-only repair is being prepared;
its review and actual coherent validation remain mandatory.
The proposed test also confuses raw `\P` with decoded LF in `getText()`, and
two mutation fixtures pass post-alignment-strip text instead of the documented
pre-strip `compatibilityInput()`. Correct those three test-only lines in the
distinct successor without changing canonical/native source expectations.
Profile Replace/Remove must not acquire
durable authority merely from an unchanged outer capsule; full carrier
restoration remains separate.

### STYLE provenance and ownership validation

The isolated source `/private/tmp/librecad-arabic-style-root.DMJiqo` is based on
`883aec664` plus only the four STYLE/list files, their authority tests and the
QtCore-only test registration. Production SHA256 values are:
`lc_textstyle.h` = `ca9f5eb2ca3e1b244f348455e005a7f6175278af1c1016c8e9cb941a155af8ae`,
`lc_textstyle.cpp` = `d4a330c2a3eeac151d5f025cc2bc8832aa43ca3109ba9185dd3eea05f2d8f1a7`,
`lc_textstylelist.h` = `26ea1357b5d7d4ded0e858e7a89058f323924ce3b4eb6c88e9e4a381c1710c92`,
`lc_textstylelist.cpp` = `4ba72adf47dd6195436aeb0206315e75f1e9358a7f517c1a097244d613438253`.
The expanded seven-case test SHA256 is
`0d957dbaf68c63587dd7d40ae560d3019f8fd964638ee691ebc49563e4929435`.
It adds literal native defaults and copied/assigned unavailable-history tests
through Replace/Remove/reset, without claiming native transport acceptance.

Initial registered builds failed with missing `rs_flags.h`; the next attempt
linked unsuccessfully because `rs_flags.cpp` was omitted. All three lanes exited
2 for each attempt. The final target directly compiles committed `rs_flags.cpp`
and includes the engine directory, without GUI linkage. Independent re-review
approves the exact tests and repaired target. Final ON/OFF/ASan focused builds
and runs exit 0: STYLE 286 / 7, binding 8,150 / 22, entity-state 322 / 7 and profile
4,544 / 12. Logs use `/private/tmp/librecad-arabic-style-root-{on,off,asan}-`
with `configure.log`/`tests-build.log` and `accepted-configure.log`/`accepted-tests-build.log`
for the failed history, then `final-configure.log`, `final-tests-build.log` and
`{style,binding,state,profile}-tests.log` for the repaired focused matrix.

Initial fresh matching-header CMake ON/OFF applications, coherent sanitizer
library and fresh qmake shadow `/private/tmp/librecad-arabic-style-qmake.bYFdGZ`
all exit 0. Each CMake lane's registered bidi and writer-lifetime suites also
exit 0, passing 541,335 / 70 and 290 / 4 respectively. Initial logs use
`/private/tmp/librecad-arabic-style-root-{on,off,asan}-application-build.log`,
`-bidi-tests.log`, `-lifetime-tests.log` and
`/private/tmp/librecad-arabic-style-root-qmake-{configure,application-build}.log`.

After all four initial builds terminate, incorporate the exact committed
`de670d842` model files/registrations into the same snapshot. The final candidate
is committed `de670d842` plus only the five STYLE source/test files above and
19 owned CMake test-registration lines. Its `CMakeLists.txt` SHA256 is
`fef1b0f3fd61bce2c6c2984f57daa29db4c972928c46801a0e762e9ca771efd3`;
qmake `src.pro` remains byte-identical to committed `de670d842` because the
production STYLE files were already registered. Pending native, filter,
deployment and other unrelated workspace edits are excluded.

Reconfigure and rebuild the final candidate in each existing CMake lane, then
run the eight explicit targets, without filters: STYLE authority 286 / 7,
binding 8,150 / 22, source state 322 / 7, profile 4,544 / 12, retained model
102,708 / 13, raw capsule 21,535 / 5, bidi 541,335 / 70 and writer lifetime 290 / 4.
Every build and run exits 0. ON/OFF build `librecad`; ASan/UBSan builds the full
coherent `librecad_lib` and instrumented test consumers. OFF remains explicitly
disabled with FreeType discovery disabled. Runtime uses
`QT_QPA_PLATFORM=offscreen`, with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`
and `UBSAN_OPTIONS=halt_on_error=1`; prebuilt Qt/Catch/font libraries remain
uninstrumented. Build type is unspecified. Regenerate qmake and all subproject
Makefiles, then rebuild its application: exit 0, with actual new model-source
compilation confirmed. Final logs use
`/private/tmp/librecad-arabic-style-combined-{on,off,asan}-configure.log`,
`-build.log`, `-<target>.log` and
`/private/tmp/librecad-arabic-style-combined-qmake-{configure,all,build}.log`.
The scoped checkpoint must match these tested source/build inputs exactly.

Commit `c8b5a193c7b4659b88a9e28ecd0a052311e6ee0c` checkpoints only these five
STYLE source/test files, the 19-line CMake target and the plan record. Root
compares the staged and then committed six source/build inputs byte-for-byte
with the tested snapshot; all match, and staged whitespace and shell-recipe
syntax checks exit 0. No native, filter, deployment or unrelated code is included.

Reproduce the final CMake build/run commands with the existing recorded caches
as follows; fresh configurations must retain their explicit backend/sanitizer
settings above. These targets are excluded from the default build and must run
explicitly.

```bash
set -e
task_source=/private/tmp/librecad-arabic-style-root.DMJiqo
task_tests="librecad_text_style_authority_tests librecad_text_profile_binding_tests librecad_text_source_state_tests librecad_text_profile_tests librecad_embedded_text_model_tests librecad_embedded_text_xdata_tests librecad_text_bidi_tests librecad_filter_writer_lifetime_tests"
for task_lane in on off asan; do
  task_build="$task_source/build-$task_lane"
  task_prefix="/private/tmp/librecad-arabic-style-combined-$task_lane"
  task_app=librecad
  if [ "$task_lane" = asan ]; then task_app=librecad_lib; fi
  cmake -S "$task_source" -B "$task_build" > "$task_prefix-configure.log" 2>&1
  cmake --build "$task_build" --target "$task_app" $task_tests -j2 > "$task_prefix-build.log" 2>&1
  for task_test in $task_tests; do
    QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
      "$task_build/$task_test" > "$task_prefix-$task_test.log" 2>&1
  done
done
(
  cd /private/tmp/librecad-arabic-style-qmake.bYFdGZ
  /opt/homebrew/bin/qmake6 "$task_source/librecad.pro" LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true \
    > /private/tmp/librecad-arabic-style-combined-qmake-configure.log 2>&1
  make qmake_all > /private/tmp/librecad-arabic-style-combined-qmake-all.log 2>&1
  make -j2 > /private/tmp/librecad-arabic-style-combined-qmake-build.log 2>&1
)
```

An existing-API probe independently checks native STYLE defaults, deep list
copying and alias-safe self-replacement. It compiles against the pre-STYLE
`de670d842` snapshot and exits 42 with all five checks failing; the same probe
against the final STYLE candidate exits 0 with all five passing. Source and
commands are `/private/tmp/librecad-style-baseline-proof.Qu3f5d/`
`style_baseline_probe.cpp` and `run-proof.sh`, SHA256
`d6d078293f79fe524ff69670d5a7731518b649327de062651029463e33c75d04` and
`fc15d82e5cf86241c9710d509b7b29b1e7c49593adba0e5189fea5c2baf7d4b6`.
Logs are `baseline.log` and `corrected.log` there. The durable seven-case
registered suite covers these contracts plus retained unavailable provenance
and explicit profile edits. This bounded service does not establish native
STYLE transport, imported-profile activation or full P5 completion.

### Canonical affine native candidate integration

On 2026-10-12, the initial shared native candidate acquires seven frozen
native/entity/test files from
`/private/tmp/librecad-arabic-transform-proposal.G8AYem/source`.
Root independently reads the complete seven-file affine/entity/test delta,
new native header and native builder/publication path. That initial review finds
no proven defect; the later allocation-boundary review below identifies a
separate unguarded exception path. The author's separate re-review is not
independent verification.
The original unguarded candidate has the following historical SHA256 values;
the current guarded candidate is recorded separately below:

| File under `librecad/src/lib/engine/document/entities` | SHA256 |
| --- | --- |
| `lc_textnative.h` | `0d9ac9359f4c67f4a24402234e65cbf13fa290216e2f21dc104a79698e922c48` |
| `lc_textnative.cpp` | `31c0399be027e3f54a43290727f7eab6e62ded6ac90bda1c44e93c358f98ef9a` |
| `rs_text.h` | `b76b4de1e657f4bdfe6d932e545a95eb0c0ea329183d13bdd13794e561ec942c` |
| `rs_text.cpp` | `9f236a9d97d265b411995add94594099dd9740c38fe51085c0eb0ff0b1464494` |
| `rs_mtext.h` | `042f54ac947ffaab3132f5f0a942c98ad84c3d33f98de256e358ee5eb681e728` |
| `rs_mtext.cpp` | `e36a039ffddda514db41c83ef5b44d71558ae27ea77ccf37478eb51cfcaae16e` |
| `tests/lc_textnative_tests.cpp` | `8dd9ae4e7218fe0fbfc19112d4d0f851c11a23eaeb7f50e7fd083abdf08462ad` |

This implements canonical deformation, single-operation virtual shear,
geometric generation, reconstructed anchors/metrics and final-cell draft
thresholds. The sampled one-way transformed-segment comparisons in the new tests
are reconstruction diagnostics, not a complete independent contour-error oracle.
The earlier continuous-outline and pinned placement gates remain required.
No native commit or full P4 acceptance is claimed. The isolated candidates below
rebuild all affected consumers against matching public headers. Verify the full
guarded ON/OFF/sanitizer native suite and both application build systems before
staging this slice. Durable P5 transport of the deformation and saved-profile
activation remain separate mandatory gates.

The root isolated candidate is `/private/tmp/librecad-arabic-native-root.8AZPfW`:
an archive of committed `c8b5a193c`, the seven files above, nine owned CMake
registration lines and two owned qmake source/header lines. It excludes the
uncommitted filter, native-format, deployment, CI and explode candidates.
`CMakeLists.txt` SHA256 is
`5fb75bf62c70333f9c675999fc3399233ee9dc7f9455c6b04292e89c69894a31`;
`librecad/src/src.pro` SHA256 is
`80e2d81f110475ea9db5a13d4ab5778e7e32ce9820fb120c95522880545e988d`.
Native tests use the existing settings-isolated application microtarget,
`LibreCAD::OpenType` and the pinned font definition. ON configuration and the
fail-fast full `librecad` application/23-executable command exit 0. Native tests
pass 135,009 assertions / 28 cases; all 22 other explicit executables pass,
including the standalone binding-copy and geometry-replacement fault checks.
The exact `run-on-validation.sh` recipe, `configure-on.log`, `build-on.log` and
`on-<target>.log` records are in this directory. Root reads every runtime footer.
This is ordinary native/regression acceptance for the frozen candidate, not
proof for the still-open native allocation/status boundary or other build lanes.
Fresh qmake shadow `/private/tmp/librecad-arabic-native-qmake.V9ZT58` configures
with `LIBRECAD_ENABLE_OPENTYPE=true DISABLE_POSTSCRIPT=true`; qmake6 and
`make qmake_all` exit 0. Logs are `qmake-configure.log` and `qmake-all.log` in
the native source snapshot. The existing Qt SDK-version warning is retained;
application `make -j2` exits 0, recorded in `qmake-build.log`. This is the
unguarded baseline application only; package/SDK acceptance remains Pending.

The native allocation review finds that an exception before geometry publication
can escape `updateNative` while leaving the previous Native/no-error status in
place after an accepted source edit. Failed attempted updates must retain the
deliberate source edit and all last-good geometry/metrics/layout/selection, but
report rebuild failure rather than current success. A rejected transform keeps
unchanged Data and may retain its valid prior render status with an explicit
operation failure or propagated exception. A postcommit observer exception is
not a precommit failure and must not cause rollback or false unavailable status.
The standalone allocator injector now reproduces `P4-NATIVE-ALLOC-001` against
the exact matching root ON archive: exit 45, `stale_update_status`, unchanged
TEXT update, single failure at allocation 1. The executable stops at that first
failure; this before-run does not establish MTEXT or accepted-source-edit case
coverage. Its test SHA256 is
`554436f82664b76170ef9763920693fc05b411451e667dfee7668cecb6c3c0f0`;
`/private/tmp/librecad-native-failure-proposal.1yuLyP` retains `before.log`,
`before.exit`, compiler inputs, archive hashes and the link map. Root verifies
the map/dependency inputs contain the matching native/TEXT/MTEXT archive
members and no shared-worktree, Catch or object-override splice.
The first strict compilation fails on 16 existing aggregate-initializer warnings
in the matching metadata header; a second build preserves those warnings while
demoting only that diagnostic and successfully links the standalone executable.
The guard is now integrated before Request preparation in both native updates.
On an ordinary update's prepublication exception it moves an allocation-free
`PreparationFailure` status into place without changing accepted Data or
last-good geometry, metrics, layout or selection. Transform preparation leaves
unchanged Data and its prior valid primary status; the guard is disarmed after
the geometry swap and before no-throw scalar/state publication and postcommit
observers. Independent source/test/registration review approves the final
delta after adding an explicit transform-primary-status assertion. The private
full-native translation unit also compiles the no-throw assertions successfully;
that compile-only result does not establish repaired runtime acceptance.
NoUpdate's last-render cache does not establish current layout freshness for
any consumer.

The original frozen guarded root candidate is
`/private/tmp/librecad-arabic-native-guarded.fQNeos`, an archive of `c8b5a193c`
with only eight owned native/entity/test files, 29 CMake registration lines and
two qmake source/header lines. The unrelated filter, native-format, explode,
deployment and CI candidates remain excluded. This historical manifest predates
the current test-only search-bound repair and the separate source-state slice:

| File under `librecad/src/lib/engine/document/entities` | SHA256 |
| --- | --- |
| `lc_textnative.h` | `590ccbd80b1cf4e57f2cf2f00fc9c5e5f69581959488decbe59005a437bcf153` |
| `lc_textnative.cpp` | `168fa36cfdbb4e66a9c4674b01fa6919ee33aa0c48ff5cee3144ba8632035fa2` |
| `rs_text.h` | `b76b4de1e657f4bdfe6d932e545a95eb0c0ea329183d13bdd13794e561ec942c` |
| `rs_text.cpp` | `8625792888e3504908b64a83e4ec57bf0a9e52c0bfbc85c949b530f2bfca2adb` |
| `rs_mtext.h` | `042f54ac947ffaab3132f5f0a942c98ad84c3d33f98de256e358ee5eb681e728` |
| `rs_mtext.cpp` | `730760dc7425c6331ea7eb76fea1dc78f6d688d90df2f0b703ccef0c3d4b27f7` |
| `tests/lc_textnative_tests.cpp` | `8dd9ae4e7218fe0fbfc19112d4d0f851c11a23eaeb7f50e7fd083abdf08462ad` |
| `tests/lc_textnative_failure_tests.cpp` | `8e3df7ad364abe30a80ac3f26024ad31555073abad11502e345cc051cf89d4bf` |

Frozen `CMakeLists.txt` SHA256 is
`a0148cce44eaecc84384a4059b29a2533202f24ef04ebe584548e89df9b26978`;
qmake source SHA256 remains `80e2d81f110475ea9db5a13d4ab5778e7e32ce9820fb120c95522880545e988d`.
The standalone failure target has its own main/settings isolation and ordinary
C++ allocator override, with no Catch/listener linkage. It checks both entity
types, seven update/edit/transform actions, single and persistent failures,
exact last-good state, retry and postcommit observer exceptions; the OFF lane
checks refusal and preserved state. Its warmed ordinary-C++ allocation boundary
does not cover Qt-private allocations, aligned allocations or cold external
resource loading, and its twin oracle is publication consistency, not typography.

Fresh enabled and explicit disabled configure exit 0. The fail-fast
`run-validation.sh` (SHA256
`5a83054946c0bc74b4bdd21a6317c6087d09f68455d223d4b32d5509d84bab9b`)
builds the application and explicitly runs all 24 executables. Commands are
`bash run-validation.sh on 2`, `bash run-validation.sh off 1`, and
`bash run-validation.sh asan 1`. All application builds and the 23 preceding
executables finish; native suites pass 135,009 / 28 in ON and ASan. The complete
OFF invocation exits 0, including six refusal/preservation checks for each
entity type and zero executed native allocation paths. Original ON exits 53:
four unchanged-update contexts finish, but the following edited-update context
exhausts the 8,192 search ceiling. The original ASan log likewise records only
four completed update contexts; its terminal exit is unavailable. Neither
result establishes the full failure matrix. ASan/UBSan uses a fresh Debug build with O1/debug,
address/undefined instrumentation and frame pointers; leak detection is disabled
at runtime and external Qt/Catch/HarfBuzz/FreeType remain uninstrumented.
Logs are `configure-<lane>.log`, `build-<lane>.log` and `<lane>-<target>.log`.
The guarded qmake shadow is
`/private/tmp/librecad-arabic-native-guarded-qmake.n1hu4r`; enabled qmake6 and
`make qmake_all` exit 0. The application build log reaches final tools linkage;
a fresh `make -j1` dependency/completion check exits 0, recorded separately in
`qmake-resume-check.log`. This is the guarded source, not a substituted
unguarded baseline build. Logs use the same
`qmake-{configure,all,build}.log` names within the guarded source snapshot;
the existing SDK warning remains and postprocessing is disabled, not package
acceptance.

The independently reviewed test-only repair measures the exact warmed operation
and exercises every allocation index 1 through N plus a checked no-hit N+1
terminal. A checked 1,000,000-allocation measurement ceiling, count drift, early
terminal or exhausted search reports incomplete failure rather than a skipped
pass. It preserves all 28 contexts, precision and existing assertions; case
begin/end and progress diagnostics make partial coverage explicit. The shared
calibrated proposal has SHA256
`70650db41b40e2c6621adbb23a551ecc1370ffd51345d36354ad8a92dcf8fe0d`;
the original frozen candidate remains unchanged. The private proposal at
`/private/tmp/librecad-native-fault-bound.dv31tu` retains patch SHA256
`4f538ae9a8941f8ad4cbdd820a7d0407eb7d81da0030a4779ef66c340e675a14`
and exact matching-header link inputs. Calibration-only controls pass all 28
N+1 terminals but explicitly do not execute injected indices. The full ON
diagnostic was explicitly superseded after the coherent current candidate was
built. Session 9908 terminates with exit 130, not a pass. Its eight completed
contexts, 97,972 injected indices and eight terminal controls remain partial
historical evidence; calibration-only controls remain separate. The sole current
serial fault process uses the combined candidate below. No duplicate injector
or allocator-interposed sanitizer runtime is authorized by these results.

### Combined state, native and block validation candidate

The distinct source-only snapshot
`/private/tmp/librecad-block-combined.DZBNvF/source` combines the repaired
owning-INSERT transaction with the exact three current compatibility-input
files and calibrated native fault test. No configured build directory, object,
archive or executable was copied. Its full 5,836-source manifest SHA256 is
`e38988a41e4b75ab0aafd2ea25f74514b65f5aa91d78d1f3208a13f8cb7076ec`;
the explicit 23-input manifest is
`b7df03bd43141ca4decaff4ab7780156be2037bcdee8640e262ace9cc1a8ec12`.
Compared with the frozen repaired owner source, only the three State files and
native failure-test translation unit differ.

The test-only comparator now also checks current source nullness, prestrip and
final compatibility buffers (value and nullness), source origin, initialized
state and text-edit status. Existing convention, immutable imported-history
and profile comparisons remain intact. Root and independent source review
confirm these checks use const-reference/nonallocating reads and do not change
fault actions or injection indices. The shared test now matches this candidate,
SHA256 `7354a691acbb1e6e68a9dcc91cbfdb31257f2202b7cf941b3319a0dbcba1d91c`.
Comparator-only patch SHA256 is
`f3ad9396e971b88a8e9ba368978576674a4fe8642951bb4310edbdf8ea430a2c`;
the four-path overlay is
`b95b86c29274c8c5d293fb49b2873c7443ecf3033b112ec3a1880118594d4d50`.
Both read-only apply checks pass. `COMBINED_INPUTS.md` records the input
identities and boundaries.

The fresh coherent ON application and all 26 registered test targets build with
exit 0. All 22 non-fault executables run unfiltered with exit 0: 3,396,412
assertions / 334 cases in total. Both full-source manifest checks pass. Exact
commands/statuses and individual footers are in `on-validation-status.log` and
its matching runtime logs. This is current combined-source evidence, not a
substitution of earlier private owner or State-only results.

The sole serial current ON fault lane records binding-copy, container-replace
and prepared-block failure executables exiting 0. The native exhaustive process
remains running in session 64685; its partial contexts are not a full pass.
`/private/tmp/librecad-text-history.SZMJHz/current-fault-status.log` retains
terminal status separately from progress diagnostics and original interrupted
history. Fresh coherent OFF application and all 26 targets now build with exit
0; all 22 non-fault suites pass unfiltered with 1,906,116 assertions / 306 cases,
and both manifest checks pass. Original session 33916 finishes with exit 0;
`root-off-status.log` and matching runtime logs preserve exact results. OFF
builds the allocator executables but deliberately does not run those overrides.
Whole-private-source O1 ASan/UBSan session 11512 also finishes 0: application
and all 22 non-fault suites, 3,396,412 / 334, both source checks 0. Root audits
1,057 built project TUs (1,044 source + 13 generated), each O1/debug with
address/undefined/frame-pointer flags, and all 23 instrumented links. There are
no shared source paths or sanitizer/runtime diagnostics. External libraries are
uninstrumented, leaks disabled and no allocator override is run. Fresh qmake
application session 44064 finishes configure/build/dependency recheck and both
source checks with 0. The root driver is
`run-root-lane.sh`, SHA256
`cf52ef04fbc7a23e662fcce6fe076f2b34f4cf41c7afd5960bec721861a4ae37`.
Each lane preserves commands/statuses and manifest-before/after checks; no root
lane runs an allocator override. ASan status SHA256 is
`631dd3ce109d6fb39bd69490cb283a0836030faf48b96abcd73f495e28ec959c`;
qmake status is
`c6ffac4d4065c6c61c8d9885c9f5fb30a7038dd51843ad946f9ca719c1d8d16b`.
The exhaustive native fault result and scoped source checkpoint remain Pending.
Application compilation/linkage is not startup/package acceptance. This does not
establish native explode or durable wire transport.

### Private durable block repair validation

The original prepared-block transaction is preserved privately in
`/private/tmp/librecad-block-transaction.LFs6xj`. Its earlier repaired export against
`883aec664`, `prepared-block-883aec664.patch`, has SHA256
`4baa42f461164e5b0f42d6164297c3f39466747c7cc451e397d89cc84e1c34c8`;
the 18-file `owned-sha256.txt` inventory has SHA256
`ebacf8b3b0e9b9748fcffcab0341b31b30891c87b7ffbc08f191cf1bbe1c3ce2`.
Independent bounded private source/test review approves explicit target-layer
membership and visibility admission, and existing-redo-slot recovery that
preserves undoability of an already-applied generic edit without allocating.
This is not a claim of rollback for arbitrary legacy geometry changes.

Before repair, the admission selector fails 3 of 12 assertions and the lifecycle
injector reports unbalanced state at allocation 3; logs are
`admission-before-fixed-fixture-run.log` and `lifecycle-before-run.log`.
Repaired private ON and coherent OFF focused runs each pass 234 assertions /
17 cases. Fault runs in both lanes cover 40 publication, 41 undo, 30 redo and
69 redo-prune C++ allocation failures, plus 35 single and 35 persistent legacy
cycle failures. Each accepted generic edit remains undoable; cycle depth,
listener silence, border policy and bounds are checked. Observed publication
and prune allocations before the postcommit observer are zero. The injector
does not cover Qt-private `malloc`.

Root reads the repaired patch/inventory identities and actual OFF logs;
independent review also inspects the before/after evidence. OFF build and both
executables exit 0; logs are `build-off-repaired.log`, `off-focused-run.log` and
`off-failure-run.log`. Fresh full-source Debug/O1 ASan+UBSan configuration, build
and both runtimes now exit 0. Root reads `asan-focused-baseline-run.log`
(234 / 17) and `asan-failure-baseline-run.log` (38 publication, 39 undo, 28 redo,
67 prune and 35 single / 35 persistent legacy failures; postcommit C++ allocation
counts zero). Leak detection is disabled; external Qt, HarfBuzz and FreeType
remain uninstrumented. These results cover only frozen private `4baa42f4`
inputs. Root integration, matching current-candidate builds and native explode
acceptance remain Pending.

A later admission audit finds that the `4baa42f4` factory still accepts embedded
NUL, controls and whitespace-only generated block names, while the existing
filter serializes the name through a NUL-terminated UTF-8 string. The intended
opaque ASCII consumer does not make that public factory domain safe. The final
private repair restricts only generated names to 1–31 ASCII characters matching
`[A-Za-z0-9_][A-Za-z0-9_-]*`. Ordinary user block naming is outside this
restriction. Earlier ON/OFF results do not cover this later correction.

The same admission boundary also accepts an ASCII casefolded INSERT reference
that ordinary case-sensitive `RS_BlockList::find` cannot resolve. Exact literal
references are required for generated output; ordinary lookup's NFC equivalence
is not permission to publish a non-ASCII alias in that generated token domain.
The private literal suite fails 17 of 49 assertions / 3 cases before repair:
15 forbidden-name admissions and two nonliteral-reference admissions (ASCII
case mismatch and a Kelvin-sign alias). Root reads the actual failing footer
and compares the validator with ordinary block lookup. Track the separate
lookup/preservation contract as `P5-BLOCK-REF-NAME-001`. The final private repair
requires the INSERT's literal name to equal its matched definition's portable
token, retaining conservative NFC/casefold collision checks and ordinary user
block naming. The Kelvin alias violates this literal generated-name contract;
it is not a second proof of ordinary lookup loss.

Final `prepared-block-883aec664.patch` SHA256 is
`3b8e8abad085ff7ae7d5628b699ac62a0e1027106f84c3d16f7522a30fc3ac36`;
the 18-entry inventory SHA256 is
`0c3f13a83e019be2649221101e1e786d546f6a839f59e3bc279e45fb40023a64`.
The 17-path production/test delta applies against the recorded `883aec664`
baseline; validation registration is separate. Final private ON/OFF/ASan
builds, focused runs and allocator runs all exit 0. Each focused suite passes
283 assertions / 20 cases; each failure suite covers 38 publication, 39 undo,
28 redo, 67 prune and 35 single / 35 persistent legacy-cycle failures, with zero
publication/prune postcommit C++ allocations before observers. Logs are
`generated-name-{on,off,asan}-{build,focused-run,failure-run}.log`.
Leak detection is disabled and external libraries remain uninstrumented.
Independent final delta/manifest/evidence review approves the bounded repair.
The 17-path core and bounded CMake/qmake registration are now integrated in the
shared working tree against `c8b5a193c` plus the frozen eight-file guarded native
candidate. The core bytes match the private source; the 19-path combined patch
SHA256 is `0894d1825d33f9b56ea8f4c97ff6553b0cec43a5e8cf588911d1c15ddedcdedc`.
This is uncommitted integration, not current-candidate acceptance. Coherent
current-header builds, runtime verification and actual native
explode/save/reopen remain Pending.

Independent current-candidate review finds an additional mandatory admission
gap, `P5-BLOCK-OWNER-001`. `validate` checks ownership of generated definitions
but not their INSERT snapshots. An otherwise matching snapshot can use the
public `setOwner(false)` API and still be admitted, contrary to the token's
exclusive-ownership contract. Ordinary INSERT update and container destruction
then omit deletion of those children; provenance clearing also skips them.
Require owning INSERT snapshots and a rejection/lifetime regression, preserving
graph/history/selection on refusal. Default builders produce owning snapshots;
this is an exposed factory-domain defect, not a demonstrated failure of the
pending native producer. The new frozen before fixture in
`/private/tmp/librecad-block-owner.cRZV73` reproduces the defect: exit 42,
53 assertions / 2 cases, with exactly two failed negative-admission checks;
the owning lifecycle control passes. An external RAII owner keeps the borrowed
child leak-safe even when the erroneous before token is admitted.

The narrow repair requires `insert && insert->isOwner()` at the existing
admission/revalidation boundary. Independent peer and root source/test reviews
approve it. Header-consistent incremental ON build and all after runtimes exit
0: owner selector 53 / 2, complete focused suite 336 / 22 and standalone fault
matrix 38 publication, 39 undo, 28 redo, 67 prune and 35 single / 35 persistent
legacy failures, with zero measured postcommit-before-observer C++ allocations.
History, graph/selection pointers and destructor counts are checked; bounds
comparisons use the existing approximate vector equality, not bitwise proof.
Only the helper CPP differs between the before/after full-source inventories.

Root integrates the two reviewed files byte-exactly. The narrow patch SHA256 is
`1276254a164bb81e6432500715c154aaca0393740e68a8417b943a08998edcea`;
helper SHA256 is
`cbb0ac159fefa089096688484dbcaa4a8f63245244070fff8a58191d8fb56516`,
focused test SHA256 is
`4197aaa64d7a7706ecc45a0198297cc4672bce5392bd4e1a88d54fc4906bb51d`.
The complete repaired 19-path block patch SHA256 is
`ccfe5b99f70f660eda2e02a14d0c36fa87843999002af182dcda4c182f1af34d`;
full 5,836-source inventory SHA256 is
`64e0e0cea16094605bf5b40235b65eceaa775af04aa48a13801fdbfc83ac8643`.
`OWNER_REPAIR_EVIDENCE.md` and `after-{build-on,snapshot-owner-run,focused-suite-run,failure-suite-run}.log`
retain actual commands, results and scope. The subsequent combined candidate
passes ON/OFF/ASan application builds and all22 non-fault suites, plus qmake,
as recorded in the combined validation section above. Its sole exhaustive
native allocation matrix and scoped checkpoint remain pending; full native
explode serialization and P5 acceptance are not established.

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
| P7 PR creation and deep refinement | P7a create/reuse and verify a draft PR; P7b independently review and deeply refine it through scoped corrective commits; P7c verify the final candidate and hand off ready for review | Verified draft-creation checkpoint and task attachment; independently verified refinement closure; P6 technical gates complete; current PR revision evidence and ready-for-review handoff with no unresolved mandatory findings; no automatic merge or release |

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
| P3c | Shared explicit paragraph/tab/stack layout and source ownership | Registered ON/OFF and sanitizer checks; shaping/source/metric/recursive budgets and failure atomicity; independent source review |
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

Execute the PR work as three explicit steps:

1. **P7a Create the draft PR.** After a coherent P5 checkpoint, inspect the
   complete committed candidate, confirm publication authority and destination,
   publish or reuse the matching draft, attach it to this task, and verify its
   published head and description. P6 platform checks may still be Pending;
   draft creation is not release acceptance.
2. **P7b Deeply refine the PR.** Independently review the whole diff and
   cross-layer behavior, triage every actionable finding, reproduce defects,
   make scoped corrective commits, rerun affected validation and obtain
   independent verification. Update the plan, evidence and PR after every round;
   repeat until the mandatory findings and review gaps are resolved. This work
   includes refining the title, description and reviewer walkthrough against
   the actual candidate, and may run alongside P6 validation.
3. **P7c Verify and hand off.** After P6 and the refinement exit criteria pass,
   perform the final whole-diff review, reconcile current feedback and CI against
   the exact published revisions, and verify the ready-for-review state before
   handing off. Merge, release and default activation remain separate decisions.

Publishing a draft does not complete P7.
On 2026-10-13 the user explicitly requested immediate PR creation while P5 was
unfinished. The resulting early draft #3099 is a publication-sequencing
exception, not a waiver of any implementation, validation or refinement gate.
Its title/body identify foundations rather than completed user-facing Arabic
support. P7a stays In progress; P7b/P7c acceptance is not established.
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
| Phase states | P7a In progress: early draft created, acceptance incomplete; P7b Planned; P7c Planned |
| PR URL and published state | [#3099 — Arabic contextual-shaping foundations and lossless text state](https://github.com/LibreCAD/LibreCAD/pull/3099); verified open/draft, 2026-10-13 15:54 UTC; attached to this task |
| Verified destination | Same-repository source LibreCAD/LibreCAD, configured origin remote, `codex/arabic-contextual-shaping`; target LibreCAD/LibreCAD:`master`; no fork or base-branch mutation |
| Frozen candidate | At creation: head `79d1c297a27518be0d6006869c0feadd4a40b933`, tree `3df28bcd32928c96d5e24c49142779541e232182`; base tip `8cd8b521b8026750b348b7b71f461acac27c25c6`; merge-base `628a1ba845931a37b8d745a8ad08d4731615812c`. Catch gitlink `5aa8d113215bd9a97ecc1a2f3fc9506947a2fa57` unchanged. This publication-record follow-up is documentation only; current PR head remains separately verifiable in GitHub metadata |
| Reproduction record | Native checkpoint recipes/results and focused helper commands linked in the PR; no fresh whole-head rehearsal or portable complete acceptance record. Local-only evidence is not public reproducibility proof |
| Last reviewed candidate | Root and independent publication audit of frozen79d1:40 commits/100 paths; all published filenames match local three-dot scope. Required committed CMake/qmake paths and pinned licensed font exist; no detected credentials/generated private implementation files. This is not a completed whole-diff semantic/refinement review |
| Review dispositions and presentation | Title/body distinguish compiled OpenType availability from font/profile activation, list unpublished private WIP and preserve failed sanitizer history. Complete reviewer walkthrough and independent per-area refinement remain pending |
| Feedback reconciliation | Creation snapshot verified; no external reviewer requested and no review disposition claimed. Retrieve revision-bound feedback/checks during P7b/P7c; queued checks are not passes |
| Remote operation reconciliation | Exact owned-branch dry-run/push0; remote79d1 verified. Connector creation denied403 before creation; existing authenticated browser then created exactly one draft. GitHub readback and task attachment succeed; no authentication/permission changes, force push, tags or duplicate PR |
| Integration status | Commit `ba1719dcc` merges master `7a8d24ddf`, retaining #3061's refined layout/width contracts and tests, canonical source APIs, empty-line guards, writer ownership and upstream staged-DWG cleanup. All six conflicts are resolved. Fresh coherent native library and merged regressions pass after correcting the unused-width guard; remote mergeability readback remains pending |
| Validation and findings | Fresh local CMake ON18/18 and OFF16/16 bounded executables pass; native bidi535,371/84, source1,065/14, writer290/4, outline12,326/8, pen39/4, replacement55/4 and isolated replacement allocator pass. qmake6 ON/OFF configure/font/shaper compiles and ON MTEXT/filter compiles pass. Workflow lint passes. New sixteen-suite headless CI and corrected format build lists are present; hosted results pending. System-engine skips are explicit, not certified resource acceptance. Mandatory integration findings and P6 release dependencies/packages remain open |
| Evidence invalidation | Initial candidate and target recorded above; compare code/resources/dependencies and exact head/base after every corrective update. A plan-only publication record does not validate the application or close an existing finding |
| Next action | Verify the corrective push/checks; continue P5 scoped implementation, then supply certified platform dependencies/packages and independently refine the complete committed candidate. Do not publish unrelated private WIP; keep this early PR draft until all readiness gates pass |

#### P7a Create and verify the draft PR

Before publication, confirm that the implementation request authorizes pushing
the scoped branch and creating/updating a PR. Reuse explicit authority already
given; ask only if publication authority or its scope is absent or unclear.
A plan-only request does not authorize a push, PR creation or PR mutation;
publication must remain within the implementation request's scope.

Resolve the source repository, push remote and owned source branch separately
from the target repository and base branch, including any fork relationship.
Verify that the published PR uses that exact source/target pair; matching branch
names alone do not identify a PR. Use read-only checks and do not infer a
destination from an unrelated remote or print credentials. Follow repository
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

Review the proposed PR diff from the recorded merge-base to the frozen head,
and inspect target-base advances separately as integration changes. A direct
base-tip-to-head comparison must not stand in for the PR diff. Include added,
deleted and renamed files, fixtures, build inputs and submodule changes, not
only a diff summary. Reconcile the published PR file list with that scope;
unexpected files or changed revisions require a new candidate assessment.

Rehearse the local acceptance checks using only the frozen committed candidate,
its recorded submodule revisions and declared dependencies/resources. Use an
isolated checkout and out-of-source builds, without copying untracked headers,
generated files, developer fonts or old application objects from the working
tree. Previously completed isolated validation may satisfy this rehearsal when
its input identities match and that comparison is recorded; do not require a
redundant run solely to rename the evidence. A missing committed input or hidden
local dependency is a finding to fix before publication, not a reviewer setup
instruction. Record platform-specific prerequisites explicitly.

#### Reproducible PR evidence

Create a compact durable record, in this plan or a linked committed review
document, that another reviewer can use from the candidate checkout:

| Record | Required contents |
| --- | --- |
| Candidate and scope | Source repository/owned branch and target repository/base branch, head/tree, target base tip, merge-base, prerequisite commits and submodule revisions; intended changed-file scope |
| Build and runtime inputs | Toolchain, platform/architecture, Qt and dependency versions, feature flags, font/fixture hashes and settings-isolation policy |
| Reproduction | Configure, explicit build and test commands, filters, required working directory, expected outputs and artifact-generation steps; fail on any unsuccessful command rather than reporting only the final command's status |
| Results | Per-command exit status, executed assertion/case counts or equivalent totals, actual tested head/merge identity, sanitizer boundaries, failures/exclusions and durable logs/reports |
| Review and disposition | Acceptance-map rows, independent coverage, stable finding IDs, corrective commits, verification evidence and remaining gates |

Move or summarize the necessary local-only evidence into reviewable repository
fixtures or durable, sanitized artifacts before relying on it in the PR. Retain
the original validation history and hashes; do not relabel local diagnostics as
coherent acceptance. A workstation path or expired CI download alone is not a
reproducible result. Do not publish private drawings, personal settings or
credentials. Large artifacts may remain outside Git when their stable location,
checksum and producer revision are recorded and access is verified. Keep small
fixtures, commands and acceptance summaries reviewable with the source.

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
publishing tags. Reuse an open PR only when its source repository/branch, target
repository/base and scope match this work; preserve its unrelated metadata.
Both new and reused PRs must be draft at the P7a checkpoint; P7c establishes
readiness after the required evidence and refinement are verified. Inspect a
reused PR's current draft/ready state as well as its head. If it is ready,
return it to draft within the authorized workflow and verify that transition
before completing P7a or publishing corrective changes.
If that transition needs maintainer authority, record the action and keep the
gate incomplete. Do not repurpose a closed, merged or unrelated PR.
If push/creation returns an uncertain outcome, inspect the remote branch and
matching source/target PR before retrying.
After creating a PR, attach its URL to the current task using the app's
artifact attachment tool; attach an existing PR when continuing its refinement.
Verify push, PR creation and task attachment separately. If creation succeeds
but attachment fails, retain the verified PR URL and retry only attachment;
do not create a duplicate PR. A rejected or non-fast-forward push requires
reconciling the remote history and branch ownership, not a force push. Keep
P7a incomplete until its publication and attachment checks actually pass.

Create a new PR as draft, or verify the matched reused PR is draft. Record its
URL/number, source and target repositories/branches, base-tip and merge-base
revisions, head SHA and current checks. Verify the published head matches the
intended committed candidate. Use the repository's
PR template when present and include the following information in its matching
sections. Otherwise use this outline; do not copy the entire implementation
plan into the PR body.

#### PR description outline

- **Summary and limits:** the user-visible opt-in Arabic improvement, language/font
  profile and its certification status, non-goals and known unsupported cases.
  Do not claim complete Arabic-script support or default activation.
- **Architecture and compatibility:** the backend decision, dependency/font/license
  changes, logical-source and native-geometry model, legacy behavior and saved
  profile/metadata policy. Explain exact-font/engine failure without suggesting
  automatic migration or metadata loss.
- **Scope and checkpoints:** the intended base, prerequisite dependencies,
  intermediate commit map and links to the committed plan and acceptance map.
- **Reviewer walkthrough:** enable the exact profile, create TEXT and MTEXT with
  the same logical Arabic/mark/mixed-direction sample, inspect metrics and native
  geometry, edit/save/reopen, then exercise both explode paths. Link reproducible
  fixtures, automated checks and relevant before/after CAD output, with expected
  results. Screenshots alone do not prove source or persistence invariants.
- **Validation:** a matrix distinguishing passed, failed, pending, skipped and
  unavailable checks, tied to tested revisions, configurations and durable run
  or artifact links. Never present an unexecuted check as passing.
- **Review and remaining actions:** independent coverage, finding/round records,
  verified corrective commits, accepted optional deferrals and outstanding
  P6/P7 gates or maintainer actions. Keep this summary current after each round.

Use repository links and durable review artifacts, not workstation-only paths
or logs containing credentials/private drawing data. Preserve applicable
template fields and maintainer-authored text when updating the description.

Choose a concise title describing the actual opt-in contextual-shaping change,
not an unsupported claim of complete Arabic support. During P7b, review the
title, description and walkthrough as deliverables: verify that commands and
sample Unicode reproduce the stated behavior, links resolve to the recorded
candidate, limitations remain prominent, and each acceptance claim has matching
evidence. Keep implementation detail in the linked plan rather than expanding
the PR body into a second, conflicting specification.

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
- [ ] The clean-candidate rehearsal and durable reproduction record identify all required inputs and actual results; no acceptance claim depends on untracked files, mismatched application objects or workstation-only evidence.
- [ ] Prepublication findings and validation history are carried into the review records; no known mandatory P0–P5 local-contract finding remains unresolved or fixed without verification.
- [ ] The draft PR exists, is open, targets the intended base and publishes the intended head; its URL is attached to this task. Uncertain push/creation outcomes are reconciled before retrying.
- [ ] The title accurately describes the delivered scope; the description links the committed plan, fixtures, reviewer walkthrough and revision-bound validation matrix, including known failures and unsupported cases.
- [ ] CI triggers, test discovery, package lanes and draft/fork restrictions are audited; each missing or pending gate has a concrete next action.
- [ ] The PR tracking record and progress ledger identify the published candidate and the next P6/P7b work. P7b/P7c and the overall P7 milestone remain incomplete.

PR creation is a delivery step, not evidence that Arabic rendering is accepted.
Do not mark these boxes from intended future actions.

#### P7b Deeply review and refine the PR

Review the entire accumulated PR diff, not just the latest corrective commit.
Give each independent reviewer the exact head, target base and merge-base,
acceptance contracts, known limits and evidence matrix. A review of another
revision is historical, not approval of the candidate. Use focused independent
review passes, parallel where useful, with these areas:

| Review area | Required scrutiny |
| --- | --- |
| Shaping and bidi | Logical versus visual order; pre-L1 shaping versus final L1 placement; X9, isolate and ZWJ/ZWNJ context across style boundaries; surrogate/grapheme/cluster mappings and Unicode/version boundaries; required substitutions and common marks; no double reversal/mirroring |
| Geometry and layout | Advances versus ink bounds; glyph origins, mark/cursive offsets and no-ink cases; height/width/oblique plus entity transforms; alignment/anchors, tabs, stacks and line spacing; contour tolerance and native coordinate limits; repeated-update stability |
| Entity lifecycle and scene integrity | Atomic publication of source/anchors, geometry, metrics and status; allocation/resource/numeric failures; ownership and borrowed children, selection, postcommit callbacks and reentrancy; current-layout freshness; undo/redo and durable output from both explode commands |
| Resources and robustness | Exact saved font/face/engine policy; missing/corrupt resources and complete-segment fallback; bounded cache keys/lifetime; thread ownership; malformed Unicode/format/font input and complexity limits |
| Compatibility and persistence | Unchanged legacy modes and shared-style isolation; profile upgrades and unknown metadata; complete retained-field registry and raw-versus-rebound handle authority; no-op/edited DXF/DWG and embedded ATTRIB precedence; consistent export admission/emission/handle planning; annotation/TABLE inheritance and MLeader cache; both explode paths and print/export parity |
| Editing and user experience | Both native text dialogs and property panels; logical Unicode retained through IME preedit/commit, cursor/selection, clipboard, invisible-control edits and undo/redo; engine-rendered preview versus Qt editing font; explicit profile selection/conversion, undo and actionable resource errors; Chinese editing regressions |
| Builds and distribution | CMake/qmake parity and explicit legacy-only builds; matching public headers across objects and archives; supported Qt/platform/architecture matrix; clean installed lookup without developer fonts/prefixes; dependency linkage, pinned font, notices/licenses and actual package contents |
| Tests and PR quality | Independent placement/format oracles, not self-round trips alone; negative and boundary cases; actual nonzero test execution; source invariants; unexplained exclusions; prerequisite scope; accurate title, description, reproducible walkthrough and evidence links |

For each assigned area, identify the invariant being checked, trace its producers
and consumers, inspect both successful and failure paths, and reproduce selected
checks from the frozen candidate. Challenge test assumptions and expected
outputs as well as production code. Record unexamined contracts as coverage
gaps with a next action; a checklist tick or an author's explanation alone does
not close them. Findings must identify an affected contract and reproducible
failure or concrete source evidence, not speculative changes outside this scope.

Challenge public admission boundaries with inputs that ordinary builders do not
produce. For ownership-sensitive factories, vary public ownership flags, parent
relationships and borrowed descendants; prove either safe ownership throughout
update/undo/pruning/destruction or transactional rejection. Document which
states are explicitly outside the contract rather than assuming constructor
defaults establish every caller's preconditions. Include provenance and
selection traversal when ownership changes what is visited.

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
- Change entity, ancestor and layer pens after native geometry is built.
  ByLayer/ByBlock paint must resolve through nested groups dynamically, while
  explicit inline ACI/RGB remains explicit. Test current entity/STYLE profile
  authority before the first update, under NoUpdate and after profile removal
  or opaque imports; cached render status must not authorize an edit, transform
  or accidental imported-profile activation.
- Measure contour error after the complete glyph/entity affine, including
  supported anisotropy, reflection and large origins. Check tiny contours,
  no-ink extents, exact coordinate boundaries and deterministic precision,
  range or budget errors without clamping or partial geometry.
- Trace the same revision-bound TEXT/MTEXT cases through shared layout, native
  geometry and representative PDF/explode output. Record the exact artifacts
  and cases approved by a competent Arabic reader able to assess joining and
  common-mark placement; the standalone CoreText probe cannot substitute for
  application acceptance.
- Exercise repeated updates, cloning, undo/redo, cache eviction, resource
  failure and destruction under sanitizers. Record instrumented sources and
  exclusions; a font/shaper-only run is not native-adapter lifecycle coverage.
- Inject failures before native update, transform and explode publication;
  verify each operation's accepted source-edit contract, previous geometry,
  metrics, selection and document ownership without partial mutation. A valid
  no-ink result is not an unavailable or stale layout. Verify both explode
  commands preserve appearance and effective paint through undo/redo and
  DXF/DWG save/reopen; refusal must retain the original entity, and an in-memory
  group alone is not proof of durable output.
- Reuse the same filter instance for DXF then DWG and DWG then DXF, including
  repeated saves and failed export followed by retry. Check writer lifetime,
  active format/version selection, STYLE handles and unchanged source/metadata;
  fresh-filter round trips alone cannot establish this lifecycle contract.
- Exercise absent, malformed, unknown and oversized embedded metadata separately.
  Challenge duplicate/foreign carrier ownership and stale native-wire baselines
  after external edits. Verify current entity/STYLE authority versus retained
  history and outer/embedded namespaces through an independent reader.
  An unsupported output must preserve the original in-memory data and any
  existing destination file, never silently omit an attribute or truncate the
  destination before a predictable refusal. A safe
  temporary refusal does not satisfy required lossless supported-output gates.
- Audit each carrier registry field against the actual retained native and
  current-source model, including protected authored state. Keep an explicit
  field-to-ID/type manifest and justify exclusions individually. Preserve full
  raw handle components (`code`, `size`, `ref`, `ref64`), owner/link presence and
  discriminants separately from resolved graph references; later handle rebinding
  must not erase historical values. Challenge every retained field with
  nondefault literal fixtures and exact decode assertions, as well as unknown,
  malformed and bounded-input cases. Identify parser-discarded data explicitly:
  a pure codec pass does not prove original-wire capture, graph validity or
  profile activation. Resolve `P5-MODEL-FIELDS-001` before claiming registry
  completeness or accepting the full-model transport.
- Verify that pre-open admission, ATTRIB reconstruction/emission and DWG child
  handle planning use the same live-host rules: exclude `FlagDeleted` hosts but
  retain invisible attributes. Exercise a live INSERT with one and then all
  attribute hosts deleted, restoration through undo/redo, and malformed
  surviving hosts. Check remaining attribute counts and sequence-end links for
  supported DXF/DWG versions through an independent reader. Predictable refusal
  must leave the prior destination and source unchanged; same-filter retry must
  succeed after correction. Resolve `P5-DELETED-ATTRIB-001`; a preflight-only fix
  or a passing standalone MTEXT case does not satisfy this export contract.
- Commit Arabic IME composition, edit a combining sequence and an invisible
  control, then undo/redo and save/reopen from each editor entry point. Verify
  logical source and profile metadata independently of the preview, and confirm
  an explicit conversion leaves unselected references to a shared style unchanged.

Review public headers, build/package inputs, new and modified tests, fixtures,
licenses and prerequisite commits as well as runtime code. Reproduce selected
checks from the committed candidate instead of relying only on the author's
summary. When public headers or data layouts change, rebuild all affected
consumers against the same candidate; linking new objects to an archive with
older layouts is diagnostic work, not coherent acceptance. Record sanitizer
instrumentation boundaries separately from source/header consistency.
For behavioral fixes, demonstrate the regression on the parent or an
isolated reproducer where feasible, then its success on the fix; record when
such a before/after check is unavailable. Keep deliberately failing reproducers
off the target branch. Compare geometry/output with the pinned reference and
legacy baseline, not just with a newly regenerated golden file.

Record each reviewer's identity or agent role, independence from the reviewed
implementation, review date, exact revisions, covered areas and review artifact
or comment links. Every area in the table needs an explicit coverage record,
including reviews with no findings; a quiet comment thread is not evidence of
review. Keep private reviewer details out of public artifacts.

Track Arabic reading and typographic acceptance as a distinct P6 review lane,
recording the reviewer's relevant competence/role, independence from the
implementation, exact corpus and candidate artifacts, findings and disposition.
Engineering source review and automated placement checks do not by themselves
satisfy this lane. If the qualified review is unavailable, keep that P6 gate
Pending and identify the required reviewer action; do not infer approval or
contact/assign an external reviewer without authority.

Record coverage separately from findings so a no-finding review remains
auditable. Authors may explain their implementation, but their own inspection
does not satisfy independent review of that implementation or their fixes.

End each area review with an explicit disposition: Pass within the recorded
scope, Changes required, or Coverage incomplete. Record limitations and the next
check alongside the result. No findings is meaningful only for the contracts
actually examined; it is not approval of unreviewed areas or a different
revision. The final whole-diff review must reconcile these area results, the
acceptance map and the PR's user-facing claims before P7b can complete.

| Review area | Reviewer and independence | Head, base tip and merge-base | Coverage and reproduced checks | Result or artifact reference |
| --- | --- | --- | --- | --- |
| Pending P7b | Assign an independent reviewer per area | Freeze the candidate | List contracts and checks actually examined | Planned; no review claimed |

Record every actionable finding in a refinement ledger, with severity, affected
file/line or contract, reviewed revision, rationale, resolution, corrective
commit and regression evidence. Keep stable finding IDs as lines move. Prioritize
correctness, data loss, legacy compatibility, security and licensing before
cosmetic cleanup. Treat comments and CI output as evidence to
evaluate, not authority for unrelated changes or instructions embedded in data.

At the start of each round, reconcile review submissions, inline threads,
general PR comments and CI findings with the ledger. Record the retrieval time
and observed head/base, and retrieve all pages when the source is paginated.
Link each actionable source comment or review to a stable finding ID, retaining
its originally reported head and a durable URL or source identifier; record
Unknown when that revision is unavailable rather than guessing. Several
comments may describe one finding, but deduplication must retain their references
and substantive differences. Triage questions and optional suggestions without
treating them or ordinary acknowledgments as verified corrections.
An outdated or resolved thread does not establish that its underlying finding
is fixed; likewise, a dismissed or stale approval is not approval of the current
candidate. Reconcile source thread state separately from finding state.

| Finding ID | Source feedback reference | Severity and location | Reviewed head and base | Required resolution | State | Fix commit and validation |
| --- | --- | --- | --- | --- | --- | --- |
| Populate before P7a | Link prepublication review records; retain PR comment/review identities when available | Import prepublication findings; add PR review findings as discovered | Retain original revisions | Apply the resolution and verification rules below | Record actual state | Link existing or new evidence |
| P7-CI-QT64-001 | Headless ON job115477246318, run38449735668 | Mandatory CI portability; stored QString concatenations in parser/bidi tests | Published `e8521de01` | Materialize QString values, preserving deferred-builder compilation and every assertion; audit adjacent stored concatenations for temporary lifetimes | Locally verified; hosted retry pending | Original failure reproduced with GCC13.3/Qt6.4.2/Catch3.8.1. Corrected Linux ON16/16 and OFF16/16 pass; Mac ON18/18 and OFF16/16 pass. Four Linux registry cases explicitly skip for the non-certified system engine |
| P7-CI-GCC-001 | Linux format job115477245064, run38449735224 | Mandatory strict compilation; public block-record layout handle lookup | Published `e8521de01` | Cast the existing NoHandle sentinel to uint32_t without changing lookup semantics or disabling warnings | Locally verified; hosted retry pending | Original GCC enum/integer error reproduced. Corrected header and both failing format test translation units compile with strict flags. Mac format runtime scope710/6 and verified279/3 pass; Linux full-format runtime not rerun |
| P7-MASTER-INTEGRATION-001 | 2026-10-13 GitHub metadata and six local merge conflicts | Mandatory compatibility; MTEXT layout, tests and filter writer ownership | Published `c40da2c8c`, master `7a8d24ddf` | Retain current master's refined indentation/paragraph/width contracts while preserving source state, empty-line guards and both export cleanup mechanisms | Locally verified; hosted pending | Merge `ba1719dcc`; fresh native library/bidi/source/writer checks and qmake MTEXT compile pass after the width correction below; no whole-feature acceptance |
| P5-WIDTH-RECONCILE-001 | Fresh merged native bidi suite at `ba1719dcc` | Mandatory compatibility; standalone MTEXT output width | Merge `ba1719dcc`, master `7a8d24ddf` | Validate the selected serialized width, not an invalid reference value deliberately replaced by master's measured-width fallback; keep strict ATTRIB width checks where raw width is actually emitted | Locally verified; hosted pending | Before: 1/84 native cases fails on infinity. After: complete bidi535,371/84 passes, with same-filter DXF/DWG fallback export/reopen for all existing samples; source1,065/14 and writer290/4 pass |
| P7-CI-DISCOVERY-001 | Published workflow/test-registration audit | Mandatory CI correctness; unfiltered CTest invoked after building an incomplete executable list | Published `c40da2c8c`, master `8cd8b521b` | Build both registered scope/verified binaries and keep the format lane independent of optional font dependencies | Locally verified; hosted pending | Fresh ON format scope710/6 and verified279/3 pass; Linux/Windows build lists include both binaries and explicitly select OpenType OFF. Existing master SVG target retained |
| P7-HEADLESS-CI-001 | Published CMake/CI audit | P2 regression coverage; sixteen excluded, unregistered foundation executables | Published `c40da2c8c` | Add one opt-in aggregate/label and ON/OFF workflow without application rebuilds; preserve separate allocator-failure runner and actual engine/skip evidence | Locally verified; hosted pending | Fresh ON16/16, OFF16/16, actionlint pass. Ubuntu uses system engines; certified-resource skips remain visible in verbose artifacts and are not acceptance |
| P6-RELEASE-DEPS-001 | 2026-10-13 release-workflow audit | Mandatory release/build completeness; default-ON backend versus missing public dependencies | Published `c40da2c8c`, master `8cd8b521b` | Provision/version/audit HarfBuzz and FreeType on all supported release architectures, universal macOS libraries, runtime resources and third-party notices; validate ON/OFF packages | Open | Existing release Linux/CodeQL lists omit HarfBuzz development packages; Windows has no public backend dependencies; macOS builds only universal FreeType. Private dependency/package proposals are excluded from this scoped update; keep draft |
| P5-SIDECAR-001 | Independent embedded-sidecar baseline audit; retention finding above | Mandatory data preservation; `appendInsertAttribSidecar`, `decodeInsertAttribMTextXData` and ATTRIB extraction/export | HEAD `a8ade2677`; no PR/base review claimed | Lossless compatible retention, explicit failure instead of dropped ATTRIB, regressions and independent verification | Open | No verified fix or transport acceptance yet |
| P5-FILTER-LIFETIME-001 | Mixed-tree diagnostic and coherent baseline/repair; cross-format lifetime finding above | Mandatory crash/lifetime failure; `RS_FilterDXFRW::fileExport`, `prepareDRWDimStyleText` and writer/version selection | Baseline `9e50184ff`; verified repair `37d2c614d` and exact source/test identities above; no PR/base review claimed | Writer ownership on all exits, same-instance cross-format/version/retry and exception regressions with independent normal/sanitizer verification | Verified | `37d2c614d`: registered ON/OFF/ASan 290 / 4 and applications pass; independent review approves and committed identities match; final whole-PR review still required |
| P5-EMBEDDED-DXF-001 | Independent Autodesk/ezdxf wire audit; marker and namespace finding above | Mandatory format/metadata authority; `writeEmbeddedMText`, `DRW_Attrib::parseCode` and ATTRIB/ATTDEF output | Merged canonical101/verified native closure juU83Q and bounded normal/sanitizer wire replays e2Dv8i/p9dA3K; no PR/base review claimed | Canonical group-101/body/outer-XDATA ordering, external fixture/reader verification, conservative legacy reading and lossless embedded carrier | Open | Independently audited native ASCII DXF2018 boundary passes710 / 6 in both lanes with identical output and ezdxf semantic checks; paired-output audit errors0/fixes0. No full-model carrier or whole-filter save/reopen acceptance; broader contract remains open |
| P5-LETTERS-UNSAFE-001 | Root-directed independent pinned HB trace and native group-provenance audit | Mandatory frozen ligature/attachment-unit failure; NativeBuilder source grouping and Explode into letters | Guarded uncommitted native source plus preserved Csh2Qp/G6nym3 narrow diagnostics; no PR/base review claimed | Preserve/coarsen actual unsafe logical-neighbor boundaries with existing grapheme/base–mark unions; retain child paint/geometry/source and independent paragraph/object boundaries | Open | Amiri rlig lookup140 yields two GIDs/two clusters but unsafe-before logical cluster1. NativeBuilder drops retained shaper/layout flag; current two-group test passes only existing appearance freeze. Earlier dismissal of one-group oracle was incorrect; conservative grouping repair and fresh validation required |
| P5-LETTERS-PAINT-ORDER-001 | Root-directed independent EGC consumer and RTL emission audit | Mandatory appearance-order failure; `LC_NativeTextLetterGroups::collect` component ordering | Frozen unsafe proposal9kzWZH plus existing EGC helper; source finding only, no PR/base review claimed | Preserve original first-emission component/leaf paint order; reject noncontiguous EGC unions before publication, with coincident-color literal and actual RTL regressions | Open | Existing sourceStart sort reverses safe RTL emission order without proving disjoint ink. A valid literal A/B/C helper tree with overlapping EGC ownership in A/C produces A/C/B. Frozen9kzWZH is unchanged; separate helper/test repair and fresh coherent validation required |
| P5-STYLE-RESURRECT-001 | Independent complete current STYLE proposal review and root emitter readback | Mandatory current-authority failure; `RS_FilterDXFRW::writeTextstyles` legacy font fallback and named consumers | Frozen source-only7PRrg9 ten-path proposal; no PR/base review claimed | Prevent old modern-host STYLE aliases from recreating renamed/removed rows; keep legacy text, dimensions and header `$TEXTSTYLE` consistent or refuse unsupported remaps before opening; preserve rename/mixed-consumer/nonfirst-header regressions | Open | Planned emission and metadata suppression omit Actual_STYLE after rename, but unchanged host.getStyle enters the legacy fallback and emits a guessed old-name/font row. Header can independently retain that missing name. Separate source successor and fresh validation required; no runtime has executed the proposal |
| P5-UNCHANGED-WIRE-001 | Native author audit and root actual canonical reader/writer readback | Mandatory preservation failure; unchanged outer-profile exception in `prepareEmbeddedAttrib` and `writeEmbeddedMText` | Frozen source-only LNyJ6L; no PR/base review claimed | Prove the admitted original native state is emit-preservable; refuse omitted/nonroundtrippable common, presence and other retained fields before publication; add literal negatives and preserve whole-native target comparison | Open | Body group60 can survive import but is not emitted; zero X-axis vector presence can also be lost. Private exact common equality does not fix writer capability. Full field audit and distinct repaired successor are required; no builds/runtimes or accepted exception |
| P4-OWNED-REPLACE-001 | Independent shared geometry-prerequisite source review | Mandatory selection preservation; `RS_EntityContainer::replaceOwnedEntities` | Originally reviewed source `16f62f6557febc04825d6cf5ddaa05bbae65a767fe7c9b25f9686421da93bfcd`; corrected source `119cf49991fd4013eac6ada6fb947d632298e9349e0a83a4dd303f2eeb7935f2` in `883aec664`; no PR review claimed | Traverse only owned retired descendants; borrowed surviving children retain selection; add regression and independent verification | Verified | Independent source/test/registration re-review and coherent registered ON/OFF/ASan pass; baseline exits 42 with 2 / 3 assertions failing. Full CMake ON/OFF applications, coherent sanitizer library and fresh qmake application pass; final whole-PR review remains required |
| P4-DECORATION-001 | Independent native adapter precision review | Mandatory geometry/tolerance failure; `LC_TextNative::Builder::stroke` | Frozen uncommitted adapter source `ba7a8e09b25a5349d945e544e8a7acc75e4ccb4c243b1b177b454efd5068aeb9`; no PR review claimed | Replace fuzzy endpoint collapse with exact converted equality; prove representable tiny underline and stack-rule behavior without relaxing numeric checks | Fixed pending verification | Independent corrective source/test re-review approves. Private ON and owned-source O1 ASan/UBSan diagnostics pass 130,712 / 21; OFF diagnostic passes 137 / 21. Coherent registered acceptance and scoped checkpoint remain pending; partial instrumentation is not full sanitizer acceptance |
| P4-AFFINE-001 | Independent inherited-transform audit and private matching-header reproducer | Mandatory canonical state/geometry failure; TEXT/MTEXT shear and absolute-inline-height scaling | Frozen guarded native source `168fa36cfdbb4e66a9c4674b01fa6919ee33aa0c48ff5cee3144ba8632035fa2` and original guarded eight-file manifest above; no PR review claimed | Durable canonical deformation, guarded single-operation virtual transforms, truthful metrics and source/clone/undo/transport reconstruction; preserve final-affine tolerance | In progress | Private baseline exits 42: 6,201 assertions / 2 cases, 7 failures. Canonical affine implementation is integrated and independent source/test review finds no proven defect. Unguarded root ON application/23 executables pass, native 135,009 / 28. Guarded ON/OFF/ASan applications and 23 preceding executables finish; qmake completion check exits 0. Original ON fault run exhausts its test ceiling; ASan fault coverage is incomplete. Reviewed calibrated test is integrated and private ON exhaustive diagnostics are running; coherent current-candidate fault acceptance and durable transport remain pending |
| P4-NATIVE-ALLOC-001 | Native author fault-test design and independent root exception-path review | Mandatory status/freshness failure; TEXT/MTEXT `updateNative` after accepted source edits | Before injector `554436f82664b76170ef9763920693fc05b411451e667dfee7668cecb6c3c0f0` against matching unguarded root; guard identities in original eight-file manifest; old-header calibrated proposal `70650db41b40e2c6621adbb23a551ecc1370ffd51345d36354ad8a92dcf8fe0d`; current combined test `7354a691acbb1e6e68a9dcc91cbfdb31257f2202b7cf941b3319a0dbcba1d91c`; no PR review claimed | Prove precommit allocation failure retains the accepted edit and last-good geometry/metrics/layout/selection but reports failed rebuild; distinguish unchanged-Data transform refusal from postcommit observer exceptions | Fixed pending verification | Matching root-archive before-run exits 45 on unchanged TEXT update allocation 1 with stale Native/no-error; this stops before later cases. Root verifies proof/input/link identities. Guard and isolated standalone target are integrated; independent final source/test/registration review approves after adding transform-primary-status preservation. Original OFF fault invocation passes; ON exits 53 at the too-small test ceiling and original ASan coverage is incomplete. Reviewed calibrated test is integrated; matching-header private ON matrix is running. No complete corrected matrix or coherent current-candidate acceptance claimed |
| P5-BLOCK-UNDO-001 | Independent private durable-block transaction review | Mandatory undo/lifecycle failure; `finishLegacyCycle` and `RS_Document::endUndoCycle` | Original patch `74194d6c4b55e583875bff8085c900e9755c9b3c38fa3aeb1aa91305ddf399c3`; core `3b8e8abad085ff7ae7d5628b699ac62a0e1027106f84c3d16f7522a30fc3ac36`, now integrated as recorded above; no PR review claimed | Recover history for an already-applied generic edit and balance cycle depth, listener silence, border policy and bounds under failed redo-tail preparation; retain legacy rollback limits | Fixed pending verification | Before lifecycle injection3 leaves state unbalanced. Independent review and private ON/OFF/ASan focused283 / 20 plus fault matrix pass. Combined current candidate ON/OFF/ASan applications/22 non-fault suites and qmake pass; sole exhaustive native fault matrix and scoped checkpoint remain pending |
| P5-BLOCK-LAYER-001 | Independent private durable-block admission review | Mandatory pointer-domain failure; detached INSERT layer validation before bounds preparation | Same original/repaired core identities and current integration as `P5-BLOCK-UNDO-001`; no PR review claimed | Check each non-null raw layer against the target graphic's layer-list membership before dereference, including null-parent roots; reject outsider/stale pointers without mutation | Fixed pending verification | Independent review and before/after outsider/dangling regressions approve membership-before-dereference; private ON/OFF/ASan283 / 20 and fault matrix pass. Combined ON/OFF/ASan applications/22 non-fault suites and qmake pass; sole exhaustive native fault matrix and scoped checkpoint remain pending |
| P5-BLOCK-VISIBLE-001 | Independent private durable-block appearance review | Mandatory appearance mismatch; definition lines versus INSERT snapshot flags | Same original/repaired core identities and current integration as `P5-BLOCK-UNDO-001`; no PR review claimed | Require the accepted visible final-outline policy for definition and snapshot lines; prove ordinary INSERT update and reconstruction preserve appearance | Fixed pending verification | Independent review and before/after admission regressions approve explicit visibility; private ON/OFF/ASan283 / 20 and fault matrix pass. Combined ON/OFF/ASan applications/22 non-fault suites and qmake pass; sole exhaustive native fault matrix and scoped checkpoint remain pending; durable output still required |
| P5-BLOCK-NAME-001 | Private author admission audit and independent root validator/export-path review | Mandatory portability/preservation failure; generated ordinary block names | Before private patch `4baa42f461164e5b0f42d6164297c3f39466747c7cc451e397d89cc84e1c34c8`; core `3b8e8abad085ff7ae7d5628b699ac62a0e1027106f84c3d16f7522a30fc3ac36`, now integrated as recorded above; no PR review claimed | Reject NUL/control/whitespace/nonportable names in the generated factory domain before publication, without changing ordinary user block naming; prove exact admitted names survive serialization | Fixed pending verification | Before literal suite fails17 / 49 across three cases. Generated domain admits only1–31 ASCII portable tokens; ordinary user naming unchanged. Private ON/OFF/ASan283 / 20/fault and combined applications/22 non-fault suites/qmake pass. Sole native fault, scoped checkpoint and serialized-name/native-explode acceptance remain pending |
| P5-BLOCK-REF-NAME-001 | Private author reference audit and independent root admission/ordinary-lookup review | Mandatory reconstruction/preservation failure; generated INSERT reference names | Before private patch `4baa42f461164e5b0f42d6164297c3f39466747c7cc451e397d89cc84e1c34c8`; core `3b8e8abad085ff7ae7d5628b699ac62a0e1027106f84c3d16f7522a30fc3ac36`, now integrated as recorded above; no PR review claimed | Require generated INSERT data.name to equal the matched definition's literal portable token, while retaining conservative collision checks and ordinary user naming behavior | Fixed pending verification | ASCII case mismatch fails ordinary lookup; Kelvin alias violates generated-domain policy, not a second lookup-loss proof. Independent review/private ON/OFF/ASan283 / 20/fault and combined applications/22 non-fault suites/qmake pass. Sole native fault, scoped checkpoint and durable reconstruction remain pending |
| P5-BLOCK-OWNER-001 | Independent current-candidate durable-block review and root ownership-path trace | P2; mandatory ownership/lifecycle failure; `LC_PreparedBlockEntityChange::Impl::validate` INSERT admission | Before combined patch `0894d1825d33f9b56ea8f4c97ff6553b0cec43a5e8cf588911d1c15ddedcdedc`; repaired two-file patch `1276254a164bb81e6432500715c154aaca0393740e68a8417b943a08998edcea`; no PR review claimed | Require owning generated INSERT snapshots; reject otherwise matching `setOwner(false)` inputs without graph/history/selection mutation, and add rejection/lifetime coverage with independent verification | Fixed pending verification | Actual before selector exits42 with2 / 53 failures across2 cases. Exact integrated guard/leak-safe fixtures receive peer/root approval; private owner53 / 2, focused336 / 22 and block fault pass. Combined ON/OFF/ASan applications/all22 non-fault suites and qmake pass. Sole exhaustive native allocation matrix and scoped checkpoint remain pending |
| P5-MODEL-FIELDS-001 | Retained native-field audit against `DRW_Entity`, `DRW_Text` and the provisional carrier registry | Mandatory preservation gap; `LC_EmbeddedTextModel` native field registry | Original schema-1 source `b4ea18a7e1f57c19da6ef0c931f97256cd52e58be5a769fe5cd4172130f0c4d6`; corrected schema-2 source `00b3883c866d0fd96023ef71e7e1663ee050e4c8dfd580dfc6acb0d8d1b25422` in `de670d842`; no PR review claimed | Complete the reviewed retained-field manifest/registry for wide handles, owner/link/presence/discriminants and embedded X-axis precedence; exact nondefault fixtures and coherent verification before bridge/transport acceptance | Verified | `de670d842`: independent four-file review and root ON/OFF/ASan each pass 102,708 / 13; CMake/qmake acceptance and committed identities recorded above. This fixes inert registry omissions only, not native capture, wire completeness, STYLE graph authority or activation; final whole-PR review remains required |
| P5-DELETED-ATTRIB-001 | Independent canonical guard/emitter/handle-planner source review | Mandatory export-set mismatch; pre-open validation, `writeInsert` and `prepareDwgEntityHandleMap` | Final private safety patch `d67ccb4931515753754cb09d24265c65efbc55339c616b9129a3997a27faf5c5`; matching-old-header evidence above, no PR review claimed | Align live-host membership across admission, emission and DWG child-handle counts; deleted/invisible/restoration regressions, destination preservation and independent supported-format checks | In progress | Historical full-fixture failures are not deletion-only proof. Final matching-boundary normal/partial-ASan diagnostics pass 21,667 / 16 without output hook and 21,679 / 16 with independent R12/R14 checks; independent whole-patch review approves bounded safety policy. Owner/deletion/capability/version/visibility before failures are retained. Current-N4 coherent root integration and full lossless carrier/independent supported-format acceptance remain pending; safe refusals do not complete this contract |

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

For corrective pushes, description edits, thread-state changes and ready/draft
transitions, record the intended operation and expected repository, branch,
head/base and state in the tracking or round record. Verify the remote result
separately from the local command outcome. If the result is uncertain, read back
the actual state before retrying; do not duplicate an action or claim a remote
transition from an attempted request.

Immediately before a remote mutation, read the current head/base, relevant
feedback and the content or state being changed. Compare them with the recorded
expectation. If another contributor changed them, reconcile that change before
proceeding; do not overwrite newly authored description text, resolve new
feedback from an older snapshot, or publish a correction over unexamined remote
commits. Prefer conditional updates where supported and verify the result by
readback; a successful request alone does not exclude a concurrent change.

Run each refinement round in this order:

1. Freeze the review candidate. The initial round covers the entire accumulated
   diff, every review area and the acceptance map. Intermediate rounds examine
   the corrective diff, any base delta and all affected contracts and consumers,
   with the complete diff available to reviewers. Identify which prior coverage
   is invalidated rather than claiming another complete review.
   Record the round ID, exact revisions, coverage and review artifacts,
   including explicit no-finding results. A final whole-diff review remains
   required after the last substantive change.
   Use the reproduction record for reviewer checks and record discrepancies
   between the documented setup and actual execution as findings. Confirm that
   the regressions can detect the violated contract, rather than merely
   compiling or exercising the changed function; use a before-fix candidate or
   bounded negative control where feasible, without committing deliberately
   broken production code or weakening expected results.
2. Triage findings by impact and contract. Record whether each is introduced by
   the PR, an in-scope prerequisite or unrelated baseline behavior. Baseline
   attribution does not waive an in-scope gate; track unrelated issues with
   evidence and apply the deferral/rejection rules above.
3. Reproduce accepted behavioral findings, add a regression where applicable
   and implement the correction. Record unavailable before/after proof rather
   than claiming it. Triage related findings together, but keep unrelated fixes
   in separate scoped commits. Apply the existing validate–document–commit loop;
   do not push an unvalidated correction or a failing-test-only checkpoint.
   Include the corrective plan, acceptance-map and finding-state updates in
   that checkpoint before pushing, rather than leaving them as unstaged notes.
   For a shared-service or public-contract fix, trace every affected consumer
   and failure path through the acceptance map. Record which checks must rerun
   and why any consumer is unaffected; a passing reproducer in one caller does
   not establish TEXT/MTEXT, annotation, persistence or export parity.
   Registry or serialization changes must update the field manifest, capture
   and bridge mappings, version/unknown-field handling and affected literal
   fixtures together. Reopen completeness and transport claims for changed
   coverage; an older green result for an incomplete registry cannot certify
   newly added fields or the native wire path.
   Algorithm, cache or geometry-budget changes also require the affected
   worst-case, cold/warm and memory checks against the recorded P0/P6 budgets;
   a small correctness fixture alone cannot establish bounded performance.
4. Push the validated corrective checkpoint on the owned branch and reconcile
   the published head and checks. Refresh the PR description and remote tracking
   state against that checkpoint. Record later-resolved commit hashes and CI
   links through the existing follow-on plan-record rule; verify its publication
   and readback before claiming final plan/PR synchronization. Keep the original
   tested revision and any justified evidence carry-forward explicit.
5. Have an independent reviewer verify corrections and their regressions before
   marking findings Verified. Resolve threads only after genuine resolution or
   an explicitly accepted deferral or independently reviewed, evidence-backed
   rejection; preserve the rationale and evidence, linking the corrective commit
   and verification to the source feedback. Change public thread state only
   within the authorized PR workflow; if the reviewer or maintainer must resolve
   it, record that handoff action rather than claiming it was done. Repeat a
   whole-diff review after the last substantive change.
6. Close the round with a revision-bound record: reviewed candidate, reviewer
   coverage, finding IDs/states, corrective commits, actual tested inputs,
   independent verification revision, published head, remaining gates and next
   action. Reconcile the plan, acceptance map and PR body against that record.
   Recheck the title, description, walkthrough and evidence links after any
   change to behavior, supported scope, validation or limitations; record the
   presentation review result with the round, without treating it as a
   substitute for technical acceptance.
   Explicitly mark invalidated evidence and reopen affected gates before the
   next round; do not carry an earlier review forward merely because its source
   thread is resolved.

Preserve earlier rounds as history rather than overwriting them with the latest
green result. A fixed number of rounds, a lack of comments or author-only approval
is not evidence of completeness.

| Round ID | Reviewed head, base tip and merge-base | Coverage and finding IDs | Corrective commits, tested inputs and published head | Independent verification revision and evidence | Disposition and next action |
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

#### PR refinement checkpoint

Complete P7b only when the following review gates hold for the recorded
candidate. P6 may still have outstanding platform or release checks; those keep
P7c and the overall P7 milestone incomplete and the PR in draft.

- [ ] Every engineering review area and mandatory acceptance contract has independent source/test-coverage review, including explicit no-finding results and the final whole-diff review after the last substantive change. Outstanding P6 execution lanes remain explicitly Pending; this review does not substitute for their results or qualified Arabic acceptance.
- [ ] All actionable feedback is reconciled with the finding ledger. Mandatory findings are independently Verified or Rejected with reviewed evidence; optional deferrals have authorized acceptance, a documented limitation and a follow-up reference.
- [ ] Corrective commits include regression checks and affected-consumer validation; changes to algorithms, caches or geometry budgets also have the required performance and memory evidence. Unexplained failures or retries remain open gates.
- [ ] The latest published head matches the reviewed candidate; review and validation records identify the actual head/base or synthetic merge tested, and any carried-forward evidence has a documented input comparison.
- [ ] The plan, acceptance map, round history, finding ledger and PR description agree. Remaining P6 checks have explicit configurations, next actions and responsible roles or required maintainer actions; none is presented as passing.
- [ ] Reviewers can reproduce the recorded checks from the candidate and declared inputs; setup discrepancies are resolved, and durable evidence does not depend solely on the author's temporary workspace.
- [ ] Each review area has a passing scoped disposition with no unexplained coverage gap; the title, description, walkthrough and evidence links have been checked against the final candidate and do not overstate its support or validation.

Record the P7b completion decision and candidate in the progress ledger and PR
tracking record. A later substantive change, base advance or newly accepted
finding reopens affected review gates; completion of an earlier round is not
permanent approval of a moving branch.

#### P7c Verify final evidence and hand off

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
Immediately before the ready transition, refresh the complete feedback/check
snapshot and record its time, head and base. If the candidate or base changed,
or new feedback invalidates acceptance, reconcile it and rerun affected gates
before proceeding. Read back the published PR and refresh feedback/checks after
the transition. Triage newly arrived actionable feedback through the ledger;
changed revisions or affected mandatory gates reopen readiness and restore draft
status as required, while optional comments alone do not invalidate acceptance.
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
| Before draft PR publication | Frozen committed candidate, coherent scoped diff, owned branch, clean-candidate rehearsal and durable reproduction record, passing local slice checks, workflow/test-discovery audit, accurate pending-gate matrix, no unrelated/private/generated content |
| After each PR refinement round | Regression proof for fixes, affected suites/builds and performance budgets, CI with actual tested head/merge identity and retained failure history, refreshed finding/decision records, explicit independent review dispositions and candidate-accurate PR presentation |
| Before P7 ready-for-review handoff | Complete technical release checklist, final whole-diff independent review, no unresolved mandatory findings, current PR revision evidence and synchronized plan/PR; then complete P7 attestation |

Build success is not test execution. The existing focused text, headless bidi,
parser, source/profile codecs, profile binding, entity-state, embedded metadata,
filter writer lifetime, shared layout, shaper, registry, native-outline and
conformance targets are `EXCLUDE_FROM_ALL`;
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
set -e
cmake --build /Users/dli/dev/LibreCAD/build-arabic-cmake --target \
  librecad_text_bidi_tests librecad_text_bidi_headless_tests \
  librecad_text_document_tests librecad_text_profile_tests librecad_text_profile_binding_tests \
  librecad_text_source_codec_tests \
  librecad_embedded_text_xdata_tests librecad_embedded_text_model_tests librecad_filter_writer_lifetime_tests \
  librecad_entity_pen_inheritance_tests librecad_entity_container_replace_tests \
  librecad_entity_container_replace_failure_tests \
  librecad_text_profile_binding_copy_failure_tests librecad_text_style_authority_tests \
  librecad_text_shaper_tests librecad_text_font_registry_tests \
  librecad_text_source_state_tests librecad_text_source_tests \
  librecad_text_runs_tests librecad_text_runs_shaping_tests librecad_text_layout_tests librecad_text_outline_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_bidi_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_bidi_headless_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_profile_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_profile_binding_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_profile_binding_copy_failure_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_style_authority_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_source_state_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_source_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_layout_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_document_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_source_codec_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_embedded_text_xdata_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_embedded_text_model_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_filter_writer_lifetime_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_entity_pen_inheritance_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_entity_container_replace_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_entity_container_replace_failure_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_shaper_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_font_registry_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_runs_tests
/Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_runs_shaping_tests
QT_QPA_PLATFORM=offscreen /Users/dli/dev/LibreCAD/build-arabic-cmake/librecad_text_outline_tests
```

Run the parser, source codec, shaper, registry, itemizer and native-outline targets again in a separate explicitly
configured `LIBRECAD_ENABLE_OPENTYPE=OFF` build; confirm the shaper/registry/integration
disabled-path cases actually execute rather than treating their smaller counts
as enabled coverage. The parser's backend-independent cases must still pass.
Run the backend-independent STYLE-authority, embedded-metadata and filter writer-lifetime targets
in both enabled and disabled configurations, and in the coherent sanitizer lane.
Run the pen inheritance and both atomic replacement targets in those same lanes;
the allocator-override executable must remain separate from Catch2 and the app.
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
- [ ] Independent placements and the separately recorded qualified Arabic-reader lane approve representative words, marks and mixed-direction drawings, with reviewer competence/independence, exact candidate artifacts and disposition recorded.
- [ ] Cold/warm layout benchmarks cover long paragraphs and repeated annotations; warm redraw does not reshape unchanged text, including MLeader.
- [ ] Cache memory and outline complexity remain bounded; sanitizer and failure-path checks pass; no developer settings are touched.
- [ ] Documented limits distinguish certified Arabic shaping from unsupported languages, advanced layout and third-party font substitution.
- [ ] Intermediate scoped commits, exact validation results, current decisions and outstanding technical gates are recorded; the plan agrees with the validated implementation.

Record latency, memory and geometry-size baselines in P0, then set release
budgets from representative drawings before optimization. Do not promise a
universal millisecond threshold from the standalone shaping experiment.

### PR handoff checklist

- [ ] The owned branch contains only intended work and documented prerequisites; the PR completed the verified P7a draft checkpoint, is published and attached to the task, and now has the verified ready-for-review state required below.
- [ ] PR workflow coverage and actual test execution are verified; missing/skipped lanes are not counted as passing, and package evidence identifies the tested artifact and producer revision.
- [ ] Independent whole-diff reviews and corrective re-reviews record reviewer/coverage provenance and exact base/head revisions; all actionable source feedback is reconciled to stable finding IDs, accepted mandatory findings are Verified, Rejected findings have reviewed rationale/evidence, and accepted lower-priority deferrals are documented.
- [ ] The P0–P6 acceptance map has no unexplained implementation, test or review gaps; refinement rounds and the PR tracking record identify the final reviewed candidate and outstanding maintainer actions.
- [ ] Reproduction commands and required fixtures are available from the candidate and declared inputs; durable validation/review artifacts identify their producer revisions and checksums without exposing private data.
- [ ] P6 technical gates are complete; required CI and installed-artifact evidence support the current PR revision, with synthetic merge identity or justified evidence carry-forward recorded where applicable.
- [ ] The current base/head have no merge conflicts and passing integration evidence; conflicting or unknown integration status is not represented as ready for review.
- [ ] The published head, final scope/commit review, plan, PR description and refinement ledger agree; a dated final feedback/check snapshot and readback confirm the actual ready-for-review state before P7 is marked Complete.
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
