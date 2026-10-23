/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const vm = require('node:vm');
const h1 = require('./preflight-receipt');
const h5 = require('./p0f-evidence');
const {setup} = require('./fixtures/evidence-transaction');
const {validateQualificationOptions, persistResult, revalidateCompletedRuns, qualifyWorkerRunner} = require('./qualify-worker-runner');

test('qualification option snapshot rejects getters and symbols without evaluating them', () => {
    let calls = 0;
    const value = {directory: '/attempt', basePath: '/base', productPath: '/product', candidatePath: '/candidate', roadmapPath: '/roadmap'};
    Object.defineProperty(value, 'candidatePath', {get() { calls++; return '/other'; }});
    assert.throws(() => validateQualificationOptions(value), /keys\/order/);
    assert.equal(calls, 0);
    const symbol = {directory: '/attempt', basePath: '/base', productPath: '/product', candidatePath: '/candidate', roadmapPath: '/roadmap'};
    symbol[Symbol('hidden')] = '/other';
    assert.throws(() => validateQualificationOptions(symbol), /keys\/order/);
});

test('qualification snapshots cannot be redirected or mutate caller options', () => {
    const value = {directory: '/attempt', basePath: '/base', productPath: '/product', candidatePath: '/candidate', roadmapPath: '/roadmap'};
    const saved = validateQualificationOptions(value);
    value.directory = '/redirected'; value.candidatePath = '/substitution';
    assert.equal(saved.directory, '/attempt'); assert.equal(saved.candidatePath, '/candidate');
    assert.ok(Object.isFrozen(saved)); assert.equal(Object.isFrozen(value), false);
    assert.throws(() => { saved.candidatePath = '/other'; }, TypeError);
});

test('qualification rejects incomplete and relative requests before publication', async () => {
    await assert.rejects(qualifyWorkerRunner({}), /keys\/order/);
    await assert.rejects(qualifyWorkerRunner({directory: 'relative', basePath: '/base', productPath: '/product',
        candidatePath: '/candidate', roadmapPath: '/roadmap'}), /canonical and absolute/);
});

test('qualification refuses existing attempt namespace and preserves external files', async t => {
    const directory = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-qualification-exists-'));
    t.diagnostic(`attempt ${directory}; retained on unexpected failure`);
    const sentinel = path.join(directory, 'sentinel');
    fs.writeFileSync(sentinel, 'independently owned attempt\n', {mode: 0o400});
    const original = fs.lstatSync(sentinel, {bigint: true});
    const source = path.join(__dirname, 'qualify-worker-runner.js');
    await assert.rejects(qualifyWorkerRunner({directory, basePath: source, productPath: source,
        candidatePath: source, roadmapPath: source}), {code: 'EEXIST'});
    assert.deepEqual(fs.readdirSync(directory), ['sentinel']);
    assert.equal(fs.readFileSync(sentinel, 'utf8'), 'independently owned attempt\n');
    const after = fs.lstatSync(sentinel, {bigint: true});
    assert.equal(after.ino, original.ino); assert.equal(after.mode, original.mode);
    assert.equal(after.mtimeNs, original.mtimeNs); assert.equal(after.ctimeNs, original.ctimeNs);
    fs.rmSync(directory, {recursive: true});
});

test('qualification CLI requires every explicit input and produces no fabricated report', () => {
    const child = spawnSync(process.execPath, [path.join(__dirname, 'qualify-worker-runner.js')], {encoding: 'utf8'});
    assert.equal(child.status, 1);
    assert.equal(child.stdout, '');
    assert.match(child.stderr, /expected --directory ABS/);
});

test('qualification result publication binds the written descriptor identity and mode, not just equal bytes', t => {
    const root = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-qualification-readback-'));
    t.diagnostic(`attempt ${root}; retained on unexpected failure`);
    const file = path.join(__dirname, 'qualify-worker-runner.js'), source = fs.readFileSync(file, 'utf8');
    for (const mode of ['inode', 'mode']) {
        const target = path.join(root, `${mode}.json`);
        let written, changed = false;
        const localFs = {...fs,
            openSync(p, flags, ...args) {
                const fd = fs.openSync(p, flags, ...args);
                if (p === target && flags & fs.constants.O_WRONLY) written = fd;
                return fd;
            },
            closeSync(fd) {
                fs.closeSync(fd);
                if (fd !== written || changed) return;
                changed = true;
                if (mode === 'mode') fs.chmodSync(target, 0o644);
                else {
                    fs.copyFileSync(target, `${target}.replacement`, fs.constants.COPYFILE_EXCL);
                    fs.chmodSync(`${target}.replacement`, 0o400);
                    fs.renameSync(`${target}.replacement`, target);
                }
            }};
        const module = {exports: {}};
        vm.runInThisContext(`(function(require,module,exports,__dirname,__filename){\n${source}\n})`)(
            name => name === 'node:fs' ? localFs : require(name), module, module.exports, __dirname, file);
        assert.throws(() => module.exports.persistResult(target, {status: 'RECORDED', kind: 'negative-readback-control'}), /readback differs/);
        assert.equal(changed, true);
        assert.deepEqual(h1.parseCanonicalJson(fs.readFileSync(target)), {status: 'RECORDED', kind: 'negative-readback-control'});
        assert.ok(fs.existsSync(target), 'the substituted artifact is retained');
    }
    fs.rmSync(root, {recursive: true});
});

test('qualification final pass revalidates the first run original evidence owner after later work', async t => {
    const f = await setup(t);
    const evidence = await h5.runP0fEvidence(f.evidenceRequest);
    assert.equal(evidence.status, 'PASS', evidence.message);
    const result = {evidence, ledger: f.ledgerReadback};
    const resultFile = persistResult(path.join(f.base, 'recorded-run.json'), {status: 'RECORDED', manifest: evidence.manifest_path});
    const runs = [{result, resultFile, captures: f.captures}];
    revalidateCompletedRuns(runs);
    const original = fs.readFileSync(evidence.manifest_path), mutated = h1.parseCanonicalJson(original, {maxBytes: 2097152});
    mutated.inputs.candidate.bytes++;
    fs.chmodSync(evidence.manifest_path, 0o600);
    fs.writeFileSync(evidence.manifest_path, h1.canonicalJson(mutated));
    fs.chmodSync(evidence.manifest_path, 0o400);
    assert.throws(() => revalidateCompletedRuns(runs), {reason: 'MANIFEST_CHANGED'});
    assert.notDeepEqual(fs.readFileSync(evidence.manifest_path), original);
    assert.ok(fs.existsSync(evidence.review_path));
    assert.equal(result.evidence.status, 'PASS', 'retain the real historical result; never rewrite it into new authority');
    fs.rmSync(f.base, {recursive: true});
});
