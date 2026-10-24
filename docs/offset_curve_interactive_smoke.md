# Offset Curve Interactive Smoke Script

Status: REPRESENTATIVE GUI CHECKS RECORDED. Actual
observations below cover identified CMake/qmake applications and interactions,
not every script step. The final versioned Save As/Open representatives are
observed; completed automated results and source/commit correspondence are
recorded separately in the roadmap and validation index. Record actual executable
identities, file bytes and observed results; do not transfer an earlier build's
observations to later bytes.

This is the manual interactive extension for roadmap 10.7. It uses real menus,
property links, picking and application Save/Open, not command aliases. No
new source-action command names or global Modify toolbar buttons are expected.

## Setup

1. Start a new drawing. Create two ellipse entities on distinct layers, one
   oblique and one equal-axis ellipse (not an entity made by the Circle tool).
   Offset them outward with Editable offset curve
   enabled. Keep a duplicate native curve that shares an embedded original.
2. Create an ordinary spline and line nearby. Put another native curve on a
   locked layer. Keep the original ellipse used for Update visible but locked:
   it is read-only input and must remain pickable for Update.
3. Enable the property sheet's Single Entity Actions and Selected Set Actions.
   Record layer, effective pen, embedded center/axes/sweep and cache reversal.
   Prepare a native with a deliberately unavailable definition using a
   supported nonuniform transform, without editing its controls directly.

## Baseline Editing

1. Confirm Distance, physical side and source geometry fields exist only for
   valid native definitions. Ordinary spline control editing remains available
   on the ordinary spline, never on a native or a generated display chord.
2. Make an accepted property edit, then Undo/Redo. Confirm one edit is one undo
   step and the definition remains editable. Reject a nonfinite/invalid value
   and cancel Edit Properties; confirm no replacement or extra undo step.
3. Preserve the committed H-sheet/H-dialog contracts: pending accepted edits
   survive selection/refill, refused/no-op edits preserve history, and modal
   cancellation leaves the original unchanged.

## Source Entry Points

1. Right-click a native display chord. Its Modify Offset Curve submenu contains
   Update from Source and Restore Original, routed to the native owner.
   It contains no Add/Append/Remove spline-control commands.
2. Repeat through each single-entity property link. An already-selected native
   stays selected. Ordinary ellipse/spline/line property views do not gain the
   native source links.
3. Select two native curves and invoke each Selected Set Actions link. Repeat
   with the ordinary spline/line also selected, including the all-types view.
   The source action targets only live top-level natives. Ordinary selections
   remain untouched and selected.
4. Confirm the new commands are not inserted into the global Modify menu,
   default Modify toolbar or default no-entity Modify context submenu. They
   use the existing offset/ellipse icons. Confirm disabled/print-preview state
   does not offer an enabled source-action proxy.

## Update from Source

1. Select multiple valid natives plus the locked/unavailable native. Invoke
   Update from Source and move repeatedly over the source ellipse. Only local
   highlighting changes; there is no preview regeneration or document/undo
   mutation while moving.
2. Pick the visible locked source ellipse in the same editable document.
   Observe busy cursor during an actual shape-changing update, with the prior
   cursor restored afterward. Confirm the picked source was not modified.
3. Confirm accepted natives use the picked ellipse's normalized value geometry
   and retain distance, physical side, reversal policy, layer and pen. Refused
   targets retain identity/geometry/selection. Check changed/unchanged/refused
   counts and specific reasons in command output.
4. Undo/Redo once: all accepted updates form one step, not one per target.
   Repeat the exact same update; it reports unchanged and creates no undo step.
5. Refuse a hidden, foreign-block or nested INSERT/dimension source. Close or
   switch the active document while awaiting a source and confirm no stale
   document mutation. Repeat inside block edit with direct block entities.
6. Move/delete the source after a successful update. The native's embedded
   definition and geometry do not track the source.

## Restore Original

1. Delete or move the external source ellipse away, then invoke Restore
   Original. The embedded normalized ellipse is inserted with the native's
   effective pen/layer; the native itself is never removed or replaced.
2. Restore two natives sharing the same original: insert only one ellipse.
   Repeat restoration: already-present count increases with zero new undo step.
   An equal live ellipse on another layer in the same container counts.
3. Confirm reversed/equivalent exact-circle forms deduplicate, but complementary
   arcs, near-circles and representably distinct large-coordinate centers do not.
   Deleted undo entities and an equal ellipse in another block do not suppress
   the insertion. Uncertain comparisons must not be called equal.
4. Include locked/unavailable native targets. Accepted insertions form one undo
   step; refusals retain selection. Verify counts/reasons. Undo/Redo affects
   only restored ellipses, never native targets.

## Persistence

1. After a shape-changing Update followed by Restore, Save As DXF R2018 to a
   fresh file, close, and reopen through the application. The native retains
   its new editable definition; the restored source is an ordinary ellipse.
   Compare source center, axes, sweep, distance, side, pen and layer.
2. Repeat independently with DWG R2018, using a separate fresh file. Record the
   actual file format/version and reopen result; do not infer DWG from DXF.
3. Repeat Save/Open after undoing Restore and after redoing Restore. The native
   stays updated in both; only the ordinary ellipse's presence changes.
4. Reopen and edit the native again. Moving/deleting the restored ellipse
   creates no dependency and does not change the native definition.
5. Record failures and unavailable observations explicitly. This script and
   authored Catch2 tests do not themselves qualify GUI or interoperability.

## Recorded Checks (2026-10-24)

The retained trace is
`/private/tmp/lc2961-impl-evidence.9GtLO2/Z-physical-observations-01.md`,
including its "Final Versioned Save As And Reopens (Z-save-filters01)" section.
These are actual CUA interactions on private drawings in the human-approved
normal profile, not a claim that the complete catalogue above was executed.

### Earlier Application Observations

The earlier sessions used these executable SHA256 identities; their observations
are not silently attributed to the subsequently rebuilt applications:

- CMake T-actions03: `864c727f640715440d67d6aa860f92924b2877baabf16a9e761d04a843cd247a`.
- qmake6 T-files01: `0f6ef6d13fd50ab84be2e2215ee02ff31a53b1059748f4a34f2dec11efab77e2`.
- CMake T-files01: `d0dcf922ec98551cd780eb33af2acc518a05ef7252e05e76551df197e1b491cd`.

| Scope | Actual observation | Limit |
| --- | --- | --- |
| Creation/settings | Editable OFF produced an ordinary spline; ON produced a native. qmake loaded saved OFF, saved ON, and a fresh process verified restored ON plus the original effective Offset options. | Not a fresh-native-key/default or isolated-profile experiment; compact-widget interaction was not observed. |
| Editing/history | Native dialog invalid radius restored its field; accepted change then Cancel did not publish. Distance acceptance had one Undo/Redo. qmake dock Distance11, same-value re-entry and invalid Major Radius0 exercised change/no-op/refusal; a valid grip release changed geometry and zero displacement added no undo. | Held-drag Escape/away-back and transient busy timing are not physical observations here. |
| Source links | qmake Update changed the embedded center and one Undo restored the prior native dialog values; Restore inserted the absent embedded ellipse and Undo removed only that insertion. CMake selected-set links reported changed/unchanged targets, preserved ordinary peers, deduplicated shared Restore, and repeated Restore added no undo. | Context-menu routing and locked/unavailable-peer physical variants were not observed; their Qt coverage is separate. |
| Trim/Trim Two | Real mixed native/line picks worked in both orders. Native/circle produced an ordinary Radius6 Arc and open degree3 spline. Native/native worked in both orders; one Undo restored both distinct native dialogs. Cancellation after the first pick preserved redo. A rounded exact-top refusal preserved state; a left-side pick succeeded. | No independently captured preview/commit comparison or physical full conic/degree matrix is claimed. |
| DXF2018 reopen | CMake conic-trim output reopened as ordinary degree3/Open spline; its second save/reopen stayed plain. Undo-before-save output reopened as Defined native, Distance3, Center(0,-64), radii12. The initial trimmed file had AC1032 header; output hashes are retained in the trace. | The later mixed-scene reopen belongs to the final applications below, not these earlier executables. |
| R12 and historical DWG | qmake R12 Save/Open selected the reopened outer carrier as Polyline; serialization did not downgrade the original in-memory native. Old generic DWG Save As wrote AC1015. | AC1015 was not DWG2018 qualification. The final positive AC1032 observations below use newly rebuilt applications and do not erase that earlier result. |

### Final Versioned Save As And Reopens

Host: macOS 27.0.1 arm64, Qt 6.12. Final executable SHA256 identities, with no
relink while open:

- CMake Release: `ef2ff0f096fa15587b9f819fab033d370111235df399df63d260ec8738917bd3`.
- qmake6: `ef1f8ce257d495f482c8041dc819f135a75da4c6f45ab7a108b40a3825721ca8`.

The final primary Save As dialogs visibly offered DWG2018; the selected filter,
fresh output bytes and subsequent Open filter were checked, not inferred from
the file extension. These sessions included the reviewed Save As slice under
qualification, now committed as `091de1a6a3e2ed684080608b1102ffe06b483ab3`.
The separate actual-SHA audit matched all three validated source blobs.

| Application / route | Actual observation | Retained scope |
| --- | --- | --- |
| CMake mixed DXF reopen | Three separate modal dialogs showed Defined native Distance3, Center(16,-64), radii12/12, outward whole source; ordinary restored Ellipse at the same center/radii; ordinary trimmed Spline Degree3/Closed0. All were cancelled. | Completes the earlier pending mixed DXF reopen; no fresh independent CAD inspection or unrecorded pen/layer comparison is implied. |
| CMake DWG2018 Save/Open | Fresh AC1032 output reopened through the DWG filter as R2018. The three dialogs retained native/ellipse/plain-spline roles and geometry. Native Distance4 acceptance followed by one Undo restored Distance3, checked in a fresh dialog. | Real post-reopen native editing and Undo, not a complete replay of earlier creation/source-action checks on the new executable. |
| CMake second DWG generation | The next Save As remembered DWG2018 in the same process. Close/Open of that output again showed the fragment as ordinary Spline Degree3/Open. | Second-generation plain fragment did not re-adopt a native definition. Filter memory was not tested across process restart. |
| CMake Trim/Undo before DWG save | Actual Tools > Modify > Trim used an ordinary vertical line as unchanged limit and the native left side as target. After one native conversion, finishing the action and one Undo restored the whole native. DWG2018 Save/Close/Open showed Defined native Distance3, Center(0,-64), not ordinary Spline. | This is Undo-before-save of native trimming, not a Save/Open test after both Undo and Redo of Restore. |
| qmake mixed DXF to DWG2018 | Actual primary Save As wrote AC1032; fresh DWG Open reported R2018. Three real dialogs showed Defined native Distance3/Center(16,-64), ordinary restored Ellipse Center(16,-64)/radii12, and ordinary trimmed Spline Degree3/Closed0. All were cancelled. | Positive interaction followed exact-path rebinding; two initial CUA binding timeouts are not credited as successful interactions. |
| qmake DWG to DXF2018 | Explicit DXF2018 Save As wrote AC1032. Close/Open through the DXF filter retained all three roles in separate native/ellipse/spline dialogs. A following cancelled Save As remembered DXF2018 in the same process. | Same-process filter memory, not restart persistence or all-version GUI coverage. |

Final output filenames below are under the retained EVID directory. Each hash
binds the actual saved file, not a projected native definition:

| Output | SHA256 |
| --- | --- |
| `Z-physical-final-cmake-mixed-2018-01.dwg` | `e2bf8e256e6f91f3acdf4fd10fedd6e9faa693ccfae4aaad887b7b0efe0fad1e` |
| `Z-physical-final-cmake-mixed-2018-second-01.dwg` | `0507d36c4764eee5756e832e41ef12f50cf043db3a4b59ac7b067c6e0be0465c` |
| `Z-physical-final-cmake-undo-native-2018-01.dwg` | `a6fe69f016d8e334d4fe48e374e3c64f00daf0468324b9f52ef5edaeeedca677` |
| `Z-physical-final-qmake-mixed-2018-01.dwg` | `21d837d8c6539c9ce10a6b7103272a52d2c291c95b47a57fae5eb07a801e56d4` |
| `Z-physical-final-qmake-mixed-2018-01.dxf` | `2f059309c5afb5a221763d95b35eb2fb4b1927ac79aa0f039264b4af54112e57` |

### Settings And Remaining Scope

Keyboard navigation was explicitly restored and observed OFF after the final
dialogs. Previously restored/verified Editable ON and the other effective Offset
options were not changed in the final sessions. Private unsaved views were
discarded No to All on Quit; scoped process inspection confirmed neither owned
GUI app remained running. Approved recent-file, path/filter and window-state
writes may remain; no exact whole-profile rollback or settings-store isolation
is claimed.

The original source fixture and mixed DXF input remained byte-identical:

- Original fixture SHA256: `bbbb2c76377761679ffc68d5bafc0397a34dcb230cb3cbf44fcd0464ab302afb`.
- `Z-physical-updated-restored-trimmed-scene-01.dxf`, AC1032, SHA256: `ae4cc141efc3e61d01e1650309bbb63265ee4624af515f9e33b4bd77dc42ebba`.

Physical context-menu routing, compact option widget operation, held-drag
Escape/away-back, locked/unavailable-peer variants, independently captured
preview/commit comparison, and Save/Open after both Undo and Redo of Restore
are not established by this trace. Fresh-native-key/default behavior and full
pen/layer/sweep comparisons are not inferred from the listed dialog fields.
Keep specific automated coverage distinct from these unobserved physical rows;
these representatives do not convert the whole manual catalogue, external-CAD
matrix or unavailable platforms into PASS.

## Automated Validation And Scope

The [validation index](offset_curve_2961_validation.md) links all R01-R15
production/test paths and the actual final results. The roadmap's
[Save As validation](plan_offset_curve_2961.md#z-save-filters-validation-2026-10-24)
records product `091de1a6a`, immediate plan checkpoint `861b58957`, the twelve
completed lanes, adjacent regressions and source/binary/blob correspondence.
The GUI observations above do not substitute for those separate results.

The final full-Release/affected-ASan XMLs contain each exact unskipped A14/B8/C54
base identity once. The 43 native mandatory SECTION detail rows per report receive
no additional identity credit. No duplicate C++ geometry graph, external matrix
replay or analyzer run was added for documentation closure.

T-files `905158174` and its modern positive/legacy read-only refusal scope remain
as qualified. Platform/static/sanitizer limitations and the physical observations
not established above stay explicit; this is not a complete manual-catalogue,
whole-CAD, all-platform, leak-free or full-ASan qualification.
