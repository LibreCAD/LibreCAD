#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD contributors
"""Generate project-authored DXF baselines (requires ezdxf==1.4.3)."""
import hashlib
import json
from pathlib import Path
import ezdxf

DEST = Path(__file__).resolve().parents[1] / 'tests/fixtures/cad-familiar'


def main():
    if ezdxf.__version__ != '1.4.3':
        raise SystemExit('Use ezdxf==1.4.3 for this fixture generator.')
    ezdxf.options.write_fixed_meta_data_for_testing = True
    DEST.mkdir(parents=True, exist_ok=True)
    doc = ezdxf.new('R2018', setup=True)
    doc.units = 4  # mm
    doc.header['$TDCREATE'] = 2451544.5
    doc.header['$TDUPDATE'] = 2451544.5
    for name, color, weight in [('GEOMETRY', 7, 25), ('ANNOTATION', 2, 18), ('LOCKED', 1, 35)]:
        doc.layers.new(name, dxfattribs={'color': color, 'lineweight': weight})
    doc.layers.get('LOCKED').lock()
    model = doc.modelspace()
    model.add_line((0, 0), (100, 0), dxfattribs={'layer': 'GEOMETRY'})
    model.add_circle((50, 25), 10, dxfattribs={'layer': 'GEOMETRY'})
    model.add_lwpolyline([(0, 0), (100, 0), (100, 50), (0, 50)], close=True, dxfattribs={'layer': 'GEOMETRY'})
    model.add_text('MODEL 100 x 50 mm', dxfattribs={'layer': 'ANNOTATION', 'height': 4, 'insert': (0, 60)})
    model.add_line((0, 75), (100, 75), dxfattribs={'layer': 'LOCKED'})
    # A model-only file avoids treating successful model output as layout proof.
    model_file = DEST / 'model-basics-r2018.dxf'
    doc.saveas(model_file)
    specs = [('Sheet A3', (420, 297), (210, 148), (200, 100), 100, 'SHEET A3 ONLY'),
             ('Sheet A4', (297, 210), (148, 105), (100, 50), 100, 'SHEET A4 ONLY')]
    for name, paper, center, size, view_height, marker in specs:
        layout = doc.layouts.new(name)
        layout.page_setup(size=paper, margins=(10, 10, 10, 10), units='mm')
        layout.add_lwpolyline([(10, 10), (paper[0]-10, 10), (paper[0]-10, paper[1]-10), (10, paper[1]-10)], close=True)
        layout.add_text(marker, dxfattribs={'height': 5, 'insert': (15, 15)})
        # 200/100 and 100/50 sizes have view heights 100: 1:1 then 1:2.
        layout.add_viewport(center=center, size=size, view_center_point=(50, 25), view_height=view_height,
                            dxfattribs={'flags': 16384})  # locked viewport
    doc.layouts.delete('Layout1')
    sheets_file = DEST / 'two-layouts-r2018.dxf'
    doc.saveas(sheets_file)
    entries = []
    for path in (model_file, sheets_file):
        reopened = ezdxf.readfile(path)
        audit = reopened.audit()
        if audit.has_errors or audit.has_fixes:
            raise SystemExit(f'{path.name}: independent DXF audit found errors/fixes')
        entries.append({'file': path.name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                        'model_entities': len(reopened.modelspace()),
                        'layouts': [{'name': layout.name, 'entities': len(layout),
                                     'paper_mm': [layout.dxf.paper_width, layout.dxf.paper_height],
                                     'viewport_scales': [v.dxf.height / v.dxf.view_height for v in layout.query('VIEWPORT') if v.dxf.id != 1]}
                                    for layout in reopened.layouts if layout.name != 'Model'],
                        'independent_audit_errors': 0, 'independent_audit_fixes': 0})
    (DEST / 'manifest.json').write_text(json.dumps({'generator': 'scripts/make-cad-baseline-fixtures.py', 'ezdxf': ezdxf.__version__,
                                                  'license': 'GPL-2.0-or-later', 'fixtures': entries}, indent=2) + '\n')
    print(json.dumps(entries, indent=2))


if __name__ == '__main__':
    main()
