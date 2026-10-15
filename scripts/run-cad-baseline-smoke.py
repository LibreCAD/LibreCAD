#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD contributors
"""Record actual file/PDF behavior; use optional cad-baseline-requirements.txt."""
import argparse
import hashlib
import json
import os
import subprocess
from collections import Counter
from pathlib import Path
import ezdxf
from pypdf import PdfReader

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / 'tests/fixtures/cad-familiar'


def dxf_summary(path):
    doc = ezdxf.readfile(path)
    audit = doc.audit()
    geometry = []
    for entity in doc.modelspace():
        kind = entity.dxftype()
        if kind == 'LINE':
            geometry.append({'type': kind, 'layer': entity.dxf.layer,
                             'start': list(entity.dxf.start), 'end': list(entity.dxf.end)})
        elif kind == 'CIRCLE':
            geometry.append({'type': kind, 'layer': entity.dxf.layer,
                             'center': list(entity.dxf.center), 'radius': entity.dxf.radius})
    return {'model_types': dict(Counter(e.dxftype() for e in doc.modelspace())),
            'line_circle_geometry': sorted(geometry, key=lambda item: json.dumps(item, sort_keys=True)),
            'layers': [layer.dxf.name for layer in doc.layers],
            'layouts': [{'name': layout.name, 'types': dict(Counter(e.dxftype() for e in layout)),
                         'paper_mm': [layout.dxf.paper_width, layout.dxf.paper_height]}
                        for layout in doc.layouts if layout.name != 'Model'],
            'audit_errors': len(audit.errors), 'audit_fixes': len(audit.fixes)}


def pdf_summary(path):
    pdf = PdfReader(path)
    return {'pages': len(pdf.pages),
            'page_sizes_mm': [[round(float(p.mediabox.width)*25.4/72, 3),
                               round(float(p.mediabox.height)*25.4/72, 3)] for p in pdf.pages],
            'extracted_text': [p.extract_text() for p in pdf.pages]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--platform', choices=['offscreen', 'cocoa'], default='offscreen',
                        help='Use cocoa for a packaged Mac app that ships only the native plugin.')
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        raise SystemExit('Use an empty output directory so stale artifacts cannot pass inspection.')
    results = []
    env = dict(os.environ, QT_QPA_PLATFORM=args.platform)

    def run(name, command, artifact=None, summary=None, expected_returncode=0):
        argv = [str(executable)] + [str(value) for value in command]
        try:
            proc = subprocess.run(argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, timeout=60)
            log = proc.stdout
            item = {'name': name, 'argv': argv, 'returncode': proc.returncode,
                    'process_expectation_passed': proc.returncode == expected_returncode}
        except subprocess.TimeoutExpired as error:
            log = str(error)
            item = {'name': name, 'timeout': True, 'process_expectation_passed': False}
        (output / f'{name}.log').write_text(log)
        if artifact and artifact.exists():
            item['artifact'] = artifact.name
            item['sha256'] = hashlib.sha256(artifact.read_bytes()).hexdigest()
            if summary:
                try:
                    item['inspection'] = summary(artifact)
                except Exception as error:
                    item['inspection_error'] = str(error)
                    item['process_expectation_passed'] = False
        elif artifact and expected_returncode == 0:
            item['process_expectation_passed'] = False
        results.append(item)
        return item

    run('help', ['--help'])
    for name in ('model-basics-r2018', 'two-layouts-r2018'):
        source = FIXTURES / f'{name}.dxf'
        pdf = output / f'{name}.pdf'
        item = run(f'{name}-pdf', ['dxf2pdf', '--fit', '--monochrome', '--margins', '10,10,10,10',
                                 '--paper', '420x297', '-o', pdf, source], pdf, pdf_summary)
        if name == 'two-layouts-r2018' and 'inspection' in item:
            observed = item['inspection']
            target_sizes = [[420.0, 297.0], [297.0, 210.0]]
            item['layout_page_count_and_sizes_match_target'] = (
                observed['pages'] == 2 and all(abs(actual - expected) <= 0.25
                    for page, target in zip(observed['page_sizes_mm'], target_sizes)
                    for actual, expected in zip(page, target)))
            item['page_size_tolerance_mm'] = 0.25
            item['layout_note'] = 'The existing command prints a model; sheet publishing is a later-phase requirement. Page count/sizes alone are insufficient to certify sheet content.'
        dwg = output / f'{name}.dwg'
        written = run(f'{name}-dwg', ['dxf2dwg', '--dwg-version', 'r2018', '-o', dwg, source], dwg)
        if written['process_expectation_passed']:
            written['dwg_magic'] = dwg.read_bytes()[:6].decode('ascii', errors='replace')
            dxf = output / f'{name}-roundtrip.dxf'
            reopened = run(f'{name}-dwg-read', ['dwg2dxf', '--dxf-version', 'r2018', '-o', dxf, dwg], dxf, dxf_summary)
            if 'inspection' in reopened:
                original = dxf_summary(source)
                observed = reopened['inspection']
                reopened['model_type_counts_match_source'] = observed['model_types'] == original['model_types']
                reopened['model_line_circle_geometry_matches_source'] = observed['line_circle_geometry'] == original['line_circle_geometry']
    retained = ROOT / 'librecad/src/lib/filters/tests/testdata/large_radial.dwg'
    dxf = output / 'retained-large-radial.dxf'
    run('retained-dwg-read', ['dwg2dxf', '-o', dxf, retained], dxf, dxf_summary)
    pdf = output / 'retained-large-radial.pdf'
    run('retained-dwg-pdf', ['dwg2pdf', '--fit', '--monochrome', '--paper', '420x297',
                             '--margins', '10,10,10,10', '-o', pdf, retained], pdf, pdf_summary)
    bad = output / 'invalid.dwg'
    bad.write_bytes(b'not a drawing\n')
    rejected = output / 'invalid-output.dxf'
    item = run('invalid-dwg-rejected', ['dwg2dxf', '-o', rejected, bad], expected_returncode=1)
    item['no_output_created'] = not rejected.exists()
    item['process_expectation_passed'] &= item['no_output_created']
    report = {'scope': 'Baseline observations, not AutoCAD compatibility certification. DWG derivatives use the same in-tree writer and reader; ezdxf independently inspects resulting DXF. Retained DWG provenance is documented separately.',
              'executable': str(executable), 'results': results}
    (output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    for item in results:
        print(item['name'], 'PASS' if item['process_expectation_passed'] else 'FAIL', item.get('inspection', ''))
    return 0 if all(item['process_expectation_passed'] for item in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
