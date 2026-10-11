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

# MText Indentation Implementation Plan

## Status And Scope

| Field | Value |
| --- | --- |
| Issue | [LibreCAD #3032](https://github.com/LibreCAD/LibreCAD/issues/3032) |
| Target | `origin/master`; no stable-branch backport assumed |
| Implementation base | `628a1ba84` (`origin/master`) |
| Reviewed upstream | `628a1ba84`, fetched 2026-10-11; intervening changes do not modify the text layout or codec |
| State | Track A implemented 2026-10-11; focused/full text tests and CMake/qmake application builds pass; native GUI validation pending |
| Recommended first delivery | Preserve space/Tab indentation with left attachment and existing unindented geometry; correct reference-width persistence separately |
| Scope question | Does the request also require paragraph controls for left and first-line/hanging indentation? |

The issue supplies no example drawing, screenshot, or description of an old
control. Source inspection identifies concrete whitespace-layout problems, but
does not establish that a paragraph-formatting dialog was removed. Do not claim
that restoring whitespace alone implements a full paragraph editor.

Track A below is the compact, independently useful restoration. Track B is an
explicitly scoped extension if paragraph controls are required. Confirm the
scope before claiming the issue is fully resolved; Track A does not depend on
waiting for that clarification.

## Progress And Commit Boundaries

| Step | Status | Commit | Completion evidence |
| --- | --- | --- | --- |
| A0. Isolate tests and characterize behavior | Implemented, automated checks pass | `567799327` | Existing settings-isolation listener linked; both editors assert isolation before construction |
| A1. Preserve leading spaces | Implemented, automated checks pass | `567799327` | Two fonts, heights and all attachments; transforms, blank parent/proxy bounds and unchanged ink widths |
| A2. Implement direction-aware Tab advances | Implemented, automated checks pass | `567799327` | Whole-line bidi; unequal-width fields, embedding/isolate context, exact and repeated stops, Qt positional oracle |
| A3. Preserve saved layout and edits | Implemented, automated checks pass | `567799327` | Generated standalone/embedded DXF/DWG round trips; independent DXF tags and LibreDWG width decoding |
| A4. Enable Tab input and validate release | Implemented; native release checks pending | `567799327` | Tab/undo/redo and local Ctrl+Tab focus escape tested in both editors; CMake and qmake application builds pass; native GUI pending |
| B0. Confirm paragraph-control scope | Awaiting scope decision | - | Accepted UI, persistence, wrapping and compatibility contract |
| B1-B4. Paragraph controls | Conditional, detailed below | - | Parser/layout/editor/export tests, not only visual editor margins |

Update this table after each implementation commit with its hash, exact tests
run, and any remaining environment limitations. Keep regression tests with the
fix they validate; do not land a failing-test-only commit on the target branch.

## Review Resolutions

These findings defined the implementation requirements. R1-R4 and R6-R7 are
covered by Track A; R5 remains a Track B dependency.

| ID | Finding | Required resolution | Evidence before implementation is accepted |
| --- | --- | --- | --- |
| R1 | Main dialog consumes Tab for focus navigation | A4: disable that override locally; retain a tested keyboard focus escape | A real key event inserts U+0009 in both MText editors, including RTL mode |
| R2 | Increasing visual X is not a correct RTL Tab origin | A2: measure fields and advance from the paragraph's flow origin, preserving full-line bidi context | Glyph positions, not only ordering, match the independent Qt oracle for unequal-width fields |
| R3 | Self-round trips cannot validate reference-width semantics | A3: preserve declared width, test serialized fields and independent layout | Zero/no-wrap, wide and intentionally narrow widths have distinct tested outcomes |
| R4 | Embedded attributes bypass `writeMText()` and can replay stale text | A3: cover `writeInsert()` and define modeled-field versus sidecar authority | No-op metadata preservation and edited outer/embedded attribute values both pass |
| R5 | Generic import decoding destroys paragraph scopes | B1: tokenize MTEXT before flattening; preserve raw spans and unsupported formatting | Nested scopes, escapes and unknown properties survive edit/save without state leakage |
| R6 | Empty bounds contract contradicts the engine | A1: allow no drawable extent; guard all bounds consumers | Empty children do not enlarge parent bounds or create snap/selection geometry |
| R7 | Focused dialog tests can write developer settings | A0: link the existing settings-isolation listener into the text target | Private settings are active before fixtures/dialogs run and disposed after shutdown |

## Implementation Notes And Verification

Track A uses transient visual fields only. Glyphs and stacks are emitted once
with their existing LFF advances; Tab-delimited fields are then translated from
the paragraph's flow origin. This avoids a second glyph-measurement algorithm.
Only left attachment retains leading visual whitespace; center/right attachment
and `getUsedTextWidth()` keep their existing ink-width contracts. Empty lines,
stacks, draft baselines and proxies do not create drawable bounds.

Both MText editors retain ordinary keyboard Tabs. Testing found that native
Ctrl+Tab did not leave the editor in this Qt setup, so each editor now installs
a widget-scoped Ctrl+Tab shortcut using its existing focus order. No global
`LC_TextEdit` policy or single-line text editor behavior was changed.

Standalone and embedded writers preserve declared reference width, including
zero and narrow positive values, and reject negative/nonfinite values. Embedded
attribute sidecar schema 3 retains the original modeled geometry and spacing
for edit detection, while still reading schemas 1 and 2. This prevents legacy
import adjustments from being mistaken for edits. Unchanged raw text and
background metadata survive; supported edits override stale sidecar fields.
Edited column/annotative layouts fail explicitly instead of discarding metadata.
Embedded DWG MTEXT has no independent EED frame, so its existing private RTL
marker is carried on the owning ATTRIB and restored during import.

Verification on macOS arm64, Qt 6.11.2 and Xcode SDK 27.0:

| Check | Result |
| --- | --- |
| `[issue3032]` with LibreDWG 0.14 oracle | 9 cases, 13,721 assertions passed |
| Entire `librecad_text_bidi_tests` | 63 cases, 16,502 assertions passed |
| `[dxf_codec],[attrib],[mtext],[visibility]` | 26 cases, 130 assertions passed |
| CMake `librecad` application compile/link | Passed |
| qmake6 application compile/link | Passed after refreshing the cached SDK configuration and stale libdxfrw dependency |
| Native application GUI | Pending: the Mac was locked when computer-use validation was attempted |
| ASan/UBSan and standalone dimension integration suite | Not run; no current focused text sanitizer/geometry-test executable was available without a broader rebuild |

The optional `LIBRECAD_TEST_DWGREAD` environment variable enables independent
JSON decoding in the generated-width regression; it introduces no default
test dependency. The oracle was actually run for all 18 combinations of
R2000/R2004/R2007/R2010/R2013/R2018 and zero/narrow/wide widths. DXF checks inspect
groups 41/71 directly and use Qt wrapping separately. Generated samples include
text crossing DXF chunk boundaries; no external drawing/font fixture was added.
R12 downgrade is checked for readable content, not exact Tab geometry.

Commands used after refreshing the existing build configuration:

```sh
cmake --build build --target librecad_text_bidi_tests --parallel 8
QT_QPA_PLATFORM=offscreen LIBRECAD_TEST_DWGREAD=/opt/homebrew/bin/dwgread \
  build/librecad_text_bidi_tests '[issue3032]' --reporter compact
QT_QPA_PLATFORM=offscreen build/librecad_text_bidi_tests --reporter compact
QT_QPA_PLATFORM=offscreen build/librecad_dxf_fast_tests \
  '[dxf_codec],[attrib],[mtext],[visibility]' --reporter compact
cmake --build build --target librecad --parallel 8
make -C build-qmake6-codex/librecad/src -j8 EXPORT_QMAKE_MAC_SDK_VERSION=27.0
```

The cached Qt installation and macOS SDK had changed since configuration,
requiring a wider initial rebuild. No source/build cache was deleted. After
autogen had processed the UI change, Unix Makefiles' existing `/fast` targets
were used for source-only iterations, avoiding repeated whole-project autogen.
Do not use that shortcut after changing UI, project inputs or generated headers.

Remaining release validation: native creation/reopening, print preview,
PDF/image output and snapping/zoom checks; dedicated dimension integration and
sanitizer coverage. These are not reported as passed by the
offscreen tests. Track B's paragraph controls, scoped formatting parser and
wrapping remain conditional and are not included in Track A.

## Verified Code Findings

Paths in this document are relative to the repository root. Function names are
the durable anchors; line numbers describe the reviewed checkout only.

| Finding | Code anchor | Consequence |
| --- | --- | --- |
| Left attachment normalizes every line to its visible minimum X | `librecad/src/lib/engine/document/entities/rs_mtext.cpp`, `updateAddLine()`, around 697 | Leading space advances are canceled when the line moves by `-minX` |
| Tab is treated as an ordinary character, then skipped as a control | Same file, `update()` and `flushBidiLine()`, around 310 and 468 | A real U+0009 contributes no drawing advance |
| DXF caret decoding preserves Tab as U+0009 | `librecad/src/lib/filters/rs_filterdxfrw.cpp`, `toNativeString()` and `toDxfString()`, around 32793 | Decoded `^I` exposes the missing renderer behavior; reverting to four spaces would be lossy |
| Export uses visible width rather than reference width | Same filter, `writeMText()`, around 30890 | Saved wrapping constraints can change even when canonical text and local geometry survive a self-round trip |
| Used width has non-export consumers | `rs_dimension.cpp`, `rs_dimradial.cpp`, `lc_dimordinate.cpp`, `lc_tolerance.cpp` | Redefining `getUsedTextWidth()` globally can change dimension spacing and label placement |
| Both editors store plain text | `librecad/src/ui/dialogs/entity/qg_dlg_mtext.cpp` and `lc_mtextpropertieseditingwidget.cpp` | Literal whitespace can survive editing, but Qt-only paragraph margins cannot survive `toPlainText()` |
| Main dialog explicitly enables Tab focus changes | `librecad/src/ui/dialogs/entity/qg_dlg_mtext.ui`, `tabChangesFocus`, around 831 | Ordinary keyboard Tab cannot insert indentation; this is a required UI fix, not a conditional smoke check |
| Embedded MTEXT has a separate writer and raw sidecar | `RS_FilterDXFRW::writeInsert()`, around 30600 | Saved text/attachment/spacing can override current modeled edits; fixing standalone export alone is incomplete |
| Generic import strips formatting groups before rendering | `RS_FilterDXFRW::toNativeString()`, around 32835 | Paragraph-state parsing after this decoder cannot recover removed brace scopes |
| Empty entity containers deliberately have invalid bounds | `RS_EntityContainer::forcedCalculateBorders()`, around 531 | Empty text must retain the no-extent sentinel, rather than fabricate finite origin geometry |
| Dedicated text target omits the shared settings listener | `CMakeLists.txt`, `librecad_text_bidi_tests`, around 2771 | Dialog destruction writes settings that the font fixture alone does not isolate |
| Reference width is not currently a wrapping constraint | `RS_MText::update()`; `RS_MTextData::width` in `rs_mtext.h` | A true first-line/hanging-indent editor needs a wrapping design, not just an offset |
| Property-panel width edits are currently discarded | `LC_MTextPropertiesEditingWidget::onWidthEditingFinished()` | Adjacent prerequisite for Track B, not a reason to widen Track A |

The left-edge normalization predates the recent bidi work. Treat this as a
layout defect plus missing Tab handling, not as evidence that Unicode bidi
processing should be removed.

A standalone offscreen Qt probe already found a positional mismatch for RTL
`A<Tab>BBB`, `AAA<Tab>B` and `ABC<Tab>D<Tab>EF`: the existing bidi cluster order
agrees with Qt, but applying the next stop while traversing increasing visual X
does not reproduce Qt's gaps. Retain these as regression shapes; do not infer
correct positioning from a passing visual-order test. This probe is not a
substitute for reproducing the full application's layout and editing paths.

## Reference Constraints

Paragraph indentation and paragraph alignment are separate controls in
[Autodesk's paragraph UI](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-Core/files/GUID-90F5A530-BB66-4264-86F0-D5F670EC8519.htm).
This is a UX reference, not proof of the issue author's intended UI.

The [Autodesk MTEXT DXF reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-5E5DB93B-F8D3-4433-ADF7-E92E250D2BAB.htm)
distinguishes group 41 reference width from group 42 character width and group
71 attachment. Do not fix indentation by moving the saved insertion point or
switching its attachment.

[Autodesk's MTEXT width documentation](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-Core/files/GUID-F94BE932-DA31-437E-9610-27F46ACD5711.htm)
defines zero width as no word wrapping. Preserve zero rather than replacing it
with measured ink width. A positive reference width is a wrapping constraint,
not a guarantee that LibreCAD currently implements wrapping.

[ezdxf's implementation notes](https://ezdxf.readthedocs.io/en/stable/dxfinternals/entities/mtext.html)
describe `^I` as Tab and default tab stops at multiples of four initial text
heights. They also describe lowercase `\p` paragraph properties, including
left indent and a first-line offset relative to it. These are implementation
findings, not a substitute for version-specific compatibility checks.

[Qt QTextEdit](https://doc.qt.io/qt-6/qtextedit.html) accepts Tab by default and
measures editor tab-stop distance in pixels.
[QTextBlockFormat](https://doc.qt.io/qt-6/qtextblockformat.html) provides margins
and first-line indentation. Neither editor pixel measurements nor transient
block formats are a drawing-unit persistence model.

## Track A: Behavior Contract

1. For left-attached LTR MText, leading U+0020 spaces and U+0009 Tabs move the
   first visible content away from the unchanged insertion point. Each explicit
   LF or `\P` starts a new line with its own indentation.
2. Preserve the existing glyph-bearing normalization for lines without leading
   whitespace. Merely removing `-minX` is not acceptable: some LFF glyphs have
   nonzero ink bearings.
3. Keep center/right attachment and all vertical attachment rules unchanged.
   Adding Tab advance must not redefine how these attachments anchor their ink.
4. Keep internal spaces, trailing spaces, formatting controls, font changes,
   stacked text, combining marks and surrogate pairs distinct. Only spaces and
   Tabs are indentation advances; bidi controls remain zero-width.
5. Unicode logical order and visual order are different. Preserve whitespace on
   its bidi-resolved visual side; do not force a logical RTL prefix onto the
   visual left or reverse numeric runs. Explicit direction-independent
   paragraph indentation belongs to Track B.
6. Empty and whitespace-only lines retain logical line spacing but have no
   drawable extent. The engine's invalid-vector sentinel is valid for that
   state; only ink-bearing bounds must be finite and ordered. Do not create
   fake selectable geometry or propagate invalid coordinates into arithmetic.
7. Drawing, draft display, printing and PDF/image export use the generated
   geometry. DXF/DWG persist canonical text and declared properties, verified
   independently in A3. Parsing/layout happens in `update()`, never in paint,
   snapping or bounding-box queries.
8. Canonical text remains unchanged: no trimming, automatic space expansion,
   synthetic bidi controls, private indentation XDATA or HTML storage.
9. Export preserves the declared reference width, including zero, independently
   of ink or indentation. This intentionally corrects serialized width even
   for unindented entities; it does not change their local glyph geometry or
   promise newly implemented wrapping.

## A0: Reproduction And Baseline

Use generated entities and the existing `SourceFonts` fixture in
`librecad/src/lib/engine/document/entities/tests/text_bidi_regression_tests.cpp`.
Set LTR explicitly; its `mtextData()` helper currently defaults to RTL.

First add `${LIBRECAD_TEST_SETTINGS_ISOLATION_SOURCE}` to the existing
`librecad_text_bidi_tests` source list. Reuse the Catch2 listener from
`librecad/src/test/lc_testsettingsisolation.cpp`; do not introduce another
settings framework or test target. Verify private INI stores are established
before `SourceFonts`, `RS_Settings` and any dialog are initialized. The font
fixture's save/restore of its own font path does not isolate the dialog's
`Draw/TextString`, font, height, alignment, angle and direction settings.
Exercise apply/cancel and destruction, including shutdown before store cleanup.

Capture glyph positions for `ABC`, `    ABC`, Tab followed by `ABC`, and
`ABC` followed by an explicit newline and four spaces plus `DEF`. Compare
against the unindented form at the same height, style and attachment.
Characterize that current spaces collapse at the left edge and current Tab
does not advance; reproduce in a live MText dialog before reporting a runtime
root cause as confirmed.

Characterize literal Tab, DXF `^I`, LF/`\P`, escaped literal
carets/backslashes, and canonical text through editor load/save. Add passing
baselines for supported conversions, recording existing escape/scope failures
separately as B1 prerequisites; do not describe the current generic decoder as
lossless or broaden Track A into an unrelated codec rewrite.
Use a second bundled font with different ink bearings; do not bake a font's
glyph bounding box into expected numeric constants.

Record the positive stored-width behavior and the two export paths before
changing either. Tests must distinguish declared width, actual ink width,
indented occupied span and any external wrapping outcome.

## A1: Preserve Leading Space Advance

Primary changes: `rs_mtext.cpp` and, only for private signatures or metrics,
`rs_mtext.h`.

1. Have `flushBidiLine()` report a small transient line-layout result: leading
   visual whitespace advance and whether the line contains drawable content.
   Compute it during the existing visual emission pass, not by scanning the
   source's first characters before bidi resolution.
2. Keep leading indentation open across zero-width formatting controls; close
   it at visible character/mark/stack content. Internal and trailing whitespace
   must not be added to the indentation result. Use actual emitted-content
   bounds to distinguish empty lines from ink-bearing lines.
3. Pass this result through both line-finalization paths: the newline/`\P`
   closure and the last-line flush. Reset it for every line.
4. For an ink-bearing left-attached line, use the existing height scale `s`
   and apply horizontal translation `s * leadingAdvance - scaledInkMinX`.
   With zero leading advance this is exactly the old translation. Leave the
   center/right cases unchanged.
5. Carry logical line/paragraph count and vertical advance separately from
   drawable bounds. Guard `updateAddLine()`, `alignVertically()`,
   `rotateLinesRefs()`, draft/proxy construction, transforms and export before
   using min/max/size. Check actual valid ink bounds, not only child count:
   an empty stack/container can have children and still have no extent.
   Keep empty text's no-extent sentinel without inventing an endpoint or proxy.
6. Keep the existing space-width convention in this change. Do not fold in a
   general rewrite of font-change spacing, LFF glyph advances or text parsing.
7. Ensure empty children cannot alter their graphic/container bounds, zoom or
   snap candidates. If tests use `getNumberOfLines()`, reconcile its LF-only
   count with parsed `\P` paragraphs explicitly; drawable-child count is not a
   logical line count.

Acceptance: four leading spaces produce four existing word-space advances,
scaled with text height, while every glyph in the unindented control retains
its old position. Rotation applies after that local-coordinate translation.

## A2: Real Tab Stops

1. Add an explicit `Tab` segment alongside `Char`, `Space` and `Stack`.
   Recognize U+0009 in the logical parser and retain it as Tab in the bidi input.
   Do not make all `Other_Control` characters advance.
2. Use a named default interval of four initial text heights in drawing units,
   represented consistently in the existing normalized LFF coordinates.
   Keep this CAD policy separate from QTextEdit's screen-pixel tab interval.
3. Resolve bidi over the whole explicit line once. Retain logical field
   membership at Tabs and the resolved clusters' source mappings; do not run
   independent bidi resolution on each field or discard embedding/isolate
   context across fields.
4. Measure Tab-delimited field advances with the same LFF glyph, space,
   formatting and stack metrics used for emission. Factor a small private
   measurement helper if needed; do not maintain two inconsistent glyph-width
   implementations. Keep measurement and placement linear in input size and
   confined to `update()`.
5. Use nonnegative flow distance `d` from the paragraph's starting edge, not
   increasing visual X. For field advance `w`, the basic LTR field interval is
   `[d, d + w]`; the RTL interval is `[-d - w, -d]`. After the field, increment
   `d` by `w`; at a Tab use `(floor(d / T) + 1) * T`. Place resolved clusters
   within these intervals, then apply existing attachment/bearing normalization.
   Preserve full-line resolved ordering; mixed embeddings and isolates crossing
   Tabs must pass the positional oracle before accepting this model.
6. Require finite nonnegative field advances and `d`, finite `T > 0`, finite
   results and strictly positive Tab progress. Test exact/near stops and large
   representable values; reject unrepresentable layout safely rather than emit
   NaNs, partial geometry or non-advancing stops. Do not loop once per stop or
   repeatedly add floating-point intervals.
7. Include only visual-leading whitespace in A1's indentation metric. Internal
   Tabs move subsequent content; logical RTL-leading whitespace can be on the
   visual right and must not become a forced left indent. Reset stops at
   explicit breaks. Preserve consecutive Tabs and mixtures in the source.
8. Add a separate `QTextLayout` positional oracle with no wrapping, explicit
   paragraph direction/alignment and a controlled monospaced font. Use glyph
   run positions/source indexes, not ambiguous bidi-boundary `cursorToX()`
   values. Normalize attachment and use the oracle's measured advances when
   checking the stop model; do not compare Qt font widths directly to LFF ink.
   Cover unequal-width fields in LTR/RTL, numbers, isolates and multiple Tabs.
   Separately verify production placement with real bundled LFF advances.
   For LibreCAD's Han RTL override, feed Qt the documented directional display
   text and map back to logical source positions; unmodified Chinese under
   native Qt bidi is not an oracle for that intentional private policy.

Custom, center, right and decimal tab stops are not part of Track A. Do not
claim imported custom `\p` tab settings are rendered correctly by default stops.
RTL MTEXT uses LibreCAD's private direction convention, not a standard DXF
RTL direction value. Qt validates that local convention; interoperability
claims must distinguish it from standard LTR MTEXT behavior.

## A3: Persistence And Export Authority

Keep `getUsedTextWidth()` as the current visible-ink metric because dimensions
and other annotations consume it. Use transient occupied-span metrics only for
layout/testing; do not add a public width cache solely for export. The persisted
reference width is already available as `getWidth()` / `RS_MTextData::width`.

### A3.1. Reference-Width Policy

Use declared reference width in both `writeMText()` and the embedded-MTEXT part
of `writeInsert()`. Preserve every finite nonnegative width exactly, including
zero/no-wrap and an intentionally narrow positive width. Never replace it with
ink width, indent-plus-ink width, or the maximum of those and declared width.
Reject negative/nonfinite values through the existing write-failure path before
emitting the affected record. Preserve insertion point, attachment, height,
spacing and the private RTL marker.

This corrects standalone serialized width even for unindented text. It does not
enable wrapping: LibreCAD currently ignores positive reference width during
layout, so a narrow-width file can wrap differently in an external renderer.
Document and test that limitation instead of silently widening the declaration.
Do not introduce a provenance flag or silently change new-entity width defaults;
default/reference-width UI policy belongs to B0/B2.

### A3.2. Standalone And Embedded Text Authority

Reuse the import conversion and output-mode encoding rules consistently in
both save paths. Keep `^I` for DXF and the current native-control representation
for DWG; do not add an independent embedded-attribute decoder.

In `writeInsert()`, classify each sidecar field as modeled/editable or unmodeled
metadata. Replay saved raw formatting text only when decoding it with the
import rules yields the unchanged canonical value. On a content edit, encode
the current entity text and synchronize the outer ATTRIB fallback value;
neither `sidecar.value` nor `sidecar.mtext.text` may override that edit. Preserve
raw text on no-op saves and unrelated metadata on whitespace edits. Do not
claim the generic decoder preserves unsupported formatting through arbitrary
edits before B1.

For modeled insertion, width, attachment, direction and spacing, compare
against the imported modeled state: current edits win; unchanged raw payload
may remain authoritative for unsupported details. Preserve tag, flags, owner
links, handles, XDATA and unrelated metadata. Audit duplicated R2018 layout
fields when a modeled field changes so stale saved geometry does not override
it. Do not erase column/annotation data as a shortcut; if an edited combination
cannot be represented safely, report the limitation rather than claim a
lossless save. Keep these rules local to MTEXT attributes, not a sidecar rewrite.

### A3.3. Persistence And Independent Proof

Generate standalone MTEXT and INSERT/ATTRIB samples in temporary directories.
For supported DXF/DWG versions verify canonical text, local glyph positions,
attachment and declared width, including text crossing DXF chunk boundaries.
For embedded attributes cover both no-op save (raw text and metadata retained)
and edited indentation (outer fallback and embedded value updated), plus RTL
markers and version-specific metadata.

Self-round trips are necessary but insufficient: the local renderer ignores
reference-width wrapping. Independently extract DXF groups 41/71 from the saved
record and check encoded DWG width against the version-specific field layout,
not only the production decoder's accessor. Feed the extracted DXF text/width
into a separate layout check with controlled metrics: select no-wrap for zero,
wrapping for positive width. Required local cases cover zero/no-wrap,
sufficiently wide positive width, and narrow width without asserting false
visual equivalence for the latter. Use Qt's wrapping/positioning with matched
synthetic or monospaced advances for the algorithmic oracle, not arbitrary
Qt-versus-LFF font geometry. Do not reuse the writer's width-selection helper
to compute expected results. An ezdxf read or render smoke check is additional
evidence when available; record its version and configuration, do not
auto-install it or silently skip a required gate.
Exact external layout claims exclude unsupported formatting and private RTL
extensions. No paid CAD account is a prerequisite for automated tests.

DXF R12 has no MTEXT; the existing writer degrades it to TEXT lines. Exact Tab
and paragraph-format round trips are outside Track A. Test that R12 output
remains readable and document the layout limitation; do not label raw `^I` in
TEXT as proof of preserved Tab layout. Users needing exact legacy appearance
can explode text to geometry before export. A richer R12 downgrade requires
its own layout-to-TEXT/geometry design, not lossy changes to the shared codec.

## A4: GUI And Rendering Validation

Keep the existing plain-text UI and disable `tabChangesFocus` locally in
`qg_dlg_mtext.ui`; the current explicit `true` is a verified input defect.
The property editor already uses the non-focus-changing default: assert that
policy in both editors without changing `LC_TextEdit` globally.
Both `QG_DlgMText` and `LC_MTextPropertiesEditingWidget` must retain spaces and
Tabs when creating, applying, canceling, reopening, copying/pasting and loading
or saving text files. Send actual key events and assert U+0009 insertion,
including after direction changes. Verify an accessible keyboard route out of
the editor with the existing dialog tab order; do not assume Ctrl+Tab works
without testing the platform's handling. Add a local fallback only if needed.

Exercise LTR/RTL switching and the Chinese RTL `LC_TextEdit` custom-layout path
with whitespace and numbers. Preserve cursor/selection indices, native undo,
IME composition and source text. Do not change single-line `RS_Text` input to
implement an MText feature. A Qt-font editor is not an exact LFF drawing preview.

Check normal/draft drawing, print preview, PDF/image output, selection and
zoom-to-extents. Blank indentation must not become geometry or a snap target;
the insertion point remains the entity reference. Confirm existing dimension
placement remains unchanged and repeated bounds/snap queries do not relayout.

## Focused Test Matrix

| Area | Required cases | Assertion |
| --- | --- | --- |
| Space indentation | 0/1/4 spaces; LF and `\P`; multiple lines | Relative first-ink displacement equals the retained advance |
| Compatibility | Two LFF fonts; top/middle/bottom; center/right; no whitespace | Existing ink normalization and attachment unchanged |
| Tabs | Leading/internal/repeated; unequal-width fields; space mixtures; exact/near/large stop; multiple heights | Direction-aware field placement and positive finite progress, source Tab retained |
| Formatting | Font switch, stack, combining mark, non-BMP character, bidi controls | No lost glyphs, controls stay zero-width, finite positions |
| Empty input | Empty text/line/stack, only spaces/Tabs/controls, leading/trailing blank paragraphs | Logical spacing retained, no-extent sentinel respected, parent bounds unchanged, no fake ink |
| Bidi | Arabic/Hebrew and digits, Chinese plus digits, embeddings/isolates across Tabs, legacy RTL | Logical source and full-line context preserved; both glyph order and positions checked |
| Transforms | Insertion, rotation, positive scale, mirror, clone, repeated update | Same geometry as an equivalent fresh entity |
| Persistence | Modern DXF/DWG; all left vertical attachments; chunked text; standalone/embedded MTEXT | Text, attachment, indentation and declared width survive; edited sidecars cannot overwrite values |
| Independent width proof | Zero/no-wrap; wide/narrow positive widths; serialized DXF/DWG fields and independent layout | Declared width is preserved; local self-round trips cannot hide external wrapping differences |
| Embedded metadata | No-op and edited ATTRIB; saved raw text, fallback value and R2018 layout metadata | Raw/unmodeled data retained when unchanged; modeled edits reflected in both values |
| Legacy export | R12 degradation | Readable output; limitations explicit, no exact-layout claim |
| Editing | Both MText editors; Tab/paste, undo/redo, cancel/apply, text-file load/save | Whitespace and cursor mapping survive without hidden formatting |
| Test isolation | Font fixture, dialog settings writes, shutdown | Private settings store active before initialization; no developer preferences changed |
| Adjacent consumers | Dimensions/tolerance labels and draft proxies | Old used-ink width semantics and correct shifted baselines |
| Performance | Long multiline text; repeated bounds/snap queries | One update-time layout; no extra whole-text passes in queries |
| Paragraph codec (B1) | Nested font groups containing `\p`, unknown properties, escapes and malformed commands | Raw scopes survive; paragraph state cannot leak after a removed closing brace |

Use existing test files and a new `[issue3032]` tag, with `[layout]` and `[gui]`
subsets. Generate fixtures in temporary directories; add no externally sourced
DWG/DXF binaries or proprietary fonts.

## Expected Change Boundary

Track A requires `rs_mtext.cpp`, private declarations in `rs_mtext.h` if needed,
both `writeMText()` and embedded-MTEXT `writeInsert()` boundaries in
`rs_filterdxfrw.cpp`, the local `qg_dlg_mtext.ui` Tab property, and regression
tests in existing test files. Add the existing settings listener to the text
target's CMake source list; this is a test-safety prerequisite, not a build
refactor. No public occupied-width API is needed.

Extend `dxf_string_codec_tests.cpp` only for missing codec assertions, not a
codec rewrite. Any other editor change must be justified by a failing local
editing/focus test.

No bidi engine replacement, new dependency, global text-width redefinition,
translation-file churn, build-system refactor or unrelated width-panel fix
belongs in this delivery. Track B changes must remain separately reviewable.

## Incremental Build And Verification

Reuse a configured CMake build with `BUILD_TESTS=ON`. The following existing
targets are sufficient; no new build target or CI workflow is needed. Link and
verify A0's isolation listener before running any dialog subset:

```sh
cmake --build build --target librecad_text_bidi_tests --parallel 8
QT_QPA_PLATFORM=offscreen build/librecad_text_bidi_tests "[issue3032][layout]"
QT_QPA_PLATFORM=offscreen build/librecad_text_bidi_tests "[issue3032][gui]"
```

Use the actual target executable location for the chosen platform/configuration.
During editing, run only the new layout subset. At the persistence milestone,
incrementally build `librecad_dxf_fast_tests` and run its `[dxf_codec]` subset,
plus the new generated standalone/embedded round trips in a focused target.
Run the positional and declared-width oracle tests alongside those cases;
do not substitute a passing LibreCAD self-round trip for independent proof.
Before delivery run the whole focused text target and the existing relevant
dimension tests. Do not
build or run the complete DWG corpus/fuzz suites on every text change.

Reuse an existing qmake6 shadow build for the application compile/link gate;
regenerate its Makefile only if project/UI inputs changed. Avoid in-source
generation, deleting caches or starting clean builds for each iteration.
Existing production files are already listed in `librecad/src/src.pro` and
CMake. Existing test files are already in the text/general targets.

Run the new subset with an existing ASan/UBSan build if available, and a native
GUI smoke test before release. Record unavailable platform/GUI checks as
pending rather than claiming they passed. Record settings-isolation evidence
and any optional external-reader results separately. `git diff --check` is
required; use `git diff --no-index --check /dev/null <file>` for an untracked
plan or other new file not yet included in the index.

## Track B: Paragraph Controls, If Required

This is not equivalent to inserting whitespace and is not implemented by
setting QTextBlockFormat margins alone. Complete these dependency-ordered
sub-plans before exposing controls:

### B1. Canonical Paragraph Model And Codec

The generic `toNativeString()` is a lossy boundary: it removes font/height/color
group braces and globally replaces escape sequences. A paragraph parser added
after that conversion cannot promise scoped or lossless preservation. For
example, removing the closing group around a font change containing `\pl2;`
can make that indentation leak into the following paragraph.

Add an MTEXT-aware token/decode boundary before flattening, shared by standalone
and embedded MTEXT imports. Preserve raw source spans, brace nesting, escaped
delimiters, unknown commands and scoped formatting. Distinguish external MTEXT
syntax from LibreCAD's canonical editor text so a literal escaped `\P` is not
decoded twice as a paragraph break. Keep the shared TEXT codec unchanged.

Parse a bounded subset of paragraph properties from those tokens into transient
per-paragraph state, initially left indentation and relative first-line indent.
Keep raw/tokenized formatting as the persistence authority for unsupported
content and canonical logical text as the editing authority for supported
content, with an explicit mapping between them. Handle reset values,
scope/inheritance, caret controls and malformed/truncated commands
deterministically. Never strip a whole command because one property is unknown.

UI edits must patch only supported properties for current/selected paragraphs,
serialize back to the standard text representation, and be idempotent. Use
locale-independent numeric serialization. Preserve unrelated raw spans during
patches, including escaped braces/backslashes, unknown fields and scoped font
commands. Add parser/serializer/import tests before changing either editor;
specifically test nested font-plus-paragraph groups followed by unformatted
paragraphs, literal escaped `\P`, resets and incomplete scopes. Verify property
inheritance and target-version behavior with generated independent-reader
samples; unresolved semantics block that property, not preservation of the
original text.

### B2. Paragraph Layout And Wrapping

Distinguish explicit paragraphs from width-wrapped visual lines. Use reference
width as a constraint only under an agreed compatibility policy: turning on
wrapping for all old files changes existing drawings and needs separate review.
Resolve that policy in B0 before calling B2 implementation-ready. Define zero
as no-wrap and keep positive width distinct from measured occupied span; do not
infer wrapping intent from the existence of an indent code or rewrite widths.
For formatted paragraphs, implement first-line versus continuation-line origins,
including hanging indents, in local coordinates before attachment and rotation.

Use grapheme-safe legal line-break boundaries and LFF-measured advances. Resolve
bidi paragraph levels before line breaking and reorder each visual line with
the correct line-level resets. Do not run independent bidi resolution on
arbitrarily split source substrings. Define no-progress handling for an
oversized cluster, indent exceeding width and invalid/nonfinite dimensions.
Do not accidentally add justification, columns or a general rich-text engine.

Reuse A's private ink/advance measurements, not a new persisted width cache.
Correct the property's discarded width edit with `setWidth()` and the existing
update/preview lifecycle, and preserve the reference width on export rather
than substituting actual ink width.

### B3. Minimal Shared UI

Expose the same supported left/first-line values in MText creation/properties
and the property editor, using the existing unit/expression input conventions.
Apply to the current paragraph or selected paragraphs; represent mixed values
without overwriting them. Keep attachment and text direction independent.
Negative first-line values express hanging indentation, subject to the layout
validation above. Do not expose unsupported right/decimal tabs or justification.

Map canonical values to Qt block formatting only as an editing view. Preserve
text-to-document position mappings, hidden-code spans, selection, IME, and one
undoable transaction per formatting command. Rebuild Chinese RTL cached layouts
when paragraph format or tab options change, not on every paint. Do not offer
controls until edits serialize and reopen correctly in both editors.

### B4. Compatibility And Completion Gate

Test first/continuation lines, hanging and mixed paragraph indents, alignment
changes, wrapping, Unicode and formatted-text editing. Verify modern DXF/DWG
round trips through an independent reader and define legacy downgrade behavior
before enabling save paths that cannot express the layout. External paid CAD
access must not be required to run the automated regression suite.

No new translation files are edited by hand; use normal translatable source
strings and the project's translation workflow. Register any genuinely needed
new helper files in both build systems, with the project's license headers.

## Review And Acceptance Gates

Before implementing, confirm that no newer upstream text changes supersede
these findings. A review must reject a fix that removes glyph-bearing
normalization, treats every control as spacing, stores only editor formatting,
alters dimension ink-width contracts, or reparses on paint/snap queries.
Also reject visual-X-only RTL Tab placement, width synthesized from ink,
sidecars that override edits, scope parsing after lossy decoding, fabricated
empty bounds, or dialog tests without settings isolation.

Track A is ready when the original left-attached whitespace examples render
and round-trip in supported MTEXT formats, unindented text retains its old
local geometry, declared-width preservation and its wrapping limitation are
verified independently, standalone and embedded edits survive, both editors
accept keyboard Tab without writing developer settings, and focused
CMake/qmake6 checks pass. Zero-indent geometry compatibility is not a promise
to retain the old incorrect exported width. Do not mark Track B complete, or
close the issue as a full paragraph-editor feature, without the scope decision
and its own acceptance evidence.
