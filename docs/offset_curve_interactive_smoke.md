# Offset Curve Interactive Smoke Script

Status: UNQUALIFIED. This script has not been executed. Run after source-action
UI integration is built and its automated prerequisites pass. Record the
actual product commit, application/build identity, platform/Qt version, file
paths and observed results. A checked box is not evidence without that record.

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
