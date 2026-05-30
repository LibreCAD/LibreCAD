# Draw Fast — Pull Request Description

> Copy this file into the GitHub Pull Request description field.

---

## Summary

This PR adds **Draw Fast**, a keyboard-driven multi-segment line tool aimed at fast architectural layout drafting, along with two global snap enhancements (**Polar Snap** toolbar toggle and **Soft Snap**) that improve angle-constrained drawing across all actions.

---

## Motivation

LibreCAD's existing line tools require separate start/end clicks for every segment and do not support typed distances with mouse-directed angles. For wall-based floor-plan work, this creates repetitive clicking.

Draw Fast addresses this by letting users:
- Type a distance once, aim with the mouse, and press Enter.
- Chain segments automatically without re-clicking a start point.
- Insert architectural elements (openings, windows, doors) mid-session with short sub-commands.
- Use feet-inch-fraction input without switching units.

---

## Feature List

### Draw Fast (`df` / `dfast`)

- New drawing action: **Draw → Lines → Draw Fast**
- Command aliases: `df`, `dfast`
- Typed distance + mouse-aimed direction → Enter to draw segment
- Auto-continues from the new endpoint
- Right-click or Escape to finish
- Full object snap support on the first click

### Architectural / Fractional Distance Parser

- New parser class `LC_ArchParser`
- Recognizes: `10-3`, `10-3 7/8`, `10-3-7/8`, `30-5/8`, `4 7/8`, `7/8`, `24'7.75`, `3-0`
- Falls back to `RS_Math::eval` when the pattern is not recognized
- Used only by Draw Fast; does not affect other tools

### Opening Sub-mode (`o` / `opening`)

- Places perpendicular wall-opening marker ticks
- Width can be typed inline (`o30`) or prompted
- Click determines which wall face the markers appear on
- Continues from the far side of the opening
- Configurable marker depth in Preferences

### Window Sub-mode (`w` / `window`)

- Draws a double-line window section in the wall
- Width can be typed inline (`w30`, `w6-3 7/8`) or prompted
- Draws immediately after width is confirmed
- Configurable offset width in Preferences

### Door Sub-mode (`d` / `door`)

- Draws a door leaf and swing arc
- Width can be typed inline (`d30`, `d2-8`) or prompted
- Mouse controls hinge side and swing direction; one click confirms
- Thick-door mode (configurable door slab thickness)
- Door design: swing-only or swing + opening line
- Door swing angle: 90° or 45°
- All door settings configurable in Preferences

### Polar Snap (global)

- New **Polar Snap** toggle button in the Snap toolbar
- Constrains line preview to the nearest configured polar angle increment
- Configurable increment in Preferences (must divide evenly into 360)
- Object snaps have higher priority than Polar Snap
- Does not constrain the first point of a drawing action (basepoint guard)

### Soft Snap (global)

- New global Preferences option: **Soft Snap** (checkbox + sensitivity angle)
- When enabled, replaces hard angle-forced snapping with sensitivity-window snapping
- Cursor snaps to polar angle only when within configured degrees of that angle
- Outside the window: cursor is completely free (H/V/Ortho projections are discarded)
- Object and grid snaps still take highest priority
- Shift still forces hard snap even when Soft Snap is on
- Implemented via `RS_Snapper::isLastSnapFree()` to distinguish true object snaps from restriction-only projection

---

## New Preferences

**Preferences → Defaults → Draw Fast Defaults**

| Setting | Default | Description |
|---|---|---|
| Opening marker depth | 4.00 | Perpendicular tick mark length for wall openings |
| Window offset width | 1.50 | Inset distance of inner window line |
| Door thickness | 1.50 | Door slab thickness (0 = single-line) |
| Door swing angle | 90° | Arc sweep (90° or 45°) |
| Door design | Swing only | Swing only / Swing with opening line |

**Preferences → Defaults → Angle Snap**

| Setting | Description |
|---|---|
| Polar snap increment | Degrees; must divide evenly into 360 |
| Soft Snap | Enable/disable gentle angle snapping |
| Soft Snap sensitivity | Degrees within which the cursor snaps to the polar angle |

---

## User Workflow Examples

### Basic wall segments

```
df → click corner → 240 Enter → aim right Enter
                  → 120 Enter → aim down  Enter
```

### Door in a wall

```
df → click → 36 Enter → d32 → (move mouse to choose swing) → click
→ 48 Enter → aim right Enter
```

### Window with architectural width

```
df → click → 3-6 Enter → w6-3 7/8 → (auto-draws) → 3-6 Enter
```

---

## Files Changed

### New files

| File | Purpose |
|---|---|
| `librecad/src/actions/drawing/draw/line/lc_actiondrawlinedirect.h` | Draw Fast action header |
| `librecad/src/actions/drawing/draw/line/lc_actiondrawlinedirect.cpp` | Draw Fast action implementation |
| `librecad/src/lib/math/lc_archparser.h` | Architectural distance parser header |
| `librecad/src/lib/math/lc_archparser.cpp` | Architectural distance parser implementation |
| `ChangeLogs/DRAW_FAST_README.md` | User-facing feature guide |
| `ChangeLogs/DRAW_FAST_PR_DESCRIPTION.md` | This file |
| `make_bundle.sh` | macOS app bundle packaging script |

### Modified files

| File | Change |
|---|---|
| `librecad/src/lib/engine/rs.h` | Added `ActionDrawLineDirect` to `RS2::ActionType` enum |
| `librecad/src/ui/lc_actionhandlerfactory.cpp` | Registered Draw Fast action instantiation |
| `librecad/src/ui/main/init/lc_actionfactory.cpp` | Added Draw Fast `QAction` and Polar Snap toolbar action |
| `librecad/src/cmd/lc_commandItems.h` | Added `df` / `dfast` command aliases |
| `librecad/src/ui/components/toolbars/qg_snaptoolbar.h` | Added `m_actionSnapAngle` member |
| `librecad/src/ui/components/toolbars/qg_snaptoolbar.cpp` | Wired Polar Snap toolbar toggle |
| `librecad/src/lib/actions/rs_actioninterface.h` | Added `m_snapToAngleStep`, `m_softSnapEnabled`, `m_softSnapSensitivityRad` |
| `librecad/src/lib/actions/rs_actioninterface.cpp` | Extended `updateSnapAngleStep()` for Polar Snap and Soft Snap settings |
| `librecad/src/lib/actions/rs_previewactioninterface.cpp` | Restructured `getSnapAngleAwarePoint()` for Soft Snap and Polar Snap |
| `librecad/src/lib/actions/rs_snapper.h` | Added `isLastSnapFree()` declaration |
| `librecad/src/lib/actions/rs_snapper.cpp` | Implemented `isLastSnapFree()` |
| `librecad/src/ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.ui` | Added Draw Fast, Polar Snap, and Soft Snap Preferences controls |
| `librecad/src/ui/dialogs/settings/options_general/qg_dlgoptionsgeneral.cpp` | Read/write for all new Preferences settings |
| `CMakeLists.txt` | Added new source files |

---

## Testing Checklist

### Draw Fast basic

- [ ] `df` starts the action
- [ ] `dfast` starts the action
- [ ] First click sets start point; object snaps work
- [ ] Type distance, aim, Enter — segment is drawn
- [ ] Continues automatically from new endpoint
- [ ] Right-click finishes the action
- [ ] Escape finishes the action
- [ ] Undo removes the last segment

### Architectural input

- [ ] `10-3` → 10′ 3″
- [ ] `10-3 7/8` → 10′ 3 7/8″
- [ ] `7/8` → 7/8″
- [ ] `3-0` → 3′ 0″
- [ ] Non-architectural input (e.g. `100+50`) falls back to math eval

### Opening mode

- [ ] `o` while Draw Fast is active prompts for width
- [ ] `o30` directly enters opening of width 30
- [ ] Opening markers appear perpendicular to previous wall direction
- [ ] Click side of wall correctly places markers
- [ ] Continues from far side

### Window mode

- [ ] `w` prompts for width
- [ ] `w30` draws window of width 30 immediately
- [ ] Window offset width is respected from Preferences
- [ ] Continues from far side

### Door mode

- [ ] `d` prompts for width
- [ ] `d30` enters door mode with width 30
- [ ] Mouse movement rotates leaf/hinge/swing interactively
- [ ] Click accepts orientation
- [ ] Thick door (thickness > 0) draws two leaf lines
- [ ] 45° swing draws correct arc sweep
- [ ] Swing with opening line draws the spanning line
- [ ] Continues from far side

### Polar Snap

- [ ] Polar Snap toolbar button toggles on/off
- [ ] With Polar Snap on, line preview snaps to configured increments
- [ ] Object snaps override Polar Snap
- [ ] First point of draw action is not constrained by Polar Snap
- [ ] Changing polar increment in Preferences takes effect on next action

### Soft Snap

- [ ] Soft Snap checkbox in Preferences enables/disables the feature
- [ ] With Soft Snap on, cursor is free when far from a polar angle
- [ ] With Soft Snap on, cursor snaps gently when near a polar angle
- [ ] Object snaps still win over Soft Snap
- [ ] Shift forces hard snap even when Soft Snap is on
- [ ] Changing sensitivity angle in Preferences takes effect on next action

---

## Notes for Reviewers

### Architecture

- Draw Fast (`LC_ActionDrawLineDirect`) follows the same pattern as `LC_ActionDrawLineSnake` and other actions in `lc_abstractactiondrawline.cpp`.
- Sub-commands (o/w/d) are intercepted in `doProcessCommand()` before normal command dispatch so they work while the action is already running.
- Multi-entity triggers (openings, windows, doors) stage all entities as `QList<RS_LineData>` + `QList<RS_ArcData>` so the entire element is one undo step.
- Door geometry uses an intersection calculation between a ray (the inside door edge) and the swing arc circle to ensure the thick-door slab is geometrically correct.

### Polar Snap and Soft Snap design

- `getSnapAngleAwarePoint()` in `RS_PreviewActionInterface` is the single control point for all angle-snap logic.
- `isLastSnapFree()` in `RS_Snapper` exposes only a `bool` — the internal `SnapType` enum stays file-local and is not added to any header.
- When Soft Snap is OFF, the hard Polar Snap and Shift paths are preserved exactly as they were before this PR.
- The `!basepoint.valid` guard prevents angle snap from using `(0, 0)` as an origin before the first point is clicked.

### Architectural parser

- `LC_ArchParser` is intentionally isolated. It does not touch `RS_Math::eval` or any global parsing path.
- The fallback to `RS_Math::eval` happens in Draw Fast's own distance-parsing helper only.
