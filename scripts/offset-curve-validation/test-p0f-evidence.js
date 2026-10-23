/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const {spawn} = require('node:child_process');
const test = require('node:test');
const {EventEmitter} = require('node:events');
const {PassThrough, Writable} = require('node:stream');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const h4 = require('./formal-ledger');
const evidence = require('./p0f-evidence');
const descriptors = require('./evidence-descriptors');
const {tree} = require('./fixtures/transaction-fixture');
const {setup, checkerPath} = require('./fixtures/evidence-transaction');
const clone = v => JSON.parse(h1.canonicalJson(v));
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

const controlRunner = path.join(__dirname, 'fixtures', 'genuine-control-child.js');
async function setupArtifactControl(t, mode = 'pass', options = {}) {
    const captureEvidence = options.captureEvidence === undefined ? true : options.captureEvidence;
    const controls = {...options, captureRunner: controlRunner, captureEvidence};
    if (mode === 'pass') return setup(t, controls);
    const f = await setup(t, {...controls, execute: false});
    f.config.preflight.runnerArgs = [mode, ...(captureEvidence ? ['--genuine-preflight-evidence'] : [])];
    fs.writeFileSync(f.configPath, h1.canonicalJson(f.config));
    const {capturePreflight} = require('./preflight-capture');
    for (let i = 0; i < 2; i++) f.captures.push(await capturePreflight({...f.config.preflight,
        receiptId: f.config.receipt_ids[i], runRoot: f.receiptRoots[i], receiptDirectory: f.receiptDirs[i]}));
    f.h3 = await require('./formal-runner').runFormalTransaction(f.request);
    assert.equal(f.h3.status, 'PASS', f.h3.message);
    f.ledgerContext = h4.captureLedgerContext({configPath: f.configPath, receiptPaths: f.request.receiptPaths});
    f.ledgerDocument = h4.buildJoinedLedger(f.h3, f.ledgerContext);
    f.ledgerReadback = h4.publishJoinedLedger(f.ledgerDocument, f.ledgerPath);
    Object.assign(f.evidenceRequest, {h3Result: f.h3, ledgerReadback: f.ledgerReadback});
    return f;
}
function artifactEnvelopes(f) {
    return f.request.receiptPaths.map(p => {
        const receipt = JSON.parse(fs.readFileSync(p));
        return h1.parseCanonicalJson(fs.readFileSync(receipt.stderr.path));
    });
}
function artifactProtected(f) {
    return [f.configPath, f.registryPath, f.checkerConfigPath, f.candidatePath, f.inventoryPath, f.raw,
        ...f.sourcePaths, ...f.receiptDirs, ...f.receiptRoots.map(p => path.dirname(p)),
        f.config.run_root, path.dirname(f.ledgerPath)].flatMap(p => tree(p));
}
function syntheticCheckerPass(context) {
    return {schema_version: 1, purpose: 'synthetic', status: 'PASS', manifest_sha256: context.manifest.sha256,
        ledger_sha256: context.ledger.sha256, source_manifest_sha256: context.source_manifest_sha256,
        case_ids: context.case_ids, output_count: context.output_count,
        checks: context.required_checks.map(id => ({id, status: 'PASS'}))};
}
function artifactFailure(error) { return error instanceof evidence.EvidenceError && error.reason === 'PREFLIGHT_ARTIFACT_CHANGED'; }

test('synthetic envelope control pins two actual 28-file artifact roots through a real checker process', async t => {
    const f = await setupArtifactControl(t), before = tree(f.base), environment = env();
    assert.equal(f.captureRunner, controlRunner); assert.ok(f.sourcePaths.includes(controlRunner));
    const envelopes = artifactEnvelopes(f);
    assert.equal(envelopes.length, 2);
    for (const [i, envelope] of envelopes.entries()) {
        assert.equal(envelope.artifacts.length, 28); assert.equal(envelope.context_sha256, f.captures[i].stdin.sha256);
        assert.match(envelope.limitations[0], /Synthetic envelope control/);
        const root = path.dirname(envelope.artifacts[0].path);
        assert.deepEqual(fs.readdirSync(root).sort(), envelope.artifacts.map(row => path.basename(row.path)).sort());
        for (const row of envelope.artifacts) {
            assert.equal(fs.lstatSync(row.path).mode & 0o7777, 0o400);
            assert.equal(fs.readFileSync(row.path).length, row.bytes); assert.equal(h1.sha256(fs.readFileSync(row.path)), row.sha256);
        }
    }
    const context = evidence.captureEvidenceContext(f.evidenceRequest), previous = evidence.publishEvidenceManifest(context);
    assert.equal(evidence.readEvidenceManifest(f.evidenceRequest.manifestPath, context).manifest_sha256, previous.manifest_sha256);
    const result = await evidence.runEvidenceChecker(previous);
    assert.equal(result.status, 'PASS', result.message); assert.equal(result.dispatch.mode, 'process');
    assert.equal(result.checker_launches, 1); assert.deepEqual(result.verification_failures, []);
    assert.equal(evidence.revalidateEvidenceManifest(result), result);
    assert.deepEqual(tree(f.base).filter(row => !row.path.startsWith(f.evidenceParent)), before.filter(row => !row.path.startsWith(f.evidenceParent)));
    assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const mode of ['missing', 'wrong-hash', 'wrong-size', 'hardlink', 'symlink', 'mode', 'extra-file', 'duplicate-path',
    'wrong-context', 'wrong-purpose', 'extra-key', 'oversized', 'too-many', 'input-overlap', 'h3-overlap', 'h4-overlap',
    'h5-overlap', 'parent-link', 'noncanonical', 'duplicate-key', 'total-cap', 'combined-count'])
    test(`bound preflight artifact control ${mode} is refused before evidence publication or checker dispatch`, async t => {
        const f = await setupArtifactControl(t, mode), before = tree(f.base), environment = env();
        let launches = 0;
        const result = await evidence.runP0fEvidence(f.evidenceRequest, {dispatch() { launches++; throw new Error('must not dispatch'); }});
        expectRefused(result, 'PREFLIGHT_ARTIFACT_CHANGED', f); assert.equal(result.stage, 'CONTEXT');
        if (mode === 'total-cap') assert.match(result.message, /total byte cap/);
        assert.equal(launches, 0); assert.deepEqual(tree(f.base), before); assert.deepEqual(env(), environment);
        fs.rmSync(f.base, {recursive: true, force: true});
    });

test('original preflight artifact bytes are rechecked before publication without creating an evidence root', async t => {
    const f = await setupArtifactControl(t), protectedBefore = artifactProtected(f), environment = env();
    const context = evidence.captureEvidenceContext(f.evidenceRequest), p = artifactEnvelopes(f)[0].artifacts[0].path;
    rewrite(p, Buffer.from('substituted after context capture\n'));
    assert.throws(() => evidence.publishEvidenceManifest(context), artifactFailure);
    assert.equal(fs.existsSync(path.dirname(f.evidenceRequest.manifestPath)), false);
    assert.equal(fs.readFileSync(p, 'utf8'), 'substituted after context capture\n');
    assert.deepEqual(artifactProtected(f), protectedBefore); assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const mutation of ['bytes', 'inode', 'mode', 'hardlink', 'membership', 'parent-link'])
    test(`issued preflight artifact ${mutation} mutation cannot be blessed by manifest readback or dispatched`, async t => {
        const f = await setupArtifactControl(t), protectedBefore = artifactProtected(f), environment = env();
        const context = evidence.captureEvidenceContext(f.evidenceRequest), previous = evidence.publishEvidenceManifest(context);
        const p = artifactEnvelopes(f)[0].artifacts[0].path, root = path.dirname(p), bytes = fs.readFileSync(p);
        if (mutation === 'bytes') rewrite(p, Buffer.from('changed external bytes\n'));
        if (mutation === 'inode') { fs.renameSync(p, `${p}.original`); fs.writeFileSync(p, bytes, {mode: 0o400}); fs.unlinkSync(`${p}.original`); }
        if (mutation === 'mode') fs.chmodSync(p, 0o600);
        if (mutation === 'hardlink') fs.linkSync(p, path.join(root, 'alias'));
        if (mutation === 'membership') fs.writeFileSync(path.join(root, 'extra'), 'extra', {mode: 0o400});
        if (mutation === 'parent-link') { fs.renameSync(root, `${root}.original`); fs.symlinkSync(`${root}.original`, root); }
        const failedPrefix = tree(f.base);
        assert.throws(() => evidence.revalidateEvidenceManifest(previous), artifactFailure);
        assert.throws(() => evidence.readEvidenceManifest(f.evidenceRequest.manifestPath, context), artifactFailure);
        let launches = 0;
        const result = await evidence.runEvidenceChecker(previous, {dispatch() { launches++; throw new Error('must not dispatch'); }});
        assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'PREFLIGHT_ARTIFACT_CHANGED');
        assert.equal(result.stage, 'REVALIDATION'); assert.equal(result.checker_launches, 0); assert.equal(launches, 0);
        assert.deepEqual(tree(f.base), failedPrefix);
        assert.deepEqual(artifactProtected(f), protectedBefore); assert.deepEqual(env(), environment);
        fs.rmSync(f.base, {recursive: true, force: true});
    });

for (const failure of [false, true]) test(`synthetic checker artifact substitution ${failure ? 'with a thrown error' : 'with a claimed PASS'} cannot pass`, async t => {
    const f = await setupArtifactControl(t), protectedBefore = artifactProtected(f), environment = env();
    const p = artifactEnvelopes(f)[1].artifacts[27].path;
    let launches = 0;
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {dispatch(context) {
        launches++; rewrite(p, Buffer.from('checker substituted external artifact\n'));
        if (failure) throw new Error('synthetic checker failed after substitution');
        return syntheticCheckerPass(context);
    }});
    assert.equal(result.status, 'FAIL'); assert.equal(result.checker_launches, 1); assert.equal(launches, 1);
    assert.equal(result.reason, failure ? 'CHECKER_THROWN' : 'PREFLIGHT_ARTIFACT_CHANGED');
    assert.ok(result.verification_failures.some(row => row.phase === (failure ? 'dispatch-failure' : 'post-dispatch') &&
        row.check === 'preflight_artifacts' && row.reason === 'PREFLIGHT_ARTIFACT_CHANGED'));
    assert.equal(fs.readFileSync(p, 'utf8'), 'checker substituted external artifact\n');
    assert.ok(result.review_path); assert.equal(JSON.parse(fs.readFileSync(result.review_path)).status, 'RECORDED');
    assert.throws(() => evidence.revalidateEvidenceManifest(result), artifactFailure);
    assert.deepEqual(artifactProtected(f), protectedBefore); assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('a failed real checker still rechecks original external preflight artifacts after dispatch', async t => {
    const f = await setupArtifactControl(t, 'pass', {checkerMode: 'fail'}), protectedBefore = artifactProtected(f), environment = env();
    const p = artifactEnvelopes(f)[0].artifacts[0].path;
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {spawn(...args) {
        const child = spawn(...args);
        child.on('close', () => rewrite(p, Buffer.from('changed after real checker failure\n')));
        return child;
    }});
    assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'PREFLIGHT_ARTIFACT_CHANGED');
    assert.equal(result.checker_launches, 1); assert.equal(result.dispatch.mode, 'process');
    assert.ok(result.verification_failures.some(row => row.phase === 'post-dispatch' && row.check === 'preflight_artifacts'));
    assert.deepEqual(artifactProtected(f), protectedBefore); assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const target of ['manifest.json', 'review.json']) test(`external preflight artifact substitution during ${target} readback cannot pass`, async t => {
    const f = await setupArtifactControl(t), protectedBefore = artifactProtected(f), environment = env();
    const p = artifactEnvelopes(f)[0].artifacts[0].path, paths = new Map(); let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(file, ...args) { const fd = fs.openSync(file, ...args); paths.set(fd, file); return fd; },
        closeSync(fd) { paths.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const n = fs.readSync(fd, buffer, offset, length, position);
            if (!changed && n > 0 && paths.get(fd) === path.join(path.dirname(f.evidenceRequest.manifestPath), target)) {
                changed = true; rewrite(p, Buffer.from('external substitution during readback\n'));
            }
            return n;
        }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io});
    assert.equal(changed, true); assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'PREFLIGHT_ARTIFACT_CHANGED');
    assert.equal(result.checker_launches, target === 'manifest.json' ? 0 : 1);
    assert.equal(fs.existsSync(f.evidenceRequest.manifestPath), true);
    assert.equal(fs.readFileSync(p, 'utf8'), 'external substitution during readback\n');
    assert.deepEqual(artifactProtected(f), protectedBefore); assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('final metadata checks catch external artifact changes after its content recheck', async t => {
    const f = await setupArtifactControl(t), protectedBefore = artifactProtected(f), environment = env();
    const p = artifactEnvelopes(f)[0].artifacts[0].path, paths = new Map(); let armed = false, changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(file, ...args) { const fd = fs.openSync(file, ...args); paths.set(fd, file); return fd; },
        closeSync(fd) { paths.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const n = fs.readSync(fd, buffer, offset, length, position);
            if (armed && !changed && n > 0 && paths.get(fd) === f.ledgerPath) {
                changed = true; rewrite(p, Buffer.from('substitution after artifact content check\n'));
            }
            return n;
        }};
    const result = await evidence.runP0fEvidence(f.evidenceRequest, {fs: io, dispatch(context) {
        armed = true; return syntheticCheckerPass(context);
    }});
    assert.equal(changed, true); assert.equal(result.status, 'FAIL'); assert.equal(result.reason, 'PREFLIGHT_ARTIFACT_CHANGED');
    assert.equal(result.checker_launches, 1);
    assert.equal(result.verification_failures[0].phase, 'post-dispatch');
    assert.equal(result.verification_failures[0].check, 'final_preflight_artifacts');
    assert.equal(fs.readFileSync(p, 'utf8'), 'substitution after artifact content check\n');
    assert.deepEqual(artifactProtected(f), protectedBefore); assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const mode of ['ordinary-text', 'ordinary-marker', 'ordinary-json-marker'])
    test(`unflagged ${mode} stderr remains ordinary through real H2/H3/H4 and H5 checker execution`, async t => {
        const f = await setupArtifactControl(t, mode, {captureEvidence: false}), before = tree(f.base), environment = env();
        assert.equal(f.captureEvidence, false);
        assert.deepEqual(f.config.preflight.runnerArgs, [mode]);
        const receipts = f.request.receiptPaths.map(p => h1.validateReceipt(p));
        for (const receipt of receipts) {
            assert.deepEqual(receipt.receipt.argv.slice(1), [controlRunner, mode]);
            const stderr = Buffer.from(receipt.snapshot.stderr.base64, 'base64');
            if (mode === 'ordinary-json-marker') assert.equal(h1.parseCanonicalJson(stderr).kind, 'ordinary-control');
            else assert.throws(() => h1.parseCanonicalJson(stderr));
            if (mode !== 'ordinary-text') assert.ok(stderr.includes(Buffer.from('genuine-preflight-evidence')));
        }
        const result = await evidence.runP0fEvidence(f.evidenceRequest);
        assert.equal(result.status, 'PASS', result.message); assert.equal(result.reason, 'H5_EVIDENCE_COMPLETE');
        assert.equal(result.checker_launches, 1); assert.equal(result.dispatch.mode, 'process');
        assert.deepEqual(result.verification_failures, []);
        assert.equal(evidence.revalidateEvidenceManifest(result), result);
        assert.deepEqual(tree(f.base).filter(row => !row.path.startsWith(f.evidenceParent)), before.filter(row => !row.path.startsWith(f.evidenceParent)));
        assert.deepEqual(env(), environment);
        fs.rmSync(f.base, {recursive: true, force: true});
    });

for (const mode of ['ordinary-marker', 'ordinary-json-marker'])
    test(`pinned evidence argv flag requires a valid envelope for ${mode} stderr`, async t => {
        const f = await setupArtifactControl(t, mode), before = tree(f.base), environment = env();
        assert.equal(f.captureEvidence, true);
        assert.deepEqual(f.config.preflight.runnerArgs, [mode, '--genuine-preflight-evidence']);
        for (const p of f.request.receiptPaths)
            assert.deepEqual(h1.validateReceipt(p).receipt.argv.slice(1), [controlRunner, mode, '--genuine-preflight-evidence']);
        let launches = 0;
        const result = await evidence.runP0fEvidence(f.evidenceRequest, {dispatch() { launches++; throw new Error('must not dispatch'); }});
        expectRefused(result, 'PREFLIGHT_ARTIFACT_CHANGED', f); assert.equal(result.stage, 'CONTEXT'); assert.equal(launches, 0);
        assert.deepEqual(tree(f.base), before); assert.deepEqual(env(), environment);
        fs.rmSync(f.base, {recursive: true, force: true});
    });

for (const mode of ['pass', 'wrong-hash'])
    test(`unflagged canonical evidence kind self opts in for ${mode} artifact controls`, async t => {
        const f = await setupArtifactControl(t, mode, {captureEvidence: false}), before = tree(f.base), environment = env();
        assert.equal(f.captureEvidence, false);
        assert.deepEqual(f.config.preflight.runnerArgs, [mode]);
        assert.ok(artifactEnvelopes(f).every(envelope => envelope.kind === 'genuine-preflight-evidence'));
        const result = await evidence.runP0fEvidence(f.evidenceRequest);
        if (mode === 'pass') {
            assert.equal(result.status, 'PASS', result.message); assert.equal(result.checker_launches, 1);
            assert.equal(result.dispatch.mode, 'process'); assert.deepEqual(result.verification_failures, []);
            assert.equal(evidence.revalidateEvidenceManifest(result), result);
        } else {
            expectRefused(result, 'PREFLIGHT_ARTIFACT_CHANGED', f); assert.equal(result.stage, 'CONTEXT');
        }
        assert.deepEqual(tree(f.base).filter(row => !row.path.startsWith(f.evidenceParent)), before.filter(row => !row.path.startsWith(f.evidenceParent)));
        assert.deepEqual(env(), environment);
        fs.rmSync(f.base, {recursive: true, force: true});
    });

test('synthetic envelope control rejects oversized stdin before parsing or creating artifacts', async t => {
    const f = await setup(t, {captureRunner: controlRunner, execute: false}), before = tree(f.base), environment = env();
    const child = require('node:child_process').spawnSync(process.execPath, [controlRunner, 'pass'], {
        cwd: f.base, env: f.config.preflight.environment, input: Buffer.alloc(32769, 0x20), timeout: 3000, maxBuffer: 65536});
    assert.equal(child.error, undefined); assert.equal(child.status, 1); assert.equal(child.signal, null);
    assert.equal(child.stdout.length, 0); assert.match(child.stderr.toString(), /control stdin context exceeds byte cap/);
    assert.deepEqual(tree(f.base), before); assert.deepEqual(env(), environment);
    fs.rmSync(f.base, {recursive: true, force: true});
});
