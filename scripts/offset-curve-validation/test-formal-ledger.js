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
const test = require('node:test');
const vm = require('node:vm');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {capturePreflight} = require('./preflight-capture');
const {runFormalTransaction} = require('./formal-runner');
const {resultFor, writeOutputs} = require('./fixtures/formal-child');
const ledger = require('./formal-ledger');
const {LIMITS, SCHEMA} = require('./ledger-descriptors');

const workerPath = path.join(__dirname, 'fixtures/formal-child.js');
const IDS = ['10000000-0000-4000-8000-000000000001', '20000000-0000-4000-8000-000000000002'];
const clone = value => JSON.parse(h1.canonicalJson(value));

async function setup(t, {modes = ['pass', 'pass'], execute = true, callback = false, omitLedgerSource = false,
    omitDescriptorSource = false, multipleOutputs = false} = {}) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-ledger-test-'));
    t.after(() => fs.rmSync(base, {recursive: true, force: true}));
    const configPath = path.join(base, 'config.json'), registryPath = path.join(base, 'registry.json');
    const candidatePath = path.join(base, 'candidate.md'), raw = path.join(base, 'input.txt');
    const inventoryPath = path.join(base, 'cases.json');
    const runParent = path.join(base, 'worker-parent'), ledgerParent = path.join(base, 'ledger-parent');
    for (const p of [runParent, ledgerParent]) fs.mkdirSync(p, {mode: 0o700});
    fs.writeFileSync(candidatePath, 'actual synthetic candidate bytes\n');
    fs.writeFileSync(raw, 'actual synthetic raw input bytes\n');
    const registry = {schema_version: 1, purpose: 'synthetic', workers: [
        {id: 'worker-a', module_path: workerPath, args: [modes[0]], case_ids: ['case-a-01', 'case-a-02'],
            outputs: multipleOutputs ? ['a-01.json', 'a-02.json'] : ['a.json'], timeout_ms: 4000, max_stream_bytes: 65536},
        {id: 'worker-b', module_path: workerPath, args: [modes[1]], case_ids: ['case-b-01'],
            outputs: ['b.json'], timeout_ms: 4000, max_stream_bytes: 65536}
    ]};
    fs.writeFileSync(registryPath, h1.canonicalJson(registry));
    fs.writeFileSync(inventoryPath, h1.canonicalJson({schema_version: 1, purpose: 'synthetic',
        cases: registry.workers.flatMap(row => row.case_ids.map(id => ({id, source_path: workerPath})))}));
    const receiptRoots = [], receiptDirs = [];
    for (let i = 0; i < 2; i++) {
        const parent = path.join(base, `root-parent-${i}`), storage = path.join(base, `receipt-parent-${i}`);
        fs.mkdirSync(parent, {mode: 0o700}); fs.mkdirSync(storage, {mode: 0o700});
        receiptRoots.push(path.join(parent, 'reserved')); receiptDirs.push(path.join(storage, 'receipt'));
    }
    const sources = ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js'];
    if (!omitLedgerSource) sources.push('formal-ledger.js');
    if (!omitDescriptorSource) sources.push('ledger-descriptors.js');
    const preflight = {purpose: 'synthetic', candidatePath, runnerPath: path.join(__dirname, 'fixtures/capture-child.js'),
        sourcePaths: [...sources.map(name => path.join(__dirname, name)), workerPath], inputPaths: [raw], fixturePaths: [],
        configPaths: [configPath, registryPath], caseInventoryPath: inventoryPath, runnerArgs: ['pass'], cwd: base,
        environment: {LC_ALL: 'C', ...(process.platform === 'darwin' ? {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})}};
    const config = {schema_version: 1, purpose: 'synthetic', registry_path: registryPath, run_root: path.join(runParent, 'run'),
        receipt_ids: IDS, receipt_roots: receiptRoots, preflight};
    fs.writeFileSync(configPath, h1.canonicalJson(config));
    const captures = [];
    if (execute) for (let i = 0; i < 2; i++) captures.push(await capturePreflight({...preflight,
        receiptId: IDS[i], runRoot: receiptRoots[i], receiptDirectory: receiptDirs[i]}));
    const request = {configPath, runRoot: config.run_root, receiptPaths: execute ? captures.map(row => row.receiptPath) :
        receiptDirs.map((p, i) => path.join(p, `${IDS[i]}.json`))};
    const result = execute ? await runFormalTransaction(request, callback ? {dispatch: context => {
        writeOutputs(context); return resultFor(context);
    }} : {}) : null;
    return {base, raw, configPath, registryPath, registry, config, captures, request, result, ledgerParent,
        contextRequest: {configPath, receiptPaths: request.receiptPaths},
        ledgerPath: path.join(ledgerParent, 'evidence', 'joined-events.json')};
}

function prepared(f) {
    assert.equal(f.result.status, 'PASS', f.result.message);
    const context = ledger.captureLedgerContext(f.contextRequest);
    const document = ledger.buildJoinedLedger(f.result, context);
    return {context, document};
}

function actualBinding(filePath, binding) {
    assert.equal(binding.path, filePath);
    const bytes = fs.readFileSync(filePath);
    assert.equal(binding.bytes, bytes.length);
    assert.equal(binding.sha256, h1.sha256(bytes));
}

test('genesis and complete joined rows bind two actual receipts, runtime, registry, cases and output bytes', async t => {
    const f = await setup(t);
    const {context, document} = prepared(f);
    assert.equal(document.purpose, 'synthetic');
    assert.equal(document.status, 'PASS');
    assert.deepEqual(Object.keys(document), SCHEMA.ledger_keys);
    assert.deepEqual(Object.keys(document.genesis), SCHEMA.genesis_keys);
    actualBinding(f.configPath, document.genesis.config);
    actualBinding(f.registryPath, document.genesis.registry);
    assert.deepEqual(document.genesis.candidate, f.captures[0].inputs.candidate);
    assert.equal(document.genesis.source_manifest_sha256, f.result.source_manifest_sha256);
    assert.equal(document.genesis.runtime.executable.sha256, h1.sha256(fs.readFileSync(document.genesis.runtime.executable_path)));
    assert.equal(document.genesis.runtime.version, process.version);
    assert.equal(document.genesis.worker_count, 2);
    assert.equal(document.genesis.case_count, 3);
    assert.equal(document.genesis.output_count, 2);
    document.genesis.receipts.forEach((row, i) => {
        assert.equal(row.ordinal, i + 1); assert.equal(row.receipt_id, IDS[i]);
        assert.deepEqual(Object.keys(row), SCHEMA.receipt_keys);
        actualBinding(f.captures[i].receiptPath, row.report);
        actualBinding(f.captures[i].receipt.stdout.path, row.stdout);
        actualBinding(f.captures[i].receipt.stderr.path, row.stderr);
    });
    assert.equal(document.genesis_sha256, h1.sha256(h1.canonicalJson(document.genesis)));
    assert.deepEqual(document.rows.map(row => row.event.case_id), ['case-a-01', 'case-a-02', 'case-b-01']);
    let previous = document.genesis_sha256;
    for (const [i, row] of document.rows.entries()) {
        assert.deepEqual(Object.keys(row), SCHEMA.row_keys);
        assert.deepEqual(Object.keys(row.event), SCHEMA.event_keys);
        assert.equal(row.event.ordinal, i + 1);
        assert.equal(row.event.status, 'PASS');
        assert.equal(row.event.previous_sha256, previous);
        assert.equal(row.event.genesis_sha256, document.genesis_sha256);
        assert.equal(row.event.source.sha256, h1.sha256(fs.readFileSync(workerPath)));
        for (const output of row.event.outputs) {
            assert.deepEqual(Object.keys(output), SCHEMA.output_keys);
            const bytes = fs.readFileSync(path.join(f.request.runRoot, output.name));
            assert.equal(output.bytes, bytes.length); assert.equal(output.sha256, h1.sha256(bytes));
        }
        assert.equal(row.sha256, h1.sha256(h1.canonicalJson(row.event))); previous = row.sha256;
    }
    assert.equal(document.tail_sha256, previous);
    assert.equal(document.row_count, 3);
    assert.equal(context.run_root, f.request.runRoot);
    assert.equal(Object.isFrozen(document.rows[0].event.outputs), true);
    assert.equal(Object.isFrozen(SCHEMA.event_keys), true);
});

test('the builder is pure and does not mutate H3 rows or derive expectations from their digest claims', async t => {
    const f = await setup(t);
    let calls = 0;
    const io = new Proxy({...h2.nodeFsAdapter}, {get(target, key) { calls++; return Reflect.get(target, key); }});
    const context = ledger.captureLedgerContext(f.contextRequest, {fs: io});
    const before = h1.canonicalJson(f.result);
    calls = 0;
    const document = ledger.buildJoinedLedger(f.result, context);
    assert.equal(calls, 0);
    assert.equal(document.row_count, 3);
    assert.deepEqual(h1.canonicalJson(f.result), before);
    assert.throws(() => ledger.buildJoinedLedger(f.result, {...context}), /original captured/);
    assert.throws(() => ledger.publishJoinedLedger(clone(document), f.ledgerPath), /original built/);
});

test('exclusive publication outside the worker root has canonical mode-0400 readback and replay', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    const workerBytes = f.result.workers.map(row => row.outputs.map(file => [file.path, fs.readFileSync(file.path)]));
    const snapshot = ledger.publishJoinedLedger(document, f.ledgerPath);
    assert.equal(snapshot.status, 'PASS');
    assert.equal(snapshot.purpose, 'synthetic');
    assert.deepEqual(fs.readdirSync(path.dirname(f.ledgerPath)), [path.basename(f.ledgerPath)]);
    assert.equal(fs.statSync(path.dirname(f.ledgerPath)).mode & 0o7777, 0o700);
    assert.equal(fs.statSync(f.ledgerPath).mode & 0o7777, 0o400);
    assert.equal(fs.statSync(f.ledgerPath).nlink, 1);
    assert.deepEqual(fs.readFileSync(f.ledgerPath), h1.canonicalJson(document));
    assert.equal(snapshot.sha256, h1.sha256(fs.readFileSync(f.ledgerPath)));
    assert.equal(ledger.revalidateJoinedLedger(snapshot), snapshot);
    const replayContext = ledger.captureLedgerContext(f.contextRequest);
    const replayDocument = ledger.buildJoinedLedger(f.result, replayContext);
    assert.deepEqual(h1.canonicalJson(replayDocument), h1.canonicalJson(document));
    assert.equal(ledger.readJoinedLedger(f.ledgerPath, replayDocument).sha256, snapshot.sha256);
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
    for (const files of workerBytes) for (const [p, bytes] of files) assert.deepEqual(fs.readFileSync(p), bytes);
    assert.equal(Object.hasOwn(snapshot, 'reviewer'), false);
    assert.equal(Object.hasOwn(snapshot, 'manifest'), false);
});

test('missing, duplicate, reordered, extra and failed event rows never form a PASS ledger', async t => {
    const f = await setup(t);
    const {context} = prepared(f);
    const controls = [
        value => { value.workers[0].result.events.pop(); },
        value => { value.workers[0].result.events[1] = value.workers[0].result.events[0]; },
        value => { value.workers[0].result.events.reverse(); },
        value => { value.workers[0].result.events.push({...value.workers[0].result.events[0], ordinal: 3}); },
        value => { value.workers[0].result.events[0].status = 'FAIL'; },
        value => { value.workers[0].result.events[0].case_id = 'invented-case'; },
        value => { value.workers[0].result.events[0].ordinal = 2; },
        value => { value.workers[0].result.case_ids.reverse(); },
        value => { value.workers.reverse(); },
        value => { value.workers.pop(); },
        value => { value.last_dispatch = value.workers[0].dispatch; }
    ];
    for (const mutate of controls) {
        const changed = clone(f.result); mutate(changed);
        assert.throws(() => ledger.buildJoinedLedger(changed, context));
    }
    assert.equal(fs.existsSync(path.dirname(f.ledgerPath)), false);
});

test('H3 status, manifest, source, runtime, argv and actual stream/result joins are explicit', async t => {
    const f = await setup(t);
    const {context} = prepared(f);
    for (const mutate of [
        value => { value.status = 'FAIL'; }, value => { delete value.status; },
        value => { value.stage = 'WORKER'; }, value => { value.reason = 'CHILD_NOT_PASS'; },
        value => { value.root_created = false; }, value => { value.exit_code = 1; },
        value => { value.worker_launches = 1; }, value => { value.verification_failures.push({reason: 'bad'}); },
        value => { value.purpose = 'validation'; }, value => { value.source_manifest_sha256 = 'a'.repeat(64); },
        value => { value.registry_sha256 = 'a'.repeat(64); },
        value => { value.workers[0].dispatch.worker_source.sha256 = 'a'.repeat(64); },
        value => { value.workers[0].dispatch.runtime.executable.sha256 = 'a'.repeat(64); },
        value => { value.workers[0].dispatch.argv.push('unrecorded'); },
        value => { value.workers[0].dispatch.environment_sha256 = 'a'.repeat(64); },
        value => { value.workers[0].dispatch.child.exit_code = 7; },
        value => { value.workers[0].dispatch.stdin.write_completed = false; },
        value => { value.workers[0].dispatch.observed.stdout++; },
        value => { value.workers[0].dispatch.stdout.sha256 = 'a'.repeat(64); },
        value => {
            const bytes = h1.canonicalJson({...value.workers[0].result, status: 'FAIL'});
            value.workers[0].dispatch.stdout = h2.bytesSnapshot(bytes);
            value.workers[0].dispatch.observed.stdout = bytes.length;
        }
    ]) {
        const changed = clone(f.result); mutate(changed);
        assert.throws(() => ledger.buildJoinedLedger(changed, context));
    }
});

test('output claims must equal actual independently read bytes, hashes, identity and membership', async t => {
    const f = await setup(t);
    const {context} = prepared(f);
    for (const mutate of [
        value => { value.workers[0].outputs[0].sha256 = 'a'.repeat(64); },
        value => { value.workers[0].outputs[0].byte_length++; },
        value => { value.workers[0].outputs[0].inode = '0'; },
        value => { value.workers[0].outputs[0].base64 = Buffer.from('wrong bytes').toString('base64'); },
        value => { value.workers[0].outputs = []; },
        value => { value.workers[0].outputs.push(value.workers[0].outputs[0]); },
        value => { value.workers[0].result.outputs = ['b.json']; }
    ]) {
        const changed = clone(f.result); mutate(changed);
        assert.throws(() => ledger.buildJoinedLedger(changed, context));
    }
    fs.writeFileSync(path.join(f.request.runRoot, 'unexpected'), 'extra');
    assert.throws(() => ledger.captureLedgerContext(f.contextRequest), /extra entries/);
});

test('failed actual H3 keeps its prefix as data and cannot publish a PASS ledger', async t => {
    const f = await setup(t, {modes: ['pass', 'malformed']});
    assert.equal(f.result.status, 'FAIL');
    assert.equal(f.result.workers.length, 1);
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
    const before = fs.readFileSync(path.join(f.request.runRoot, 'a.json'));
    const context = ledger.captureLedgerContext(f.contextRequest);
    assert.throws(() => ledger.buildJoinedLedger(f.result, context), error => error.reason === 'H3_NOT_COMPLETE');
    assert.equal(fs.existsSync(path.dirname(f.ledgerPath)), false);
    assert.deepEqual(fs.readFileSync(path.join(f.request.runRoot, 'a.json')), before);
});

test('synthetic callbacks remain explicit synthetic joins rather than process/validation evidence', async t => {
    const f = await setup(t, {callback: true});
    const {context, document} = prepared(f);
    assert.equal(document.purpose, 'synthetic');
    assert.equal(document.rows.every(row => row.event.dispatch_mode === 'synthetic-callback'), true);
    const changed = clone(f.result);
    changed.workers[0].dispatch.mode = 'process';
    assert.throws(() => ledger.buildJoinedLedger(changed, context));
});

test('context requires real H4 source and trusted purpose bootstrap before any dependency getter', async t => {
    const f = await setup(t, {omitLedgerSource: true});
    assert.equal(f.result.status, 'PASS');
    assert.throws(() => ledger.captureLedgerContext(f.contextRequest), error => error.reason === 'SOURCE_UNPINNED');
    const g = await setup(t, {execute: false});
    g.config.purpose = g.config.preflight.purpose = 'validation';
    fs.writeFileSync(g.configPath, h1.canonicalJson(g.config));
    let calls = 0;
    const dependencies = Object.defineProperty({}, 'fs', {get() { calls++; throw new Error('must not read'); }});
    assert.throws(() => ledger.captureLedgerContext(g.contextRequest, dependencies),
        error => error.reason === 'SYNTHETIC_INJECTION_FORBIDDEN');
    assert.equal(calls, 0);
    assert.equal(fs.existsSync(path.dirname(g.ledgerPath)), false);
});

test('builder refuses accessors, cycles, sparse arrays and oversized strings without executing code', async t => {
    const f = await setup(t);
    const {context} = prepared(f);
    let calls = 0;
    const value = clone(f.result);
    Object.defineProperty(value, 'status', {get() { calls++; throw new Error('must not execute'); }});
    assert.throws(() => ledger.buildJoinedLedger(value, context), /accessors/);
    assert.equal(calls, 0);
    const cycle = clone(f.result); cycle.last_dispatch = cycle;
    assert.throws(() => ledger.buildJoinedLedger(cycle, context), /acyclic/);
    const sparse = clone(f.result); delete sparse.workers[0];
    assert.throws(() => ledger.buildJoinedLedger(sparse, context));
    const oversized = clone(f.result); oversized.message = 'x'.repeat(LIMITS.stringBytes + 1);
    assert.throws(() => ledger.buildJoinedLedger(oversized, context), /byte cap/);
});

test('ledger publication refuses worker-root/input/receipt overlap without altering those trees', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    for (const p of [path.join(f.request.runRoot, 'ledger.json'), path.join(f.captures[0].receiptDirectory, 'ledger.json'),
        path.join(f.base, 'ledger.json')]) {
        assert.throws(() => ledger.publishJoinedLedger(document, p), error => error.reason === 'PATH_POLICY');
    }
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
});

test('exclusive evidence-directory race preserves the external winner and sentinel', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    let identity;
    const io = {...h2.nodeFsAdapter, mkdirSync(p, options) {
        if (p === path.dirname(f.ledgerPath)) {
            fs.mkdirSync(p, options); fs.writeFileSync(path.join(p, 'sentinel'), 'external winner');
            identity = h2.statIdentity(fs.lstatSync(p, {bigint: true}));
        }
        return fs.mkdirSync(p, options);
    }};
    assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath, {fs: io}), error => {
        assert.equal(error.reason, 'PATH_EXISTS');
        assert.equal(error.details.directory_created, false); assert.equal(error.details.file_created, false);
        return true;
    });
    assert.equal(h2.statIdentity(fs.lstatSync(path.dirname(f.ledgerPath), {bigint: true})), identity);
    assert.equal(fs.readFileSync(path.join(path.dirname(f.ledgerPath), 'sentinel'), 'utf8'), 'external winner');
    assert.equal(fs.existsSync(f.ledgerPath), false);
});

test('failed/zero writes keep the exact partial file and owned directory instead of cleaning up', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    let writes = 0;
    const io = {...h2.nodeFsAdapter, writeSync(fd, bytes, offset, length) {
        if (++writes === 1) return fs.writeSync(fd, bytes, offset, Math.min(7, length));
        return 0;
    }};
    assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath, {fs: io}), error => {
        assert.equal(error.reason, 'SHORT_WRITE');
        assert.equal(error.details.directory_created, true); assert.equal(error.details.file_created, true);
        return true;
    });
    assert.deepEqual(fs.readFileSync(f.ledgerPath), h1.canonicalJson(document).subarray(0, 7));
    assert.equal(fs.statSync(f.ledgerPath).mode & 0o7777, 0o600);
    assert.deepEqual(fs.readdirSync(path.dirname(f.ledgerPath)), [path.basename(f.ledgerPath)]);
});

test('unsupported directory fsync is explicit while file fsync errors refuse publication', async t => {
    for (const kind of ['directory', 'file']) {
        const f = await setup(t);
        const {document} = prepared(f);
        const io = {...h2.nodeFsAdapter, fsyncSync(fd) {
            const directory = fs.fstatSync(fd).isDirectory();
            if ((kind === 'directory') === directory) throw Object.assign(new Error('injected fsync'), {code: directory ? 'ENOTSUP' : 'EIO'});
            fs.fsyncSync(fd);
        }};
        if (kind === 'directory') {
            const published = ledger.publishJoinedLedger(document, f.ledgerPath, {fs: io});
            assert.equal(published.durability_limitations.length, 2);
            assert.equal(published.durability_limitations.every(text => text.includes('ENOTSUP')), true);
        } else {
            assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath, {fs: io}), error => error.details.cause_code === 'EIO');
            assert.equal(fs.existsSync(f.ledgerPath), true);
        }
    }
});

test('publication refuses changed input/output/receipt snapshots before creating evidence', async t => {
    for (const kind of ['input', 'output', 'receipt']) {
        const f = await setup(t);
        const {document} = prepared(f);
        const p = kind === 'input' ? f.raw : kind === 'output' ? path.join(f.request.runRoot, 'a.json') : f.captures[0].receipt.stdout.path;
        fs.chmodSync(p, 0o600); fs.appendFileSync(p, 'changed'); fs.chmodSync(p, 0o400);
        assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath));
        assert.equal(fs.existsSync(path.dirname(f.ledgerPath)), false);
    }
});

test('revalidation detects canonical event/genesis/chain mutations rather than trusting rewritten hashes', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    const first = ledger.publishJoinedLedger(document, f.ledgerPath);
    for (const mutate of [
        value => { value.rows[0].event.case_id = 'invented'; },
        value => { value.rows.reverse(); }, value => { value.rows.pop(); },
        value => { value.rows.push(value.rows[0]); },
        value => { value.genesis.receipts[0].report.sha256 = 'a'.repeat(64); },
        value => { value.genesis.candidate.sha256 = 'a'.repeat(64); }
    ]) {
        const changed = clone(document); mutate(changed);
        changed.genesis_sha256 = h1.sha256(h1.canonicalJson(changed.genesis));
        let previous = changed.genesis_sha256;
        for (const row of changed.rows) {
            row.event.genesis_sha256 = changed.genesis_sha256; row.event.previous_sha256 = previous;
            row.sha256 = h1.sha256(h1.canonicalJson(row.event)); previous = row.sha256;
        }
        changed.row_count = changed.rows.length; changed.tail_sha256 = previous;
        fs.chmodSync(f.ledgerPath, 0o600); fs.writeFileSync(f.ledgerPath, h1.canonicalJson(changed)); fs.chmodSync(f.ledgerPath, 0o400);
        assert.throws(() => ledger.revalidateJoinedLedger(first));
    }
});

test('revalidation pins original inode, link count, mode, directory identity and exact membership', async t => {
    for (const kind of ['inode', 'hardlink', 'mode', 'directory-mode', 'extra', 'missing', 'symlink', 'parent']) {
        const f = await setup(t);
        const {document} = prepared(f);
        const snapshot = ledger.publishJoinedLedger(document, f.ledgerPath);
        const directory = path.dirname(f.ledgerPath);
        if (kind === 'inode') {
            const bytes = fs.readFileSync(f.ledgerPath);
            fs.renameSync(f.ledgerPath, path.join(f.base, 'original-ledger'));
            fs.writeFileSync(f.ledgerPath, bytes, {mode: 0o400});
        }
        if (kind === 'hardlink') fs.linkSync(f.ledgerPath, path.join(f.base, 'ledger-alias'));
        if (kind === 'mode') fs.chmodSync(f.ledgerPath, 0o600);
        if (kind === 'directory-mode') fs.chmodSync(directory, 0o750);
        if (kind === 'extra') fs.writeFileSync(path.join(directory, 'extra'), 'extra');
        if (kind === 'missing') fs.renameSync(f.ledgerPath, path.join(f.base, 'missing-ledger'));
        if (kind === 'symlink') {
            fs.renameSync(directory, directory + '.original'); fs.symlinkSync(directory + '.original', directory);
        }
        if (kind === 'parent') {
            fs.renameSync(f.ledgerParent, f.ledgerParent + '.original'); fs.mkdirSync(f.ledgerParent, {mode: 0o700});
        }
        assert.throws(() => ledger.revalidateJoinedLedger(snapshot), kind);
    }
});

test('readback substitution after writing cannot become a new trusted ledger baseline', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    const paths = new Map();
    let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(p, ...args) { const fd = fs.openSync(p, ...args); paths.set(fd, p); return fd; },
        closeSync(fd) {
            const p = paths.get(fd); paths.delete(fd); fs.closeSync(fd);
            if (!changed && p === f.ledgerPath) {
                changed = true;
                const bytes = fs.readFileSync(p);
                fs.renameSync(p, path.join(f.base, 'original-ledger'));
                fs.writeFileSync(p, bytes, {mode: 0o400});
            }
        }};
    assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath, {fs: io}), error => error.reason === 'FILE_CHANGED');
    assert.equal(changed, true);
    assert.deepEqual(fs.readFileSync(f.ledgerPath), h1.canonicalJson(document));
    assert.equal(fs.existsSync(path.join(f.base, 'original-ledger')), true);
});

test('existing evidence is never overwritten or accepted as a fresh publication', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    const snapshot = ledger.publishJoinedLedger(document, f.ledgerPath);
    const identity = h2.statIdentity(fs.lstatSync(f.ledgerPath, {bigint: true}));
    assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath), error => error.reason === 'PATH_EXISTS');
    assert.equal(h2.statIdentity(fs.lstatSync(f.ledgerPath, {bigint: true})), identity);
    assert.equal(ledger.revalidateJoinedLedger(snapshot), snapshot);
});

test('late ledger/output mutation during cross-input readback is detected at the final boundary', async t => {
    for (const kind of ['ledger', 'output']) {
        const f = await setup(t);
        const {document} = prepared(f);
        ledger.publishJoinedLedger(document, f.ledgerPath);
        const watched = kind === 'ledger' ? f.ledgerPath : path.join(f.request.runRoot, 'a.json');
        const paths = new Map();
        let read = false, changed = false;
        const io = {...h2.nodeFsAdapter,
            openSync(p, ...args) { const fd = fs.openSync(p, ...args); paths.set(fd, p); return fd; },
            closeSync(fd) { paths.delete(fd); fs.closeSync(fd); },
            readSync(fd, ...args) {
                const count = fs.readSync(fd, ...args);
                if (paths.get(fd) === watched && count > 0) read = true;
                return count;
            },
            lstatSync(p) {
                if (p === f.raw && read && !changed) {
                    changed = true;
                    fs.chmodSync(watched, 0o600); fs.appendFileSync(watched, 'late mutation'); fs.chmodSync(watched, 0o400);
                }
                return fs.lstatSync(p, {bigint: true});
            }};
        assert.throws(() => ledger.readJoinedLedger(f.ledgerPath, document, {fs: io}));
        assert.equal(changed, true);
        assert.equal(fs.readFileSync(watched).subarray(-13).toString(), 'late mutation');
    }
});

test('canonical parsing rejects malformed, duplicate-key, invalid UTF8 and reordered persisted ledgers', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    ledger.publishJoinedLedger(document, f.ledgerPath);
    const reordered = {...document};
    delete reordered.schema_version; reordered.schema_version = 1;
    for (const bytes of [Buffer.from([0xff, 0x0a]), Buffer.from('not JSON\n'),
        Buffer.from('{"schema_version":1,"schema_version":1}\n'),
        Buffer.concat([h1.canonicalJson(document), Buffer.from('\n')]),
        Buffer.from(h1.canonicalJson(document).toString().replace(/\n$/, '\r\n')),
        h1.canonicalJson(reordered)]) {
        fs.chmodSync(f.ledgerPath, 0o600); fs.writeFileSync(f.ledgerPath, bytes); fs.chmodSync(f.ledgerPath, 0o400);
        assert.throws(() => ledger.readJoinedLedger(f.ledgerPath, document));
    }
});

test('an owned evidence directory replaced after the initial descriptor closes is refused', async t => {
    const f = await setup(t);
    const {document} = prepared(f);
    const directory = path.dirname(f.ledgerPath);
    const paths = new Map();
    let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync(p, ...args) { const fd = fs.openSync(p, ...args); paths.set(fd, p); return fd; },
        closeSync(fd) {
            const p = paths.get(fd); paths.delete(fd); fs.closeSync(fd);
            if (!changed && p === directory) {
                changed = true; fs.renameSync(directory, directory + '.original');
                fs.mkdirSync(directory, {mode: 0o700}); fs.writeFileSync(path.join(directory, 'external'), 'sentinel');
            }
        }};
    assert.throws(() => ledger.publishJoinedLedger(document, f.ledgerPath, {fs: io}), error => error.reason === 'DIRECTORY_CHANGED');
    assert.equal(changed, true);
    assert.equal(fs.existsSync(directory + '.original'), true);
    assert.equal(fs.readFileSync(path.join(directory, 'external'), 'utf8'), 'sentinel');
    assert.equal(fs.existsSync(f.ledgerPath), false);
});

test('each joined case binds the complete ordered multi-output inventory of its worker', async t => {
    const f = await setup(t, {multipleOutputs: true});
    const {context, document} = prepared(f);
    assert.equal(document.genesis.output_count, 3);
    for (const row of document.rows.slice(0, 2)) {
        assert.deepEqual(row.event.outputs.map(file => file.name), ['a-01.json', 'a-02.json']);
        for (const file of row.event.outputs) {
            const bytes = fs.readFileSync(path.join(f.request.runRoot, file.name));
            assert.equal(file.bytes, bytes.length); assert.equal(file.sha256, h1.sha256(bytes));
        }
    }
    const changed = clone(f.result);
    changed.workers[0].outputs.reverse();
    assert.throws(() => ledger.buildJoinedLedger(changed, context), error => error.reason === 'OUTPUT_MISMATCH');
    const snapshot = ledger.publishJoinedLedger(document, f.ledgerPath);
    assert.equal(ledger.revalidateJoinedLedger(snapshot), snapshot);
});

test('shared ledger descriptors are import-free frozen data usable without production helpers', () => {
    const sandbox = {module: {exports: null}, require() { throw new Error('descriptor must not import code'); }};
    const source = fs.readFileSync(path.join(__dirname, 'ledger-descriptors.js'), 'utf8');
    vm.runInNewContext(source, sandbox, {timeout: 1000});
    assert.deepEqual(h1.canonicalJson(sandbox.module.exports), h1.canonicalJson({LIMITS, SCHEMA}));
    const inert = value => {
        if (value && typeof value === 'object') {
            assert.equal(Object.isFrozen(value), true);
            Object.values(value).forEach(inert);
        } else assert.equal(['string', 'number', 'boolean'].includes(typeof value) || value === null, true);
    };
    inert(sandbox.module.exports);
    assert.equal(SCHEMA.revision, h1.REVISION);
    assert.equal(Object.hasOwn(ledger, 'SCHEMA'), false);
    assert.equal(Object.hasOwn(ledger, 'LIMITS'), false);
});

test('actual source closure pins the descriptor and refuses an undeclared descriptor for either purpose', async t => {
    const f = await setup(t);
    const declared = f.captures[0].inputs.manifest.files.find(row => row.path === path.join(__dirname, 'ledger-descriptors.js'));
    assert.ok(declared);
    assert.equal(declared.roles.includes('source'), true);
    assert.ok(declared.bytes > 0);
    assert.equal(declared.sha256, h1.sha256(fs.readFileSync(declared.path)));
    prepared(f);
    for (const purpose of ['synthetic', 'validation']) {
        const g = await setup(t, {omitDescriptorSource: true, execute: purpose === 'synthetic'});
        if (purpose === 'validation') {
            // Negative configuration only: no fabricated validation receipts or execution evidence.
            g.config.purpose = g.config.preflight.purpose = 'validation';
            g.config.preflight.runnerPath = g.config.preflight.sourcePaths[0];
            const inventoryPath = g.config.preflight.caseInventoryPath;
            const inventory = JSON.parse(fs.readFileSync(inventoryPath)); inventory.purpose = 'validation';
            fs.writeFileSync(inventoryPath, h1.canonicalJson(inventory));
            fs.writeFileSync(g.configPath, h1.canonicalJson(g.config));
        } else assert.equal(g.result.status, 'PASS');
        assert.throws(() => ledger.captureLedgerContext(g.contextRequest), error => error.reason === 'SOURCE_UNPINNED');
        assert.equal(fs.existsSync(path.dirname(g.ledgerPath)), false);
        if (purpose === 'validation') assert.equal(fs.existsSync(g.request.runRoot), false);
    }
});
