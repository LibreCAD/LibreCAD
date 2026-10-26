/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const {runFormalTransaction} = require('./formal-runner');
const {nodeFsAdapter} = require('./input-capture');
const inventory = require('./refusal-inventory');
const fixture = require('./fixtures/transaction-fixture');
const {resultFor, writeOutputs} = require('./fixtures/formal-child');
const post = require('./post-refusal-definitions');

const cases = new Map();
const mismatches = [];
function register(id, definition) {
    assert.equal(cases.has(id), false, `duplicate execution registration: ${id}`);
    cases.set(id, definition);
}

function cliArguments(change) {
    return {capture: false, prepare: f => ({args: change(fixture.argumentsFor(f.request), f)})};
}

register('H6-CLI-01', cliArguments(args => args.slice(1)));
register('H6-CLI-02', cliArguments(args => [...args, '--unknown']));
register('H6-CLI-03', cliArguments(args => [...args, '--run']));
register('H6-CLI-04', cliArguments(args => args.filter((value, i) => i !== 2)));
register('H6-CLI-05', cliArguments(args => args.slice(0, 5)));
register('H6-CLI-06', cliArguments(args => args.slice(0, 7)));
register('H6-CLI-07', cliArguments((args, f) => [...args, '--preflight-receipt', path.join(f.base, 'third.json')]));
register('H6-CLI-08', cliArguments(args => [...args.slice(0, 8), args[6]]));
register('H6-CLI-09', {duplicateId: true});
register('H6-CLI-10', {prepare: f => {
    f.request.receiptPaths.reverse();
}});
register('H6-CLI-11', cliArguments(args => [...args.slice(0, 6), 'relative.json', ...args.slice(7)]));
register('H6-ID-01', {prepare: f => { fs.unlinkSync(f.request.receiptPaths[0]); }});

function revision(value) {
    return {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.revision = value; })};
}

register('H6-ID-02a', revision('V506'));
register('H6-ID-02b', revision('V507'));
register('H6-ID-02c', revision('V508'));
register('H6-ID-02d', revision('V509'));
register('H6-ID-02e', revision('V510'));
register('H6-ID-02f', revision('V511'));
register('H6-ID-02g', revision('V512'));
register('H6-ID-02h', revision('V513-pre-consolidation'));

function staleCandidate(version) {
    return {prepare: f => fixture.rewriteReceipt(f, 0, row => {
        // Synthetic negative metadata only; never a historical routing path.
        row.candidate = {path: path.join(f.inputDirectory, `synthetic-stale-${version}.md`), bytes: version.length,
            sha256: fixture.hash(Buffer.from(version)),
            git_blob: crypto.createHash('sha1').update(`synthetic-${version}`).digest('hex')};
        row.preflight_result.candidate = row.candidate;
    })};
}

register('H6-ID-03a', staleCandidate('V509'));
register('H6-ID-03b', staleCandidate('V510'));
register('H6-ID-03c', staleCandidate('V511'));
register('H6-ID-03d', staleCandidate('V512'));
register('H6-ID-03e', staleCandidate('V513-pre-consolidation'));
register('H6-ID-04', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.source_manifest_sha256 = fixture.hash(Buffer.from('synthetic-stale-manifest'));
    row.preflight_result.source_manifest_sha256 = row.source_manifest_sha256;
})});
register('H6-ID-05', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.candidate.bytes++;
    row.preflight_result.candidate = row.candidate;
})});
register('H6-ID-06', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.source_manifest_sha256 = fixture.hash(Buffer.from('other-manifest-field'));
    row.preflight_result.source_manifest_sha256 = row.source_manifest_sha256;
})});
register('H6-ID-07', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.environment_sha256 = fixture.hash(Buffer.from('other-environment'));
})});
register('H6-ID-08', {prepare: f => {
    // Change both receipts to reach the runtime comparison, not pair argv equality.
    for (let i = 0; i < 2; i++) fixture.rewriteReceipt(f, i, row => {
        row.argv[0] = path.join(f.inputDirectory, 'other-node');
    });
}});
register('H6-ID-09', {prepare: f => {
    for (let i = 0; i < 2; i++) fixture.rewriteReceipt(f, i, row => {
        row.argv[1] = path.join(f.inputDirectory, 'other-runner.js');
    });
}});
register('H6-ID-10', {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.cwd = f.base; })});
register('H6-ID-11', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.join(path.dirname(f.config.receipt_roots[0]), 'other-root'));
}});

function reportBytes(change) {
    return {prepare: f => {
        const p = f.request.receiptPaths[0];
        fixture.replace(p, change(fs.readFileSync(p)));
    }};
}

register('H6-TREE-01a', reportBytes(() => Buffer.from([0xc3, 0x28, 0x0a])));
register('H6-TREE-01b', reportBytes(() => Buffer.from('{"schema_version":\n')));
register('H6-TREE-02a', reportBytes(value => Buffer.from(value.toString().replace('{', '{"schema_version":2,'))));
register('H6-TREE-02b', reportBytes(value => {
    const row = JSON.parse(value);
    return fixture.bytes(Object.fromEntries(Object.entries(row).reverse()));
}));
register('H6-TREE-02c', reportBytes(value => {
    const row = JSON.parse(value);
    delete row.status;
    return fixture.bytes(row);
}));
register('H6-TREE-02d', reportBytes(value => fixture.bytes({...JSON.parse(value), unexpected: true})));
register('H6-TREE-02e', reportBytes(value => Buffer.concat([Buffer.from([0xef, 0xbb, 0xbf]), value])));
register('H6-TREE-02f', reportBytes(value => Buffer.concat([value.subarray(0, -1), Buffer.from('\r\n')])));
register('H6-TREE-02g', reportBytes(value => Buffer.concat([value, Buffer.from('\n')])));
register('H6-TREE-02h', reportBytes(value => Buffer.from(` ${value.toString()}`)));
register('H6-TREE-03a', {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.status = 'FAIL'; })});
register('H6-TREE-03b', {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.child.exit_code = 7; })});
register('H6-TREE-03c', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.child.exit_code = null;
    row.child.signal = 'SIGTERM';
})});
register('H6-TREE-03d', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.child.exit_code = null;
    row.child.error = {code: 'ENOENT', message: 'synthetic preflight spawn failure'};
})});
register('H6-TREE-04a', {prepare: f => fixture.setStdout(f, Buffer.alloc(0))});
register('H6-TREE-04b', {prepare: f => {
    const value = fs.readFileSync(fixture.readReceipt(f).stdout.path);
    fixture.setStdout(f, Buffer.concat([value, value]));
}});
register('H6-TREE-04c', {prepare: f => {
    fixture.setStdout(f, Buffer.from(`${JSON.stringify(fixture.readReceipt(f).preflight_result, null, 2)}\n`));
}});
register('H6-TREE-05d', {prepare: f => {
    const p = fixture.readReceipt(f).stdout.path;
    fixture.replace(p, Buffer.concat([fs.readFileSync(p), Buffer.from('unbound stdout')]));
}});
register('H6-TREE-05e', {prepare: f => {
    fixture.replace(fixture.readReceipt(f).stderr.path, Buffer.from('unbound stderr'));
}});
register('H6-TREE-05f', {prepare: f => {
    const target = path.join(path.dirname(f.request.receiptPaths[0]), '30000000-0000-4000-8000-000000000003.json');
    fs.renameSync(f.request.receiptPaths[0], target);
    f.request.receiptPaths[0] = target;
}});
register('H6-TREE-06a', {prepare: f => { fs.chmodSync(f.request.receiptPaths[0], 0o600); }});
register('H6-TREE-06b', {prepare: f => {
    fs.unlinkSync(f.request.receiptPaths[0]);
    fs.mkdirSync(f.request.receiptPaths[0], {mode: 0o400});
}});
register('H6-TREE-07a', {prepare: f => { fs.unlinkSync(fixture.readReceipt(f).stderr.path); }});
register('H6-TREE-07b', {prepare: f => {
    fs.writeFileSync(path.join(path.dirname(f.request.receiptPaths[0]), 'extra'), 'extra', {mode: 0o400});
}});
register('H6-TREE-08a', {prepare: f => {
    const p = f.request.receiptPaths[0];
    fs.renameSync(p, path.join(f.derived, 'symlink-target'));
    fs.symlinkSync(path.join(f.derived, 'symlink-target'), p);
}});
register('H6-TREE-08b', {prepare: f => {
    fs.linkSync(f.request.receiptPaths[0], path.join(f.derived, 'hardlink-alias'));
}});
register('H6-TREE-08c', {prepare: f => {
    const alias = path.join(f.derived, 'storage-alias');
    fs.symlinkSync(path.dirname(f.request.receiptPaths[0]), alias);
    f.request.receiptPaths[0] = path.join(alias, path.basename(f.request.receiptPaths[0]));
}});
register('H6-TREE-09a', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.dirname(f.request.receiptPaths[1]));
}});
register('H6-TREE-09b', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.dirname(f.config.receipt_roots[1]));
}});
register('H6-TREE-09c', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.dirname(fixture.workerPath));
}});

register('H6-RACE-01', {prepare: f => {
    let changed = false;
    let preserved;
    return {changesRunParent: true, dependencies: {beforeCreate() {
        fs.renameSync(f.runParent, `${f.runParent}.original`);
        fs.mkdirSync(f.runParent, {mode: 0o700});
        changed = true;
        preserved = [fixture.tree(f.runParent), fixture.tree(`${f.runParent}.original`)];
    }}, assertAfter() {
        assert.equal(changed, true, 'parent replacement hook must be reached');
        assert.deepEqual([fixture.tree(f.runParent), fixture.tree(`${f.runParent}.original`)], preserved);
    }};
}});

function receiptRace(change) {
    return {prepare: f => {
        let afterMutation;
        return {derivedAfter: () => afterMutation, dependencies: {beforeCreate() {
            change(f);
            afterMutation = fixture.tree(f.derived);
        }}, assertAfter() { assert.ok(afterMutation, 'receipt mutation hook must be reached'); }};
    }};
}

register('H6-RACE-02a', receiptRace(f => fixture.rewriteReceipt(f, 0, row => {
    row.environment_sha256 = fixture.hash(Buffer.from('changed-between-validations'));
}, {syncStdout: false})));
register('H6-RACE-02b', receiptRace(f => {
    const p = fixture.readReceipt(f).stderr.path;
    fixture.replace(p, Buffer.concat([fs.readFileSync(p), Buffer.from('changed-between-validations')]));
}));
register('H6-RACE-03', {prepare: f => {
    const target = fixture.readReceipt(f).stderr.path;
    const descriptors = new Map();
    let armed = false;
    let altered = 0;
    return {dependencies: {beforeCreate() { armed = true; }, fs: {...nodeFsAdapter,
        openSync(p, ...args) {
            const fd = fs.openSync(p, ...args);
            descriptors.set(fd, p);
            return fd;
        },
        closeSync(fd) { descriptors.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const count = fs.readSync(fd, buffer, offset, length, position);
            if (armed && descriptors.get(fd) === target && count > 0) {
                buffer[offset] ^= 1;
                altered++;
            }
            return count;
        }
    }}, assertAfter() { assert.ok(altered > 0, 'actual receipt readback must be altered'); }};
}});
register('H6-RACE-04', {prepare: f => {
    let external;
    return {externalRoot: true, changesRunParent: true, dependencies: {fs: {...nodeFsAdapter,
        mkdirSync(p, options) {
            if (p === f.request.runRoot) {
                fs.mkdirSync(p, options);
                fs.writeFileSync(path.join(p, 'external-sentinel'), 'external race winner\n', {mode: 0o400, flag: 'wx'});
                external = fixture.tree(p);
            }
            return fs.mkdirSync(p, options);
        }
    }}, assertAfter() {
        assert.ok(external, 'external root race must reach exclusive creation');
        assert.deepEqual(fixture.tree(f.request.runRoot), external);
    }};
}});
register('H6-JOIN-01', {prepare: f => { f.request.receiptPaths[1] = f.request.receiptPaths[0]; }});
register('H6-JOIN-02', {duplicateId: true});
register('H6-JOIN-03', {prepare: f => { f.request.receiptPaths.reverse(); }});

function workerCase(mutate) {
    return {prepare: f => {
        const launches = [];
        let prefix;
        return {dependencies: {dispatch(context) {
            launches.push(context.worker_id);
            if (context.ordinal === 2) prefix = fixture.statRecord(path.join(context.run_root, 'a.json'));
            writeOutputs(context);
            const result = resultFor(context);
            return context.ordinal === 2 ? mutate(result) : result;
        }}, assertAfter(result) {
            assert.deepEqual(launches, ['worker-a', 'worker-b'], 'no third worker may launch');
            assert.equal(result.worker_launches, 2);
            assert.equal(result.workers.length, 1, 'first verified prefix must remain');
            assert.equal(result.workers[0].worker_id, 'worker-a');
            assert.deepEqual(fixture.statRecord(path.join(f.request.runRoot, 'a.json')), prefix);
            assert.equal(result.workers[0].outputs[0].sha256, prefix.sha256);
            assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
            assert.deepEqual(result.verification_failures, []);
        }};
    }};
}

register('H6-WORKER-01', workerCase(() => undefined));
register('H6-WORKER-02', workerCase(row => { delete row.status; return row; }));
register('H6-WORKER-03', workerCase(row => { row.status = 'UNKNOWN'; return row; }));
register('H6-WORKER-04', workerCase(row => { row.status = 'FAIL'; return row; }));
register('H6-WORKER-05', workerCase(row => { row.outputs = []; return row; }));
register('H6-WORKER-06', workerCase(row => { row.outputs.push('extra.json'); return row; }));
register('H6-WORKER-07', workerCase(row => { row.outputs.push(row.outputs[0]); return row; }));
register('H6-WORKER-08', workerCase(row => { row.status = undefined; return row; }));

function assertPositive(f, result) {
    assert.equal(result.worker_launches, 3);
    assert.equal(result.root_created, true);
    assert.deepEqual(result.workers.map(row => row.worker_id), ['worker-a', 'worker-b', 'worker-c']);
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json', 'c.json']);
    for (const worker of result.workers) {
        assert.deepEqual(worker.dispatch.argv, [process.execPath, fixture.workerPath, 'pass']);
        assert.equal(worker.dispatch.mode, 'process');
        const stderr = JSON.parse(Buffer.from(worker.dispatch.stderr.base64, 'base64').toString());
        assert.deepEqual(stderr.environment, f.config.preflight.environment);
        const context = JSON.parse(Buffer.from(worker.dispatch.stdin.base64, 'base64').toString());
        assert.equal(context.run_root, f.request.runRoot);
        assert.equal(context.worker_id, worker.worker_id);
        for (const output of worker.outputs) {
            assert.equal(output.sha256, fixture.hash(fs.readFileSync(output.path)));
            assert.equal(output.byte_length, fs.statSync(output.path).size);
            assert.equal(fs.statSync(output.path).mode & 0o7777, 0o400);
        }
    }
    assert.deepEqual(result.verification_failures, []);
}

register('H6-CONTROL-01', {prepare: f => ({assertAfter: result => assertPositive(f, result)})});
register('H6-CONTROL-02', {prepare: f => ({assertAfter: result => assertPositive(f, result)})});
register('H6-CONTROL-03', {prepare: f => ({args: ['--preflight-receipt', f.request.receiptPaths[0],
    '--run-dir', f.request.runRoot, '--run', '--preflight-receipt', f.request.receiptPaths[1],
    '--config', f.request.configPath], assertAfter: result => assertPositive(f, result)})});

for (const [id, definition] of post.DEFINITIONS) register(id, definition);

function roadmapOriginalRows() {
    // Qualification pins an immutable copy as a real H2 input. Normal local
    // tests still check the live roadmap; later plan commits cannot rewrite an
    // already captured qualification's expectations.
    const roadmapPath = process.env.LC_OFFSET_ROADMAP_SNAPSHOT ||
        path.join(__dirname, '..', '..', 'docs', 'plan_offset_curve_2961.md');
    require('./input-capture').canonicalPath(roadmapPath);
    return parseRoadmapOriginalRows(fs.readFileSync(roadmapPath, 'utf8'));
}

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
    // Any marker, including a malformed/partial pair, forbids legacy fallback.
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
            const first = parts[0].match(/^(H6-[A-Z]+-)(\d{2}[a-z]?)$/);
            assert.ok(first && parts.slice(1).every(value => /^\d{2}[a-z]?$/.test(value)), `${where}: malformed IDs`);
            ids = parts.map((value, i) => i === 0 ? value : `${first[1]}${value}`);
        }
        return ids.map(id => {
            assert.equal(seen.has(id), false, `H6 inventory: duplicate ID ${id}`);
            seen.add(id);
            return {id, stage, reason};
        });
    }).sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0);
}

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

test('H6 roadmap parser expands legacy and marker tables to identical independent expectations', () => {
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

test('H6 roadmap parser rejects missing duplicate reversed and malformed boundaries', () => {
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

test('H6 roadmap parser rejects malformed tables rows classifications and ranges', () => {
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

test('H6 roadmap parser rejects duplicate identities including list and range overlaps', () => {
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

test('H6 roadmap parser enforces text and table admission caps', () => {
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

test('inventory preserves every expanded original identity and exactly accounts for executable and pending rows', t => {
    const all = inventory.INVENTORY.map(row => row.id);
    const sortedUnique = [...new Set(all)].sort();
    assert.equal(all.length, sortedUnique.length, 'inventory must not conceal duplicate identities');
    assert.deepEqual(inventory.IDS, sortedUnique);
    assert.equal(inventory.COUNT, sortedUnique.length);
    const roadmap = roadmapOriginalRows();
    assert.equal(roadmap.length, 71, 'the roadmap must preserve all 71 original expectations');
    assert.equal(inventory.ORIGINAL_IDS.length, 71);
    assert.equal(inventory.EXECUTABLE.length, 93, 'all 93 executable obligations must remain registered');
    assert.deepEqual(inventory.ORIGINAL_IDS, roadmap.map(row => row.id));
    assert.deepEqual(inventory.INVENTORY.filter(row => row.original).map(({id, stage, reason}) => ({id, stage, reason}))
        .sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0), roadmap,
    'original expected classifications must not broaden away from the roadmap');
    assert.deepEqual([...cases.keys()].sort(), inventory.EXECUTABLE.map(row => row.id).sort());
    assert.deepEqual([...inventory.EXECUTABLE, ...inventory.PENDING].map(row => row.id).sort(), sortedUnique);
    for (const row of inventory.PENDING) {
        assert.equal(cases.has(row.id), false, 'pending declaration cannot masquerade as an executed case');
        assert.ok(row.dependency && row.blockedBy);
        t.diagnostic(`PENDING ${row.id}: ${row.dependency}: ${row.blockedBy}`);
    }
    t.diagnostic(`Inventory: ${inventory.COUNT} unique obligations; ${inventory.ORIGINAL_IDS.length} original; ` +
        `${inventory.EXECUTABLE.length} executable; ${inventory.PENDING.length} pending declarations, not completed tests.`);
});

for (const entry of [...inventory.EXECUTABLE].sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0)) test(`${entry.id}: ${entry.condition}`, async t => {
    const definition = cases.get(entry.id);
    assert.ok(definition, `no implementation for ${entry.id}`);
    if (definition.execution === 'post') {
        const observed = await definition.execute(t);
        const error = observed.exception;
        const actual = {protocol: observed.protocol, status: observed.result ? observed.result.status : null,
            stage: error ? error.stage : observed.result.stage,
            reason: error ? error.reason : observed.result.reason,
            exceptionName: error ? error.name : null};
        const expected = {protocol: entry.protocol, status: entry.status, stage: entry.stage,
            reason: entry.reason, exceptionName: entry.exceptionName || null};
        assert.deepEqual(actual, expected, entry.id);
        assert.equal(observed.observations.scope, entry.scope);
        assert.equal(observed.observations.h3_worker_launches, 2);
        assert.equal(observed.observations.original_trees_unchanged, true);
        assert.equal(observed.observations.original_h3_unchanged, true);
        assert.equal(observed.observations.environment_unchanged, true);
        observed.cleanup();
        return;
    }
    const f = await fixture.setup(definition);
    t.diagnostic(`attempt artifacts: ${f.base}; retained on failure`);
    const scenario = definition.prepare ? (await definition.prepare(f) || {}) : {};
    const before = fixture.immutableSnapshot(f);
    const derivedBefore = fixture.tree(f.derived);
    const parentBefore = fixture.tree(f.runParent);
    let unexpectedLaunches = 0;
    const dependencies = {...scenario.dependencies};
    if (entry.phase === 'pre-root' && entry.ingress === 'transaction') dependencies.dispatch = () => {
        unexpectedLaunches++;
        throw new Error('pre-root refusal unexpectedly dispatched a worker');
    };
    let result;
    try {
        result = entry.ingress === 'cli' ? fixture.realCli(scenario.args || fixture.argumentsFor(f.request)) :
            await runFormalTransaction(f.request, dependencies);
    } finally {
        assert.deepEqual(fixture.immutableSnapshot(f), before, 'original captures, all pinned inputs, and full environment must remain immutable');
        assert.deepEqual(fixture.tree(f.derived), scenario.derivedAfter ? scenario.derivedAfter() : derivedBefore,
            'derived tree must retain exactly the deliberate mutation, with no runner writes');
    }
    const expected = {status: entry.status, stage: entry.stage, reason: entry.reason};
    const actual = {status: result.status, stage: result.stage, reason: result.reason};
    if (JSON.stringify(actual) !== JSON.stringify(expected)) {
        const mismatch = {id: entry.id, expected, actual, message: result.message, artifacts: f.base};
        mismatches.push(mismatch);
        t.diagnostic(`PRODUCTION_BOUNDARY_MISMATCH ${JSON.stringify(mismatch)}`);
    }
    if (entry.phase === 'pre-root') {
        assert.equal(unexpectedLaunches, 0);
        assert.equal(result.worker_launches, 0);
        assert.equal(result.root_created, false);
        assert.deepEqual(result.workers, []);
        assert.equal(fs.existsSync(f.request.runRoot), Boolean(scenario.externalRoot));
        if (!scenario.changesRunParent) assert.deepEqual(fixture.tree(f.runParent), parentBefore);
    } else {
        assert.equal(result.root_created, true);
        assert.equal(fs.existsSync(f.request.runRoot), true);
    }
    if (scenario.assertAfter) scenario.assertAfter(result);
    assert.deepEqual(actual, expected, `${entry.id}: ${result.message}`);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test.after(() => {
    // These are failure observations for the parent, not a broadened oracle.
    process.stdout.write(`# H6_PRODUCTION_MISMATCHES ${JSON.stringify(mismatches)}\n`);
    process.stdout.write(`# H6_PENDING_DECLARATIONS ${JSON.stringify(inventory.PENDING)}\n`);
});
