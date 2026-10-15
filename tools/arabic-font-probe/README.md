# Arabic font audit probe

This standalone headless probe reproduces the Amiri 1.003 experiment in the
Arabic contextual shaping plan. It checks all 21,952 ordered triples of the
specified core-letter alphabet, feature sensitivity, uncoded glyph coverage and
fixed design metrics. The 15 additional samples cover marks, ligatures, joining
controls and selected extended letters. It does not validate LibreCAD geometry,
source mapping, persistence, installed packages or the full Arabic language.

Use the bundled `librecad/support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf`.
Before running, verify its SHA-256 is
`cd2550c0f4c05eb341bf97958211aaa39382bca96577ba3a67d4a3b4912c43c0`.
The probe checks face metrics but does not implement a checksum algorithm.

The original temporary audit understated uncoded coverage: sample printing used
`std::map::operator[]`, inserting 25 uncoded GIDs into its cmap lookup before
counting triples. Eighteen occur in the triple corpus, misclassifying 125
triples. The immutable lookup correctly reports 20,944 triples containing an
uncoded glyph and 387 distinct uncoded output GIDs. An independent SFNT cmap
reader plus `hb-shape` reproduces both the corrected and historical counts.
This correction does not change font bytes, output placements or feature counts.

From the repository root, with public HarfBuzz and FreeType development packages:

```sh
c++ -std=c++17 -O2 -Wall -Wextra tools/arabic-font-probe/main.cpp $(pkg-config --cflags --libs harfbuzz freetype2) -o /tmp/librecad-arabic-font-probe
shasum -a 256 librecad/support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf
/tmp/librecad-arabic-font-probe librecad/support/fonts/opentype/amiri-1.003/Amiri-Regular.ttf
```

On a macOS HarfBuzz build exposing its CoreText shaper, run the same command
with `--coretext` appended. This compares glyph IDs, absolute origins and total
advance against a separate shaping engine with a maximum one-font-unit error.
An unavailable requested shaper is a failure, not a skipped pass. Omitting the
option prints `CoreText=NOT_RUN`. Outlines are not compared by this probe.

Reference versions are HarfBuzz 14.6.0 and FreeType 2.14.3. The executable prints
actual versions and returns nonzero for load/shape errors, changed audit counts
or incompatible metric/placement results. Engine changes require investigation
and explicit profile compatibility decisions, not automatic fixture replacement.

This experiment deliberately uses `MONOTONE_GRAPHEMES`, matching the original
audit. The production shaper uses `MONOTONE_CHARACTERS` for UTF-16 provenance
and needs its own source-map assertions. All shaping uses RTL, `Arab`, `ar`,
OpenType default features and the font's UPEM design-unit scale. Unicode source
is never changed into presentation forms.
