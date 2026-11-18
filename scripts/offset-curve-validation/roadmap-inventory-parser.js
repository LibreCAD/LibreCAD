/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');

function parseRoadmapOriginalRows(plan) {
    assert.ok(Buffer.byteLength(plan, 'utf8') <= 16 * 1024 * 1024, 'H6 inventory: text exceeds 16 MiB');
    const lines = plan.replace(/\r\n/g, '\n').split('\n');
    const markerPrefix = 'lc2961-h6-original-inventory';
    const marked = plan.includes(markerPrefix);
    function boundary(token, prefix = false) {
        const matches = lines.flatMap((line, i) => (prefix ? line.startsWith(token) : line === token) ? [i] : []);
        assert.equal(matches.length, 1, `H6 inventory: expected exactly one boundary ${token}`);
        return matches[0];
    }
    if (marked) {
        assert.equal(lines.filter(line => line.includes(markerPrefix)).length, 2,
            'H6 inventory: expected exactly two marker lines');
    }
    const begin = boundary(marked ? '<!-- lc2961-h6-original-inventory-v1:begin -->' :
        '### 13.1 Preserved H6 Refusal Inventory');
    const end = boundary(marked ? '<!-- lc2961-h6-original-inventory-v1:end -->' :
        '**Correction to inherited expectations:**', !marked);
    assert.ok(begin < end, 'H6 inventory: reversed boundaries');

    let first = begin + 1;
    let last = end;
    while (first < last && lines[first].trim() === '') first++;
    while (first < last && lines[last - 1].trim() === '') last--;
    const body = lines.slice(first, last);
    const header = body.findIndex(line => line.trim() ===
        '| IDs | Single condition(s), in suffix order | Expected boundary |');
    assert.ok(header >= 0, 'H6 inventory: missing table header');
    assert.ok(marked ? header === 0 : body.slice(0, header).every(line =>
        !line.includes('|') && !/^#{1,6}\s/.test(line.trim())), 'H6 inventory: unexpected content before table');
    assert.equal((body[header + 1] || '').trim(), '|---|---|---|', 'H6 inventory: malformed table separator');
    const table = body.slice(header + 2);
    assert.ok(table.length > 0, 'H6 inventory: empty table');
    assert.ok(table.length <= 128, 'H6 inventory: table exceeds 128 data rows');
    assert.ok(body.slice(header).every(line => line.length <= 8192),
        'H6 inventory: table row exceeds 8192 characters');

    const seen = new Set();
    return table.flatMap((line, index) => {
        const where = `H6 inventory: table row ${index + 1}`;
        const cells = line.trim().split('|');
        assert.ok(cells.length === 5 && cells[0] === '' && cells[4] === '', `${where}: malformed columns`);
        const [cell, condition, classification] = cells.slice(1, 4).map(value => value.trim());
        assert.ok(condition.length > 0, `${where}: empty condition`);
        const expected = classification.match(/^([A-Z][A-Z0-9_]*) \/ ([A-Z][A-Z0-9_]*)$/);
        assert.ok(expected, `${where}: malformed stage / reason`);
        const [, stage, reason] = expected;
        const range = cell.match(/^(H6-[A-Z]+-)(\d{2})([a-z])\.\.(\d{2})([a-z])$/);
        let ids;
        if (range) {
            assert.ok(range[2] === range[4] && range[3] < range[5], `${where}: malformed ID range`);
            ids = Array.from({length: range[5].charCodeAt(0) - range[3].charCodeAt(0) + 1}, (_, i) =>
                `${range[1]}${range[2]}${String.fromCharCode(range[3].charCodeAt(0) + i)}`);
        } else {
            const parts = cell.split(',').map(value => value.trim());
            const start = parts[0].match(/^(H6-[A-Z]+-)(\d{2}[a-z]?)$/);
            assert.ok(start && parts.slice(1).every(value => /^\d{2}[a-z]?$/.test(value)), `${where}: malformed IDs`);
            ids = parts.map((value, i) => i === 0 ? value : `${start[1]}${value}`);
        }
        return ids.map(id => {
            assert.equal(seen.has(id), false, `H6 inventory: duplicate ID ${id}`);
            seen.add(id);
            return {id, stage, reason};
        });
    }).sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0);
}

module.exports = {parseRoadmapOriginalRows};
