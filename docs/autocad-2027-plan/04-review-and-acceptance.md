<!--
Copyright (C) 2026 LibreCAD contributors
This planning document may be redistributed and/or modified under the GNU
General Public License, version 2 or (at your option) any later version.
See the repository LICENSE file. No warranty is provided.
-->

# Independent research review and acceptance plan

Reviewed October 11, 2026 against `628a1ba845931a37b8d745a8ad08d4731615812c`. Three Luna agents drafted the UI, formats/layouts/PDF, and Mac delivery studies. The parent independently inspected the core source claims and official reference pages and consolidated the [master plan](00-master-plan.md). This is a planning review, not a build or runtime certification.

## Source findings that govern implementation

| Finding | Evidence inspected independently | Consequence |
| --- | --- | --- |
| Properties already exists and edits selected entities | `librecad/src/ui/main/init/lc_widgetfactory.cpp`, 262–274; `librecad/src/ui/dock_widgets/property_sheet/lc_propertysheetwidget.cpp`, 417–527, 546–593 | Reuse and refine the actual Property Sheet; do not build a competing inspector |
| Mixed-value aggregation already exists | `librecad/src/lib/properties/lc_property_multi.cpp`, 72–123 | Improve presentation/coverage and verify transactions; do not treat multi-selection as wholly new |
| Current property edits use a delayed undoable batch | `lc_propertysheetwidget.cpp`, 509–526 | Test rapid edits, command cancellation, document close/switch and pending callbacks before changing editor commit behavior |
| Native menus/actions and shortcuts have existing machinery | `lc_menufactory_main.cpp`, 57–93; `lc_actionfactory.cpp`, 172–177, 897 onward; `rs_commands.cpp`, 43–151 | Share actions and extend current shortcuts/aliases rather than registering parallel handlers |
| AutoCAD-like command names are unevenly registered | `librecad/src/cmd/lc_commandItems.h`, 104–120, 743–848, 914–933 | LINE/TRIM/OFFSET names already exist; MOVE/PLINE/LAYER/PROPERTIES need specific inventory and workflow adaptation |
| Layer visibility and freeze need a semantic decision | `librecad/src/lib/engine/document/layers/rs_layer.h`, 44–58 | The main layer data has frozen/locked/print/construction; `visibleInLayerList` describes list visibility. Do not present independent AutoCAD Off/Freeze behavior without extending and testing the model/adapter/render contract |
| Layout tabs intentionally do not switch rendering | `librecad/src/ui/main/lc_layouttabbar.h`, 37–53; `qc_mdiwindow.cpp`, 175–194; `lc_layouttabbar.cpp`, 165–175 | Existing tabs are a metadata/UI foundation; paper-space scenes and interaction remain required |
| Active layout setter changes metadata and dirty state | `librecad/src/lib/engine/document/rs_graphic.cpp`, 798–839 | Decide view-state versus drawing-state semantics and add undoable document APIs before exposing layout editors |
| Special paper-space block ownership is a concrete import gap | `librecad/src/lib/filters/rs_filterdxfrw.cpp`, 4724–4775, 5008–5019 | Route paper entities into layout-owned containers; imported layout names alone do not demonstrate content retention |
| XREF embedding is present | `rs_filterdxfrw.cpp`, 4747–4755, 4778–4871 | Audit binding/snapshot behavior and preservation; a live-reference manager is a distinct feature |
| PDF tiling is not multi-layout publishing | `librecad/src/main/console_dxf2pdf/pdf_print_loop.cpp`, 267–335; `librecad/src/lib/printing/lc_printing.cpp`, 200–243 | Create a shared per-sheet composition path; keep model tiling separately available |
| Existing layout tests are excluded from the main CMake test list | `CMakeLists.txt`, 2665–2669 | Port/re-enable relevant coverage; do not cite test-file presence as passing coverage |
| DWG inventory is qualified static evidence | `libraries/libdxfrw/DWG_VERSION_SUPPORT_STATUS.md`, 5–7, 26–46; `docs/gap-report.md`, 1–7, 16–38 | Same-reader reread or parser recognition cannot establish full compatibility |
| Mac CI already describes a universal product | `.github/workflows/build-all.yml`, 181–237; `pixi.toml`, 1–34; `pixi.lock` osx-arm64 Qt entries | Preserve universal baseline; add a reproducible native arm64 profile and pin one tested dependency/toolchain source |
| Package architecture verification checks required slices | `scripts/verify-osx-package.sh`, 611–643 | Universal libraries with arm64 are valid; lack of arm64 in any needed component fails the native release |

These are scope/architecture findings rather than assertions that a specific user's file has failed. No drawing was opened or exported in a running app during this research.

## Corrections made during review

- Replaced any suggestion that Properties or layout metadata is wholly absent with the actual source-backed starting point.
- Qualified layout tabs as metadata selection, not complete paper-space editing/rendering.
- Qualified combined CLI PDFs: each input can contribute multiple model-tiled pages; this differs from choosing layouts within a drawing.
- Recognized existing XREF embedding and shortcut registration rather than describing them as missing infrastructure.
- Allowed universal third-party dependencies containing arm64; an exact thin-binary policy is optional size/labeling work.
- Kept the user's Windows-style UI reference as the design target; Mac product documentation supplements native platform behavior.
- Used the AutoCAD 2018/AC1032 DWG family for AutoCAD 2027 files; newer application features still require individual compatibility evidence.
- Fixed the proposed embedded-control API to `QWidgetAction::setDefaultWidget`; menus, toolbars and ribbon buttons must share QAction state.
- Separated current licensing notices and SDK questions from a claim that a new SDK is compatible or supported on Mac arm64.
- Verified ODA's published macOS arm64 support directly in the Drawings SDK product table. Availability of a suitable current package and integration/license fit still require validation; native platform support itself is advertised.

## Evidence boundaries and unresolved work

What this research establishes: current code locations and architecture, a reasoned implementation order, official product/platform reference behavior, and tests needed to advertise features honestly.

What it does not establish:

- A fresh compile/link result for this checkout, or a working `.app`/DMG.
- Native arm64 GUI behavior, keyboard/trackpad behavior, Retina appearance, accessibility, performance or clean-machine install.
- Pixel similarity against a live AutoCAD 2027 installation; wireframes are proposals.
- Actual DWG/DXF retention/editability/round-trip for the user's drawings, especially custom entities, dynamic/associative content and references.
- Model/paper-space output fidelity, text/font matching or PDF sheet accuracy.
- ODA's exact obtainable SDK package, toolchain/ABI fit and distribution/license fit. The official Drawings SDK platform table does advertise macOS arm64 static and dynamic libraries; that is platform evidence, not a successful integration.
- Whether GPLv3+ LibreDWG could be incorporated into this particular mixed-notice source tree under the desired distribution model, and whether its modern-DWG writer limitations meet the required subset.

Access to an AutoCAD reference installation and representative files is needed for the implementation comparisons. These are future evidence requirements, not a reason to leave the requested planning work unfinished.

## Proposed release acceptance scenarios

All scenarios are future tests. Record application commit, artifact hash, Qt/Xcode versions, chip, OS and fixture provenance with the results. Do not mark a gate passing merely because a corresponding test source exists.

| ID | Scenario | Expected result |
| --- | --- | --- |
| UI-01 | Launch with a fresh settings profile on a MacBook-size display | Top command groups, Properties/Layers access and bottom command input are visible; canvas retains usable space; no hidden off-screen dialogs |
| UI-02 | Resize, collapse ribbon, float a dock, switch monitors, relaunch, then reset workspace | State restores within current screens; reset returns CAD Familiar without deleting user drawings or alias preferences |
| UI-03 | Invoke the same operation from menu, ribbon and command line | Each reaches the same underlying operation; enabled/checked state and document context agree |
| PR-01 | Select one line, multiple lines with differing colors, then a mixed line/circle selection | Count/type are explicit; mixed values show Varies; only applicable geometry fields are editable |
| PR-02 | Change a shared property on 100 eligible selected entities | One accepted edit applies consistently, one undo restores all originals, redo repeats; active defaults are not unexpectedly changed |
| PR-03 | Enter invalid numeric data; cancel; edit rapidly; switch or close the document during a pending edit | Invalid/canceled edits leave geometry unchanged; no stale document pointers, wrong-document updates or partial batches |
| LY-01 | Create/rename/set current/change pen/lock/plot a layer; save and reopen | State persists, duplicate/invalid names are rejected, entities retain correct identity and locked edits are blocked |
| LY-02 | Implement distinct Off/Freeze; change each, isolate/unisolate, then undo | Display, selection and file serialization follow the documented distinction; restoration preserves prior states |
| CMD-01 | LINE, PLINE, MOVE, COPY, OFFSET, TRIM, EXTEND using the supported options | Documented prompts/options work, exact geometry results match expectations, invalid keywords do not mutate drawing |
| CMD-02 | Use relative/polar coordinates, temporary snaps and a rotated UCS; press Escape and repeat | Correct WCS geometry, predictable cancellation/repetition and undo; Mac shortcuts do not steal command input |
| DW-01 | Open AC1032 DWG created by AutoCAD 2027 and equivalent DXF | Supported content count/ownership/units/styles agree; missing fonts/references and unsupported families appear in a diagnostic report |
| DW-02 | Save As, reopen in LibreCAD and an independent target reader, compare to source | Declared subset persists with correct handles/owners and visual output; losses are reported before final output |
| DW-03 | Introduce malformed/truncated input, failed writes and unsupported raw data invalidated by an edit | Clear failure/limitations, temporary cleanup, source unchanged; no falsely successful export |
| LA-01 | Create two layouts with distinct borders/notes and shared model geometry | Tabs switch to distinct paper scenes; model remains shared; layout rename/order/copy/delete and undo survive save/reopen |
| VP-01 | Add two rectangular viewports on one sheet at different scales; lock one and freeze a layer in the other | Views scale/clip independently; no model copies; lock protects view scale/pan; viewport layer state does not alter global layer state |
| VP-02 | Activate model editing through a viewport, then return to paper | Selection/snapping/input use the correct space and transform; properties indicate the active context |
| PDF-01 | Publish two selected layouts with different sizes/orientations in specified order | Exactly two pages, correct order/media boxes, no unrelated model tiles, distinct sheet content |
| PDF-02 | Compare text, dimensions, hatch, lineweights, colors and dashed lines at two viewport scales | Reference appearance and defined numeric/text tolerances pass; font substitution and unsupported plot styles are reported |
| MAC-01 | Install the packaged app on a clean Apple Silicon account/machine | App/Qt/plugins/helpers run arm64, resolve bundle dependencies, open supported formats and print/export |
| MAC-02 | Keyboard-only and VoiceOver navigation; trackpad and wheel mouse; mixed display scaling | Controls have usable focus/names; no double scaling or inaccurate hit tests; pan/zoom remain predictable |

## Quantitative sheet checks

Set final tolerances after measuring the baseline and chosen PDF engine; record them per fixture rather than using subjective “looks correct.” Initial candidate checks:

- PDF page width/height and printable margins against saved page setup, using a proposed 0.1 mm geometry tolerance for vector-only synthetic fixtures.
- A 1000 mm model line at 1:100 occupies 10 mm on the sheet. A 40 ft line at 1 inch = 40 ft occupies 25.4 mm. Check vector coordinates independently of display DPI.
- Sheet conversion: `paper_mm = model_length_in_drawing_units × mm_per_drawing_unit × physical_scale_ratio`. Unitless drawings require an explicit unit assumption; never infer feet/mm from their magnitude.
- Count PDF pages and compare their order and media boxes before image comparison. Compare Unicode text content independently of glyph placement and visual shape; outlining text changes extraction expectations and must be a deliberate option.
- Record all reference assets/fonts and their substitutions. Compare rasterized edges with an anti-aliasing tolerance established on the same renderer/DPI; do not confuse small font-rendering differences with a geometry error.

For large-coordinate engineering drawings, include nonzero UCS/OCS and high-magnitude WCS coordinates while keeping double precision. Verify viewport transforms using independent point/length calculations before judging canvas screenshots.

## Performance protocol

Build small, medium and large redistributable/generated drawings, including repeated/nested blocks, hatches, text, many layers and multi-layout sheets. Record entity counts and complexity rather than calling a fixture “large.” Measure open time, memory, pan/zoom responsiveness, select/property refresh, layer operations, layout switching and PDF publish time on a named Mac.

Use baseline-relative regression thresholds agreed in Phase 0. New ribbon/dock composition should not materially slow geometry operations. Viewport composition needs caching and dirty-region planning based on actual profiles; a 3D/Metal rewrite is not implied by AutoCAD Mac 2027's graphics architecture. Keep canceled background work and document switching in the performance tests so responsiveness does not conceal incorrect state.

## Release evidence package

Before advertising the full requested release, retain:

1. Reproducible build profile and artifact SHA-256, package/signature/dependency reports and native Mac launch evidence.
2. Version/entity capability matrix with tested recognized/retained/rendered/editable/round-trip levels and known-loss policies.
3. Fixture provenance and reference-source versions; independent DWG/DXF open/audit results.
4. UI task recordings/screenshots and actual versus expected results for property/layer/command workflows.
5. Layout/viewport ownership inventory and save/reopen checks; sheet/PDF page and vector/text/visual comparison results.
6. Native Mac device/display/accessibility/performance report and an explicit support matrix.

Source/fixture tests, headless rendering, package integrity and interactive GUI validation are separate evidence categories. Each required category needs its own result.

## Documentation verification for this research deliverable

The parent inspected the added documents, reconciled the confirmed user decisions, checked core source anchors, compared high-risk claims with implementation, and reviewed cited primary documentation. Final consolidation checks found five Markdown documents, 29 valid local links, balanced code fences and no trailing whitespace. Git status showed only the new planning directory; tracked source files and the staging area were unchanged. No application build or runtime tests were run. Implementation, Git commits and publishing are outside this research deliverable.

Relevant official reference pages checked directly by the parent:

- [AutoCAD 2027 ribbon](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-D20EF1D7-4135-48A7-B68E-65BF3BFF3D70.htm)
- [AutoCAD 2027 layout viewports](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-2B5D404A-DCAB-4AF6-A5C1-51593B38F519.htm)
- [AutoCAD 2027 named page setups](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-2C5C9DE4-0740-4D7F-A781-745A04F588B0.htm)
- [AutoCAD 2027 multi-layout publishing](https://help.autodesk.com/cloudhelp/2027/ENU/AutoCAD-Core/files/GUID-556BF24C-3996-4F6B-87AC-2BAC0FD9EE21.htm)
- [Autodesk developer note on 2027 interoperability](https://blog.autodesk.io/autocad-2027-interoperability-for-customization/)
- [Qt 6.8 macOS architecture and deployment targets](https://doc.qt.io/qt-6.8/macos.html)
- [Qt native menus](https://doc.qt.io/qt-6/qmenubar.html#nativeMenuBar-prop)
- [Qt main-window/dock framework](https://doc.qt.io/qt-6/qmainwindow.html)
- [ODA Drawings SDK, including macOS arm64 platform table](https://www.opendesign.com/products/drawings)
- [Autodesk RealDWG 2027 system requirements](https://aps.autodesk.com/developer/overview/realdwg-api)
- [LibreDWG upstream, including writer limitations](https://github.com/LibreDWG/libredwg)

The specialist documents include additional primary sources. Their recommendations remain design judgments; official product documentation does not itself validate this fork.
