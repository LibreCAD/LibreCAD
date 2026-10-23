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
const vm = require('node:vm');
const {spawn} = require('node:child_process');
const test = require('node:test');
const {EventEmitter} = require('node:events');
const {PassThrough, Writable} = require('node:stream');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {capturePreflight} = require('./preflight-capture');
const {runFormalTransaction} = require('./formal-runner');
const h4 = require('./formal-ledger');
const evidence = require('./p0f-evidence');
const descriptors = require('./evidence-descriptors');
const {tree} = require('./fixtures/transaction-fixture');
const workerPath = path.join(__dirname, 'fixtures', 'formal-child.js');
const checkerPath = path.join(__dirname, 'fixtures', 'evidence-checker.js');
const ids = ['10000000-0000-4000-8000-000000000001', '20000000-0000-4000-8000-000000000002'];
const clone = v => JSON.parse(h1.canonicalJson(v));

async function setup(t, {checkerMode = 'pass', execute = true, purpose = 'synthetic', omitSource,
    omitCaptures = false, omitCheckerConfig = false, timeout = 3000, streamCap = 65536, emptySource = false} = {}) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-h5-'));
    t.diagnostic(`attempt ${base}`);
    const configPath = path.join(base, 'formal-config.json'), registryPath = path.join(base, 'registry.json');
    const checkerConfigPath = path.join(base, 'checker-config.json'), candidatePath = path.join(base, 'candidate.md');
    const inventoryPath = path.join(base, 'cases.json'), raw = path.join(base, 'input.txt');
    const runParent = path.join(base, 'worker-parent'), ledgerParent = path.join(base, 'ledger-parent'), evidenceParent = path.join(base, 'evidence-parent');
    for (const p of [runParent, ledgerParent, evidenceParent]) fs.mkdirSync(p, {mode: 0o700});
    fs.writeFileSync(candidatePath, 'actual synthetic H5 candidate\n');
    fs.writeFileSync(raw, 'actual synthetic H5 input\n');
    const env = {LC_ALL: 'C', ...(process.platform === 'darwin' ? {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})};
    const checker = {schema_version: 1, purpose, module_path: checkerPath, args: [checkerMode], cwd: base,
        environment: env, timeout_ms: timeout, max_stream_bytes: streamCap};
    fs.writeFileSync(checkerConfigPath, h1.canonicalJson(checker));
    const workers = ['a', 'b'].map(name => ({id: `worker-${name}`, module_path: workerPath, args: ['pass'],
        case_ids: [`case-${name}-01`], outputs: [`${name}.json`], timeout_ms: 3000, max_stream_bytes: 65536}));
    fs.writeFileSync(registryPath, h1.canonicalJson({schema_version: 1, purpose, workers}));
    fs.writeFileSync(inventoryPath, h1.canonicalJson({schema_version: 1, purpose,
        cases: workers.flatMap(worker => worker.case_ids.map(id => ({id, source_path: workerPath})))}));
    const roots = [], receiptDirs = [];
    for (let i = 0; i < 2; i++) {
        const rootParent = path.join(base, `root-parent-${i}`), receiptParent = path.join(base, `receipt-parent-${i}`);
        fs.mkdirSync(rootParent, {mode: 0o700}); fs.mkdirSync(receiptParent, {mode: 0o700});
        roots.push(path.join(rootParent, 'reserved')); receiptDirs.push(path.join(receiptParent, 'receipt'));
    }
    const sourceNames = ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js', 'formal-ledger.js',
        'ledger-descriptors.js', 'p0f-evidence.js', 'evidence-descriptors.js'].filter(name => name !== omitSource);
    const sourcePaths = sourceNames.map(name => path.join(__dirname, name)).concat(workerPath, checkerPath);
    if (emptySource) {
        const p = path.join(base, 'empty-source.js'); fs.writeFileSync(p, ''); sourcePaths.push(p);
    }
    const preflight = {purpose, candidatePath, runnerPath: path.join(__dirname, 'fixtures', 'capture-child.js'), sourcePaths,
        inputPaths: [raw], fixturePaths: [], configPaths: [configPath, registryPath, ...(omitCheckerConfig ? [] : [checkerConfigPath])],
        caseInventoryPath: inventoryPath, runnerArgs: ['pass'], cwd: base, environment: env};
    const config = {schema_version: 1, purpose, registry_path: registryPath, run_root: path.join(runParent, 'run'),
        receipt_ids: ids, receipt_roots: roots, preflight};
    fs.writeFileSync(configPath, h1.canonicalJson(config));
    const captures = [];
    let h3 = null, ledgerReadback = null;
    const request = {configPath, runRoot: config.run_root, receiptPaths: receiptDirs.map((p, i) => path.join(p, `${ids[i]}.json`))};
    if (execute) {
        for (let i = 0; i < 2; i++) captures.push(await capturePreflight({...preflight,
            receiptId: ids[i], runRoot: roots[i], receiptDirectory: receiptDirs[i]}));
        h3 = await runFormalTransaction(request); assert.equal(h3.status, 'PASS', h3.message);
        const context = h4.captureLedgerContext({configPath, receiptPaths: request.receiptPaths});
        const document = h4.buildJoinedLedger(h3, context);
        ledgerReadback = h4.publishJoinedLedger(document, path.join(ledgerParent, 'ledger', 'joined-events.json'));
    }
    const evidenceRequest = {configPath, receiptPaths: request.receiptPaths, h3Result: h3, ledgerReadback,
        preflightCaptures: omitCaptures ? null : captures, checkerConfigPath,
        manifestPath: path.join(evidenceParent, 'attempt', 'manifest.json')};
    return {base, config, request, h3, captures, raw, ledgerReadback, evidenceParent, checkerConfigPath, evidenceRequest};
}
function env() { return Object.keys(process.env).sort().map(k => [k, process.env[k]]); }
function rewrite(p, bytes) {
    const mode = fs.statSync(p).mode & 0o7777;
    fs.chmodSync(p, 0o600); fs.writeFileSync(p, bytes); fs.chmodSync(p, mode);
}
function expectRefused(result, reason, f) {
    assert.equal(result.status, 'REFUSED', result.message); assert.equal(result.reason, reason, result.message);
    assert.equal(result.directory_created, false); assert.equal(result.checker_launches, 0);
    assert.equal(fs.existsSync(path.dirname(f.evidenceRequest.manifestPath)), false);
}

test('real H2/H3/H4 evidence publishes an absent manifest and actual automated process review', async t => {
    const f = await setup(t), before = tree(f.base), environment = env();
    const context = evidence.captureEvidenceContext(f.evidenceRequest);
    const previous = evidence.publishEvidenceManifest(context);
    assert.equal(previous.status, 'READY');
    assert.equal(evidence.revalidateEvidenceManifest(previous), previous);
    const reread = evidence.readEvidenceManifest(f.evidenceRequest.manifestPath, context);
    assert.equal(reread.manifest_sha256, previous.manifest_sha256);
    const result = await evidence.runEvidenceChecker(previous);
    assert.equal(result.status, 'PASS', result.message); assert.equal(result.reason, 'H5_EVIDENCE_COMPLETE');
    assert.equal(evidence.revalidateEvidenceManifest(result), result);
    assert.equal(result.checker_launches, 1); assert.equal(result.dispatch.mode, 'process');
    assert.equal(result.dispatch.child.exit_code, 0); assert.equal(result.dispatch.stdin.write_completed, true);
    assert.deepEqual(result.dispatch.argv, [process.execPath, checkerPath, 'pass']);
    const stderr = JSON.parse(Buffer.from(result.dispatch.stderr.base64, 'base64'));
    assert.deepEqual(stderr.environment, f.config.preflight.environment);
    assert.equal(stderr.stdin_sha256, result.dispatch.stdin.sha256);
    const manifest = JSON.parse(fs.readFileSync(f.evidenceRequest.manifestPath));
    assert.deepEqual(Object.keys(manifest), descriptors.SCHEMA.manifest_keys);
    assert.equal(manifest.inputs.files.length > 10, true);
    for (const row of manifest.inputs.files) {
        assert.equal(row.sha256, h1.sha256(fs.readFileSync(row.path)));
        if (row.roles.some(role => role !== 'input')) assert.ok(row.bytes > 0);
    }
    const transport = JSON.parse(fs.readFileSync(manifest.preflight_transport.path));
    transport.captures.forEach((row, i) => {
        assert.deepEqual(row.observed, f.captures[i].stdin);
        assert.equal(row.origin, 'caller-supplied-capture-snapshot');
        assert.deepEqual({...row.expected, write_completed: true}, row.observed);
    });
    assert.deepEqual(JSON.parse(fs.readFileSync(manifest.h3_diagnostics.path)), f.h3);
    assert.equal(manifest.ledger.sha256, f.ledgerReadback.sha256);
    const review = JSON.parse(fs.readFileSync(result.review_path));
    assert.deepEqual(Object.keys(review), descriptors.SCHEMA.review_keys);
    assert.equal(review.kind, 'automated-checker-review'); assert.equal(Object.hasOwn(review, 'reviewer'), false);
    assert.equal(review.status, 'RECORDED'); assert.equal(review.error, null);
    assert.equal(review.result.status, 'PASS'); assert.equal(review.manifest_sha256, previous.manifest_sha256);
    const root = path.dirname(f.evidenceRequest.manifestPath);
    assert.deepEqual(fs.readdirSync(root).sort(), ['h3-diagnostics.json', 'manifest.json', 'preflight-transport.json', 'review.json']);
    for (const name of fs.readdirSync(root)) { assert.equal(fs.statSync(path.join(root, name)).mode & 0o7777, 0o400); assert.equal(fs.statSync(path.join(root, name)).nlink, 1); }
    assert.equal(evidence.revalidateEvidenceManifest(previous), previous);
    assert.deepEqual(tree(f.base).filter(row => !row.path.startsWith(f.evidenceParent)), before.filter(row => !row.path.startsWith(f.evidenceParent)));
    assert.deepEqual(env(), environment);
    assert.deepEqual(result.verification_failures, []);
    await assert.rejects(evidence.runEvidenceChecker(previous), e => e.reason === 'ALREADY_DISPATCHED');
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('unavailable preflight stdin is explicit and never fabricated as observed transport', async t => {
    const f = await setup(t, {omitCaptures: true});
    const result = await evidence.runP0fEvidence(f.evidenceRequest);
    assert.equal(result.status, 'PASS', result.message);
    assert.ok(result.limitations.some(s => s.includes('not supplied')));
    const rows = JSON.parse(fs.readFileSync(path.join(path.dirname(f.evidenceRequest.manifestPath), 'preflight-transport.json'))).captures;
    assert.equal(rows.every(row => row.observed === null && row.origin === 'unavailable'), true);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('inert evidence descriptors are immutable and contain no executable helpers', () => {
    assert.equal(Object.values(descriptors).some(v => typeof v === 'function'), false);
    assert.equal(Object.isFrozen(descriptors.SCHEMA.manifest_keys), true);
    assert.throws(() => descriptors.CHECKS.push('fake-review'), TypeError);
});

test('validation purpose rejects own/hidden/inherited fs, spawn and dispatch hooks without evaluating getters', async t => {
    const f = await setup(t, {execute: false, purpose: 'validation'}), before = tree(f.base);
    let reads = 0;
    for (const key of ['fs', 'spawn', 'dispatch']) for (const kind of ['own', 'hidden', 'inherited']) {
        const holder = Object.defineProperty({}, key, {enumerable: kind === 'own', get() { reads++; throw new Error('forbidden getter'); }});
        const dependencies = kind === 'inherited' ? Object.create(holder) : holder;
        expectRefused(await evidence.runP0fEvidence(f.evidenceRequest, dependencies), 'SYNTHETIC_INJECTION_FORBIDDEN', f);
    }
    const fsProxy = new Proxy({...h2.nodeFsAdapter}, {get(target, key) { reads++; return Reflect.get(target, key); }});
    expectRefused(await evidence.runP0fEvidence(f.evidenceRequest, {fs: fsProxy}), 'SYNTHETIC_INJECTION_FORBIDDEN', f);
    assert.equal(reads, 0); assert.deepEqual(tree(f.base), before);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('missing required H4 export is a named refusal, not an undefined helper or fake success', async t => {
    const f = await setup(t);
    const module = {exports: {}};
    const load = vm.runInThisContext(`(function(require, module, exports, __dirname) {\n${fs.readFileSync(path.join(__dirname, 'p0f-evidence.js'), 'utf8')}\n})`);
    load(name => name === './formal-ledger' ? {...h4, buildJoinedLedger: undefined} : require(name), module, module.exports, __dirname);
    const result = await module.exports.runP0fEvidence(f.evidenceRequest);
    assert.equal(result.status, 'REFUSED'); assert.equal(result.reason, 'DEPENDENCY_MISSING', result.message);
    assert.match(result.message, /buildJoinedLedger/);
    assert.equal(fs.existsSync(path.dirname(f.evidenceRequest.manifestPath)), false);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const source of ['p0f-evidence.js', 'evidence-descriptors.js']) test(`actual source closure requires ${source}`, async t => {
    const f = await setup(t, {omitSource: source});
    expectRefused(await evidence.runP0fEvidence(f.evidenceRequest), 'SOURCE_UNPINNED', f);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('checker configuration must be an actual captured config, not a caller-supplied tuple', async t => {
    const f = await setup(t, {omitCheckerConfig: true});
    expectRefused(await evidence.runP0fEvidence(f.evidenceRequest), 'CONFIG_UNPINNED', f);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('dummy H3 source/transport claims and cloned H4 readbacks cannot produce evidence', async t => {
    const f = await setup(t), before = tree(f.base);
    for (const mutate of [
        r => { r.h3Result = clone(r.h3Result); r.h3Result.workers[0].dispatch.worker_source.bytes = 0; },
        r => { r.h3Result = clone(r.h3Result); r.h3Result.workers[0].dispatch.stdin.base64 = Buffer.from('dummy').toString('base64'); },
        r => { r.ledgerReadback = clone(r.ledgerReadback); },
        r => { r.preflightCaptures = r.preflightCaptures.map(row => ({receiptPath: row.receiptPath, stdin: clone(row.stdin)})); r.preflightCaptures[0].stdin.write_completed = false; },
        r => { r.preflightCaptures = [...r.preflightCaptures].reverse(); }
    ]) {
        const request = {...f.evidenceRequest}; mutate(request);
        const result = await evidence.runP0fEvidence(request);
        assert.equal(result.status, 'REFUSED', result.message); assert.equal(result.checker_launches, 0);
        assert.equal(fs.existsSync(path.dirname(request.manifestPath)), false);
    }
    assert.deepEqual(tree(f.base), before);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const [mode, reason] of [['empty', 'RESULT_MISSING'], ['malformed', 'RESULT_PARSE_FAILED'], ['missing-status', 'STATUS_MISSING'],
    ['unknown', 'STATUS_UNKNOWN'], ['fail', 'CHECKER_FAILED'], ['wrong-hash', 'RESULT_MISMATCH'], ['extra-check', 'CHECK_INVENTORY'],
    ['nonzero', 'CHILD_NOT_PASS'], ['signal', 'CHILD_NOT_PASS'], ['timeout', 'CHILD_NOT_PASS'], ['overflow', 'CHILD_NOT_PASS']])
    test(`real checker ${mode} cannot publish a successful review`, async t => {
        const f = await setup(t, {checkerMode: mode, timeout: mode === 'timeout' ? 100 : 3000, streamCap: mode === 'overflow' ? 1024 : 65536});
        const before = tree(path.dirname(f.ledgerReadback.ledger_path));
        const result = await evidence.runP0fEvidence(f.evidenceRequest);
        assert.equal(result.status, 'FAIL', result.message); assert.equal(result.reason, reason, result.message);
        assert.equal(result.checker_launches, 1); assert.equal(result.dispatch.mode, 'process');
        assert.deepEqual(tree(path.dirname(f.ledgerReadback.ledger_path)), before);
        const review = JSON.parse(fs.readFileSync(result.review_path));
        assert.equal(review.status, 'RECORDED'); assert.ok(review.error);
        assert.deepEqual(result.verification_failures, []);
        if (mode === 'timeout') assert.equal(result.dispatch.failure, 'checker timeout');
        if (mode === 'overflow') { assert.equal(result.dispatch.stdout.bytes, 1024); assert.ok(result.dispatch.observed.stdout > 1024); }
        fs.rmSync(f.base, {recursive: true, force: true});
    });

for (const [mode, check] of [['mutate-output', 'outputs'], ['mutate-input', 'inputs'], ['mutate-receipt', 'receipts'],
    ['mutate-ledger', 'ledger'], ['mutate-manifest', 'manifest']]) test(`actual checker ${mode} is detected after dispatch, including failure paths`, async t => {
    const f = await setup(t, {checkerMode: mode});
    const result = await evidence.runP0fEvidence(f.evidenceRequest);
    assert.equal(result.status, 'FAIL', result.message); assert.equal(result.stage, 'REVALIDATION');
    assert.equal(result.checker_launches, 1);
    assert.ok(result.verification_failures.some(row => row.check === check), JSON.stringify(result.verification_failures));
    if (result.review_path) assert.ok(JSON.parse(fs.readFileSync(result.review_path)).error);
    assert.equal(fs.existsSync(f.evidenceRequest.manifestPath), true);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('ENOENT is valid absence, EACCES is not, and existing manifest links are preserved', async t => {
    const f = await setup(t);
    const io = {...h2.nodeFsAdapter, lstatSync(p) {
        if (p === f.evidenceRequest.manifestPath) throw Object.assign(new Error('synthetic EACCES'), {code: 'EACCES'});
        return h2.nodeFsAdapter.lstatSync(p);
    }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    expectRefused(result, 'IO_ERROR', f); assert.equal(result.cause_code, 'EACCES');
    const root = path.dirname(f.evidenceRequest.manifestPath);
    fs.mkdirSync(root, {mode: 0o700});
    for (const type of ['symlink', 'hardlink', 'regular']) {
        const target = path.join(f.base, `external-${type}`); fs.writeFileSync(target, 'external', {mode: 0o400});
        if (type === 'symlink') fs.symlinkSync(target, f.evidenceRequest.manifestPath);
        else if (type === 'hardlink') fs.linkSync(target, f.evidenceRequest.manifestPath);
        else fs.writeFileSync(f.evidenceRequest.manifestPath, 'external', {mode: 0o400});
        const before = tree(root);
        const refused = await evidence.runP0fEvidence(f.evidenceRequest);
        assert.equal(refused.status, 'REFUSED'); assert.equal(refused.reason, 'PATH_EXISTS');
        assert.deepEqual(tree(root), before);
        fs.unlinkSync(f.evidenceRequest.manifestPath);
    }
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('manifest paths cannot overlap H3/H4 exact membership directories or protected inputs', async t => {
    const f = await setup(t);
    for (const root of [f.config.run_root, path.dirname(f.ledgerReadback.ledger_path), path.dirname(f.request.receiptPaths[0])]) {
        const before = tree(root);
        const result = await evidence.runP0fEvidence({...f.evidenceRequest, manifestPath: path.join(root, 'manifest.json')});
        assert.equal(result.status, 'REFUSED'); assert.equal(result.reason, 'PATH_POLICY', result.message);
        assert.deepEqual(tree(root), before);
    }
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('publication parent identity/link replacement and readback corruption preserve failed prefixes', async t => {
    const f = await setup(t);
    const context = evidence.captureEvidenceContext(f.evidenceRequest);
    fs.renameSync(f.evidenceParent, `${f.evidenceParent}.original`);
    fs.mkdirSync(f.evidenceParent, {mode: 0o700});
    assert.throws(() => evidence.publishEvidenceManifest(context), e => e.stage === 'REVALIDATION');
    fs.rmSync(f.evidenceParent, {recursive: true});
    fs.renameSync(`${f.evidenceParent}.original`, f.evidenceParent);
    const descriptors = new Map(); let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(p, ...args) { const fd = fs.openSync(p, ...args); descriptors.set(fd, p); return fd; },
        closeSync(fd) { descriptors.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const n = fs.readSync(fd, buffer, offset, length, position);
            if (n > 0 && descriptors.get(fd) === f.evidenceRequest.manifestPath) { buffer[offset] ^= 1; changed = true; }
            return n;
        }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    assert.equal(changed, true); assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'READBACK_MISMATCH');
    assert.equal(result.checker_launches, 0); assert.equal(fs.existsSync(f.evidenceRequest.manifestPath), true);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('late output substitution while reading the published review cannot return PASS', async t => {
    const f = await setup(t); const descriptors = new Map(); let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(p, ...args) { const fd = fs.openSync(p, ...args); descriptors.set(fd, p); return fd; },
        closeSync(fd) { descriptors.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const n = fs.readSync(fd, buffer, offset, length, position);
            if (!changed && n > 0 && descriptors.get(fd) === path.join(path.dirname(f.evidenceRequest.manifestPath), 'review.json')) {
                changed = true; rewrite(path.join(f.config.run_root, 'a.json'), Buffer.from('late substituted output'));
            }
            return n;
        }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    assert.equal(changed, true); assert.equal(result.status, 'FAIL'); assert.equal(result.stage, 'REVALIDATION');
    assert.ok(result.verification_failures.some(row => row.check === 'outputs'));
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('checker callback/process exceptions still recheck evidence and preserve diagnostics', async t => {
    const f = await setup(t);
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {dispatch() {
        rewrite(f.raw, Buffer.from('mutated on callback failure')); throw new Error('synthetic checker throw');
    }});
    assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'CHECKER_THROWN');
    assert.ok(result.verification_failures.some(row => row.check === 'inputs'));
    assert.match(result.dispatch.exception.message, /checker throw/);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('spawn failures, stdin errors and invalid child handles have bounded failure envelopes', async t => {
    for (const kind of ['spawn', 'invalid', 'stdin']) {
        const f = await setup(t);
        const result = await evidence.runP0fEvidence(f.evidenceRequest, {spawn() {
            if (kind === 'spawn') throw Object.assign(new Error('synthetic EACCES'), {code: 'EACCES'});
            if (kind === 'invalid') return {};
            const child = new EventEmitter();
            child.stdin = new Writable({write(bytes, enc, callback) { callback(Object.assign(new Error('synthetic EPIPE'), {code: 'EPIPE'})); }});
            child.stdout = new PassThrough(); child.stderr = new PassThrough(); child.kill = () => true;
            setImmediate(() => { child.stderr.end('checker stderr'); child.emit('close', 7, null); });
            return child;
        }});
        assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'CHILD_NOT_PASS'); assert.equal(result.checker_launches, 1);
        assert.ok(result.dispatch.child.error);
        fs.rmSync(f.base, {recursive: true, force: true});
    }
});

test('actual zero-byte source is refused by H2 before evidence publication', async t => {
    const f = await setup(t, {execute: false, emptySource: true});
    const before = tree(f.base);
    const result = await evidence.runP0fEvidence(f.evidenceRequest);
    assert.equal(result.status, 'REFUSED'); assert.match(result.message, /empty source/);
    assert.equal(result.checker_launches, 0); assert.deepEqual(tree(f.base), before);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const mutation of ['inode', 'mode', 'symlink', 'hardlink', 'membership']) test(`original manifest ${mutation} mutation cannot be blessed by a fresh read`, async t => {
    const f = await setup(t), context = evidence.captureEvidenceContext(f.evidenceRequest);
    const previous = evidence.publishEvidenceManifest(context), p = f.evidenceRequest.manifestPath, bytes = fs.readFileSync(p);
    if (mutation === 'inode') { fs.unlinkSync(p); fs.writeFileSync(p, bytes, {mode: 0o400}); }
    if (mutation === 'mode') fs.chmodSync(p, 0o600);
    if (mutation === 'symlink') { fs.renameSync(p, path.join(f.base, 'manifest-target')); fs.symlinkSync(path.join(f.base, 'manifest-target'), p); }
    if (mutation === 'hardlink') fs.linkSync(p, path.join(f.base, 'manifest-link'));
    if (mutation === 'membership') fs.writeFileSync(path.join(path.dirname(p), 'extra'), 'extra', {mode: 0o400});
    assert.throws(() => evidence.revalidateEvidenceManifest(previous));
    assert.throws(() => evidence.readEvidenceManifest(p, context));
    const result = await evidence.runEvidenceChecker(previous);
    assert.equal(result.status, 'FAIL'); assert.equal(result.checker_launches, 0);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('original H4 ledger identity is mandatory even when replacement bytes match', async t => {
    const f = await setup(t), p = f.ledgerReadback.ledger_path, bytes = fs.readFileSync(p);
    fs.unlinkSync(p); fs.writeFileSync(p, bytes, {mode: 0o400});
    const result = await evidence.runP0fEvidence(f.evidenceRequest);
    assert.equal(result.status, 'REFUSED'); assert.equal(result.checker_launches, 0);
    assert.equal(fs.existsSync(path.dirname(f.evidenceRequest.manifestPath)), false);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('exclusive publication preserves an external directory race winner', async t => {
    const f = await setup(t), root = path.dirname(f.evidenceRequest.manifestPath);
    let external;
    const io = {...h2.nodeFsAdapter, mkdirSync(p, options) {
        if (p === root) {
            fs.mkdirSync(p, options); fs.writeFileSync(path.join(p, 'external-sentinel'), 'external', {mode: 0o400});
            external = tree(p);
        }
        return fs.mkdirSync(p, options);
    }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    assert.equal(result.status, 'REFUSED'); assert.equal(result.reason, 'PATH_EXISTS');
    assert.equal(result.directory_created, false); assert.equal(result.checker_launches, 0);
    assert.ok(external); assert.deepEqual(tree(root), external);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('exclusive file publication never overwrites a late manifest', async t => {
    const f = await setup(t), target = f.evidenceRequest.manifestPath;
    let external;
    const io = {...h2.nodeFsAdapter, openSync(p, flags, ...args) {
        if (p === target && flags & fs.constants.O_CREAT) {
            fs.writeFileSync(p, 'external late file', {mode: 0o400, flag: 'wx'});
            external = fs.lstatSync(p, {bigint: true});
        }
        return fs.openSync(p, flags, ...args);
    }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'PATH_EXISTS'); assert.equal(result.checker_launches, 0);
    assert.ok(external); assert.equal(fs.readFileSync(target, 'utf8'), 'external late file');
    assert.equal(h2.statIdentity(fs.lstatSync(target, {bigint: true})), h2.statIdentity(external));
    assert.equal(result.partial_paths.includes(target), false, 'external file is not owned evidence');
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('a symlinked publication parent is refused without following or changing it', async t => {
    const f = await setup(t), alias = path.join(f.base, 'parent-alias');
    fs.symlinkSync(f.evidenceParent, alias);
    const before = tree(f.base);
    const result = await evidence.runP0fEvidence({...f.evidenceRequest, manifestPath: path.join(alias, 'attempt', 'manifest.json')});
    assert.equal(result.status, 'REFUSED'); assert.equal(result.checker_launches, 0);
    assert.deepEqual(tree(f.base), before);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('checker process failure does not suppress post-dispatch output checks', async t => {
    const f = await setup(t, {checkerMode: 'fail'});
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {spawn(...args) {
        const child = spawn(...args);
        child.on('close', () => rewrite(path.join(f.config.run_root, 'a.json'), Buffer.from('substitution after failed checker')));
        return child;
    }});
    assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'OUTPUT_CHANGED');
    assert.ok(result.verification_failures.some(row => row.phase === 'post-dispatch' && row.check === 'outputs'));
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('synthetic callback omission/status failures are explicit and callback wait is bounded', async t => {
    for (const [kind, reason] of [['missing', 'RESULT_MISSING'], ['status', 'STATUS_MISSING'], ['timeout', 'CHECKER_TIMEOUT']]) {
        const f = await setup(t, {timeout: 100});
        const result = await evidence.runP0fEvidence(f.evidenceRequest, {dispatch() {
            if (kind === 'missing') return undefined;
            if (kind === 'status') return {};
            return new Promise(() => {});
        }});
        assert.equal(result.status, 'FAIL'); assert.equal(result.reason, reason, result.message);
        assert.equal(result.dispatch.mode, 'synthetic-callback');
        fs.rmSync(f.base, {recursive: true, force: true});
    }
});

test('manifest pre-readback output substitution prevents dispatch and retains partial evidence', async t => {
    const f = await setup(t), paths = new Map(); let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(p, ...args) { const fd = fs.openSync(p, ...args); paths.set(fd, p); return fd; },
        closeSync(fd) { paths.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const n = fs.readSync(fd, buffer, offset, length, position);
            if (!changed && n > 0 && paths.get(fd) === f.evidenceRequest.manifestPath) {
                changed = true; rewrite(path.join(f.config.run_root, 'a.json'), Buffer.from('substitution before manifest readback'));
            }
            return n;
        }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    assert.equal(changed, true); assert.equal(result.status, 'FAIL'); assert.equal(result.checker_launches, 0);
    assert.ok(result.verification_failures.some(row => row.check === 'outputs'));
    assert.equal(fs.existsSync(f.evidenceRequest.manifestPath), true);
    fs.rmSync(f.base, {recursive: true, force: true});
});
