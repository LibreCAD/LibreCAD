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
const h2 = require('./input-capture');
const h5 = require('./p0f-evidence');
const {setup} = require('./fixtures/evidence-transaction');
const {validateQualificationOptions, persistResult, prepareQualificationTemporaryRoot,
    revalidateQualificationTemporaryRoot, revalidateCompletedRuns, qualifyWorkerRunner} = require('./qualify-worker-runner');

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

function temporaryFixture(t) {
    const directory = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-qualification-temp-test-'));
    const owner = prepareQualificationTemporaryRoot(directory);
    t.diagnostic(`attempt ${directory}; temporary root ${owner.root}; retained on unexpected failure`);
    const snapshot = path.join(owner.root, 'roadmap-snapshot.md');
    fs.writeFileSync(snapshot, 'actual pinned roadmap bytes\n', {mode: 0o400});
    const source = path.join(__dirname, 'qualify-worker-runner.js');
    const inventory = path.join(directory, 'cases.json');
    fs.writeFileSync(inventory, h1.canonicalJson({schema_version: 1, purpose: 'synthetic',
        cases: [{id: 'owned-temp', source_path: source}]}), {mode: 0o400});
    const captured = h2.captureInputs({purpose: 'synthetic', candidatePath: source, runnerPath: source,
        sourcePaths: [source], inputPaths: [snapshot, owner.manifestPath], fixturePaths: [], configPaths: [], caseInventoryPath: inventory});
    return {directory, owner, snapshot, captured};
}

test('qualification temporary root is private, exclusive, recorded and forwarded unchanged to a real Node child', t => {
    const f = temporaryFixture(t), record = h1.parseCanonicalJson(fs.readFileSync(f.owner.manifestPath));
    const stat = fs.lstatSync(f.owner.root, {bigint: true});
    assert.equal(Number(stat.mode & 0o7777n), 0o700);
    assert.equal(fs.lstatSync(f.owner.manifestPath).mode & 0o7777, 0o400);
    assert.deepEqual(record, {schema_version: 1, kind: 'qualification-temporary-root', path: f.owner.root,
        device: String(stat.dev), inode: String(stat.ino), mode: 0o700, uid: String(stat.uid), gid: String(stat.gid)});
    assert.equal(fs.realpathSync.native(f.owner.root), f.owner.root);
    if (process.platform === 'darwin') assert.equal(path.dirname(f.owner.root), '/private/tmp');
    const environment = {LC_ALL: 'C', TMPDIR: f.owner.root, LC_OFFSET_ROADMAP_SNAPSHOT: f.snapshot};
    const child = spawnSync(process.execPath, ['-e', 'process.stdout.write(JSON.stringify({tmp:require("node:os").tmpdir(),environment:process.env}))'],
        {env: environment, encoding: 'utf8', timeout: 30000, maxBuffer: 8192});
    assert.equal(child.status, 0, child.stderr);
    const observed = JSON.parse(child.stdout);
    assert.equal(observed.tmp, f.owner.root);
    for (const [key, value] of Object.entries(environment)) assert.equal(observed.environment[key], value);
    assert.equal(h2.revalidateInputs(f.captured), f.captured);
    revalidateQualificationTemporaryRoot(f.owner);
    assert.throws(() => revalidateQualificationTemporaryRoot({...f.owner}), /original.*owner/);
    assert.throws(() => { f.owner.root = '/substitution'; }, TypeError);
    fs.mkdirSync(path.join(f.owner.root, 'real-child-fixture'), {mode: 0o700});
    revalidateQualificationTemporaryRoot(f.owner); // Child membership changes are permitted, not directory replacement.
    fs.rmSync(f.owner.root, {recursive: true}); fs.rmSync(f.directory, {recursive: true});
});

test('qualification temporary-root original mode remains guarded by both the owner and H2', t => {
    const f = temporaryFixture(t);
    fs.chmodSync(f.owner.root, 0o755);
    assert.throws(() => revalidateQualificationTemporaryRoot(f.owner), /directory identity changed/);
    assert.throws(() => h2.revalidateInputs(f.captured), /directory identity changed/);
    assert.ok(fs.existsSync(f.snapshot), 'refusal must not discard original diagnostic bytes');
    fs.rmSync(f.owner.root, {recursive: true}); fs.rmSync(f.directory, {recursive: true});
});

test('qualification temporary-root same-byte replacement cannot acquire the original owner or H2 ancestry', t => {
    const f = temporaryFixture(t), bytes = fs.readFileSync(f.snapshot), saved = `${f.owner.root}-original`;
    fs.renameSync(f.owner.root, saved);
    fs.mkdirSync(f.owner.root, {mode: 0o700});
    fs.writeFileSync(f.snapshot, bytes, {mode: 0o400});
    assert.throws(() => revalidateQualificationTemporaryRoot(f.owner), /directory identity changed/);
    assert.throws(() => h2.revalidateInputs(f.captured), /directory identity changed/);
    assert.deepEqual(fs.readFileSync(path.join(saved, 'roadmap-snapshot.md')), bytes);
    assert.deepEqual(fs.readFileSync(f.snapshot), bytes);
    fs.rmSync(f.owner.root, {recursive: true}); fs.rmSync(saved, {recursive: true}); fs.rmSync(f.directory, {recursive: true});
});

test('qualification temporary-root symlink substitution is refused without touching its target', t => {
    const f = temporaryFixture(t), saved = `${f.owner.root}-original`;
    const sentinel = fs.readFileSync(f.owner.manifestPath), before = fs.lstatSync(f.owner.manifestPath, {bigint: true});
    fs.renameSync(f.owner.root, saved);
    fs.symlinkSync(f.directory, f.owner.root);
    assert.throws(() => revalidateQualificationTemporaryRoot(f.owner), /directory identity changed/);
    assert.throws(() => h2.revalidateInputs(f.captured), /directory identity changed/);
    assert.deepEqual(fs.readFileSync(f.owner.manifestPath), sentinel);
    assert.equal(h2.statIdentity(fs.lstatSync(f.owner.manifestPath, {bigint: true})), h2.statIdentity(before));
    fs.unlinkSync(f.owner.root); fs.rmSync(saved, {recursive: true}); fs.rmSync(f.directory, {recursive: true});
});

test('qualification temporary-root manifest rejects same-byte inode, mode and hardlink substitutions', t => {
    const attempts = [];
    for (const mode of ['inode', 'mode', 'hardlink']) {
        const f = temporaryFixture(t); attempts.push(f);
        const bytes = fs.readFileSync(f.owner.manifestPath);
        if (mode === 'inode') {
            const replacement = path.join(f.directory, 'replacement.json');
            fs.writeFileSync(replacement, bytes, {mode: 0o400});
            fs.renameSync(replacement, f.owner.manifestPath);
        } else if (mode === 'mode') fs.chmodSync(f.owner.manifestPath, 0o644);
        else fs.linkSync(f.owner.manifestPath, path.join(f.directory, 'manifest-alias'));
        assert.throws(() => revalidateQualificationTemporaryRoot(f.owner));
        assert.throws(() => h2.revalidateInputs(f.captured));
        assert.deepEqual(fs.readFileSync(f.owner.manifestPath), bytes);
    }
    for (const f of attempts) { fs.rmSync(f.owner.root, {recursive: true}); fs.rmSync(f.directory, {recursive: true}); }
});

test('qualification refuses an unsafe shared temporary parent before creating a root', t => {
    const directory = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-qualification-temp-parent-'));
    t.diagnostic(`attempt ${directory}; retained on unexpected failure`);
    const parent = path.join(directory, 'shared'); fs.mkdirSync(parent); fs.chmodSync(parent, 0o777);
    const file = path.join(__dirname, 'qualify-worker-runner.js'), module = {exports: {}};
    const localFs = {...fs, realpathSync: Object.assign(p => fs.realpathSync(p), {native: () => parent})};
    vm.runInThisContext(`(function(require,module,exports,__dirname,__filename){\n${fs.readFileSync(file, 'utf8')}\n})`)(
        name => name === 'node:fs' ? localFs : require(name), module, module.exports, __dirname, file);
    assert.throws(() => module.exports.prepareQualificationTemporaryRoot(directory), /trusted owner and sticky bit/);
    assert.deepEqual(fs.readdirSync(parent), []);
    assert.deepEqual(fs.readdirSync(directory), ['shared']);
    fs.chmodSync(parent, 0o1777);
    const owner = module.exports.prepareQualificationTemporaryRoot(directory);
    assert.equal(path.dirname(owner.root), parent);
    module.exports.revalidateQualificationTemporaryRoot(owner);
    fs.rmSync(directory, {recursive: true});
});

test('qualification generation pins its short temp snapshot and manifest without executing full qualification', async t => {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-qualification-temp-forward-'));
    t.diagnostic(`attempt ${base}; retained on unexpected failure`);
    const directory = path.join(base, 'qualification'), file = path.join(__dirname, 'qualify-worker-runner.js');
    const source = fs.readFileSync(file, 'utf8'), module = {exports: {}};
    const stop = new Error('bounded control stops before real preflight suite execution');
    let observed;
    vm.runInThisContext(`(function(require,module,exports,__dirname,__filename){\n${source}\n})`)(name =>
        name === './preflight-capture' ? {capturePreflight: async request => { observed = request; throw stop; }} : require(name),
    module, module.exports, __dirname, file);
    await assert.rejects(module.exports.qualifyWorkerRunner({directory, basePath: file, productPath: file,
        candidatePath: file, roadmapPath: file}), error => error === stop);
    assert.ok(observed, 'actual inventory generation must reach the preflight boundary');
    const record = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, 'temporary-root.json')));
    const snapshot = path.join(record.path, 'roadmap-snapshot.md');
    assert.equal(observed.environment.TMPDIR, record.path);
    assert.equal(observed.environment.LC_OFFSET_ROADMAP_SNAPSHOT, snapshot);
    assert.ok(observed.inputPaths.includes(path.join(directory, 'temporary-root.json')));
    assert.ok(observed.inputPaths.includes(snapshot));
    assert.deepEqual(fs.readFileSync(snapshot), fs.readFileSync(file));
    const definitions = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, 'attempt-1', 'suite-definitions.json')), {maxBytes: 1048576});
    assert.equal(definitions.environment.TMPDIR, record.path);
    assert.equal(definitions.environment.LC_OFFSET_ROADMAP_SNAPSHOT, snapshot);
    assert.ok(definitions.inputs.some(row => row.path === snapshot && row.sha256 === h1.sha256(fs.readFileSync(file))));
    const formal = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, 'attempt-1', 'formal-config.json')), {maxBytes: 1048576});
    const inputs = h2.captureInputs(formal.preflight);
    const independentlyDerived = require('./independent-evidence-checker').deriveInputManifest(formal.preflight);
    assert.deepEqual(independentlyDerived, inputs.manifest, 'the independent audit must reconstruct the actual additional input and environment');
    h2.revalidateInputs(inputs);
    const failure = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, 'attempt-1', 'capture-1-failure.json')));
    assert.equal(failure.status, 'FAIL'); assert.match(failure.message, /bounded control stops/);
    assert.equal(fs.existsSync(path.join(directory, 'qualification.json')), false);
    fs.rmSync(record.path, {recursive: true}); fs.rmSync(base, {recursive: true});
});

test('qualification journals the real failed pipeline before a changed temp-root boundary refuses', async t => {
    const driverPath = path.join(__dirname, 'qualify-worker-runner.js');
    const pipelinePath = path.join(__dirname, 'run-worker-pipeline.js');
    const pipeline = require('./run-worker-pipeline');
    const f = await setup(t, {captureOnly: true, workerMode: 'fail', extraSources: [driverPath, pipelinePath]});
    const root = path.join(f.base, 'qualification-boundary'); fs.mkdirSync(root, {mode: 0o700});
    const source = fs.readFileSync(driverPath, 'utf8');
    const begin = '        const result = await runWorkerPipeline(', end = '        const semantics =';
    const first = source.indexOf(begin), last = source.indexOf(end, first);
    assert.ok(first >= 0 && last > first && source.indexOf(begin, first + 1) === -1,
        'exercise the unique production pipeline publication block, not a copied implementation');
    let actual, prefix, prefixIdentity, temporary, calls = 0;
    const wrapped = {...pipeline, async runWorkerPipeline(request) {
        actual = await pipeline.runWorkerPipeline(request); calls++;
        assert.equal(actual.status, 'FAIL'); assert.equal(actual.phase, 'H3');
        assert.equal(actual.reason, 'WORKER_FAILED'); assert.equal(actual.h3.worker_launches, 1);
        const artifact = path.join(f.request.runRoot, 'a.json');
        prefix = fs.readFileSync(artifact); prefixIdentity = h2.statIdentity(fs.lstatSync(artifact, {bigint: true}));
        fs.chmodSync(temporary.root, 0o755);
        return actual;
    }};
    const module = {exports: {}};
    // Run the actual production block and real H2/H3 pipeline; instrument only
    // the return boundary. This is a synthetic refusal control, not qualification.
    const invoke = `${source}\nmodule.exports = {...module.exports, boundary: async function({formalPath, formal, receiptPaths,
        ledgerParent, checkerPath, evidenceParent, captures, temporary, root, ordinal}) {\n${source.slice(first, last)}\n}};\n`;
    vm.runInThisContext(`(function(require,module,exports,__dirname,__filename){\n${invoke}\n})`)(
        name => name === './run-worker-pipeline' ? wrapped : require(name), module, module.exports, __dirname, driverPath);
    temporary = module.exports.prepareQualificationTemporaryRoot(root);
    t.diagnostic(`temporary root ${temporary.root}; retained on unexpected failure`);
    const receipts = f.request.receiptPaths.map(p => ({path: p, bytes: fs.readFileSync(p), identity: h2.statIdentity(fs.lstatSync(p, {bigint: true}))}));
    await assert.rejects(module.exports.boundary({formalPath: f.configPath, formal: f.config, receiptPaths: f.request.receiptPaths,
        ledgerParent: f.ledgerParent, checkerPath: f.checkerConfigPath, evidenceParent: f.evidenceParent,
        captures: f.captures, temporary, root, ordinal: 1}), /directory identity changed/);
    assert.equal(calls, 1);
    const journalPath = path.join(root, 'pipeline-result.json'), journal = fs.readFileSync(journalPath);
    assert.deepEqual(journal, h1.canonicalJson(actual), 'preserve the exact actual pipeline result before rejecting its later boundary');
    assert.equal(fs.lstatSync(journalPath).mode & 0o7777, 0o400);
    const recorded = h1.parseCanonicalJson(journal, {maxBytes: 16777216});
    assert.equal(recorded.status, 'FAIL'); assert.equal(recorded.reason, 'WORKER_FAILED');
    assert.equal(recorded.h3.worker_launches, 1); assert.equal(recorded.ledger, null); assert.equal(recorded.evidence, null);
    assert.deepEqual(fs.readdirSync(f.request.runRoot), ['a.json']);
    assert.deepEqual(fs.readFileSync(path.join(f.request.runRoot, 'a.json')), prefix);
    assert.equal(h2.statIdentity(fs.lstatSync(path.join(f.request.runRoot, 'a.json'), {bigint: true})), prefixIdentity);
    for (const row of receipts) {
        assert.deepEqual(fs.readFileSync(row.path), row.bytes);
        assert.equal(h2.statIdentity(fs.lstatSync(row.path, {bigint: true})), row.identity);
    }
    assert.deepEqual(fs.readdirSync(f.ledgerParent), []);
    assert.deepEqual(fs.readdirSync(f.evidenceParent), []);
    assert.equal(fs.existsSync(path.join(root, 'qualification.json')), false);
    assert.throws(() => module.exports.revalidateQualificationTemporaryRoot(temporary), /directory identity changed/);
    fs.rmSync(temporary.root, {recursive: true}); fs.rmSync(f.base, {recursive: true});
});
