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
| Issue | [LibreCAD #3032](https://github.com/LibreCAD/LibreCAD/issues/3032), "Reincorporate paragraph indentation for MText while maintaining left alignment", opened 2026-10-08. No sample drawing, no screenshot, no comments by 2026-10-11 |
| Target | `origin/master`. `origin/2.2.1` has the same leading-space defect; back-porting A1 is decision D5 |
| Reviewed code | `628a1ba84` (`origin/master`, 2026-10-11). The first version of this plan reviewed `6242f6e43`; the one commit in between (#3055) changes `lc_dialog.cpp`, a base of the MText dialog, and adds its tests |
| State | D1 to D6 were accepted as recommended on 2026-10-11. A0, A1, A2a, A3.1, A4 and B1 are implemented by merged #3061; see "Implementation". A3.3 is implemented by follow-up #3059. A2b, A3.2 and B2 to B5 are open. Native application and external-CAD smoke checks remain outstanding |
| Cause of the report | `4b5cd9d9e` "MText: fix alignment" on master (2024-05-17, the fix for #1786 "MTEXT alignment is broken"), back-ported as `933d5378c` (2024-06-17), first in 2.2.1_rc3 and released in v2.2.1: every left-attached line is moved by its ink minimum, which cancels leading spaces |
| Second regression, master only | `117c8710b` (#2603, 2026-06-17): DXF `^I` now becomes U+0009, which is drawn as nothing. Every release drew four spaces |
| First delivery | A0, A1, A2a and A3.1: leading spaces indent again, a Tab advances again, and the saved width covers the indented line. With D2 and D6 accepted, A4 and B1 are part of it |
| Decisions | D1 to D6 accepted as recommended, 2026-10-11 |
| Other work on the issue | #3059 is narrowed to the independent embedded-attribute export fix (A3.3), on top of merged #3061 |

Track A restores whitespace indentation and is independently useful. Track B
adds paragraph formatting, which no LibreCAD release has had. Do not claim that
Track A implements a paragraph editor, and do not close the issue as a
paragraph-editor feature without D1.

## What The Issue Asks For

The reporter asks to "reincorporate" indentation and to let it work "even when
the text alignment is set to the left". The released behavior, read from the
release tags (only the master column was executed):

| Behavior | Through 2.2.0.2 | 2.2.1_rc3 to v2.2.1.5, `origin/2.2.1` | master `628a1ba84` |
| --- | --- | --- | --- |
| Leading spaces, left-attached line | Kept: a left line is not moved horizontally | Collapsed by `933d5378c` | Collapsed by `4b5cd9d9e` |
| Leading spaces, center or right | Shift the ink: the move uses the ink size, not its position | No effect: anchored on ink | No effect |
| DXF `^I` | Four spaces | Four spaces (collapsed at a line start) | U+0009 since `117c8710b`; no advance anywhere |
| U+0009 in the text | Drawn as the U+FFFD glyph | Drawn as the U+FFFD glyph | Skipped: no glyph, no advance |
| Tab key in the MText dialog | Inserts U+0009 (Qt default) | Moves focus since `e9410df82` (2023-06-10, #1540, first in 2.2.1_rc1) | Moves focus |
| Glyph advance | Pen to right ink edge: left bearing kept | Ink width only | Ink width only |
| Paragraph or indent control | None | None | None |

The first column was checked at the tags 2.2.0 and 2.2.0.2; its leading-space
row also at 2.0.0, 2.1.0, 2.1.3 and 2.2.1_rc1 and rc2.

Conclusions:

1. The only released behavior that matches the report is leading-space
   indentation of left-attached lines. It worked through 2.2.0.2 and stopped in
   v2.2.1. A1 restores it.
   The collapse came with the alignment change made for #1786, which anchors
   every attachment on the line's ink. A1 must keep that change, so it adds the
   leading advance back and leaves the ink anchoring alone.
2. No release had a paragraph-format control. "Paragraph (§)" in the dialog is
   an entry of the insert-symbol list. Track B is new work, not a restoration.
3. A Tab never produced a tab stop in any release. But the releases drew a DXF
   `^I` as four spaces, and master draws it as nothing. Shipping master without
   A2a would be a second, new regression for imported drawings.
4. Single-line TEXT is not affected: `RS_Text` keeps leading spaces (measured:
   `    ABC` puts `A` at 27.0). It skips a Tab like MText does.

## Evidence

How the facts in this plan were obtained. "Measured" means executed against the
library built from `628a1ba84`; "read" means source inspection only.

| Method | What it covered |
| --- | --- |
| Engine and filter probe | A Catch2 file outside the repository, compiled with the flags of `librecad_text_bidi_tests` and linked against the built `librecad_lib`. Bundled LFF fonts, settings redirected to a temporary directory. Layout, codec, DXF R12, R2000 and R2007 and DWG R2004 round trips |
| A1 prototype | A patched copy of `rs_mtext.cpp` linked in place of the library object; same probe cases |
| Qt probes | PyQt6 6.11.0 with Qt 6.11.2, offscreen: `QTextLayout` Tab positions, key events in `QTextEdit`, `QTextEdit::setText()`, `QSettings` INI round trip |
| Independent MTEXT layout | ezdxf 1.4.4, `MTextExplode`: positions of the exploded words for a given content and group 41 |
| Sample drawings | 824 DWG and DXF files from the local sample collections, each imported in its own process with a 120 s limit: 811 finished (13 drawings of 6 MB to 189 MB did not), 774 imported, 186 contain MTEXT |
| Read only | Editor and property-panel paths, embedded-attribute writer, libdxfrw reader and writer, bounds consumers |

The probe sources are not in the repository. A0 turns the same cases into
tests. Numbers below are LFF units at the nominal height 9 with `standard.lff`,
left attachment and left-to-right direction unless stated.

### Layout Today

| Text | Result |
| --- | --- |
| `ABC` | Ink `A` 0 to 6, `B` 9 to 14, `C` 17 to 21; used width 21 |
| One space, four spaces or a Tab before `ABC` | Identical to `ABC` |
| `A B` and `A    B` | `B` starts at 15.75 and at 36.0: one word space is 6.75 |
| `A<Tab>B` | `B` starts at 9.0: the Tab adds nothing |
| `ABC`, line break (LF or `\P`), `    DEF` | The second line starts at 0 |
| Empty text, `    `, a lone Tab | One line without children; bounds invalid; used width 0, used height 9 |
| `ABC`, two line breaks, `DEF` | The blank line keeps its line pitch; entity bounds stay finite |
| Center or right attachment | Leading and trailing spaces have no effect |
| RightToLeft direction (`unicode.lff`, Latin and Hebrew) | Leading and trailing spaces have no effect; ink minimum at 0 |

Measured cost of `RS_MText::update()`: about 13 microseconds per glyph, 105 ms
for 8000 and 212 ms for 16000 characters on one line (best of three on an idle
machine).

### A1 Prototype

The prototype adds the advance of the whitespace before the first content back
to the left-attached move, for directions other than RightToLeft. Of 38 probe
cases 8 change, all of them left-attached left-to-right lines with leading
spaces: shift 6.75 for one space, 27.0 for four, 7.5 at height 2.5, 13.5 for
two spaces in `romanc`; a second line after LF or `\P`; a left-to-right line
that holds Hebrew. The other 30 are identical: no leading space, Tab, inner and
trailing spaces, empty text, center, right, all six RightToLeft cases, vertical
attachments.

The prototype counted the blanks in logical order. A1 specifies the pen
position in visual order instead; the two agree unless explicit directional
formatting characters reorder the blanks.

Two side effects were measured with the same prototype and must be handled in
A1:

| Consumer | Before | With the prototype |
| --- | --- | --- |
| Tolerance frame, field `  0.05` then `A` (text height 2.5) | Ink 1.25 to 7.08; divider at 8.33; `A` at 9.58 | Ink 5.00 to 10.83: it crosses the divider and overlaps `A`, because the frame advances by the ink width only |
| Stack `X\S 1^ 2;Y` | Numerator at 9.0; `Y` at 13.78 | Numerator at 12.0; `Y` still at 13.78, because the stack advances by the bounding size only |

### Saving Today

| Case | Result |
| --- | --- |
| Text with leading spaces, trailing spaces, Tab and line break | Reads back identical from DXF R2000, DXF R2007 and DWG R2004. Group 1 holds the blanks verbatim, `^I` and `\P` |
| Group 41 of a standalone MTEXT | Always the ink width, whatever the entity declares: declared 100, 250 and 3 all save 21 for text whose ink is 21 wide, and declared 0 saves 14 for `A<Tab>B`. The value read back becomes the declared width |
| DXF R12 | One TEXT per non-empty line; leading spaces kept, Tab written as `^I` |
| `toNativeString()` | `^I` to Tab, `^J` to LF, `^M` dropped, `^` plus space to `^`, `\P` to LF, `\~` to a plain space. Blanks are not trimmed. `\pi2,l4;ABC` is kept verbatim |
| Paragraph codes when drawn | `\pi2,l4;ABC` and `\pxi-3,l3,t3;1.^IABC` are drawn as literal text, code included |

A font group loses its braces and its font code on import, and a paragraph code
inside it stays:

```text
{\fArial|b0|i0;\pl2;ABC}\PDEF   is stored as   \pl2;ABC <line break> DEF
```

### What An Independent Reader Does

ezdxf 1.4.4 lays out MTEXT itself. Positions are in text heights from the
insertion point:

| Content | Group 41 | Result |
| --- | --- | --- |
| Four spaces, `ABC` | 0 (no wrap) | `ABC` at 1.55: leading spaces are indentation |
| Four spaces, `ABC` | 21, the ink width LibreCAD saves today | `ABC` at 0: the indent is lost |
| Four spaces, `ABC` | 48 (indent plus ink) or 100 | `ABC` at 1.55 |
| `^I`, `ABC` | 100 | `ABC` at 4.0 |
| `A`, `^I`, `B` | 100 | `B` at 4.0 |
| `^I` anywhere | 0 | No tab stop: ezdxf creates stops only inside a positive width and otherwise uses a space |
| `\pxi-3,l3,t3;1.^IABC DEF\P2.^IGHI` | 100 | `1.` at 0, `ABC` at 3.0; `2.` at 0, `GHI` at 3.0 |

This is ezdxf's interpretation, not AutoCAD's. It confirms three things:
leading spaces are indentation for another MTEXT reader, default tab stops are
four text heights apart, and a saved width smaller than indent plus ink destroys
the indent that A1 restores.

### How Common The Constructs Are

In the 186 sample drawings that contain MTEXT (10782 MTEXT entities, 5963 of
them left-attached, 1824 with more than one line), counted after import:

| Construct | Entities | Drawings |
| --- | --- | --- |
| A Tab | 56 (52 with a Tab inside a line) | 12 |
| A `\p` paragraph code | 66 (indent 37, tab stops 43, alignment 29) | 17 |
| A line that starts with blanks | 70 | 17 |
| Any of the three | | 29 of 186 |
| Declared width 0 | 505 | |

The collections contain duplicates and synthetic test files, so read these as
orders of magnitude. In the 219 ASCII DXF samples, 97 of 98 paragraph codes (in
3 files) sit outside any `{}` group, and one file uses a property
(`\pxsm1,ql;`) that the ezdxf notes do not list.

### Qt As A Positional Oracle

- Flow-origin Tab model (A2) against `QTextLayout`, monospaced font, stops every
  8 cells: 22 of 22 cases match with a worst edge error of 0.00 px. The cases
  cover left-to-right, right-to-left Latin, Hebrew, Arabic, digits, Hebrew
  fields in a left-to-right line, and embeddings and isolates that span a Tab.
  Text that ends exactly on a stop goes to the next stop.
- Applying the next stop along increasing visual X gives wrong gaps for
  right-to-left lines: `A<Tab>BBB` gets 5 cells instead of 7.
- `RS_MText::computeBidiVisualOrder` matches Qt's visible order across Tabs in
  10 of 10 strings: Tab-separated fields keep their logical order along the
  paragraph direction.
- Key events in `QTextEdit`, `tabChangesFocus` false: a Tab event that carries
  the text `"\t"` inserts U+0009; with everything selected it replaces the whole
  text; Shift+Tab moves focus to the previous widget; Ctrl+Tab inserts nothing
  and keeps focus; a Tab event with empty text inserts nothing and moves focus.
- `QTextEdit::setText()` guesses the format: `a <b> c    d` comes back as
  `a c d`. Plain blanks, Tabs and line breaks survive.
- `QSettings` INI files return strings with leading blanks, trailing blanks,
  Tabs and line breaks unchanged.

## Decisions For The Maintainer

All six were accepted as recommended on 2026-10-11. D5 is carried out as
#3062, a pull request of its own against `2.2.1`.

| ID | Question | Facts | Recommendation |
| --- | --- | --- | --- |
| D1 | Does Track A answer #3032, with paragraph controls as a separate feature? | No release had such a control; leading spaces worked through 2.2.0.2 | Yes. Ask the reporter for a sample only if they object |
| D2 | Should Tab insert a Tab in the main MText dialog? | `tabChangesFocus` was set on purpose by `e9410df82`, a tab-order change. The property editor and the tool-options editor already insert a Tab. The dialog opens with all text selected, so a Tab typed out of habit would replace the text (undo restores it). Shift+Tab leaves the editor | Insert, for consistency with the other two editors; keep select-all; test Shift+Tab as the way out |
| D3 | Which width goes to group 41? | Today: ink width. Declared width as stored would write 100 for every new MText and a stale ink width for every text edited after a LibreCAD save. See A3.2 | Ship A3.1 now. Adopt the binding rule of A3.2 later, not "declared as stored" |
| D4 | Should RightToLeft lines get an indent from the right edge? | In a RightToLeft line the blanks on the visual left are the logically trailing ones. Indenting by them would move existing drawings | No change for RightToLeft lines in Track A |
| D5 | Back-port A1 to `origin/2.2.1`? | Same defect since 2.2.1_rc3; that branch has the older, simpler layout loop, and two QtTest targets that its CI does not build | Maintainer's call; A1 is the only step that applies there |
| D6 | Stage Track B so that `\p` codes are at least not drawn as text? | 66 entities in 17 sample drawings show codes as literal text, in every release | Yes: B1 alone is small and removes visible garbage |

## Progress And Commit Boundaries

| Step | Status | Commit | Completion evidence |
| --- | --- | --- | --- |
| A0. Isolate tests and pin the baseline | Done | `f02391aa2` | Both listeners linked into the text target; the baseline is pinned by the tests of the next rows |
| A1. Leading spaces indent again | Done | `e12c9fb6e`, `3e4cbb8a6` | Indent tests; unindented, center, right and RightToLeft geometry unchanged; tolerance and stack cases |
| A1 on `2.2.1` | Done, as #3062 | On that branch | A QtTest target of its own, with the standard font next to it; three of its four cases fail without the change. A `^I` of a file is four spaces there, which indent as well |
| A2a. A Tab advances | Done | `e12c9fb6e`, `3e4cbb8a6`, `aca5fd66f` | Stops in lines that flow from the left; four word spaces in a RightToLeft line and in single-line TEXT; never zero |
| A2b. Stops in logical RightToLeft lines | Open | - | Flow-origin placement matches the Qt oracle |
| A3.1. Saved width covers the line | Done | `e12c9fb6e`, `3e4cbb8a6`, `aca5fd66f` | Group 41 is not smaller than indent plus ink in DXF R2000, R2007, R2018 and DWG R2004 |
| A3.2. Width policy | Open; accepted for a later change | - | Binding and non-binding boxes both survive |
| A3.3. Embedded attribute text | Done in #3059, but for three defects next to it that are corrected separately (see A3.3); not checked with another CAD program | - | An edit made with a tool is saved once, in every format; a save without a change repeats the record; a format without the MTEXT gets the text as the value |
| A4. Tab key and input paths | Done | `24af2b21d`, `3e4cbb8a6`, `aca5fd66f` | Key events that carry their character; both dialogs, the property editor and the tool options; Shift+Tab and Ctrl+Tab as the ways out |
| B0. Paragraph scope | Decided by D1 and D6 | - | B1 now; B2 to B5 stay conditional |
| B1. Paragraph commands are not drawn | Done | `e12c9fb6e`, `3e4cbb8a6` | Commands hidden and not applied; other text literal |
| B2 to B5. Applying the commands, wrapping and controls | Conditional | - | See Track B |

The hashes are those of the branch of the pull request. Update this table
after each implementation commit with its hash, the exact tests run, and what
could not be checked. Keep regression tests in the commit of the fix they
validate; do not land a failing-test-only commit.

## Implementation

What the first delivery does, where it departs from the steps as they are
written below, and how it was checked. The pull request was reviewed in depth
after it was opened; "Review Of The Pull Request" lists what that changed.

- **A1.** As written. `flushBidiLine()` returns the advance of the spaces and
  tabs in front of the first ink of a line that flows from the left, and
  `updateAddLine()` adds it back to the left-attached move. A stack without ink
  moves the pen but is no indentation. The used width of a left-attached line
  counts from the insertion point, and a stack advances by the used width of
  its parts.
- **A2a.** A field is the text between two tabs. The *leftmost* ink of a field
  is on the stop, not the ink of its first letter: in `iso3098` the second
  letter of `iBC` reaches left of the first, in a field as at the start of a
  line. The stop a tab advances to is measured from the far end of the ink,
  because the pen of a font without letter spacing can be left of it. In a
  center- or right-attached line the stops count from the first ink, so that
  blanks in front of the line stay without effect. Single-line TEXT draws a
  tab as four word spaces: the `^I` regression was the same there.
- **A3.1.** Text without a paragraph command is saved with the width it uses,
  as before; that width now counts the indentation. Text with a paragraph
  command keeps the width it was given when that is larger, because the
  indentations of the command are not laid out here and another program needs
  the room for them. A width of 0, which does not wrap, stays 0 for such text.
- **A4.** `tabChangesFocus` is false in both dialogs. Shift+Tab and Ctrl+Tab
  leave a text box that types tabs: in `LC_TextEdit`, which the MText dialog
  and the property editor use, and through a key filter in the content dialog
  and in the tool options. Windows delivers Shift+Tab with a tab as its text,
  which Qt would type otherwise. The tool-options editor loads plain text.
- **B1.** `paragraphCommandEnd()` accepts `x`, `i`, `l`, `r`, `b`, `a`, `q`,
  `s` and `t` with their values, separated by commas or by nothing, up to a
  `;` within 256 characters; `x` alone or `t` without stops stays text. It
  accepts all 358 commands found in the sample drawings. An accepted command
  is not drawn. It is not applied; B2 says what that needs.

| Check | Result |
| --- | --- |
| New tests, tag `[issue3032]` | 22 cases, 7415 assertions, in `librecad_text_bidi_tests` |
| The first version of these tests on the sources before the change | 11 of 15 failed. The other 4 pin behavior that was right already |
| The tests written after the three reviews, on the first version of the change | 11 of 20 fail: each finding of the reviews has a test |
| Five mutations for the points of the further review, one at a time | Each fails in the one case written for it |
| `librecad_text_bidi_tests` | 76 cases pass |
| `librecad_tests`, the complete executable | 3359 cases pass, 70 are skipped, none fails |
| `librecad_fast_tests`, `librecad_dxf_fast_tests`, `librecad_dwg_fast_tests`, `librecad_dwg_write_fast_tests`, `librecad_text_bidi_headless_tests` | All pass |
| `librecad_text_bidi_tests` built with the address and undefined-behavior sanitizers | 76 cases pass, no report |
| Every MText of the 186 sample drawings that contain one, laid out by the sources before the change and by this one | 14888 entities, dimension labels included. 14776 are identical. All 112 that differ have a leading blank, a tab or a paragraph command |
| `RS_MText::update()`, 16000 characters, best of 3 | One line of letters 212 ms before and 227 ms after; a tab every 8 characters 358 ms and 367 ms; indented lines 227 ms and 224 ms |
| ezdxf 1.4.4 on a saved sample drawing | Reads it without audit errors. Its own layout indents the lines with leading blanks and puts the fields behind a tab on the same stops, four text heights apart |
| Rendering of that sample through the print renderer | Looked at next to the rendering by the sources before the change |

Not checked: the editors and the drawing by hand in the running application; a
qmake build (no file was added or renamed); any other CAD program; any
platform but macOS, so the Windows form of Shift+Tab is tested only as a
synthetic key event.

### Review Of The Pull Request

Three independent reviews of the first version (the layout, what consumes it
and saving, tests and editors) found the following. Each fix has a test.

| Finding | Outcome |
| --- | --- |
| The values of a paragraph command were applied as multiples of the text height. In the files AutoCAD writes they are drawing units (see B2); the title of one sample drawing moved by 3000 units | Commands are hidden and not applied |
| `tz` in a command was rejected, so 20 of 358 real commands were still drawn | Accepted |
| On Windows Shift+Tab reaches Qt with a tab as its text. The text box typed it, and the content dialog could not be left from the keyboard except by Escape | Shift+Tab and Ctrl+Tab are handled before the text is typed |
| In a centered or right-attached line the blanks in front changed which stop a tab reached | The stops count from the first ink there |
| A stack without ink counted as indentation: the one case in which text without blank, tab or command differed from before | Only spaces and tabs count |
| A negative height, which scaling by a negative factor produces, turned the indentation around | The indentation is a length |
| The saved width of text with a skipped indentation shrank | Such text keeps a larger given width |
| Single-line TEXT still drew `^I` as nothing | Four word spaces |
| Fourteen of 22 mutations of the new code passed the tests | Tests for the four that change the picture, and for near misses of a stop, far positions, turned text, the 250-byte record boundary and the tool options |
| One test case checked three editors and could leave settings behind when it failed | One case per editor; a dialog is rejected on the way out |

A further review, of those fixes, found nothing serious. It laid out 35699
random texts without leading blank, tab or command with the sources before the
change and with this one (three fonts, four directions, three attachments,
stacks, escapes, Hebrew and marks) and found the same ink and the same used
width in all of them, and its own model of the tab rules agreed with the
layout on every glyph of 9000 random lines. Its smaller points:

| Finding | Outcome |
| --- | --- |
| The tool-options editor is a plain text box. Shift+Tab as Windows sends it typed a tab there, and Ctrl+Tab did nothing | The same key filter as in the content dialog |
| Text with a paragraph command and a box of width 0, which does not wrap, was saved with the width it uses. A width that is not finite made the export fail | 0 stays 0; only a finite width is kept |
| Twelve of 34 mutations of the reworked layout passed the tests; three of them change the picture | Tests for those three: a line whose ink starts with a later letter, a pen that rounding left a hair short of a stop, a mark behind a tab |

### Limits

- **Mirroring.** `RS_MText::mirror()` turns left into right attachment, and a
  right-attached line ignores its leading blanks. A mirrored indented text
  therefore sits the indentation off its mirror image.
- **R12.** Export writes each line as TEXT with its tabs and paragraph commands
  as they are. A tab is four word spaces there, and a command is drawn.
- **Text that reads as a command.** `\pt100;`, `\pa4;` and `\ps2;` follow the
  grammar, so `C:\pt100;D:\x` is drawn as `C:D:\x`. This needs a part of a
  path that spells a property and its value behind the `p`, and a `;` right
  after it; a `\p` followed by anything else stays text.
- **Other readers.** The saved width is still LibreCAD's own measure of the
  widest line. Where the font of another reader is wider, or where the reader
  counts blanks that LibreCAD does not (in front of right-attached text) or
  applies an indentation command, it wraps the line. Of fifteen sample
  entities ezdxf wraps four as this change saves them and six as the sources
  before the change save them. A3.2 is what removes the rest.

## Review Resolutions

R1 to R7 come from the first review. The last column is the verdict of the
2026-10-11 deep review. R8 to R16 are new.

| ID | Finding | Resolution | Deep-review verdict |
| --- | --- | --- | --- |
| R1 | The main dialog consumes Tab for focus | A4 | Correct, but it was a deliberate change, so it is decision D2. Four more content paths exist |
| R2 | Increasing visual X is not a correct right-to-left Tab origin | A2b | Confirmed: 22 of 22 against Qt for the flow-origin model, wrong gaps for the visual-X rule |
| R3 | Self-round trips cannot validate reference-width semantics | A3 | Correct. "Preserve the declared width exactly" has four holes; see A3.2 |
| R4 | Embedded attributes bypass `writeMText()` and can replay stale text | A3.3 | Understated: the saved text and layout fields always win, there is no encoder, and only AC1032 writes the record |
| R5 | Generic import decoding destroys paragraph scopes | B2 | Correct. Only `{\f`, `{\H` and `{\C` groups lose their braces; `\p...;` survives as literal text. 97 of 98 sampled codes are outside any group |
| R6 | Empty bounds contract contradicts the engine | A1 | The engine already behaves: no ink, invalid bounds, line pitch kept, no NaN. One consumer misreads the sentinel; see R15 |
| R7 | Focused dialog tests can write developer settings | A0 | Imprecise: the fixture opens a shared per-user test store named `LibreCAD-tests`, not the application's preferences. The risk is interference between test processes |
| R8 | "Visually leading whitespace" indents RightToLeft lines by their trailing blanks | A1 applies only to lines that flow from the left | New |
| R9 | Stack parts and tolerance fields are left-attached MTexts | A1 defines the used width as the occupied span and both consumers advance by it | New, measured |
| R10 | The saved width under-declares an indented line | A3.1 | New, measured with ezdxf |
| R11 | `^I` regressed on master | A2a is release-blocking | New |
| R12 | Per-line ink normalization discards the origin that tab stops need | A2a: the first ink after a Tab is exactly on the stop | New |
| R13 | A glyph advance exists only as a side effect of creating the glyph | A2b needs a shared advance helper | New |
| R14 | The standalone and the embedded writer disagree on width | A3.2 | New: the embedded writer already saves the declared width |
| R15 | An MText without ink can be window-selected around the drawing origin | Separate fix; do not fold in | New, measured |
| R16 | Test helpers hide Tab behavior | A0 and A4 | New: key helpers send events with empty text; the fixture defaults to RightToLeft and `iso3098` |

## Verified Code Findings

Paths are relative to the repository root. Function names are the durable
anchors; line numbers describe `628a1ba84` only. Short names: `rs_mtext.cpp` is
`librecad/src/lib/engine/document/entities/rs_mtext.cpp`, the filter is
`librecad/src/lib/filters/rs_filterdxfrw.cpp`.

| Finding | Code anchor | Consequence |
| --- | --- | --- |
| Left attachment moves each line by its ink minimum | `rs_mtext.cpp`, `updateAddLine()` 697 to 748; the move is at 722 to 724 | Leading advances are canceled |
| A space is an advance segment; a Tab is an ordinary character that is skipped as a control | `update()` 342 to 346 and 441 to 445; `flushBidiLine()` 514 to 518 and 538 to 540 | U+0009 adds no advance |
| Emission always runs left to right in visual order; ByStyle and TopToBottom resolve as left-to-right | `flushBidiLine()` 495 to 496 | Only RightToLeft has its flow origin on the right |
| A glyph advances by its ink width plus letter spacing; the left bearing is dropped | `addLetter()` 641 to 673 | Fonts with bearings are crammed; see "Adjacent Defects" |
| The used width is the widest line's ink width; nothing else reads the member | `updateAddLine()` 728 to 730 | It excludes indentation; redefining it is contained |
| Stack parts are nested left-attached MTexts, advanced by their bounding size | `createUpperLower()` 675 to 686; `flushBidiLine()` 541 to 560 | A1 reaches them |
| Caret decoding keeps Tab as U+0009 | Filter, `toDxfString()` 32793 and `toNativeString()` 32835 | Reverting to four spaces would be lossy; the renderer must advance instead |
| Carets are encoded for DXF only | The `toDxfString()` callers pass `m_dwgW == nullptr` (read) | DWG keeps control characters raw; see "Adjacent Defects" |
| Standalone export writes the ink width | Filter, `writeMText()` 30771; the assignment is at 30885 | Long-standing (2.2.0 has the same line), not a regression |
| The embedded writer writes the declared width and the raw text | Filter, `writeInsert()` 30493; 30607 and 30608 | No `toDxfString()`: a line break fails the DXF save, a Tab goes out raw (read) |
| Saved embedded text and layout fields always replace the entity's | Filter, 30585 and 30620 to 30629 | An edited attribute is saved with its old text |
| Embedded MTEXT is written for AC1032 only | `libraries/libdxfrw/src/libdxfrw.cpp`, `writeAttributeR2018Features()` 4841 | The default format AC1021 saves the outer ATTRIB only |
| Import takes the width from group 41; a missing group gives 1.0 | Filter, `mtextEntityFromDRW()` 8051 and 8143; `libraries/libdxfrw/src/drw_entities.h` 1728 | 1.0 is the TEXT width-factor default |
| A new MText gets reference width 100 | `librecad/src/actions/drawing/draw/text/lc_action_draw_mtext.cpp` 87 | The value was never a layout input |
| Dimension labels are center-attached | `rs_dimension.cpp`, `createDimTextData()` 335; used by radial, angular, ordinate and arc dimensions | A1 does not reach them |
| The tolerance frame builds left-attached fields from untrimmed text and advances by the used width | `lc_tolerance.cpp`, `createTextLabels()` 254 to 273 | A1 reaches it |
| Empty containers have invalid bounds that read as (0, 0) | `rs_entitycontainer.cpp`, `forcedCalculateBorders()` 531 and 550 to 555 (read) | Consumers must test validity; `RS_Entity::hasValidBorders()` exists and is unused |
| The reference width is not a wrapping constraint | `RS_MText::update()`; `RS_MTextData::width` | A paragraph layout needs a wrapping design |
| The property editor's width field is read-only; the Properties dock applies width | `lc_mtextpropertieseditingwidget.ui` 82; `lc_propertiesprovider_mtext.cpp` 58 to 62 | The earlier wording "edits are discarded" was imprecise |
| The text test target lists neither shared listener | `CMakeLists.txt`, `librecad_text_bidi_tests` 2772 to 2777; the variables are defined at 2535 and 2544 | It runs on the shared test store |

Content reaches an MText through more paths than the two editors named in the
first version of this plan (read at `628a1ba84`):

| Path | Widget and behavior |
| --- | --- |
| `QG_DlgMText`: new MText, and editing from the Entity Info dock | `LC_TextEdit`; `setPlainText()` and `toPlainText()`; `tabChangesFocus` true (`qg_dlg_mtext.ui` 831); selects all text on open (`qg_dlg_mtext.cpp` 263 to 268); the only path with text-file load and save |
| `LC_MTextPropertiesEditingWidget`: Modify, Properties | `LC_TextEdit`, plain text; Qt default, so Tab already inserts U+0009 |
| Tool-options bar while drawing | Plain `QTextEdit`, loaded with `setText()` in `LC_MTextOptionsWidget::doUpdateByAction()`, which guesses the format; Tab inserts |
| Property-sheet tool option `a_text` | Single-line editor; cut at the first line break |
| Properties dock, "Content" row | `LC_MultilineTextEditDialog`: `QPlainTextEdit` with `tabChangesFocus` true (`lc_multilinetexteditdialog.ui` 60) |
| Plugins | `doc_plugin_interface.cpp`; no editing |

`LC_SingleLineTextEdit` sets `tabChangesFocus` for single-line TEXT; leave it.

## Reference Constraints

Paragraph indentation and paragraph alignment are separate controls in
[Autodesk's paragraph UI](https://help.autodesk.com/cloudhelp/2021/ENU/AutoCAD-Core/files/GUID-90F5A530-BB66-4264-86F0-D5F670EC8519.htm):
tab stops, left indent for the first line and for the hanging lines, right
indent, alignment and spacing. This is a UX reference, not proof of the
reporter's intent.

The [Autodesk MTEXT DXF reference](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-5E5DB93B-F8D3-4433-ADF7-E92E250D2BAB.htm)
defines group 41 as the reference rectangle width, group 42 as the horizontal
width of the characters (read-only, ignored if supplied), group 71 as the
attachment point and group 72 as the drawing direction with the values 1 (left
to right), 3 (top to bottom) and 5 (by style). There is no right-to-left value;
LibreCAD keeps that in its own XDATA marker. Do not fix indentation by moving
the insertion point or switching the attachment.

[Autodesk's MTEXT width documentation](https://help.autodesk.com/cloudhelp/2018/ENU/AutoCAD-Core/files/GUID-F94BE932-DA31-437E-9610-27F46ACD5711.htm)
states that with a width of 0 word wrap is off and the object is as wide as its
longest line. A positive width is a wrapping constraint. LibreCAD does not wrap.

[ezdxf's MTEXT notes](https://ezdxf.readthedocs.io/en/stable/dxfinternals/entities/mtext.html)
describe `^I` as the tabulator, `^J` as a line break like `\P`, `^M` as ignored
and default tab stops at 4, 8, 12 and so on. The unit comes from ezdxf's
renderer source (`4.0 * cap_height`), not from that page. The notes describe
`\p` properties as factors of the initial text height: `l` is the left indent,
`i` the first-line indent relative to `l`, `r` the right indent, `q` the
alignment, `t` the tab stops, `*` a reset; an `x` is required with `q` or `t`.
Its parser scopes paragraph properties by `{}` groups. All of this is one
implementation's reading, not a version-specific compatibility statement.

[Qt QTextEdit](https://doc.qt.io/qt-6/qtextedit.html) accepts Tab by default and
measures its tab distance in pixels (80 by default).
[QTextBlockFormat](https://doc.qt.io/qt-6/qtextblockformat.html) provides
margins and first-line indentation. Neither pixel measurements nor block
formats are a drawing-unit persistence model.

## Track A: Behavior Contract

"A line that flows from the left" means a line of an entity whose drawing
direction is LeftToRight, TopToBottom or ByStyle.

1. In a left-attached line that flows from the left, the whitespace before the
   first ink moves that ink away from the unchanged insertion point by the sum
   of its advances. Each line after LF or `\P` has its own indentation.
2. A line without leading whitespace keeps today's geometry exactly. The
   ink-edge normalization stays: 89 of 90 printable ASCII glyphs of `iso3098`
   have a left bearing, up to 5.31 units, so removing it would move text.
3. RightToLeft lines are not changed by A1 (D4).
4. Center, right and all vertical attachments keep their rules. Leading
   whitespace has no visible effect there, because those attachments anchor the
   ink. A Tab between two fields still separates them; the stops of such a line
   count from its first ink.
5. A Tab is never zero-width. In a line that flows from the left it advances to
   the next stop; stops are four text heights apart, measured from the line's
   flow origin. The first ink after a Tab is exactly on the stop.
6. Only U+0020 and U+0009 are advances. Inner and trailing spaces, formatting
   codes, font changes, stacks, combining marks and surrogate pairs keep their
   behavior; bidi controls stay zero-width; U+00A0 and U+3000 stay glyph
   lookups.
7. An empty or whitespace-only line keeps its line pitch and has no ink and no
   bounds. That is already true; do not invent geometry for it.
8. The used width of a left-attached line that flows from the left is its
   occupied span: from the insertion X to the far ink edge. For every other
   line it stays the ink width. Stack parts and tolerance fields advance by it.
9. Layout happens in `update()` only, never in paint, snap or bounds queries.
10. The stored text is not changed: no trimming, no expansion of Tabs, no
    synthetic bidi controls, no private indentation data, no HTML.
11. The saved reference width is never smaller than the occupied span unless it
    is a binding authored box (A3). Unindented text keeps its local geometry;
    its saved width may change under A3.2.

## A0: Tests And Baseline

Use generated entities and the `SourceFonts` fixture in
`librecad/src/lib/engine/document/entities/tests/text_bidi_regression_tests.cpp`.

1. Add `${LIBRECAD_TEST_SETTINGS_ISOLATION_SOURCE}` to the source list of
   `librecad_text_bidi_tests`, and `${LIBRECAD_TEST_TEMP_DIRECTORY_SOURCE}` if
   the new tests write files. The listener starts at `testRunStarting`, and the
   same test file already runs under it in `librecad_tests`. Until this is
   done, do not run the text target at the same time as another test
   executable: they share the `LibreCAD-tests` store.
2. Set the direction and the font explicitly. `mtextData()` defaults to
   RightToLeft and `iso3098`; the latter has the advance defect described under
   "Adjacent Defects". Use `standard` and `romans` (no bearings) for exact
   positions and `iso3098` only for the bearing cases.
3. `glyphNames()` also asserts that glyph insertion points do not decrease.
   Keep children in visual order.
4. Pin the measured baseline before changing the layout: the rows of "Layout
   Today" that A1 and A2 will change, and the rows that must not change. Assert
   displacements relative to the unindented control; do not copy a font's glyph
   box into expected constants.
5. Pin the codec rows of "Saving Today" that are correct. Record the lossy ones
   (brace scopes, `\~`) as B2 prerequisites; do not call the decoder lossless
   and do not widen Track A into a codec rewrite.
6. A dialog test that ends with `reject()` writes no `Draw/*` setting. One that
   accepts writes `TextHeight`, `TextFont`, `TextDefault`, `TextAlignment`,
   `TextLineSpacingFactor`, `TextString`, `TextAngle` and `TextLeftToRight`.
   Exercise both, and destruction before the store is cleaned up.

A hand reproduction in the running application is still open. It is a smoke
check for the release, not a prerequisite for A1: the probe drives the same
`RS_MText` code that the application draws.

## A1: Leading Whitespace Indents Again

Change `rs_mtext.cpp`, and `rs_mtext.h` only for private members.

1. In `flushBidiLine()`, for a line that flows from the left, record the pen X
   at the moment the first child is added to the line. Before that moment only
   space segments move the pen, so this value is the leading advance in visual
   order. Report it together with the line. For a RightToLeft line report 0.
2. Report it through both finalization paths: `closeLine` for LF and `\P`, and
   the last-line flush. Reset it for every line.
3. In the left branch of `updateAddLine()`, for a line with valid ink bounds,
   move by `scale * lead - min.x`. With `lead` 0 this is today's move. Leave
   the center and right branches alone. A line without ink is not moved.
4. Set the used width as contract item 8 says. Verified consumers: dimension
   labels are center-attached and do not change; the tolerance frame then
   advances by indent plus ink; the standalone exporter then writes the
   occupied span, which is A3.1.
5. Advance a stack by the used width of its parts instead of their bounding
   size, so that `X\S 1^ 2;Y` widens instead of overlapping. Both parts must be
   measured the same way.
6. Keep the space-width convention: a space advances by the style font's word
   spacing, whatever `\f` font is active. Do not fold in a rewrite of font
   spacing, glyph advances or the parser.
7. "No ink" is not the same as "no children": a stack with blank parts has a
   child and no extent, and the early returns of `update()` leave a reset box.
   Test ink with valid, ordered bounds (`hasValidBorders()` does that), not
   with `count()`. As read, `alignVertically()` and `rotateLinesRefs()` derive
   an empty line's baseline from the (0, 0) sentinel, and `cloneProxy()` and
   `drawDraft()` test `count()` only; guard them where a test shows an effect.

Acceptance: four leading spaces move the first ink by four word spaces, scaled
with the text height, and every glyph of the unindented control keeps its
position. Rotation applies after the local move. The eight changed and thirty
unchanged prototype cases become tests, plus the tolerance and stack cases.

## A2: Tab Advances

### A2a. Lines That Flow From The Left, And A Floor Everywhere Else

1. Add a `Tab` kind next to `Char`, `Space` and `Stack` in `LC_BidiSegment`.
   Recognize U+0009 in the logical pass. Feed exactly one UTF-16 unit per Tab
   into the bidi input, so that cluster indexes still map to segments. Do not
   make other control characters advance.
2. Use a named interval `T` of four text heights: 36 in the 9-unit line frame.
   Keep it separate from the editor's pixel tab distance.
3. In a line that flows from the left, a Tab moves the pen to
   `(floor(d / T) + 1) * T`, where `d` is the pen distance from the flow
   origin. Text that ends exactly on a stop goes to the next one. Use this one
   expression; do not loop over stops or add intervals repeatedly.
4. Place the leftmost ink after a Tab exactly on the stop, as the leftmost
   ink of a line already is exactly on the line start: a glyph's left bearing
   must not shift a column. Pen-exact stops would be simpler and identical for
   fonts without bearings, but two lines that start with different glyphs would
   then disagree by the difference of their bearings (up to 4 units in
   `iso3098`).
5. A leading Tab counts as leading whitespace for A1: the first ink of
   `<Tab>ABC` is at `T`, and it lines up with the field after `1.<Tab>` on
   another line.
6. In a center- or right-attached line the stops count from the first ink
   of the line, so that blanks in front of it stay without effect.
7. In a RightToLeft line a Tab advances by four word spaces until A2b lands.
   Legacy right-to-left layout (marker 1) keeps that fixed advance for good: it
   is what the releases that wrote such drawings displayed for `^I`.
8. Require finite, nonnegative `d` and finite results, and strictly positive
   progress at every Tab. Test a stop hit exactly, a near miss on either side,
   several Tabs in a row, and a large representable position.

A2a closes R11. It must land before the next release even if A2b does not.

### A2b. Logical RightToLeft Lines

1. Resolve bidi over the whole line once, as today. Keep the logical field of
   every cluster; do not resolve fields separately and do not drop embedding or
   isolate context at a Tab.
2. Measure the advance of every Tab-delimited field before placing glyphs.
   Today an advance exists only as a side effect of creating a glyph insert or
   a nested stack text. Factor one private helper that both `addLetter()` and
   the measurement use; do not keep two width formulas. Create a stack's nested
   texts once and reuse them for placement.
3. With `d` as the nonnegative distance from the flow origin and `w` as the
   field advance, a field occupies `[R - d - w, R - d]`, where `R` is the right
   end of the laid-out line. After a field add `w` to `d`; at a Tab apply the
   stop expression of A2a. Place the resolved clusters inside these intervals,
   then apply the attachment.
4. Columns of different lines line up only where the lines share `R`: with
   right attachment. With left attachment each line has its own right end. Say
   so in the test; it is the same in Qt for lines of different natural width.
5. Keep a `QTextLayout` oracle test: no wrapping, explicit direction, a
   monospaced font, glyph-run positions and string indexes rather than
   `cursorToX()`. Compare the model with the oracle's own advances; never
   compare Qt font widths with LFF ink. Cover fields of unequal width, digits,
   isolates and several Tabs. Check production placement separately with LFF
   advances. For the Han right-to-left policy feed Qt the text that
   `directionalText()` produces and map back to source positions.

Custom, center, right and decimal tab stops belong to Track B. Do not claim
that imported `\pt` settings are honored by the default stops.

## A3: Saving

### A3.1. The Saved Width Covers The Line

With A1.4 the standalone writer needs no change: it writes the used width,
which becomes the occupied span. Verify it rather than assume it:

1. Read group 41 from the saved record with a parser that does not trim. The
   existing `recordGroupValues()` helper strips leading blanks and Tabs and
   cannot check indented text.
2. Assert that group 41 is not smaller than indent plus ink for an indented
   left-attached entity, in the DXF record and in the value read back from DWG.
3. Keep the ezdxf check as optional evidence: record its version; do not
   install it automatically and do not make it a required gate.

### A3.2. Width Policy (D3)

Facts:

- The standalone writer replaces every declared width by the ink width. An
  authored box is lost on the first save, and later typing in another program
  wraps at the snug width.
- The embedded writer already saves the declared width.
- "Save the declared width as stored" breaks in four ways. A new MText has
  width 100, so another program would wrap every new line wider than 100
  drawing units. A file saved by LibreCAD holds ink widths that go stale with
  the next edit, including the indent that A1 restores. A missing group 41
  reads as 1.0. A negative width can be stored: import does not validate it and
  `scale()` multiplies it by the X factor (read).

Recommended rule, which keeps both kinds of box:

1. A box is binding when the entity is read from a file with a positive width
   smaller than the occupied span of its text. Classify once, at the first
   layout after import. A new entity is not binding. Neither is a box that
   equals the ink width of the widest line: that is the value LibreCAD itself
   has always written, and after A1 it is smaller than the occupied span of
   every indented text in an older LibreCAD file.
2. Save 0 as 0.
3. Save a binding box as declared. LibreCAD still draws that text unwrapped;
   document the limitation and keep it until B3.
4. Save any other box as the larger of the declared width and the occupied
   span. It is never narrower than what LibreCAD draws.
5. Store the classification next to `legacyRtlLayout` in `RS_MTextData`. It is
   not written to the file: the same comparison reproduces it on the next
   import.
6. Use the same rule in both writers. Save the absolute value of a negative
   width. Do not fail the whole export for a width.

The first version of this plan ruled out a provenance flag and a maximum. Both
rules protected authored boxes; the classification does that directly. If D3
rejects the flag, keep today's policy with A3.1 and leave the declared width
to B3.

### A3.3. Embedded Attribute Text

This is independent of the issue and can be a separate change.

#3059 does this on top of merged #3061 (`cb7c07584`). It leaves the merged
layout, the editors and the A3.1 width policy as they are; A3.2 remains open,
and the first draft's export of the declared width is not carried over. What
it does, where it departs from the steps as they are written below, and how it
was checked:

- **Which text is an attribute.** The tools edit by putting a changed clone in
  the place of an entity, and a copy is a clone too. A text read from an
  ATTRIB therefore names its block reference by a lineage, which a clone has
  in common with what it was made from. An edited attribute is written once,
  an undone edit or an erased text not at all, a moved block reference keeps
  its attributes, and a copy of a block reference with its texts has its own.
  A text that no block reference claims is written as a TEXT or MTEXT of its
  own when it shows something.
- **Edits (steps 1 and 2).** The text of the drawing wins wherever it differs
  from the record it was read from. To tell, the text keeps the values that
  the import gave it next to that record, in memory only and in one layout. A
  save without a change repeats the record with its inline codes, and a
  degree to radian round trip is no rotation. A content that starts with
  `\A0;`, `\A1;` or `\A2;` is written only next to the vertical alignment it
  was read with.
- **The value of the ATTRIB itself (step 3).** Only DXF and DWG R2018 hold the
  MTEXT of an attribute. In every other format the value is the text on one
  line without codes (`RS_MText::toPlainLine()`), edited or not: AutoCAD
  leaves the value empty, and it is all such a format shows. Next to the MTEXT
  a value from the file stays, an empty one also after an edit, and one that
  was there follows an edited text.
- **The place of the ATTRIB itself.** Group 11 is the insertion point of the
  MTEXT in the coordinate system of the ATTRIB, groups 72 and 74 follow the
  attachment point, and group 10 is where the baseline of the value starts.
  The first point of the file is kept only while its record is written
  unchanged. A justified single-line attribute was written with groups 10 and
  11 swapped, on master too, and moved with every save; no longer.
- **The embedded object in DXF.** It is opened by `101 / Embedded Object`, as
  AutoCAD writes it and ezdxf expects it; group 100, which LibreCAD wrote, is
  still read. Its content is cut into group 3 chunks like that of a standalone
  MTEXT, which is not what step 4 says. A standalone MTEXT skips the embedded
  object that AutoCAD appends for columns.
- **Line spacing.** Group 73 is written from the drawing as 1 or 2, for a
  standalone MTEXT too, and only 2 is read as exact.
- **What the drawing cannot rebuild (step 5).** An edited attribute is written
  without its annotation-scale payload and column layout, which the model
  cannot rebuild; the export report names what was left out. Only DWG R2018
  holds either, and only next to the untouched text it was read with. Step 5
  wanted them kept. The first version of #3059 refused the save instead, in
  every format and for autosave too.
- **Handles (step 5).** In DWG R2018 the MTEXT inside an attribute names the
  layer and the text style of its attribute by their handles in the file
  being written. A handle of the file it was read from is another object
  there, or none, and is not written.
- **A DXF export that the filter refuses** leaves no file and replaces none.

Not done by #3059:

- The fields of the MTEXT inside an attribute are read from DWG and written
  to it in an order that is not AutoCAD's. That is corrected separately.
- A DWG export onto an existing file fails on Windows. That is corrected
  separately.
- The value of a single-line attribute is read as if it were MTEXT content,
  so the `\P` of a path becomes a line break. That is corrected separately.
- A text that is not selected together with its block reference, a hidden
  attribute for one, stays behind when the reference is moved or copied.
- For a block reference with another extrusion than (0,0,1), the first point
  of the ATTRIB itself is its alignment point, and its rotation is not
  converted.

| Check | Result |
| --- | --- |
| `lc_attribute_export_tests.cpp`, tag `[attribute-export]`, in `librecad_dxf_fast_tests`, which CI builds | 50 cases. One needs AutoCAD's own sample drawing and is skipped without it |
| Its case on single edits: each property changed alone by a clone in the place of the text, then all at once, on two fixtures, saved to nine formats | The change is in the groups it belongs to and in no other; a format of 2018 opens as the drawing shows it, any other as one line with the same height, angle, style, place and justification |
| Sixteen ways to lose one property of an edit, or the edit as a whole, put into the export one at a time | Each turns that case red; a lost property in the rows of that property |
| `librecad_dxf_fast_tests`, `librecad_dwg_fast_tests`, `librecad_dwg_write_fast_tests`, `librecad_text_bidi_tests` | 664, 985 (4 skipped), 415 and 76 cases pass |
| `librecad_tests`, the complete executable | 3511 cases: 3440 pass, 71 are skipped, none fails |
| ezdxf 1.4.4 on the 433 DXF files that the cases write | All load, none with an audit error. Each of the 148 attributes written with an MTEXT has it for ezdxf too |
| 317 sample drawings, 53 of them with attributes, saved to five formats before and after | The same 127 of 265 saves succeed. Every changed ATTRIB record is one of the changes above |

Not checked: Windows and Linux; AutoCAD or another CAD program on a saved
file. The DWG output was read back by LibreCAD only: LibreDWG reads the MTEXT
inside an attribute in the same field order as LibreCAD does, so it is no
second opinion.

The steps as they were planned, before #3059:

1. Encode the embedded text with `toDxfString()`, with the same caret mode as
   the standalone writer. As read, a line break in an edited attribute fails
   the DXF save today.
2. Replay the saved raw text only when decoding it gives the entity's current
   text. Otherwise write the encoded current text. Apply the same test to the
   saved attachment, direction and spacing.
3. Define the outer ATTRIB value for an edited multi-line attribute before
   coding. The test fixture's outer `line 1` and embedded `line 1\Pline 2`
   show that it is not a copy. Formats older than AC1032 save only the outer
   record and, as read, write it without height, angle and style for an MText
   attribute.
4. Embedded text is one group 1; chunking into group 3 exists only for
   standalone MTEXT.
5. Keep tag, flags, owner links, handles, XDATA and unmodeled layout data. If
   an edited combination cannot be saved safely, report it; do not erase column
   or annotation data as a shortcut.

### A3.4. Proof

Generate samples in temporary directories. For DXF R2000, DXF R2007, DXF R2018
and DWG check the text, local glyph positions, attachment and width, including
text that crosses the 250-byte chunk boundary. A self-round trip is necessary
and not sufficient: LibreCAD ignores the width when it draws, so check the
saved field itself and, where available, an independent layout.

DXF R12 has no MTEXT; the writer emits one TEXT per line. Leading spaces
survive because TEXT keeps them. Tab layout does not, and a raw `^I` in TEXT is
not proof that it does. Assert readable output and state the limitation.

## A4: Tab Key And Input Paths

Depends on D2, which was accepted: Tab types a tab.

1. Set `tabChangesFocus` to false in `qg_dlg_mtext.ui` and in
   `lc_multilinetexteditdialog.ui`. `LC_SingleLineTextEdit` keeps it true.
2. A text box that types tabs is left with Shift+Tab, and with Ctrl+Tab in the
   other direction. Handle both keys before Qt looks at the text of the event.
   Windows delivers Shift+Tab as `Key_Backtab` with a tab as its text, which a
   text box types; X11 and macOS deliver it without. `LC_TextEdit` does this
   for the MText dialog and the property editor, the content dialog for its
   `QPlainTextEdit`. The Control key is Qt's Meta modifier on macOS. In the
   MText dialog the text box is the seventh of thirty widgets in the tab order,
   so Ctrl+Tab is what reaches the other twenty-three from it.
3. Send key events that carry the text `"\t"`, as a real key press does, and
   Shift+Tab in both forms. The helpers `keyPress()` in
   `text_bidi_regression_tests.cpp` and `key()` in `lc_dialog_tests.cpp` send
   an empty text by default; a Tab event without text types nothing and moves
   the focus, in either setting. Assert U+0009 in the document for both
   dialogs and the property editor, in left-to-right and right-to-left mode.
4. Pin the select-all case: the dialog opens with all text selected, a Tab
   replaces it, and one undo restores it.
5. Load the tool-options editor with `setPlainText()`. `setText()` guesses the
   format and drops blanks from text that looks like HTML.
6. Every path in the content table keeps leading blanks, Tabs and line breaks
   through create, apply, cancel and reopen. A new MText is reopened through
   the `Draw/TextString` setting, which keeps them. Text-file load and save
   exist only in `QG_DlgMText`.
7. Check the Chinese right-to-left `LC_TextEdit` path with whitespace and
   digits: cursor and selection indexes, native undo, composition. Do not
   change single-line TEXT input for an MText feature. The editor is not an LFF
   preview.
8. Check normal and draft drawing, print preview, PDF and image output,
   selection and zoom to extents. Indentation must not become geometry or a
   snap target. Dimension labels must not move.

## Focused Test Matrix

| Area | Cases | Assertion |
| --- | --- | --- |
| Space indentation | 0, 1 and 4 spaces; LF and `\P`; several lines; two heights | First-ink displacement equals the advance |
| Unchanged geometry | `standard`, `romans`, `iso3098`; top, middle, bottom; center, right; no whitespace; RightToLeft with leading and trailing blanks | Identical to the baseline |
| Tabs | Leading, inner, repeated; mixed with spaces; a stop hit exactly; near misses; large positions; two heights | Stop arithmetic; strictly positive progress; source Tab kept |
| Columns | A leading Tab on one line, `1.<Tab>` on the next; first glyphs with different bearings | Equal first-ink X after the Tab |
| Right-to-left Tabs | Unequal fields, digits, isolates, several Tabs; legacy marker | Qt oracle for positions; fixed advance for legacy |
| Formatting | Font switch, stack with blanks in its parts, combining mark, non-BMP character, bidi controls | No lost glyph; controls zero-width; stack widens |
| Empty input | Empty text, empty line, blank stack part, only spaces or Tabs, leading and trailing blank lines | Line pitch kept; no ink; parent bounds unchanged |
| Consumers | Tolerance field with leading blanks; dimension label; multileader text | Frame divider clear of the text; dimension unchanged |
| Transforms | Move, rotate, positive scale, mirror, clone, repeated update | Same geometry as a fresh entity |
| Saved width | Indented left-attached text; binding box; non-binding box; 0; new entity | A3.1 and, after D3, A3.2 |
| Persistence | DXF R2000, R2007, R2018; DWG; chunked text; all left attachments | Text, attachment and indentation survive |
| Embedded attributes | Unedited and edited; AC1032 and older | Unedited records unchanged; edits saved |
| Legacy export | R12 | Readable; no layout claim |
| Editing | Every content path; Tab with real event text; paste; undo; cancel and apply; file load and save | Whitespace and cursor mapping survive |
| Test isolation | Listener in the text target; accept and reject | Private store active before the fixture |
| Performance | 16000 characters on one line; repeated bounds and snap queries | Not slower than the measured baseline; no layout in queries |
| Paragraph commands | The commands of real drawings, unknown properties, malformed commands, a Windows path such as `C:\plans\a.dxf` | Commands hidden; other text literal; a larger given width is kept on saving |

Use the existing test files and a new `[issue3032]` tag with `[layout]` and
`[gui]` subsets. GUI cases must live in `text_bidi_regression_tests.cpp` to be
part of the text target. Generate fixtures in temporary directories; add no
external DWG or DXF binaries and no proprietary fonts.

## Expected Change Boundary

| Step | Files |
| --- | --- |
| A0 | `CMakeLists.txt` (one or two source-list entries), test files |
| A1, A2 | `rs_mtext.cpp`, `rs_mtext.h` (segment kind, private members), tests |
| A3.1 | Tests only |
| A3.2 | `rs_mtext.h` (one flag), `rs_mtext.cpp`, both writers in `rs_filterdxfrw.cpp`, tests |
| A3.3 | `writeInsert()` in `rs_filterdxfrw.cpp`, tests |
| A4 | `qg_dlg_mtext.ui`, `lc_multilinetexteditdialog.ui`, `lc_m_text_options_widget.cpp`, tests |

`lc_tolerance.cpp` needs no change if A1.4 is implemented as written; it needs
a test, and none covers tolerance frames today. No new public width accessor is
needed.

Not part of this delivery: a replacement of the bidi engine, a new dependency,
translation-file edits, a build-system refactor, the property editor's width
field, and every item under "Adjacent Defects".

## Incremental Build And Verification

Reuse a configured CMake build with `BUILD_TESTS=ON`. The existing targets are
enough; no new target or CI workflow is needed.

```sh
cmake --build build --target librecad_text_bidi_tests --parallel 8
QT_QPA_PLATFORM=offscreen build/librecad_text_bidi_tests "[issue3032]" --order decl
```

The cases are also tagged `[layout]` or `[gui]`.

Use the actual executable location of the platform and configuration. While
editing, run only the new layout subset. For A3 build `librecad_dxf_fast_tests`
and run its `[dxf_codec]` subset, and put the generated standalone and embedded
round trips in the existing round-trip test files. Before delivery run the
whole text target and the dimension tests of `librecad_tests`, which is not
part of the default build: build that target explicitly. Do not run the DWG
corpus or fuzz suites for a text change.

Reuse an existing qmake6 shadow build for the application compile and link
gate; regenerate its Makefile only if project or UI inputs changed. The
production and test files touched here are already listed in
`librecad/src/src.pro` and CMake.

Run the new subset with an existing ASan/UBSan build if one is available, and
do a native GUI smoke test before release. Record a check that could not be run
as pending; do not report it as passed. `git diff --check` is required; use
`git diff --no-index --check /dev/null <file>` for a file that is not in the
index yet.

## Track B: Paragraph Formatting, If Required

Inserting whitespace is not paragraph formatting, and editor margins alone are
not a persistence model. The steps are ordered by dependency; B1 is useful on
its own.

### B1. Do Not Draw Paragraph Commands As Text

Done. `RS_MText::update()` recognizes `\p...;` as a command that draws
nothing, and keeps the stored text unchanged. LibreCAD users type raw
backslashes, so the recognition is conservative: the command has to follow the
grammar from its `p` to a `;` within 256 characters. Anything else, such as
`C:\plans\a.dxf`, stays text. The grammar covers the 358 commands of the
sample drawings: `i`, `l`, `r`, `b`, `a` with a number or `*`; `q` with one of
`l`, `c`, `r`, `j`, `d`, `*`; `s` with an optional `m`, `e` or `a` and a
number; `t` with a list of numbers, each optionally led by `c`, `r` or `d`, or
`z` for no stops; `x`; commas between properties or nothing.

The command is not applied. A line is laid out as if the command were not
there, and a tab advances to the default stops.

### B2. Apply Paragraph Commands

LibreCAD does not wrap, so every line is a first line. The first-line position
`l + i` and the stops of `t` could be applied without any wrapping design, and
the common list form `\pi-12,l12,t12;1.^IText` would then draw as AutoCAD
draws it. Three things have to be settled first.

**Units.** In the files AutoCAD writes, the values are drawing units, read here
with LibreDWG:

| Drawing | Text height | Command | Values over height |
| --- | --- | --- | --- |
| `blocks_and_tables_-_metric.dwg` | 4 | `\pi-12,l12,t12;` | -3, 3, 3 |
| `blocks_and_tables_-_imperial.dwg` | 0.140625 | `\pi-0.421875,l0.421875,t0.421875;` | -3, 3, 3 |
| `truetype.dwg` | 0.3 | `\pl0.899488,t8.38542;` | 3, 28 |

The ezdxf notes call the values factors of the text height, and its own sample
(text height 2.5, `\pxi-3,l4t4;`) fits either reading. The first version of
this step followed the notes and moved the title of a sample drawing by 3000
units. Do not apply a value before the writer's convention is known for the
file at hand, or shown to be the same for every writer.

**Scaling.** Absolute values have to follow the entity. `RS_MText::scale()`
changes the height, and a block insert scales the texts of its block that way.
Either the commands in the text are rewritten with scaled numbers, in a
locale-independent form, or the entity carries a factor that is saved with it.

**What other readers do with the command.** A reader that applies an
indentation needs room for it. A3.1 keeps the given width of such text for
that reason; once the indentation is applied here, the used width covers it and
the special case can go.

Then the codec. `toNativeString()` removes the braces of `{\f`, `{\H` and
`{\C` groups and replaces escapes globally. A paragraph state that is scoped
by groups cannot be rebuilt after that: the `\pl2;` of `{\fArial;\pl2;ABC}`
then applies to whatever follows. Whether AutoCAD scopes paragraph properties
by groups is not verified; ezdxf does. In the 219 ASCII DXF samples 97 of 98
commands are outside any group.

Add an MTEXT-aware token boundary before flattening, shared by standalone and
embedded import. Keep raw source spans, brace nesting, escaped delimiters and
unknown commands. Distinguish external MTEXT syntax from LibreCAD's editor text
so that an escaped `\P` is not decoded twice. Leave the TEXT codec alone.

Parse a bounded set of properties into per-paragraph state. Keep the raw form
as the persistence authority for what is not supported and the logical text as
the editing authority for what is. Handle reset values, inheritance and
malformed or truncated commands deterministically. Never drop a whole command
because one property is unknown. Edits patch only supported properties, write
locale-independent numbers and are idempotent.

### B3. Paragraph Layout And Wrapping

Distinguish explicit paragraphs from wrapped lines. Turning wrapping on for old
files changes existing drawings; agree on the compatibility policy in B0 first.
Zero means no wrap. Do not infer wrapping from the presence of an indent code.
Implement first-line and continuation-line origins, including hanging indents,
in local coordinates before attachment and rotation.

Break lines at legal boundaries with LFF advances. Resolve bidi paragraph
levels before breaking and reorder each visual line with the correct resets; do
not resolve arbitrarily split substrings. Define what happens when a cluster is
wider than the box or the indent exceeds the width. Do not add justification,
columns or a general rich-text engine by accident.

With wrapping in place, a binding box (A3.2) is finally drawn as authored, and
the property editor's read-only width field can become editable.

### B4. Shared Controls

Expose the same left and first-line values in the MText dialog and the property
editor, with the existing unit and expression input. Apply them to the current
or the selected paragraphs and show mixed values without overwriting them. Keep
attachment and direction independent. A negative first-line value is a hanging
indent.

Map values to Qt block formats only as an editing view. Keep text-to-document
position mappings, selection, composition and one undo step per command.
Rebuild cached Chinese right-to-left layouts when a paragraph format changes,
not on every paint. Do not offer a control until its edits save and reopen
correctly in every content path.

### B5. Compatibility Gate

Test first and continuation lines, hanging and mixed indents, alignment
changes, wrapping, Unicode and formatted text. Verify DXF and DWG round trips
through an independent reader, and define the downgrade for formats that cannot
hold the layout before enabling those save paths. Automated tests must not need
a paid CAD program.

Use normal translatable strings and the project's translation workflow; do not
edit translation files by hand. Register new helper files in both build
systems, with the project's license header and an `lc_` name.

## Adjacent Defects Found During The Review

None of these belongs to this plan. They are listed so that the implementer
does not fold them in and does not write tests that trip over them.

| Defect | Evidence | Since |
| --- | --- | --- |
| MText drops the left bearing from glyph advances. In `iso3098`, `iii` is three strokes on one spot and `HELLO` has touching letters; TEXT spaces the same strings correctly | Measured | 2.2.1_rc3 |
| DWG save keeps a literal caret raw, and import always decodes carets: `m^2` reads back as `mp`, and `a^Ib` as `a`, Tab, `b` | Measured, DWG R2004 | DWG writing on master |
| The MTEXT line-spacing style is not saved: Exact reads back as AtLeast from DXF and DWG. The filter sets one field, libdxfrw writes and reads another | Measured | master; 2.2.0 and v2.2.1.5 wrote the field the filter sets |
| R12 export accumulates line offsets: lines of pitch 15 land at 0, -15, -45, -90 | Measured | 2.2.0 or earlier |
| `RS_Entity::isInWindow()` does not test validity: an MText without ink at (100, 100) is selected by a window around (0, 0) and not by one around its insertion point | Measured | Not traced |
| The tool-options editor loaded text with `setText()`; fixed with A4 | Measured in Qt; the call was read | Not traced |
| The tolerance frame logs every field at error level (`LC_ERR` in `createTextLabels()`) | Read | Not traced |
| Text that is laid out while its layer is frozen is not attached: centered `AB` has its ink at 0 to 6 instead of -7 to -1, because the borders of a letter skip invisible strokes | Measured in the review of the pull request | Not traced |
| The tool-options editor reloads its text on every change, which puts the cursor back to the start | Read; not tried in the application | Not traced |
| The Properties dock row "Line spacing style" calls `setLineSpacingFactor()` | Read | Not traced |
| An empty right- or middle-aligned TEXT can get a second point at the 1e10 scale | Read, not confirmed | Not traced |

## Review And Acceptance Gates

Before implementing, confirm that no newer upstream change to
`rs_mtext.cpp`, the text codec or the MText editors supersedes these findings.

Reject a change that does any of the following:

- removes the ink-edge normalization, moves an unindented line, or otherwise
  reopens #1786;
- indents RightToLeft lines, center lines or right lines by leading blanks;
- lets any Tab be zero-width, or makes every control character an advance;
- places right-to-left Tab fields by increasing visual X;
- lets a stack or a tolerance field overlap its neighbor;
- changes the stored text, or stores only editor formatting;
- saves a width smaller than the occupied span of a non-binding box;
- lets saved attribute text override an edit;
- lays out text in paint, snap or bounds queries;
- adds dialog tests to a target without the settings listener;
- sends Tab key events without text and concludes from them.

A1, A2a and A3.1 are ready when the left-attached whitespace cases render as
specified, unindented text keeps its geometry, the tolerance and stack cases
pass, group 41 covers the indented line, and the focused CMake and qmake6
checks pass. A2b, A3.2, A3.3 and A4 have their own rows in the progress table.
Do not mark Track B complete, or close the issue as a paragraph-editor feature,
without D1 and its own acceptance evidence.
