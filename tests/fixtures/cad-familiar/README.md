# CAD Familiar baseline fixtures

Project-authored drawings, GPL-2.0-or-later. No customer or Autodesk drawing content is included. Generate with `scripts/make-cad-baseline-fixtures.py` using **ezdxf 1.4.3**. Fixed metadata makes regeneration deterministic. The manifest records exact hashes, entity counts, paper sizes, viewport scales, and independent audit results.

| Fixture | Purpose | Expected contents |
| --- | --- | --- |
| `model-basics-r2018.dxf` | Model import, selection, Properties, Layers, model PDF | Five model entities: line, circle, closed polyline, text, locked-layer line; GEOMETRY, ANNOTATION, LOCKED layers; blank default Layout1 |
| `two-layouts-r2018.dxf` | Detect paper-space loss and distinguish sheet publishing from model printing | Same model; Sheet A3 420 × 297 mm at viewport scale 1:1; Sheet A4 297 × 210 mm at 1:2; separate border and unique title in each layout; locked viewports |

Each sheet includes a default paper viewport plus one user model viewport. Four entities per sheet therefore means border, title, default viewport, model viewport. Expected scales exclude the default viewport.

The Phase 0 export experiment creates DWG derivatives in ignored build output. They are self-generated interoperability probes, **not** independently validated AutoCAD DWGs. The separate retained DWG fixtures in `librecad/src/lib/filters/tests/testdata/PROVENANCE.md` provide a different source for read checks. A genuine AutoCAD 2027-created two-layout drawing and reference PDF still need to be supplied or created in a licensed reference installation.

For future layout publishing acceptance, the PDF must contain exactly two pages, in sheet order, with different page sizes and the corresponding unique title. A model-only PDF or a tiled PDF does not satisfy that requirement.
