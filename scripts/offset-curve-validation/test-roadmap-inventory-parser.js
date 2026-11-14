/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const test = require('node:test');
const {parseRoadmapOriginalRows} = require('./roadmap-inventory-parser');

const parserTable = [
    '| IDs | Single condition(s), in suffix order | Expected boundary |',
    '|---|---|---|',
    '| H6-CLI-01 | Missing --run | CLI / RUN_MODE_REQUIRED |',
    '| H6-CLI-05,06,07 | Zero, one, three receipt paths | CLI / RECEIPT_COUNT |',
    '| H6-ID-02a..02c | Synthetic stale revisions | IDENTITY / REVISION_MISMATCH |'
].join('\n');

function parserDocument(table, format) {
    return format === 'markers' ?
        `## Renumbered contract\n<!-- lc2961-h6-original-inventory-v1:begin -->\n${table}\n` +
            '<!-- lc2961-h6-original-inventory-v1:end -->\n' :
        `### 13.1 Preserved H6 Refusal Inventory\n\nRanges expand to separate identities.\n\n${table}\n\n` +
            '**Correction to inherited expectations:** Historical explanation.\n';
}

test('roadmap parser expands legacy and marker tables to identical independent expectations', () => {
    const expected = [
        {id: 'H6-CLI-01', stage: 'CLI', reason: 'RUN_MODE_REQUIRED'},
        {id: 'H6-CLI-05', stage: 'CLI', reason: 'RECEIPT_COUNT'},
        {id: 'H6-CLI-06', stage: 'CLI', reason: 'RECEIPT_COUNT'},
        {id: 'H6-CLI-07', stage: 'CLI', reason: 'RECEIPT_COUNT'},
        {id: 'H6-ID-02a', stage: 'IDENTITY', reason: 'REVISION_MISMATCH'},
        {id: 'H6-ID-02b', stage: 'IDENTITY', reason: 'REVISION_MISMATCH'},
        {id: 'H6-ID-02c', stage: 'IDENTITY', reason: 'REVISION_MISMATCH'}
    ];
    for (const format of ['legacy', 'markers']) {
        const plan = parserDocument(parserTable, format);
        assert.deepEqual(parseRoadmapOriginalRows(plan), expected, format);
        assert.deepEqual(parseRoadmapOriginalRows(plan.replace(/\n/g, '\r\n')), expected, `${format}: CRLF`);
        const outside = '| H6-OUTSIDE-99 | Not part of the original table | WRONG / WRONG |\n';
        assert.deepEqual(parseRoadmapOriginalRows(outside + plan + outside), expected, `${format}: bounded`);
    }
    assert.deepEqual(parseRoadmapOriginalRows(parserDocument(parserTable, 'markers') +
        parserDocument('not a table', 'legacy')), expected, 'markers take precedence over obsolete headings');
});

test('roadmap parser rejects missing duplicate reversed and malformed boundaries', () => {
    const marked = parserDocument(parserTable, 'markers');
    const legacy = parserDocument(parserTable, 'legacy');
    const begin = '<!-- lc2961-h6-original-inventory-v1:begin -->';
    const end = '<!-- lc2961-h6-original-inventory-v1:end -->';
    const heading = '### 13.1 Preserved H6 Refusal Inventory';
    const correction = '**Correction to inherited expectations:**';
    const invalid = [
        ['no boundaries', parserTable],
        ['missing begin', marked.replace(begin, '')],
        ['missing end', marked.replace(end, '')],
        ['duplicate begin', marked.replace(begin, `${begin}\n${begin}`)],
        ['duplicate end', marked.replace(end, `${end}\n${end}`)],
        ['duplicate pair', marked + marked],
        ['reversed markers', `${end}\n${parserTable}\n${begin}`],
        ['inline begin', marked.replace(begin, `prefix ${begin}`)],
        ['inline end', marked.replace(end, `${end} suffix`)],
        ['malformed marker', marked.replace(':begin', ':BEGIN')],
        ['partial markers must not fall back', marked.replace(end, '') + legacy],
        ['malformed markers must not fall back', marked.replace(/inventory-v1/g, 'inventory-v2') + legacy],
        ['missing legacy heading', legacy.replace(heading, '')],
        ['missing legacy correction', legacy.replace(correction, '')],
        ['duplicate legacy heading', `${heading}\n${legacy}`],
        ['duplicate legacy correction', `${legacy}\n${correction}`],
        ['reversed legacy boundaries', `${correction}\n${parserTable}\n${heading}`],
        ['legacy crosses a section', legacy.replace(parserTable, `### Another section\n${parserTable}`)]
    ];
    for (const [label, plan] of invalid) assert.throws(() => parseRoadmapOriginalRows(plan), /H6 inventory/, label);
});

test('roadmap parser rejects malformed tables rows classifications and ranges', () => {
    const row = '| H6-CLI-01 | Missing --run | CLI / RUN_MODE_REQUIRED |';
    const invalidTables = [
        ['empty table', ''],
        ['missing header', parserTable.split('\n').slice(1).join('\n')],
        ['missing separator', parserTable.replace('|---|---|---|\n', '')],
        ['malformed separator', parserTable.replace('|---|---|---|', '|---|---|')],
        ['no data rows', parserTable.split('\n').slice(0, 2).join('\n')],
        ['duplicate header', `${parserTable}\n${parserTable}`],
        ['unknown row', `${parserTable}\nnot a table row`],
        ['blank interior row', parserTable.replace(row, `${row}\n`)],
        ['truncated row', parserTable.replace(row, '| H6-CLI-01 | Missing --run |')],
        ['missing outer pipe', parserTable.replace(row, row.slice(1))],
        ['extra column', parserTable.replace(row, `${row} extra |`)],
        ['empty ID', parserTable.replace('H6-CLI-01', '')],
        ['empty condition', parserTable.replace('Missing --run', '')],
        ...['CLI', 'CLI / ', ' / RUN_MODE_REQUIRED', 'CLI / RUN_MODE_REQUIRED / EXTRA',
            'cli / RUN_MODE_REQUIRED', 'CLI / not-a-reason'].map(value =>
            [`bad classification ${value}`, parserTable.replace('CLI / RUN_MODE_REQUIRED', value)]),
        ...['H6-CLI-1', 'H6-CLI-001', 'H6-CLI-01A', 'not-an-id', 'H6-CLI-05,',
            'H6-CLI-05,,07', 'H6-CLI-05,H6-CLI-06', 'H6-ID-02c..02a', 'H6-ID-02a..02a',
            'H6-ID-02a..03c', 'H6-ID-02a..', 'H6-ID-02..03', 'H6-ID-02A..02C',
            'H6-ID-02a..02c,04'].map(value => [`bad IDs ${value}`, parserTable.replace('H6-CLI-01', value)])
    ];
    for (const format of ['legacy', 'markers']) {
        for (const [label, table] of invalidTables) {
            assert.throws(() => parseRoadmapOriginalRows(parserDocument(table, format)), /H6 inventory/,
                `${format}: ${label}`);
        }
    }
});

test('roadmap parser rejects duplicate identities including list and range overlaps', () => {
    const invalidTables = [
        parserTable.replace('H6-CLI-05,06,07', 'H6-CLI-05,06,05'),
        parserTable.replace('H6-CLI-01', 'H6-CLI-05'),
        parserTable + '\n| H6-CLI-01 | Duplicate row | CLI / RUN_MODE_REQUIRED |',
        parserTable + '\n| H6-ID-02b..02d | Overlapping range | IDENTITY / REVISION_MISMATCH |',
        parserTable + '\n| H6-ID-02c | Overlapping singleton | IDENTITY / REVISION_MISMATCH |'
    ];
    for (const format of ['legacy', 'markers']) {
        for (const table of invalidTables) {
            assert.throws(() => parseRoadmapOriginalRows(parserDocument(table, format)), /H6 inventory.*duplicate ID/,
                `${format}: ${table}`);
        }
    }
});

test('roadmap parser enforces text and table admission caps', () => {
    const textLimit = 16 * 1024 * 1024;
    const header = parserTable.split('\n').slice(0, 2).join('\n');
    const rows = Array.from({length: 129}, (_, i) =>
        `| H6-CAP-${String(Math.floor(i / 26) + 1).padStart(2, '0')}${String.fromCharCode(97 + i % 26)} | ` +
            'Admission case | CLI / ROW_CAP |');
    const rowPrefix = '| H6-CAP-01 | ';
    const rowSuffix = ' | CLI / ROW_CAP |';
    const fullRow = rowPrefix + 'x'.repeat(8192 - rowPrefix.length - rowSuffix.length) + rowSuffix;
    for (const format of ['legacy', 'markers']) {
        const plan = parserDocument(parserTable, format);
        const expected = parseRoadmapOriginalRows(plan);
        const fullText = plan + 'x'.repeat(textLimit - Buffer.byteLength(plan, 'utf8'));
        assert.deepEqual(parseRoadmapOriginalRows(fullText), expected, `${format}: exactly 16 MiB`);
        assert.throws(() => parseRoadmapOriginalRows(fullText + 'x'), /H6 inventory: text exceeds 16 MiB/,
            `${format}: over text cap`);
        assert.throws(() => parseRoadmapOriginalRows(plan + '\r\n'.repeat(textLimit / 2)),
            /H6 inventory: text exceeds 16 MiB/, `${format}: text cap precedes CRLF normalization`);
        assert.throws(() => parseRoadmapOriginalRows(plan + '\u00e9'.repeat(textLimit / 2)),
            /H6 inventory: text exceeds 16 MiB/, `${format}: text cap counts UTF-8 bytes`);
        assert.equal(parseRoadmapOriginalRows(parserDocument(`${header}\n${rows.slice(0, 128).join('\n')}`,
            format)).length, 128, `${format}: exactly 128 data rows`);
        assert.throws(() => parseRoadmapOriginalRows(parserDocument(`${header}\n${rows.join('\n')}`, format)),
            /H6 inventory: table exceeds 128 data rows/, `${format}: over data row cap`);
        assert.deepEqual(parseRoadmapOriginalRows(parserDocument(`${header}\n${fullRow}`, format)),
            [{id: 'H6-CAP-01', stage: 'CLI', reason: 'ROW_CAP'}], `${format}: exactly 8192 row characters`);
        for (const table of [`${header}\n ${fullRow}`, `${header}\n${fullRow} `,
            `${header.split('\n')[0]}${' '.repeat(8193)}\n|---|---|---|\n${rows[0]}`,
            `${header.split('\n')[0]}\n|---|---|---|${' '.repeat(8193)}\n${rows[0]}`]) {
            assert.throws(() => parseRoadmapOriginalRows(parserDocument(table, format)),
                /H6 inventory: table row exceeds 8192 characters/, `${format}: raw table row cap`);
        }
    }
});
