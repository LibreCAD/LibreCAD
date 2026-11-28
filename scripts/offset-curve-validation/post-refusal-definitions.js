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
const h1 = require('./preflight-receipt');
const h4 = require('./formal-ledger');
const h5 = require('./p0f-evidence');
const {setup, checkerPath} = require('./fixtures/evidence-transaction');
const {tree, statRecord, environment} = require('./fixtures/transaction-fixture');

// No node:test registration on import. These definitions belong in the parent's
// existing ID registry, once each. Exception protocols are not runner envelopes:
// status:null means the owned API throws, not that it returned FAIL or REFUSED.
const declarations = [
    ['H6-TREE-05a', 'Persisted report binding references different genuine report bytes', 'H5.readEvidenceManifest', 'post-evidence',
        'throw', null, 'READBACK', 'MANIFEST_MISMATCH', 'EvidenceError'],
    ['H6-TREE-05b', 'Persisted report binding hash substitution', 'H5.readEvidenceManifest', 'post-evidence',
        'throw', null, 'READBACK', 'MANIFEST_MISMATCH', 'EvidenceError'],
    ['H6-TREE-05c', 'Persisted report binding size substitution', 'H5.readEvidenceManifest', 'post-evidence',
        'throw', null, 'READBACK', 'MANIFEST_MISMATCH', 'EvidenceError'],
    ['H6-JOIN-04', 'Self-consistent H3 output byte/hash claim differs from actual persisted output', 'H4.buildJoinedLedger', 'post-ledger',
        'throw', null, 'JOIN', 'OUTPUT_MISMATCH', 'LedgerError'],
    ['H6-EVIDENCE-01', 'Missing required H4 export through the actual H5 dependency guard', 'H5.runP0fEvidence', 'pre-evidence',
        'result', 'REFUSED', 'CONFIG', 'DEPENDENCY_MISSING', null],
    ['H6-EVIDENCE-02', 'Dummy H3 worker-source tuple', 'H5.runP0fEvidence', 'pre-evidence',
        'result', 'REFUSED', 'JOIN', 'DISPATCH_MISMATCH', null],
    ['H6-EVIDENCE-03', 'Absent manifest publication and real checker process succeed', 'H5.runP0fEvidence', 'post-evidence',
        'result', 'PASS', 'COMPLETE', 'H5_EVIDENCE_COMPLETE', null],
    ['H6-EVIDENCE-04', 'Actual checker substitutes an output after checking it', 'H5.runP0fEvidence', 'post-dispatch',
        'result', 'FAIL', 'REVALIDATION', 'OUTPUT_CHANGED', null],
    ['H6-POST-01', 'Persisted genesis mutation against original H4 readback', 'H4.revalidateJoinedLedger', 'post-ledger',
        'throw', null, 'READBACK', 'LEDGER_MISMATCH', 'LedgerError'],
    ['H6-POST-02', 'Persisted event-ledger mutation against original H4 readback', 'H4.revalidateJoinedLedger', 'post-ledger',
        'throw', null, 'READBACK', 'LEDGER_MISMATCH', 'LedgerError'],
    ['H6-POST-03', 'Persisted evidence manifest mutation against original H5 readback', 'H5.revalidateEvidenceManifest', 'post-evidence',
        'throw', null, 'REVALIDATION', 'MANIFEST_CHANGED', 'EvidenceError']
];
const DEFINITIONS = new Map(declarations.map(([id, condition, scope, boundaryPhase, protocol, status, stage, reason, exceptionName]) =>
    [id, Object.freeze({execution: 'post', fixture: 'evidence', condition, scope,
        phase: 'post-root', boundaryPhase, rootScope: scope.startsWith('H4.') ? 'ledger' : 'evidence',
        expected: Object.freeze({protocol, status, stage, reason, exceptionName}),
        execute: t => execute(id, t)})]));
assert.equal(DEFINITIONS.size, declarations.length, 'duplicate post-boundary identity');

const clone = value => JSON.parse(h1.canonicalJson(value));
const within = (p, root) => p === root || p.startsWith(`${root}${path.sep}`);

function snapshot(f, allowed = []) {
    return {tree: tree(f.base).filter(row => !allowed.some(p => within(row.path, p))),
        pinned: f.captures[0].inputs.manifest.files.filter(row => !within(row.path, f.base))
            .map(row => row.path).sort().map(statRecord), environment: environment()};
}
function rewrite(p, value) {
    const before = statRecord(p), mode = fs.lstatSync(p).mode & 0o7777;
    fs.chmodSync(p, 0o600); fs.writeFileSync(p, h1.canonicalJson(value)); fs.chmodSync(p, mode);
    return {path: p, before, after: statRecord(p)};
}
async function observe(fn) {
    try { return {protocol: 'result', result: await fn(), exception: null}; }
    catch (exception) { return {protocol: 'throw', result: null, exception}; }
}
function assertProtocol(observed, expected) {
    assert.equal(observed.protocol, expected.protocol, 'owned API protocol changed');
    if (observed.protocol === 'throw') {
        assert.equal(observed.exception.name, expected.exceptionName, observed.exception.message);
        assert.equal(observed.exception.stage, expected.stage, observed.exception.message);
        assert.equal(observed.exception.reason, expected.reason, observed.exception.message);
    } else {
        assert.equal(observed.result.status, expected.status, observed.result.message);
        assert.equal(observed.result.stage, expected.stage, observed.result.message);
        assert.equal(observed.result.reason, expected.reason, observed.result.message);
    }
}
function publishPrefix(f) {
    const context = h5.captureEvidenceContext(f.evidenceRequest);
    const previous = h5.publishEvidenceManifest(context);
    assert.equal(previous.status, 'READY');
    assert.equal(h5.revalidateEvidenceManifest(previous), previous);
    return {context, previous};
}
function withMissingExport() {
    const source = fs.readFileSync(path.join(__dirname, 'p0f-evidence.js'), 'utf8');
    const module = {exports: {}};
    const load = vm.runInThisContext(`(function(require, module, exports, __dirname) {\n${source}\n})`);
    // Only remove the named dependency. All remaining H1/H2/H4 validators and
    // the H5 guard are actual implementations; no global or PASS oracle changes.
    load(name => name === './formal-ledger' ? {...h4, buildJoinedLedger: undefined} : require(name),
        module, module.exports, __dirname);
    return module.exports;
}
function assertEvidenceFiles(f, result) {
    const root = path.dirname(f.evidenceRequest.manifestPath);
    assert.equal(fs.lstatSync(root).mode & 0o7777, 0o700);
    const names = ['h3-diagnostics.json', 'manifest.json', 'preflight-transport.json', 'review.json'];
    assert.deepEqual(fs.readdirSync(root).sort(), names);
    for (const name of names) {
        const stat = fs.lstatSync(path.join(root, name));
        assert.equal(stat.isFile(), true); assert.equal(stat.nlink, 1); assert.equal(stat.mode & 0o7777, 0o400);
    }
    const review = h1.parseCanonicalJson(fs.readFileSync(result.review_path), {maxBytes: 4194304});
    assert.equal(review.status, 'RECORDED', 'recorded checker data is not a fabricated overall PASS');
    assert.equal(review.kind, 'automated-checker-review');
    assert.equal(Object.hasOwn(review, 'reviewer'), false);
    assert.equal(result.dispatch.mode, 'process');
    assert.equal(result.dispatch.child.exit_code, 0);
    assert.equal(result.dispatch.child.signal, null);
    assert.equal(result.dispatch.child.error, null);
    assert.equal(result.dispatch.stdin.write_completed, true);
    const checker = h1.parseCanonicalJson(fs.readFileSync(f.checkerConfigPath));
    assert.deepEqual(result.dispatch.argv, [f.captures[0].inputs.runtime.executable_path, checkerPath, ...checker.args]);
    return review;
}

/** Execute one identity without registering a test or adapting a throw to FAIL.
 * Call returned cleanup() only after the parent's own protocol assertions pass.
 */
async function execute(id, t) {
    const definition = DEFINITIONS.get(id);
    assert.ok(definition, `unknown post-boundary identity: ${id}`);
    const f = await setup(t, id === 'H6-EVIDENCE-04' ? {checkerMode: 'mutate-output'} : {});
    const base = f.base;
    const originalH3 = h1.canonicalJson(f.h3);
    assert.equal(f.h3.status, 'PASS'); assert.equal(f.h3.worker_launches, 2);
    assert.equal(f.captures.length, 2);
    assert.equal(h4.revalidateJoinedLedger(f.ledgerReadback), f.ledgerReadback);
    const needsEvidence = id.startsWith('H6-TREE-05') || id === 'H6-POST-03';
    const prefix = needsEvidence ? publishPrefix(f) : null;
    const target = id.startsWith('H6-TREE-05') || id === 'H6-POST-03' ? f.evidenceRequest.manifestPath :
        id === 'H6-POST-01' || id === 'H6-POST-02' ? f.ledgerReadback.ledger_path :
            id === 'H6-EVIDENCE-04' ? path.join(f.config.run_root, 'a.json') : null;
    const createsEvidence = id === 'H6-EVIDENCE-03' || id === 'H6-EVIDENCE-04';
    const allowed = [...(target ? [target] : []), ...(createsEvidence ? [f.evidenceParent] : [])];
    const before = snapshot(f, allowed), beforeTree = tree(base);
    let mutation = null, afterMutation = null, checkerAttempts = 0, observed;
    const realSpawn = (...args) => {
        checkerAttempts++;
        assert.equal(args[0], f.captures[0].inputs.runtime.executable_path);
        assert.equal(args[1][0], checkerPath, 'no additional H3 worker may dispatch');
        return spawn(...args);
    };

    if (id.startsWith('H6-TREE-05')) {
        const manifest = clone(prefix.previous.manifest);
        if (id === 'H6-TREE-05a') manifest.receipts[0].report = clone(manifest.receipts[1].report);
        if (id === 'H6-TREE-05b') manifest.receipts[0].report.sha256 = h1.sha256(fs.readFileSync(f.request.receiptPaths[1]));
        if (id === 'H6-TREE-05c') manifest.receipts[0].report.bytes++;
        mutation = rewrite(target, manifest); afterMutation = tree(base);
        observed = await observe(() => h5.readEvidenceManifest(target, prefix.context));
    } else if (id === 'H6-JOIN-04') {
        const candidate = clone(f.h3), output = candidate.workers[0].outputs[0];
        const wrongBytes = Buffer.from(output.base64, 'base64');
        const offset = wrongBytes.indexOf(Buffer.from('worker-a'));
        assert.ok(offset >= 0);
        wrongBytes[offset + 'worker-'.length] = 'x'.charCodeAt(0);
        output.base64 = wrongBytes.toString('base64'); output.sha256 = h1.sha256(wrongBytes);
        assert.equal(output.byte_length, wrongBytes.length);
        assert.equal(output.sha256, h1.sha256(Buffer.from(output.base64, 'base64')));
        assert.notEqual(output.sha256, h1.sha256(fs.readFileSync(output.path)));
        const context = h4.captureLedgerContext({configPath: f.configPath, receiptPaths: f.request.receiptPaths});
        observed = await observe(() => h4.buildJoinedLedger(candidate, context));
    } else if (id === 'H6-EVIDENCE-01') {
        const owner = withMissingExport();
        observed = await observe(() => owner.runP0fEvidence(f.evidenceRequest, {spawn: realSpawn}));
    } else if (id === 'H6-EVIDENCE-02') {
        const candidate = clone(f.h3);
        candidate.workers[0].dispatch.worker_source.bytes = 0;
        observed = await observe(() => h5.runP0fEvidence({...f.evidenceRequest, h3Result: candidate}, {spawn: realSpawn}));
    } else if (createsEvidence) {
        assert.equal(fs.existsSync(f.evidenceRequest.manifestPath), false);
        const outputBefore = target ? statRecord(target) : null;
        observed = await observe(() => h5.runP0fEvidence(f.evidenceRequest, {spawn: realSpawn}));
        if (target) mutation = {path: target, before: outputBefore, after: statRecord(target)};
    } else if (id === 'H6-POST-01' || id === 'H6-POST-02') {
        const ledger = clone(f.ledgerReadback.ledger);
        if (id === 'H6-POST-01') ledger.genesis.candidate.bytes++;
        else ledger.rows[0].event.case_id = 'synthetic-substituted-case';
        mutation = rewrite(target, ledger); afterMutation = tree(base);
        observed = await observe(() => h4.revalidateJoinedLedger(f.ledgerReadback));
    } else if (id === 'H6-POST-03') {
        const manifest = clone(prefix.previous.manifest);
        manifest.inputs.candidate.bytes++;
        mutation = rewrite(target, manifest); afterMutation = tree(base);
        observed = await observe(() => h5.revalidateEvidenceManifest(prefix.previous));
    } else {
        assert.fail(`identity has no production execution: ${id}`);
    }

    const after = snapshot(f, allowed), afterTree = tree(base);
    assert.deepEqual(after, before, 'all original input trees, receipts, pinned sources/runtime, environment, and unmutated prefix files must remain identical');
    assert.deepEqual(h1.canonicalJson(f.h3), originalH3, 'the original H3 diagnostics must remain immutable');
    if (afterMutation) assert.deepEqual(afterTree, afterMutation, 'production refusal must preserve the deliberately mutated prefix without further writes');
    if (!target && !createsEvidence) assert.deepEqual(afterTree, beforeTree, 'pre-evidence/join refusals must not write any tree');
    assertProtocol(observed, definition.expected);
    const expectedAttempts = createsEvidence ? 1 : 0;
    assert.equal(checkerAttempts, expectedAttempts);
    if (observed.protocol === 'result') {
        assert.equal(observed.result.checker_launches, checkerAttempts);
        if (!createsEvidence) {
            assert.equal(observed.result.directory_created, false);
            assert.equal(fs.existsSync(path.dirname(f.evidenceRequest.manifestPath)), false);
        }
    }
    if (!needsEvidence && !createsEvidence) assert.equal(fs.existsSync(path.dirname(f.evidenceRequest.manifestPath)), false);
    if (needsEvidence) assert.deepEqual(fs.readdirSync(path.dirname(target)).sort(),
        ['h3-diagnostics.json', 'manifest.json', 'preflight-transport.json'], 'no checker review may be dispatched/published');
    if (id === 'H6-JOIN-04' || id.startsWith('H6-TREE-05') || id === 'H6-EVIDENCE-01' || id === 'H6-EVIDENCE-02' || id === 'H6-POST-03')
        assert.equal(h4.revalidateJoinedLedger(f.ledgerReadback), f.ledgerReadback, 'the genuine H4 prefix must remain valid');
    if (createsEvidence) {
        const review = assertEvidenceFiles(f, observed.result);
        if (id === 'H6-EVIDENCE-03') {
            assert.equal(review.error, null); assert.equal(review.result.status, 'PASS');
            assert.deepEqual(observed.result.verification_failures, []);
            assert.equal(h5.revalidateEvidenceManifest(observed.result), observed.result);
        } else {
            assert.ok(review.error);
            assert.ok(observed.result.verification_failures.some(row => row.phase === 'post-dispatch' && row.check === 'outputs'));
            const stdout = h1.parseCanonicalJson(Buffer.from(observed.result.dispatch.stdout.base64, 'base64'));
            assert.equal(stdout.status, 'PASS', 'hostile checker really claimed PASS before H5 refused');
            assert.notEqual(mutation.before.sha256, mutation.after.sha256);
            assert.deepEqual(mutation.before.metadata.slice(0, 4), mutation.after.metadata.slice(0, 4), 'substitution preserves inode/mode/link count, not content');
        }
    }
    if (mutation) assert.deepEqual(statRecord(mutation.path), mutation.after, 'the caller-owned mutation must be retained');
    assert.deepEqual(snapshot(f, allowed), before, 'final production readbacks must not mutate protected inputs');
    return {id, protocol: observed.protocol, exception: observed.exception, result: observed.result,
        observations: {artifacts: base, scope: definition.scope, boundary_phase: definition.boundaryPhase,
            roots: {h3: f.config.run_root, ledger: path.dirname(f.ledgerReadback.ledger_path), evidence: path.dirname(f.evidenceRequest.manifestPath)},
            h3_worker_launches: f.h3.worker_launches, checker_dispatch_attempts: checkerAttempts,
            original_trees_unchanged: true, original_h3_unchanged: true, environment_unchanged: true,
            environment_sha256: h1.sha256(h1.canonicalJson(before.environment)),
            protected_before: {tree: before.tree, pinned: before.pinned}, protected_after: {tree: after.tree, pinned: after.pinned},
            mutation, before_tree: beforeTree, after_tree: tree(base)},
        cleanup: () => fs.rmSync(base, {recursive: true, force: true})};
}

module.exports = Object.freeze({DEFINITIONS, execute});
