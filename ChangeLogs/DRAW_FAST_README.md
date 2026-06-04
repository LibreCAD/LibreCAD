# Draw Fast — Feature Guide

**Branch:** `direct-line-master`

Draw Fast is a fast, keyboard-driven line-drawing tool optimized for architectural layouts such as walls, openings, windows, and doors. It is designed for workflows where you want to type a distance, aim with the mouse, and press Enter repeatedly without switching tools.

---

## Table of Contents

1. [Quick Reference](#quick-reference)
2. [Starting Draw Fast](#starting-draw-fast)
3. [Basic Workflow](#basic-workflow)
4. [Distance Input](#distance-input)
5. [Architectural / Fractional Input](#architectural--fractional-input)
6. [Opening Mode](#opening-mode)
7. [Window Mode](#window-mode)
8. [Door Mode](#door-mode)
9. [Preferences](#preferences)
10. [Polar Snap](#polar-snap)
11. [Soft Snap](#soft-snap)
12. [Example Workflows](#example-workflows)
13. [Notes and Limitations](#notes-and-limitations)
14. [Build and Run (macOS)](#build-and-run-macos)

---

## Quick Reference

| Command | Shortcut | Notes |
|---|---|---|
| Start Draw Fast | **df** or **dfast** | From command line |
| Draw segment | type distance → aim → **Enter** | Repeats from new endpoint |
| Opening | **o**, **o30**, **opening 30** | Width inline or prompted |
| Window | **w**, **w30**, **window 30** | Width inline or prompted |
| Door | **d**, **d30**, **door 30** | Width inline or prompted |
| Finish / cancel | **Right-click** or **Escape** | |
| Polar Snap toggle | Snap toolbar button | Global, all actions |
| Soft Snap | Preferences → Defaults → Angle Snap | Global, all actions |

---

## Starting Draw Fast

From the command line, type:

```
df
```

or:

```
dfast
```

Draw Fast can also be started from the **Draw → Lines** menu.

---

## Basic Workflow

1. Start Draw Fast (`df`).
2. Click the **start point** on the canvas. Object snaps, grid snap, and endpoint snaps are all active at this step.
3. Type a **distance** in the command line (e.g., `240`).
4. **Aim** with the mouse in the desired direction. The preview line shows in real time.
5. Press **Enter** to commit the segment.
6. Draw Fast automatically continues from the new endpoint.
7. Right-click or press **Escape** to finish.

Each successive segment begins where the previous one ended. There is no need to re-click a start point between segments.

---

## Distance Input

Draw Fast accepts plain numeric distances:

```
240
12.5
0.75
```

It also accepts standard LibreCAD math expressions, which are evaluated the same way as the coordinate bar:

```
12*25.4
100/3
```

---

## Architectural / Fractional Input

When working in feet and inches, Draw Fast accepts a convenient shorthand. The parser reads the input before falling back to standard math evaluation.

| Input | Interpreted as |
|---|---|
| `3-0` | 3′-0″ |
| `10-3` | 10′-3″ |
| `10-3 7/8` | 10′-3 7/8″ |
| `10-3-7/8` | 10′-3 7/8″ |
| `30-5/8` | 30 5/8″ |
| `237-7/8` | 237 7/8″ |
| `4 7/8` | 4 7/8″ |
| `7/8` | 7/8″ |
| `24'7.75` | 24′-7.75″ |
| `235.75` | 235.75 |

**Note:** This parser is used only by Draw Fast. It does not change how other tools interpret distances. If the architectural format is not recognized, Draw Fast falls back to standard math evaluation (`RS_Math::eval`).

---

## Opening Mode

An **opening** is a gap in a wall, represented by two short perpendicular marker lines at either side of the gap.

### Commands (while Draw Fast is active)

```
o
opening
o30
o 30
opening30
opening 30
```

### Behavior

- Requires a **previous segment** to be drawn first. Opening follows the direction of that segment.
- If no width is included in the command, Draw Fast prompts you to type one.
- After typing the width, **click on the side of the wall** where the opening should appear. The click determines which face the opening markers are drawn from.
- Draw Fast continues from the far side of the opening.

### Preference

**Opening marker depth** — controls how far the perpendicular tick marks extend from the wall line.  
Set in: **Preferences → Defaults → Draw Fast Defaults**

---

## Window Mode

A **window** is drawn as a double-line section in the wall with optional offset lines indicating the window thickness.

### Commands (while Draw Fast is active)

```
w
window
w30
w 30
window30
window 30
w30-5/8
```

Architectural widths work inline:

```
w6-3 7/8
```

### Behavior

- Requires a **previous segment** to be drawn first. Window follows the direction of that segment.
- If no width is included in the command, Draw Fast prompts you to type one.
- The window is drawn immediately after the width is confirmed; no additional click is needed.
- Draw Fast continues from the far side of the window.

### Preference

**Window offset width** — controls the inset distance of the inner window lines.  
Set in: **Preferences → Defaults → Draw Fast Defaults**

---

## Door Mode

A **door** is drawn as a door leaf with a swing arc. The leaf and arc rotate interactively as you move the mouse so you can set both the hinge side and the swing direction before clicking.

### Commands (while Draw Fast is active)

```
d
door
d30
d 30
door30
door 30
d30-1/2
```

Architectural widths work inline:

```
d2-8
d2-10 1/2
```

### Behavior

- Requires a **previous segment** to be drawn first. The door is placed along the previous wall direction.
- If no width is included in the command, Draw Fast prompts you to type one.
- Move the mouse to choose:
  - **Swing side** — which side of the wall the door opens toward.
  - **Hinge side** — which end of the opening is the hinge.
- One **click** accepts the current orientation.
- Draw Fast continues from the far side of the door opening.

### Door display options

| Setting | Effect |
|---|---|
| **Door thickness = 0** | Single-line door leaf |
| **Door thickness > 0** | Thick door slab (two parallel leaf lines + arc) |
| **Swing only** | Leaf + arc only |
| **Swing with opening line** | Leaf + arc + a straight line across the opening |

### Preferences

Set in: **Preferences → Defaults → Draw Fast Defaults**

| Setting | Description |
|---|---|
| **Door thickness** | Width of the door slab. Set to 0 for single-line doors. |
| **Door swing angle** | 90° (quarter circle) or 45° arc sweep |
| **Door design** | Swing only / Swing with opening line |

---

## Preferences

All Draw Fast settings are grouped under:

**Preferences → Defaults → Draw Fast Defaults**

| Setting | Default | Description |
|---|---|---|
| Opening marker depth | 4.00 | Length of the perpendicular tick marks on each side of a wall opening |
| Window offset width | 1.50 | Distance between wall line and inner window line |
| Door thickness | 1.50 | Thickness of the door slab (0 = single-line) |
| Door swing angle | 90° | Arc sweep angle for the door swing |
| Door design | Swing only | Whether to include a straight opening-span line |

---

## Polar Snap

Polar Snap is a **global** snap mode that constrains line preview to the nearest configured angle increment. It is not specific to Draw Fast.

### Enabling

Toggle the **Polar Snap** button in the Snap toolbar. The toolbar button is labeled with an angle icon.

### Polar snap increment

Set in: **Preferences → Defaults → Angle Snap → Polar snap increment**

The increment must divide evenly into 360. Valid examples:

| Increment | Snap angles |
|---|---|
| 15° | 0, 15, 30, 45, 60, 75, 90, … |
| 22.5° | 0, 22.5, 45, 67.5, 90, … |
| 30° | 0, 30, 60, 90, … |
| 45° | 0, 45, 90, 135, … |
| 90° | 0, 90, 180, 270 |

### Priority rules

- **Object snaps** (endpoint, midpoint, intersection, center, grid) always take precedence over Polar Snap.
- Polar Snap does **not** constrain the first point of a drawing action. It only activates once a valid base point has been established.
- When **Soft Snap** is also enabled, Polar Snap is softened (see below).

---

## Soft Snap

Soft Snap is a **global** Preferences setting that makes angle snapping gentle rather than hard-forced.

### Enabling

**Preferences → Defaults → Angle Snap → Soft Snap** (checkbox)

### Behavior

- When Soft Snap is **off**: Polar Snap and Shift angle snap force the cursor to the nearest polar angle on every move.
- When Soft Snap is **on**: the cursor snaps to a polar angle only when it is within the configured **sensitivity angle** of that angle. Outside the sensitivity window, drawing remains completely free.
- Object snaps and grid snaps still take priority over Soft Snap.
- H/V/Ortho restriction buttons are softened when Soft Snap is on — if you are outside the sensitivity window, the cursor returns to the raw position rather than the restriction-projected position.
- Holding **Shift** while Soft Snap is on still forces a hard snap to the polar angle.

### Sensitivity setting

**Preferences → Defaults → Angle Snap → Soft Snap sensitivity** — degrees

| Sensitivity | Effect |
|---|---|
| 1° | Snaps only when very close to a polar angle |
| 3° (default) | Comfortable balance — snaps when near, frees elsewhere |
| 10° | Wide capture zone, snaps frequently |

---

## Example Workflows

### Quick wall layout

```
df
(click corner)
240          → aim right → Enter   (24″ wide module)
120          → aim down  → Enter
240          → aim left  → Enter
(right-click to close manually or continue)
```

### Wall with a door opening

```
df
(click start of wall)
36           → aim right → Enter   (wall before door)
d32          → (mouse to choose swing) → click
(Draw Fast continues from far side of door)
48           → aim right → Enter   (wall after door)
```

### Wall with a window using architectural input

```
df
(click start of wall)
3-6          → aim right → Enter
w6-3 7/8     → (window draws automatically)
3-6          → aim right → Enter
```

---

## Notes and Limitations

- **Opening, Window, and Door modes require a previous segment.** If Draw Fast has not yet drawn any segment in the current session, these sub-modes will prompt for a base direction.
- **Undo** removes the most recently drawn group of entities (all lines and arcs from one action trigger are undone together).
- **Auto-continuation** — Draw Fast automatically starts the next segment from the end of the previous one. You do not need to click a new start point between segments.
- The architectural parser recognizes only specific patterns. Unusual combinations may fall through to standard math evaluation.
- Polar Snap and Soft Snap are **global** settings. When enabled, they affect all draw actions that call `getSnapAngleAwarePoint()`, not only Draw Fast.
- The macOS app bundle is built separately from CMake. See [Build and Run](#build-and-run-macos) below.

---

## Build and Run (macOS)

### Build

```bash
cmake --build build-master --target librecad -j$(sysctl -n hw.logicalcpu)
```

### Create local app bundle

A packaging script is included:

```bash
bash make_bundle.sh
```

This produces `dist/LibreCAD.app`, which can be launched directly with:

```bash
open dist/LibreCAD.app
```

### Create a DMG for sharing

```bash
bash make_bundle.sh dmg
# Produces: dist/LibreCAD.dmg
```

### Create a ZIP for sharing

```bash
bash make_bundle.sh zip
# Produces: dist/LibreCAD-macos-arm64.zip
```

The bundle script requires Qt 6 installed via Homebrew at `/opt/homebrew/Cellar/qtbase/6.11.1`.
