/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const crypto = require('node:crypto');
const test = require('node:test');
const h1 = require('./preflight-receipt');
const {captureInputs, revalidateInputs, nodeFsAdapter, LIMITS} = require('./input-capture');

function fixture(t) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-input-test-'));
    t.after(() => fs.rmSync(base, {recursive: true, force: true}));
    const candidatePath = path.join(base, 'candidate.md');
    const source = path.join(base, 'source.js');
    const raw = path.join(base, 'raw.txt');
    const caseInventoryPath = path.join(base, 'cases.json');
    fs.writeFileSync(candidatePath, 'actual candidate bytes\n');
    fs.writeFileSync(source, 'module.exports = 42;\n');
    fs.writeFileSync(raw, 'actual raw input\n');
    const cases = [{id: 'capture-01', source_path: source}, {id: 'capture-02', source_path: source}];
    fs.writeFileSync(caseInventoryPath, h1.canonicalJson({schema_version: 1, purpose: 'synthetic', cases}));
    const config = {purpose: 'synthetic', candidatePath, runnerPath: path.join(__dirname, 'fixtures', 'capture-child.js'),
        sourcePaths: [source], inputPaths: [raw], fixturePaths: [], configPaths: [], caseInventoryPath};
    return {base, config, source, raw, cases};
}

test('actual immutable candidate/source/runtime bytes bind a sorted exact manifest', t => {
    const {config, source} = fixture(t);
    const env = JSON.stringify(process.env);
    const result = captureInputs(config);
    const bytes = fs.readFileSync(config.candidatePath);
    assert.equal(result.candidate.sha256, crypto.createHash('sha256').update(bytes).digest('hex'));
    assert.equal(result.candidate.bytes, bytes.length);
    assert.equal(result.candidate.git_blob, crypto.createHash('sha1').update(Buffer.from(`blob ${bytes.length}\0`)).update(bytes).digest('hex'));
    assert.equal(result.sourceManifestSha256, h1.sha256(h1.canonicalJson(result.manifest)));
    assert.deepEqual(result.caseIds, ['capture-01', 'capture-02']);
    assert.deepEqual(result.manifest.files.map(row => row.path), result.manifest.files.map(row => row.path).sort());
    for (const row of result.manifest.files) {
        assert.equal(row.sha256, h1.sha256(fs.readFileSync(row.path)));
        assert.ok(row.bytes > 0);
    }
    assert.ok(result.manifest.files.some(row => row.path === source));
    assert.ok(result.manifest.files.some(row => row.roles.includes('runner')));
    assert.ok(result.manifest.files.some(row => row.roles.includes('runtime-executable')));
    assert.equal(result.runtime.exec_path, process.execPath);
    assert.equal(result.runtime.version, process.version);
    assert.equal(Object.isFrozen(result.snapshots[0].guards), true);
    assert.throws(() => result.manifest.files.push({}), TypeError);
    assert.equal(revalidateInputs(result), result);
    assert.equal(JSON.stringify(process.env), env);
});

test('missing raw inputs and supplied digest tuples are refused, not fabricated', t => {
    const {config, raw} = fixture(t);
    fs.unlinkSync(raw);
    assert.throws(() => captureInputs(config), /ENOENT/);
    assert.throws(() => captureInputs({...config, sourceManifestSha256: 'a'.repeat(64)}), /supplied/);
    assert.throws(() => captureInputs({...config, candidate: {bytes: 0}}), /supplied/);
});

test('zero-byte source records are refused even with validation purpose', t => {
    const {config, source, cases} = fixture(t);
    fs.writeFileSync(source, '');
    config.purpose = 'validation';
    fs.writeFileSync(config.caseInventoryPath, h1.canonicalJson({schema_version: 1, purpose: 'validation', cases}));
    assert.throws(() => captureInputs(config), /empty source/);
});

test('case inventory rejects duplicates, wrong order, uncaptured definitions, and purpose mismatch', t => {
    const {config, cases} = fixture(t);
    for (const [rows, purpose, message] of [
        [[cases[0], cases[0]], 'synthetic', /sorted and unique/],
        [[cases[1], cases[0]], 'synthetic', /sorted and unique/],
        [[{id: 'case', source_path: '/uncaptured.js'}], 'synthetic', /not a declared/],
        [cases, 'validation', /purpose mismatch/]
    ]) {
        fs.writeFileSync(config.caseInventoryPath, h1.canonicalJson({schema_version: 1, purpose, cases: rows}));
        assert.throws(() => captureInputs(config), message);
    }
});

test('source and configuration lists must be explicit, bounded and unique', t => {
    const {config, source} = fixture(t);
    assert.throws(() => captureInputs({...config, inputPaths: undefined}), /explicit bounded/);
    assert.throws(() => captureInputs({...config, sourcePaths: []}), /actual source/);
    assert.throws(() => captureInputs({...config, sourcePaths: [source, source]}), /duplicate paths/);
    assert.throws(() => captureInputs({...config, configPaths: Array(LIMITS.files + 1).fill(source)}), /bounded/);
});

test('symlink and hardlink raw inputs are refused', t => {
    const {config, raw, base} = fixture(t);
    const alias = path.join(base, 'alias');
    fs.symlinkSync(raw, alias);
    assert.throws(() => captureInputs({...config, inputPaths: [alias]}), /symlink|single-link/);
    fs.unlinkSync(alias);
    fs.linkSync(raw, alias);
    assert.throws(() => captureInputs(config), /single-link/);
});

test('input byte, permission, and byte-identical inode mutations invalidate snapshots', t => {
    const {config, raw} = fixture(t);
    let pinned = captureInputs(config);
    fs.appendFileSync(raw, 'changed');
    assert.throws(() => revalidateInputs(pinned), /changed/);
    pinned = captureInputs(config);
    fs.chmodSync(raw, 0o400);
    assert.throws(() => revalidateInputs(pinned), /changed/);
    pinned = captureInputs(config);
    const bytes = fs.readFileSync(raw);
    fs.renameSync(raw, raw + '.original');
    fs.writeFileSync(raw, bytes, {mode: 0o400});
    assert.throws(() => revalidateInputs(pinned), /changed/);
});

test('replacement of an ancestor between execution boundaries is refused', t => {
    const {config, base} = fixture(t);
    const pinned = captureInputs(config);
    fs.renameSync(base, base + '.old');
    t.after(() => fs.rmSync(base + '.old', {recursive: true, force: true}));
    fs.mkdirSync(base);
    for (const name of fs.readdirSync(base + '.old')) fs.renameSync(path.join(base + '.old', name), path.join(base, name));
    assert.throws(() => revalidateInputs(pinned), /directory identity changed/);
});

test('unrelated sibling changes do not invalidate retained input directories', t => {
    const {config, base} = fixture(t);
    const pinned = captureInputs(config);
    fs.writeFileSync(path.join(base, 'unrelated'), 'sibling');
    assert.doesNotThrow(() => revalidateInputs(pinned));
});

test('read-time input mutation and missing no-follow capability are refused', t => {
    const {config, raw} = fixture(t);
    let changed = false;
    const io = {...nodeFsAdapter, readSync: (...args) => {
        if (!changed && fs.fstatSync(args[0], {bigint: true}).ino === fs.lstatSync(raw, {bigint: true}).ino) {
            changed = true;
            fs.appendFileSync(raw, 'raced');
        }
        return nodeFsAdapter.readSync(...args);
    }};
    assert.throws(() => captureInputs(config, {io}), /changed/);
    assert.equal(changed, true);
    assert.throws(() => captureInputs(config, {io: {...nodeFsAdapter, constants: {...fs.constants, O_NOFOLLOW: undefined}}}), /no-follow/);
});

test('oversized raw inputs fail before reading unbounded data', t => {
    const {config, raw} = fixture(t);
    const fd = fs.openSync(raw, 'r+');
    try { fs.ftruncateSync(fd, LIMITS.fileBytes + 1); } finally { fs.closeSync(fd); }
    assert.throws(() => captureInputs(config), /byte cap/);
});

test('missing inventory and fake imported snapshots cannot become evidence', t => {
    const {config} = fixture(t);
    fs.unlinkSync(config.caseInventoryPath);
    assert.throws(() => captureInputs(config), /ENOENT/);
    assert.throws(() => revalidateInputs(Object.freeze({snapshots: []})), /original in-process/);
});

test('validation input capture cannot use injected synthetic filesystem records', t => {
    const {config, cases} = fixture(t);
    const actual = {...config, purpose: 'validation'};
    fs.writeFileSync(config.caseInventoryPath, h1.canonicalJson({schema_version: 1, purpose: 'validation', cases}));
    assert.throws(() => captureInputs(actual, {io: {...nodeFsAdapter}}), /restricted to synthetic/);
    const pinned = captureInputs(actual);
    assert.throws(() => revalidateInputs(pinned, {io: {...nodeFsAdapter}}), /restricted to synthetic/);
    assert.doesNotThrow(() => revalidateInputs(pinned));
});

test('runtime read substitution is detected without modifying the actual executable', t => {
    const {config} = fixture(t);
    const pinned = captureInputs(config);
    const executable = fs.lstatSync(pinned.runtime.executable_path, {bigint: true});
    let substituted = false;
    const io = {...nodeFsAdapter, readSync: (...args) => {
        const count = fs.readSync(...args);
        const stat = fs.fstatSync(args[0], {bigint: true});
        if (count > 0 && stat.dev === executable.dev && stat.ino === executable.ino) {
            args[1][args[2]] ^= 1;
            substituted = true;
        }
        return count;
    }};
    assert.throws(() => revalidateInputs(pinned, {io}), /captured input changed/);
    assert.equal(substituted, true);
    assert.doesNotThrow(() => revalidateInputs(pinned));
});

test('actual empty raw input is distinct from an empty fabricated source record', t => {
    const {config, raw} = fixture(t);
    fs.writeFileSync(raw, '');
    const pinned = captureInputs(config);
    const row = pinned.manifest.files.find(file => file.path === raw);
    assert.deepEqual(row.roles, ['input']);
    assert.equal(row.bytes, 0);
    assert.equal(row.sha256, h1.sha256(Buffer.alloc(0)));
    assert.doesNotThrow(() => revalidateInputs(pinned));
});
