/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const vm = require('node:vm');
const h1 = require('./preflight-receipt');
const h5 = require('./p0f-evidence');
const pipeline = require('./run-worker-pipeline');
const {setup} = require('./fixtures/evidence-transaction');

const modulePath = path.join(__dirname, 'run-worker-pipeline.js');
const fixture = (t, options = {}) => setup(t, {captureOnly: true, extraSources: [modulePath], ...options});
const requestFor = f => ({...f.request, ledgerPath: f.ledgerPath, checkerConfigPath: f.checkerConfigPath,
    manifestPath: f.evidenceRequest.manifestPath, preflightCaptures: f.captures});
const argumentsFor = f => ['--run', '--config', f.configPath, '--run-dir', f.request.runRoot,
    ...f.request.receiptPaths.flatMap(p => ['--preflight-receipt', p]), '--ledger', f.ledgerPath,
    '--checker-config', f.checkerConfigPath, '--evidence-manifest', f.evidenceRequest.manifestPath];
const clean = f => fs.rmSync(f.base, {recursive: true});
const environment = () => JSON.stringify(Object.keys(process.env).sort().map(key => [key, process.env[key]]));

test('pipeline executes real workers, owned ledger and checker with actual capture snapshots', async t => {
    const f = await fixture(t), before = environment();
    const result = await pipeline.runWorkerPipeline(requestFor(f));
    assert.equal(result.status, 'PASS', result.message);
    assert.equal(result.phase, 'COMPLETE');
    assert.equal(result.exit_code, 0);
    assert.equal(result.completion_scope, 'automated-worker-evidence-only');
    assert.equal(result.h3.worker_launches, 2);
    assert.equal(result.ledger.status, 'PASS');
    assert.equal(result.evidence.checker_launches, 1);
    assert.equal(result.evidence.status, 'PASS');
    for (const p of [f.ledgerPath, f.evidenceRequest.manifestPath, result.evidence.review_path])
        assert.equal(fs.statSync(p).mode & 0o7777, 0o400);
    assert.equal(environment(), before);
    assert.ok(!fs.existsSync(f.receiptRoots[0]) && !fs.existsSync(f.receiptRoots[1]));
    clean(f);
});

test('pipeline real CLI forwards all arguments and labels unavailable observed transport', async t => {
    const f = await fixture(t), before = environment();
    const child = spawnSync(process.execPath, [modulePath, ...argumentsFor(f)], {encoding: 'utf8', maxBuffer: 16777216});
    assert.equal(child.status, 0, child.stdout + child.stderr);
    assert.equal(child.stderr, '');
    const result = h1.parseCanonicalJson(Buffer.from(child.stdout), {maxBytes: 16777216});
    assert.equal(result.status, 'PASS');
    assert.equal(result.h3.worker_launches, 2);
    assert.equal(result.evidence.checker_launches, 1);
    assert.ok(result.evidence.limitations.some(text => text.includes('derived, not observed transport')));
    assert.equal(environment(), before);
    clean(f);
});

test('pipeline source must be pinned before any worker or output creation', async t => {
    const f = await setup(t, {captureOnly: true});
    const result = await pipeline.runWorkerPipeline(requestFor(f));
    assert.equal(result.status, 'REFUSED');
    assert.equal(result.reason, 'SOURCE_UNPINNED');
    assert.equal(result.h3, null);
    assert.ok(!fs.existsSync(f.request.runRoot));
    assert.ok(!fs.existsSync(path.dirname(f.ledgerPath)));
    assert.ok(!fs.existsSync(f.evidenceRequest.manifestPath));
    clean(f);
});

test('pipeline retains real worker failure prefix and never invokes ledger or checker', async t => {
    const f = await fixture(t, {workerMode: 'fail'});
    const result = await pipeline.runWorkerPipeline(requestFor(f));
    assert.equal(result.status, 'FAIL');
    assert.equal(result.phase, 'H3');
    assert.equal(result.reason, 'WORKER_FAILED');
    assert.equal(result.h3.worker_launches, 1);
    assert.equal(result.h3.root_created, true);
    assert.equal(result.ledger, null);
    assert.equal(result.evidence, null);
    assert.deepEqual(fs.readdirSync(f.request.runRoot), ['a.json']);
    assert.ok(!fs.existsSync(path.dirname(f.ledgerPath)));
    assert.ok(!fs.existsSync(f.evidenceRequest.manifestPath));
    clean(f);
});

test('pipeline preserves existing ledger sentinel and completed worker prefix on H4 refusal', async t => {
    const f = await fixture(t);
    fs.mkdirSync(path.dirname(f.ledgerPath), {mode: 0o700});
    const sentinel = path.join(path.dirname(f.ledgerPath), 'sentinel');
    fs.writeFileSync(sentinel, 'external ledger owner\n', {mode: 0o400});
    const stat = fs.statSync(sentinel, {bigint: true});
    const result = await pipeline.runWorkerPipeline(requestFor(f));
    assert.equal(result.status, 'FAIL');
    assert.equal(result.phase, 'H4');
    assert.equal(result.reason, 'PATH_EXISTS');
    assert.equal(result.h3.status, 'PASS');
    assert.equal(result.h3.worker_launches, 2);
    assert.equal(result.ledger, null);
    assert.equal(result.evidence, null);
    assert.equal(fs.readFileSync(sentinel, 'utf8'), 'external ledger owner\n');
    assert.equal(fs.statSync(sentinel, {bigint: true}).ino, stat.ino);
    assert.ok(!fs.existsSync(f.evidenceRequest.manifestPath));
    clean(f);
});

test('pipeline checker refusal retains original ledger and published evidence', async t => {
    const f = await fixture(t, {checkerMode: 'fail'});
    const result = await pipeline.runWorkerPipeline(requestFor(f));
    assert.equal(result.status, 'FAIL');
    assert.equal(result.phase, 'H5');
    assert.equal(result.h3.status, 'PASS');
    assert.equal(result.ledger.status, 'PASS');
    assert.equal(result.evidence.checker_launches, 1);
    assert.notEqual(result.evidence.status, 'PASS');
    assert.ok(fs.existsSync(f.ledgerPath));
    assert.ok(fs.existsSync(f.evidenceRequest.manifestPath));
    clean(f);
});

test('pipeline normalizes post-root H5 refusal to overall FAIL without losing its exact result', async t => {
    const f = await fixture(t);
    fs.mkdirSync(path.dirname(f.evidenceRequest.manifestPath), {mode: 0o700});
    const sentinel = path.join(path.dirname(f.evidenceRequest.manifestPath), 'external-sentinel');
    fs.writeFileSync(sentinel, 'external evidence owner\n', {mode: 0o400});
    const identity = fs.statSync(sentinel, {bigint: true}).ino;
    const result = await pipeline.runWorkerPipeline(requestFor(f));
    assert.equal(result.status, 'FAIL');
    assert.equal(result.exit_code, 1);
    assert.equal(result.phase, 'H5');
    assert.equal(result.h3.status, 'PASS');
    assert.equal(result.ledger.status, 'PASS');
    assert.equal(result.evidence.status, 'REFUSED');
    assert.equal(result.evidence.checker_launches, 0);
    assert.equal(fs.readFileSync(sentinel, 'utf8'), 'external evidence owner\n');
    assert.equal(fs.statSync(sentinel, {bigint: true}).ino, identity);
    clean(f);
});

test('pipeline final revalidation detects manifest mutation at the real H5 return boundary', async t => {
    const f = await fixture(t);
    const source = fs.readFileSync(modulePath, 'utf8');
    const module = {exports: {}};
    const load = vm.runInThisContext(`(function(require,module,exports,__dirname,__filename){\n${source}\n})`);
    let mutations = 0;
    const owner = {...h5, async runP0fEvidence(...args) {
        const actual = await h5.runP0fEvidence(...args);
        assert.equal(actual.status, 'PASS', actual.message);
        const manifest = h1.parseCanonicalJson(fs.readFileSync(actual.manifest_path), {maxBytes: 2097152});
        manifest.inputs.candidate.bytes++;
        fs.chmodSync(actual.manifest_path, 0o600);
        fs.writeFileSync(actual.manifest_path, h1.canonicalJson(manifest));
        fs.chmodSync(actual.manifest_path, 0o400);
        mutations++;
        return actual;
    }};
    // The actual H5 transaction/owner and all validators run. Only its return
    // boundary is instrumented; no production injection or fabricated PASS.
    load(name => name === './p0f-evidence' ? owner : require(name), module, module.exports, __dirname, modulePath);
    const result = await module.exports.runWorkerPipeline(requestFor(f));
    assert.equal(mutations, 1);
    assert.equal(result.status, 'FAIL');
    assert.equal(result.phase, 'H5');
    assert.equal(result.stage, 'REVALIDATION');
    assert.equal(result.reason, 'MANIFEST_CHANGED');
    assert.equal(result.h3.worker_launches, 2);
    assert.equal(result.evidence.status, 'PASS', 'retain the original invalidated result rather than rewriting its history');
    assert.equal(result.evidence.checker_launches, 1);
    assert.ok(fs.existsSync(result.evidence.manifest_path));
    assert.ok(fs.existsSync(result.evidence.review_path));
    assert.throws(() => h5.revalidateEvidenceManifest(result.evidence), {reason: 'MANIFEST_CHANGED'});
    clean(f);
});

test('pipeline rejects overlapping output scopes before dispatch', async t => {
    const f = await fixture(t);
    const result = await pipeline.runWorkerPipeline({...requestFor(f), ledgerPath: path.join(f.request.runRoot, 'events.json')});
    assert.equal(result.status, 'REFUSED');
    assert.equal(result.reason, 'PATH_POLICY');
    assert.equal(result.h3, null);
    assert.ok(!fs.existsSync(f.request.runRoot));
    clean(f);
});

test('pipeline request and capture getters are rejected without execution', async t => {
    let calls = 0;
    const input = {};
    Object.defineProperty(input, 'configPath', {get() { calls++; return '/unused'; }});
    const first = await pipeline.runWorkerPipeline(input);
    assert.equal(first.status, 'REFUSED');
    assert.equal(first.reason, 'ARG_INVALID');
    const f = await fixture(t), capture = {receiptPath: f.captures[0].receiptPath};
    Object.defineProperty(capture, 'stdin', {get() { calls++; return f.captures[0].stdin; }});
    const second = await pipeline.runWorkerPipeline({...requestFor(f), preflightCaptures: [capture, f.captures[1]]});
    assert.equal(second.status, 'REFUSED');
    assert.equal(second.reason, 'ARG_INVALID');
    assert.equal(calls, 0);
    assert.ok(!fs.existsSync(f.request.runRoot));
    clean(f);
});

test('pipeline rejects receipt array getters, unknown fields and all dependency hooks', async t => {
    const f = await fixture(t);
    let calls = 0;
    const receipts = [...f.request.receiptPaths];
    Object.defineProperty(receipts, '0', {get() { calls++; return f.request.receiptPaths[0]; }});
    assert.equal((await pipeline.runWorkerPipeline({...requestFor(f), receiptPaths: receipts})).reason, 'ARG_INVALID');
    assert.equal((await pipeline.runWorkerPipeline({...requestFor(f), unknown: true})).reason, 'ARG_INVALID');
    const hook = {};
    Object.defineProperty(hook, 'dispatch', {get() { calls++; return () => {}; }});
    assert.equal((await pipeline.runWorkerPipeline(requestFor(f), hook)).reason, 'ARG_INVALID');
    assert.equal((await pipeline.runWorkerPipeline(requestFor(f), {dispatch() { calls++; }})).reason, 'INJECTION_FORBIDDEN');
    assert.equal(calls, 0);
    assert.ok(!fs.existsSync(f.request.runRoot));
    clean(f);
});

test('pipeline CLI parser preserves shared H3 receipt cardinality and duplicate guards', () => {
    const argv = ['--run', '--config', '/config.json', '--run-dir', '/run',
        '--preflight-receipt', '/receipt-a.json', '--preflight-receipt', '/receipt-b.json',
        '--ledger', '/ledger/events.json', '--checker-config', '/checker.json', '--evidence-manifest', '/evidence/manifest.json'];
    assert.deepEqual(pipeline.parsePipelineArguments(argv).receiptPaths, ['/receipt-a.json', '/receipt-b.json']);
    assert.throws(() => pipeline.parsePipelineArguments([...argv, '--ledger', '/other/events.json']), {reason: 'ARG_DUPLICATE_OPTION'});
    assert.throws(() => pipeline.parsePipelineArguments(argv.slice(0, -2)), {reason: 'ARG_MISSING_VALUE'});
    assert.throws(() => pipeline.parsePipelineArguments([...argv, '--preflight-receipt', '/receipt-c.json']), {reason: 'RECEIPT_COUNT'});
    assert.throws(() => pipeline.parsePipelineArguments(argv.map(p => p === '/receipt-b.json' ? '/receipt-a.json' : p)), {reason: 'RECEIPT_PATH_DUPLICATE'});
    assert.throws(() => pipeline.parsePipelineArguments(argv.map(p => p === '/checker.json' ? 'relative.json' : p)), {reason: 'ARG_PATH_INVALID'});
});

test('pipeline real CLI refuses incomplete arguments without filesystem effects', () => {
    const child = spawnSync(process.execPath, [modulePath, '--run'], {encoding: 'utf8'});
    assert.equal(child.status, 2);
    const result = h1.parseCanonicalJson(Buffer.from(child.stdout));
    assert.equal(result.status, 'REFUSED');
    assert.equal(result.reason, 'ARG_MISSING_VALUE');
    assert.equal(child.stderr, '');
});
