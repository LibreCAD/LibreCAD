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
const {spawnSync} = require('node:child_process');
const {EventEmitter} = require('node:events');
const {PassThrough, Writable} = require('node:stream');
const test = require('node:test');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {capturePreflight} = require('./preflight-capture');
const {runFormalTransaction} = require('./formal-runner');
const {parseRunArguments} = require('./run-formal-gates');
const {resultFor, writeOutputs} = require('./fixtures/formal-child');
const registryApi = require('./formal-registry');

const cliPath = path.join(__dirname, 'run-formal-gates.js');
const workerPath = path.join(__dirname, 'fixtures', 'formal-child.js');
const IDS = ['10000000-0000-4000-8000-000000000001', '20000000-0000-4000-8000-000000000002'];

function tree(directory) {
    const rows = [];
    function visit(p) {
        const stat = fs.lstatSync(p, {bigint: true});
        rows.push([p, h2.statIdentity(stat), stat.isFile() ? h1.sha256(fs.readFileSync(p)) : null]);
        if (stat.isDirectory()) for (const name of fs.readdirSync(p).sort()) visit(path.join(p, name));
    }
    visit(directory);
    return rows;
}

async function setup(t, {modes = ['pass', 'pass'], capture = true, duplicateId = false,
    registryMutation, configMutation, inventoryFromRegistry = false} = {}) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-formal-test-'));
    t.after(() => fs.rmSync(base, {recursive: true, force: true}));
    const configPath = path.join(base, 'formal-config.json');
    const registryPath = path.join(base, 'registry.json');
    const candidatePath = path.join(base, 'candidate.md');
    const raw = path.join(base, 'raw.txt');
    const inventoryPath = path.join(base, 'cases.json');
    const runParent = path.join(base, 'formal-parent');
    const runRoot = path.join(runParent, 'run');
    fs.mkdirSync(runParent, {mode: 0o700});
    fs.writeFileSync(candidatePath, 'actual candidate source\n');
    fs.writeFileSync(raw, 'actual raw formal input\n');
    const receiptRoots = [];
    const receiptDirectories = [];
    for (let i = 0; i < 2; i++) {
        const rootParent = path.join(base, `preflight-root-parent-${i}`);
        const receiptParent = path.join(base, `receipt-parent-${i}`);
        fs.mkdirSync(rootParent, {mode: 0o700});
        fs.mkdirSync(receiptParent, {mode: 0o700});
        receiptRoots.push(path.join(rootParent, 'reserved-root'));
        receiptDirectories.push(path.join(receiptParent, 'receipt'));
    }
    const workers = [
        {id: 'worker-a', module_path: workerPath, args: [modes[0]], case_ids: ['case-a-01', 'case-a-02'],
            outputs: ['a.json'], timeout_ms: 3000, max_stream_bytes: 65536},
        {id: 'worker-b', module_path: workerPath, args: [modes[1]], case_ids: ['case-b-01'],
            outputs: ['b.json'], timeout_ms: 3000, max_stream_bytes: 65536}
    ];
    const registry = {schema_version: 1, purpose: 'synthetic', workers};
    if (registryMutation) registryMutation(registry);
    fs.writeFileSync(registryPath, h1.canonicalJson(registry));
    const cases = (inventoryFromRegistry ? registry.workers.flatMap(worker => worker.case_ids).sort() :
        ['case-a-01', 'case-a-02', 'case-b-01']).map(id => ({id, source_path: workerPath}));
    fs.writeFileSync(inventoryPath, h1.canonicalJson({schema_version: 1, purpose: 'synthetic', cases}));
    const preflight = {purpose: 'synthetic', candidatePath, runnerPath: path.join(__dirname, 'fixtures', 'capture-child.js'),
        sourcePaths: ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js'].map(name => path.join(__dirname, name)).concat(workerPath),
        inputPaths: [raw], fixturePaths: [], configPaths: [configPath, registryPath], caseInventoryPath: inventoryPath,
        runnerArgs: ['pass'], cwd: base, environment: {LC_ALL: 'C', ...(process.platform === 'darwin' ?
            {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})}};
    const config = {schema_version: 1, purpose: 'synthetic', registry_path: registryPath, run_root: runRoot,
        receipt_ids: [...IDS], receipt_roots: receiptRoots, preflight};
    if (configMutation) configMutation(config);
    fs.writeFileSync(configPath, h1.canonicalJson(config));
    const receipts = [];
    if (capture) for (let i = 0; i < 2; i++) receipts.push(await capturePreflight({...preflight,
        receiptId: duplicateId ? IDS[0] : IDS[i], runRoot: receiptRoots[i], receiptDirectory: receiptDirectories[i], timeoutMs: 10000}));
    const request = {configPath, runRoot, receiptPaths: receipts.length ? receipts.map(row => row.receiptPath) :
        receiptDirectories.map((p, i) => path.join(p, `${IDS[i]}.json`))};
    return {base, raw, runParent, registry, config, request, receipts};
}

function argumentsFor(request) {
    return ['--run', '--config', request.configPath, '--run-dir', request.runRoot,
        '--preflight-receipt', request.receiptPaths[0], '--preflight-receipt', request.receiptPaths[1]];
}

function realCli(argv) {
    const child = spawnSync(process.execPath, [cliPath, ...argv], {maxBuffer: 8388608, timeout: 30000});
    assert.equal(child.error, undefined);
    const result = h1.parseCanonicalJson(child.stdout, {maxBytes: 8388608});
    assert.equal(child.status, result.exit_code);
    return result;
}

function assertRefused(result, stage, reason, request) {
    assert.equal(result.status, 'REFUSED', JSON.stringify(result));
    assert.equal(result.stage, stage, result.message);
    assert.equal(result.reason, reason, result.message);
    assert.equal(result.worker_launches, 0);
    assert.equal(result.root_created, false);
    assert.equal(fs.existsSync(request.runRoot), false);
}

function rewriteReceipt(capture, mutate) {
    const receipt = JSON.parse(fs.readFileSync(capture.receiptPath, 'utf8'));
    mutate(receipt);
    const stdout = h1.canonicalJson(receipt.preflight_result);
    receipt.stdout.bytes = stdout.length;
    receipt.stdout.sha256 = receipt.stdout.file_sha256 = h1.sha256(stdout);
    for (const [p, bytes] of [[receipt.stdout.path, stdout], [capture.receiptPath, h1.canonicalJson(receipt)]]) {
        fs.chmodSync(p, 0o600);
        fs.writeFileSync(p, bytes);
        fs.chmodSync(p, 0o400);
    }
}

test('real CLI dispatches the pinned worker registry and binds actual output bytes/argv/environment', async t => {
    const f = await setup(t);
    const before = tree(f.base);
    const env = JSON.stringify(Object.entries(process.env).sort());
    const result = realCli(argumentsFor(f.request));
    assert.equal(result.status, 'PASS', result.message);
    assert.equal(result.reason, 'H3_DISPATCH_COMPLETE');
    assert.equal(result.worker_launches, 2);
    assert.deepEqual(result.workers.map(row => row.worker_id), ['worker-a', 'worker-b']);
    assert.equal(result.source_manifest_sha256, f.receipts[0].inputs.sourceManifestSha256);
    assert.equal(result.registry_sha256, h1.sha256(fs.readFileSync(f.config.registry_path)));
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
    for (const record of result.workers) {
        const output = record.outputs[0];
        assert.equal(output.sha256, h1.sha256(fs.readFileSync(output.path)));
        assert.equal(output.byte_length, fs.statSync(output.path).size);
        assert.deepEqual(record.dispatch.argv, [process.execPath, workerPath, 'pass']);
        assert.equal(record.dispatch.worker_source.sha256, h1.sha256(fs.readFileSync(workerPath)));
        assert.equal(record.dispatch.runtime.version, process.version);
        assert.equal(record.dispatch.runtime.executable.sha256, h1.sha256(fs.readFileSync(process.execPath)));
        const stderr = JSON.parse(Buffer.from(record.dispatch.stderr.base64, 'base64'));
        assert.deepEqual(stderr.environment, f.config.preflight.environment);
        assert.equal(stderr.stdin_sha256, record.dispatch.stdin.sha256);
    }
    const after = tree(f.base).filter(row => row[0] !== f.runParent && !row[0].startsWith(f.request.runRoot));
    assert.deepEqual(after, before.filter(row => row[0] !== f.runParent));
    assert.equal(JSON.stringify(Object.entries(process.env).sort()), env);
    assert.deepEqual(result.verification_failures, []);
});

test('real CLI supplies distinct bounded section-13.1 argument classifications without touching inputs', async t => {
    const f = await setup(t, {capture: false});
    const valid = argumentsFor(f.request);
    const cases = [
        [valid.slice(1), 'RUN_MODE_REQUIRED'],
        [[...valid, '--unknown'], 'ARG_UNKNOWN_OPTION'],
        [[...valid, '--run'], 'ARG_DUPLICATE_OPTION'],
        [valid.filter((value, i) => i !== 2), 'ARG_MISSING_VALUE'],
        [valid.slice(0, 5), 'RECEIPT_COUNT'],
        [valid.slice(0, 7), 'RECEIPT_COUNT'],
        [[...valid, '--preflight-receipt', '/third.json'], 'RECEIPT_COUNT'],
        [[...valid.slice(0, 8), valid[6]], 'RECEIPT_PATH_DUPLICATE'],
        [[...valid.slice(0, 6), 'relative.json', ...valid.slice(7)], 'ARG_RELATIVE_PATH']
    ];
    const before = tree(f.base);
    for (const [args, reason] of cases) assertRefused(realCli(args), 'CLI', reason, f.request);
    assert.deepEqual(tree(f.base), before);
});

test('parser and actual CLI share one request contract', async t => {
    const f = await setup(t, {capture: false});
    assert.deepEqual(parseRunArguments(argumentsFor(f.request)), f.request);
    assert.throws(() => parseRunArguments(Array(65).fill('--run')), /bounded/);
    assert.throws(() => parseRunArguments(['--run', '--bogus']), error => error.reason === 'ARG_UNKNOWN_OPTION');
});

test('duplicate receipt IDs have the same transaction join refusal through API and actual CLI', async t => {
    const f = await setup(t, {duplicateId: true});
    const before = tree(f.base);
    assertRefused(await runFormalTransaction(f.request), 'RECEIPT_JOIN', 'RECEIPT_ID_DUPLICATE', f.request);
    assertRefused(realCli(argumentsFor(f.request)), 'RECEIPT_JOIN', 'RECEIPT_ID_DUPLICATE', f.request);
    assert.deepEqual(tree(f.base), before);
});

test('reversed expected receipt pair refuses before root/dispatch through both ingresses', async t => {
    const f = await setup(t);
    const reversed = {...f.request, receiptPaths: [...f.request.receiptPaths].reverse()};
    const before = tree(f.base);
    assertRefused(await runFormalTransaction(reversed), 'RECEIPT_JOIN', 'RECEIPT_ORDER_MISMATCH', f.request);
    assertRefused(realCli(argumentsFor(reversed)), 'RECEIPT_JOIN', 'RECEIPT_ORDER_MISMATCH', f.request);
    assert.deepEqual(tree(f.base), before);
});

test('receipt case counts and hashes must match the actual pinned H2 inventory', async t => {
    for (const kind of ['count', 'hash']) {
        const f = await setup(t);
        for (const receipt of f.receipts) rewriteReceipt(receipt, row => {
            const gate = row.preflight_result.inventory_gate;
            if (kind === 'count') gate.expected_count = gate.actual_count = 1;
            else gate.expected_ids_sha256 = gate.actual_ids_sha256 = 'a'.repeat(64);
        });
        const before = tree(f.base);
        assertRefused(await runFormalTransaction(f.request), 'IDENTITY', 'CASE_INVENTORY_MISMATCH', f.request);
        assertRefused(realCli(argumentsFor(f.request)), 'IDENTITY', 'CASE_INVENTORY_MISMATCH', f.request);
        assert.deepEqual(tree(f.base), before);
    }
});

test('missing receipt uses the named I/O boundary and launches no workers', async t => {
    const f = await setup(t, {capture: false});
    assertRefused(realCli(argumentsFor(f.request)), 'RECEIPT_IO', 'RECEIPT_NOT_FOUND', f.request);
});

test('registry rejects empty/unpinned/duplicate inventories before ownership', async t => {
    for (const [mutate, reason] of [
        [registry => { registry.workers = []; }, 'WORKERS_NOT_CONFIGURED'],
        [registry => { registry.workers[1].id = registry.workers[0].id; }, 'WORKER_DUPLICATE'],
        [registry => { registry.workers[1].case_ids = ['case-a-01']; }, 'CASE_DUPLICATE'],
        [registry => { registry.workers[1].outputs = ['A.json']; }, 'OUTPUT_DUPLICATE'],
        [registry => { registry.workers[1].module_path = '/not-pinned.js'; }, 'WORKER_SOURCE_UNPINNED'],
        [registry => { registry.workers.pop(); }, 'CASE_INVENTORY_MISMATCH']
    ]) {
        const f = await setup(t, {registryMutation: mutate});
        assertRefused(await runFormalTransaction(f.request), 'REGISTRY', reason, f.request);
    }
});

test('current H2 format requires H3 source/config dependencies rather than dummy manifests', async t => {
    const f = await setup(t, {capture: false, configMutation: config => {
        config.preflight.sourcePaths = [workerPath];
    }});
    assertRefused(await runFormalTransaction(f.request), 'REGISTRY', 'RUNNER_SOURCE_UNPINNED', f.request);
    const g = await setup(t, {capture: false, configMutation: config => { config.preflight.configPaths = []; }});
    assertRefused(await runFormalTransaction(g.request), 'REGISTRY', 'CONFIG_UNPINNED', g.request);
});

test('missing/undefined/unknown/FAIL worker status never defaults to PASS', async t => {
    for (const [kind, reason] of [['missing-result', 'WORKER_RESULT_MISSING'], ['undefined', 'WORKER_STATUS_MISSING'],
        ['missing', 'WORKER_STATUS_MISSING'], ['unknown', 'WORKER_STATUS_UNKNOWN'], ['FAIL', 'WORKER_FAILED']]) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {dispatch: context => {
            writeOutputs(context);
            const row = resultFor(context);
            if (kind === 'missing-result') return undefined;
            if (kind === 'undefined') row.status = undefined;
            else if (kind === 'missing') delete row.status;
            else row.status = kind;
            return row;
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, reason);
        assert.equal(result.worker_launches, 1);
        assert.deepEqual(fs.readdirSync(f.request.runRoot), ['a.json']);
    }
});

test('reported missing/extra/duplicate outputs and reordered/failed events refuse independently', async t => {
    for (const [mutate, reason] of [
        [row => { row.outputs = []; }, 'WORKER_OUTPUT_MISSING'],
        [row => { row.outputs.push('extra.json'); }, 'WORKER_OUTPUT_UNEXPECTED'],
        [row => { row.outputs.push(row.outputs[0]); }, 'WORKER_OUTPUT_DUPLICATE'],
        [row => { row.events.reverse(); }, 'WORKER_EVENT_MISMATCH'],
        [row => { row.events[0].status = 'FAIL'; }, 'WORKER_EVENT_MISMATCH'],
        [row => { row.events = Array(row.events.length); }, 'WORKER_EVENT_MISMATCH']
    ]) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {dispatch: context => {
            writeOutputs(context);
            const row = resultFor(context);
            mutate(row);
            return row;
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, reason, result.message);
        assert.equal(result.worker_launches, 1);
    }
});

test('real worker missing/extra files, malformed/empty result, nonzero exit, and cap/timeouts preserve diagnostics', async t => {
    for (const [mode, reason] of [['missing-file', 'OUTPUT_MISSING'], ['extra-file', 'OUTPUT_UNEXPECTED'],
        ['wrong-mode', 'OUTPUT_METADATA_MISMATCH'], ['empty', 'WORKER_RESULT_MISSING'], ['malformed', 'WORKER_RESULT_PARSE_FAILED'],
        ['status-missing', 'WORKER_STATUS_MISSING'], ['status-unknown', 'WORKER_STATUS_UNKNOWN'], ['fail', 'WORKER_FAILED'],
        ['outputs-missing', 'WORKER_OUTPUT_MISSING'], ['outputs-extra', 'WORKER_OUTPUT_UNEXPECTED'], ['outputs-duplicate', 'WORKER_OUTPUT_DUPLICATE'],
        ['events-reversed', 'WORKER_EVENT_MISMATCH'],
        ['pass-after-exit-failure', 'CHILD_NOT_PASS'], ['signal', 'CHILD_NOT_PASS'], ['overflow', 'CHILD_NOT_PASS'], ['timeout', 'CHILD_NOT_PASS']]) {
        const f = await setup(t, {modes: [mode, 'pass'], registryMutation: registry => {
            if (mode === 'overflow') registry.workers[0].max_stream_bytes = 1024;
            if (mode === 'timeout') registry.workers[0].timeout_ms = 1000;
        }});
        const result = await runFormalTransaction(f.request);
        assert.equal(result.status, 'FAIL', result.message);
        assert.equal(result.reason, reason, `${mode}: ${result.message}`);
        assert.equal(result.worker_launches, 1);
        assert.equal(result.last_dispatch.mode, 'process');
        assert.ok(result.last_dispatch.stderr.bytes > 0);
        assert.equal(result.last_dispatch.stdout.sha256, h1.sha256(Buffer.from(result.last_dispatch.stdout.base64, 'base64')));
        if (mode === 'overflow') {
            assert.equal(result.last_dispatch.stdout.bytes, 1024);
            assert.ok(result.last_dispatch.observed.stdout > 1024);
        }
        if (mode === 'pass-after-exit-failure') assert.equal(result.last_dispatch.child.exit_code, 7);
        if (mode === 'signal') {
            assert.equal(result.last_dispatch.child.exit_code, null);
            assert.equal(result.last_dispatch.child.signal, 'SIGTERM');
            assert.deepEqual(fs.readdirSync(f.request.runRoot), ['a.json']);
        }
    }
});

test('callback exception and partial output preserve successful prefix and never dispatch later work', async t => {
    const f = await setup(t, {registryMutation: registry => { registry.workers[1].outputs = ['b-1.json', 'b-2.json']; }});
    const result = await runFormalTransaction(f.request, {dispatch: context => {
        writeOutputs(context, context.ordinal === 1 ? 'pass' : 'missing-file');
        if (context.ordinal === 2) throw new Error('worker callback exploded');
        return resultFor(context);
    }});
    assert.equal(result.status, 'FAIL');
    assert.equal(result.reason, 'WORKER_THROWN');
    assert.equal(result.workers.length, 1);
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b-2.json']);
    assert.equal(result.workers[0].outputs[0].sha256, h1.sha256(fs.readFileSync(path.join(f.request.runRoot, 'a.json'))));
    assert.match(result.last_dispatch.exception.message, /exploded/);
});

test('partial actual outputs and reordered declared output results have distinct refusals', async t => {
    for (const kind of ['partial', 'reordered']) {
        const f = await setup(t, {registryMutation: registry => { registry.workers[0].outputs = ['a-1.json', 'a-2.json']; }});
        const result = await runFormalTransaction(f.request, {dispatch: context => {
            writeOutputs(context, kind === 'partial' ? 'missing-file' : 'pass');
            const row = resultFor(context);
            if (kind === 'reordered') row.outputs.reverse();
            return row;
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, kind === 'partial' ? 'OUTPUT_MISSING' : 'WORKER_OUTPUT_MISMATCH');
        assert.equal(result.worker_launches, 1);
        assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), kind === 'partial' ? ['a-2.json'] : ['a-1.json', 'a-2.json']);
    }
});

test('non-Error callback exceptions retain a bounded serializable WORKER_THROWN diagnosis', async t => {
    const getterError = Object.defineProperty({}, 'message', {get() { throw new Error('getter failed'); }});
    for (const thrown of [null, undefined, 'plain string', getterError,
        {code: 'E\0TEST', message: 'x'.repeat(2047) + '\ud83d\ude00\0tail'}]) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {dispatch: context => {
            writeOutputs(context);
            throw thrown;
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, 'WORKER_THROWN');
        assert.equal(result.worker_launches, 1);
        const diagnostic = result.last_dispatch.exception;
        assert.equal(diagnostic.message.length <= 2048, true);
        assert.equal(diagnostic.message.includes('\0'), false);
        assert.equal(diagnostic.code.includes('\0'), false);
        assert.equal(Buffer.from(diagnostic.message).toString('utf8'), diagnostic.message);
        assert.equal(h1.parseCanonicalJson(h1.canonicalJson(result)).reason, 'WORKER_THROWN');
        assert.deepEqual(result.verification_failures, []);
        assert.deepEqual(fs.readdirSync(f.request.runRoot), ['a.json']);
    }
});

test('source and receipt mutation between validation boundaries refuse pre-root with no worker launches', async t => {
    for (const kind of ['input', 'receipt']) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {beforeCreate: () => {
            const p = kind === 'input' ? f.raw : f.receipts[0].receipt.stdout.path;
            fs.chmodSync(p, 0o600);
            fs.appendFileSync(p, 'mutated');
            if (kind === 'receipt') fs.chmodSync(p, 0o400);
        }});
        assertRefused(result, 'REVALIDATION', kind === 'input' ? 'INPUT_CHANGED' : 'RECEIPT_CHANGED', f.request);
    }
});

test('original root-parent replacement refuses pre-root and preserves both external directories', async t => {
    const f = await setup(t);
    const result = await runFormalTransaction(f.request, {beforeCreate: () => {
        fs.renameSync(f.runParent, f.runParent + '.original');
        fs.mkdirSync(f.runParent, {mode: 0o700});
    }});
    assertRefused(result, 'ROOT_PREFLIGHT', 'PARENT_IDENTITY_CHANGED', f.request);
    assert.equal(fs.existsSync(f.runParent + '.original'), true);
});

test('original receipt root and storage parent guards are retained across pre/post dispatch', async t => {
    for (const kind of ['reserved-root', 'receipt-storage']) for (const boundary of ['pre-root', 'post-dispatch']) {
        const f = await setup(t);
        const p = kind === 'reserved-root' ? path.dirname(f.config.receipt_roots[0]) :
            path.dirname(path.dirname(f.request.receiptPaths[0]));
        const change = () => fs.chmodSync(p, 0o750);
        const result = await runFormalTransaction(f.request, boundary === 'pre-root' ? {beforeCreate: change} : {
            dispatch: context => { writeOutputs(context); change(); return resultFor(context); }
        });
        assert.equal(result.status, boundary === 'pre-root' ? 'REFUSED' : 'FAIL', result.message);
        assert.equal(result.stage, 'REVALIDATION', result.message);
        assert.equal(result.reason, 'RECEIPT_CHANGED', result.message);
        assert.equal(result.worker_launches, boundary === 'pre-root' ? 0 : 1);
        assert.equal(result.root_created, boundary !== 'pre-root');
        assert.equal(fs.existsSync(f.request.runRoot), boundary !== 'pre-root');
        assert.equal(fs.statSync(p).mode & 0o7777, 0o750);
    }
});

test('exclusive-root CAS preserves an independently created root/sentinel without dispatch', async t => {
    const f = await setup(t);
    let created;
    const io = {...h2.nodeFsAdapter, mkdirSync: (p, options) => {
        if (p === f.request.runRoot) {
            fs.mkdirSync(p, options);
            fs.writeFileSync(path.join(p, 'sentinel'), 'external root');
            created = tree(p);
        }
        return fs.mkdirSync(p, options);
    }};
    const result = await runFormalTransaction(f.request, {fs: io});
    assert.equal(result.status, 'REFUSED');
    assert.equal(result.stage, 'ROOT_CAS');
    assert.equal(result.reason, 'ROOT_ALREADY_EXISTS');
    assert.equal(result.root_created, false);
    assert.equal(result.worker_launches, 0);
    assert.deepEqual(tree(f.request.runRoot), created);
});

test('post-dispatch input and previous-output changes refuse before any later launch', async t => {
    for (const kind of ['input', 'output']) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {dispatch: context => {
            writeOutputs(context);
            if (kind === 'input') fs.appendFileSync(f.raw, 'changed after dispatch');
            else if (context.ordinal === 2) {
                const p = path.join(context.run_root, 'a.json');
                fs.chmodSync(p, 0o600); fs.appendFileSync(p, 'changed'); fs.chmodSync(p, 0o400);
            }
            return resultFor(context);
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.stage, 'REVALIDATION');
        assert.equal(result.reason, kind === 'input' ? 'INPUT_CHANGED' : 'OUTPUT_CHANGED');
        assert.equal(result.worker_launches, kind === 'input' ? 1 : 2);
    }
});

test('validation purpose refuses synthetic dependency injection before ownership', async t => {
    const f = await setup(t, {capture: false, configMutation: config => { config.purpose = config.preflight.purpose = 'validation'; }});
    const before = tree(f.base);
    for (const dependencies of [
        {dispatch: () => undefined},
        Object.create({dispatch: () => undefined}),
        Object.create({spawn: () => undefined}),
        Object.defineProperty({}, 'beforeCreate', {value: () => undefined}),
        Object.defineProperty({}, 'fs', {value: h2.nodeFsAdapter})
    ]) assertRefused(await runFormalTransaction(f.request, dependencies), 'REGISTRY', 'SYNTHETIC_INJECTION_FORBIDDEN', f.request);
    assert.deepEqual(tree(f.base), before);
});

test('validation establishes purpose through trusted I/O without consulting injected adapters', async t => {
    const f = await setup(t, {capture: false, configMutation: config => { config.purpose = config.preflight.purpose = 'validation'; }});
    const before = tree(f.base);
    let calls = 0;
    const adapter = new Proxy({...h2.nodeFsAdapter}, {get(target, key) { calls++; return Reflect.get(target, key); }});
    const result = await runFormalTransaction(f.request, {fs: adapter});
    assertRefused(result, 'REGISTRY', 'SYNTHETIC_INJECTION_FORBIDDEN', f.request);
    assert.equal(calls, 0);
    assert.deepEqual(tree(f.base), before);
});

test('validation refuses own and inherited hook getters without evaluating them', async t => {
    const f = await setup(t, {capture: false, configMutation: config => { config.purpose = config.preflight.purpose = 'validation'; }});
    const before = tree(f.base);
    let reads = 0;
    for (const key of ['fs', 'spawn', 'dispatch', 'beforeCreate']) for (const shape of ['own', 'hidden', 'inherited']) {
        const holder = Object.defineProperty({}, key, {enumerable: shape === 'own', get() {
            reads++; throw new Error('forbidden hook getter executed');
        }});
        const dependencies = shape === 'inherited' ? Object.create(holder) : holder;
        const result = await runFormalTransaction(f.request, dependencies);
        assertRefused(result, 'REGISTRY', 'SYNTHETIC_INJECTION_FORBIDDEN', f.request);
        assert.equal(reads, 0);
    }
    assert.deepEqual(tree(f.base), before);
});

test('registry/result schema descriptors are immutable and all result statuses are explicit', () => {
    assert.equal(Object.isFrozen(registryApi.WORKER_KEYS), true);
    assert.equal(Object.isFrozen(registryApi.LIMITS), true);
    assert.throws(() => registryApi.WORKER_KEYS.push('run'), TypeError);
});

test('oversized derived worker context is a pre-root refusal, not an owned failed attempt', async t => {
    const f = await setup(t, {inventoryFromRegistry: true, registryMutation: registry => {
        registry.workers[0].case_ids = Array.from({length: 128}, (_, i) => `case-a-${String(i).padStart(3, '0')}-${'x'.repeat(116)}`);
        registry.workers[0].outputs = Array.from({length: 127}, (_, i) => `out-${String(i).padStart(3, '0')}-${'x'.repeat(120)}`);
    }});
    const before = tree(f.base);
    assertRefused(await runFormalTransaction(f.request), 'REGISTRY', 'LIMIT_INVALID', f.request);
    assert.deepEqual(tree(f.base), before);
});

test('synchronous process error and stdin error retain bounded diagnostics without a second launch', async t => {
    for (const kind of ['spawn', 'stdin']) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {spawn: () => {
            if (kind === 'spawn') throw Object.assign(new Error('injected spawn EACCES'), {code: 'EACCES'});
            const child = new EventEmitter();
            child.stdin = new Writable({write: (bytes, encoding, callback) => callback(Object.assign(new Error('injected EPIPE'), {code: 'EPIPE'}))});
            child.stdout = new PassThrough();
            child.stderr = new PassThrough();
            child.kill = () => true;
            setImmediate(() => { child.stdout.end('partial stdout\n'); child.stderr.end('partial stderr\n'); child.emit('close', 7, null); });
            return child;
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, 'WORKER_PROCESS_ERROR');
        assert.equal(result.worker_launches, 1);
        assert.equal(result.last_dispatch.child.error.code, kind === 'spawn' ? 'EACCES' : 'EPIPE');
        if (kind === 'stdin') {
            assert.equal(Buffer.from(result.last_dispatch.stdout.base64, 'base64').toString(), 'partial stdout\n');
            assert.equal(Buffer.from(result.last_dispatch.stderr.base64, 'base64').toString(), 'partial stderr\n');
            assert.equal(result.last_dispatch.stdin.write_completed, false);
        }
    }
});

test('asynchronous spawn errors and invalid synthetic child handles cannot produce PASS', async t => {
    for (const kind of ['async-error', 'invalid-child']) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {spawn: () => {
            if (kind === 'invalid-child') return {stdin: {}, stdout: {}, stderr: {},
                kill() { throw new Error('invalid synthetic kill'); }, on() {}};
            const child = new EventEmitter();
            child.stdin = new Writable({write: (bytes, encoding, callback) => callback()});
            child.stdout = new PassThrough();
            child.stderr = new PassThrough();
            child.kill = () => true;
            setImmediate(() => {
                child.emit('error', Object.assign(new Error('injected asynchronous ENOENT'), {code: 'ENOENT'}));
                child.stdout.end(); child.stderr.end('spawn diagnostic\n'); child.emit('close', null, null);
            });
            return child;
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, 'WORKER_PROCESS_ERROR');
        assert.equal(result.worker_launches, 1);
        assert.equal(result.last_dispatch.child.error.code, kind === 'async-error' ? 'ENOENT' : 'INVALID_CHILD');
        if (kind === 'async-error') assert.equal(Buffer.from(result.last_dispatch.stderr.base64, 'base64').toString(), 'spawn diagnostic\n');
        assert.deepEqual(fs.readdirSync(f.request.runRoot), []);
    }
});

test('symlink and hardlink outputs cannot be admitted as actual single-link output bytes', async t => {
    for (const kind of ['symlink', 'hardlink']) {
        const f = await setup(t);
        const result = await runFormalTransaction(f.request, {dispatch: context => {
            const output = path.join(context.run_root, context.outputs[0]);
            if (kind === 'symlink') fs.symlinkSync(f.raw, output);
            else {
                const target = path.join(context.run_root, 'undeclared-link-target');
                fs.writeFileSync(target, 'linked bytes', {mode: 0o400});
                fs.linkSync(target, output);
                fs.unlinkSync(target);
                fs.linkSync(output, path.join(f.base, 'external-output-alias'));
            }
            return resultFor(context);
        }});
        assert.equal(result.status, 'FAIL');
        assert.equal(result.reason, 'OUTPUT_METADATA_MISMATCH', result.message);
        assert.equal(result.worker_launches, 1);
    }
});

test('owned root replacement after the first descriptor closes is not blessed by a fresh guard sample', async t => {
    const f = await setup(t);
    const paths = new Map();
    let changed = false;
    const io = {...h2.nodeFsAdapter,
        openSync: (p, ...args) => { const fd = fs.openSync(p, ...args); paths.set(fd, p); return fd; },
        closeSync: fd => {
            const p = paths.get(fd);
            paths.delete(fd);
            fs.closeSync(fd);
            if (!changed && p === f.request.runRoot) {
                changed = true;
                fs.renameSync(p, p + '.original');
                fs.mkdirSync(p, {mode: 0o700});
                fs.writeFileSync(path.join(p, 'external-sentinel'), 'replacement root');
            }
        }};
    const result = await runFormalTransaction(f.request, {fs: io});
    assert.equal(result.status, 'FAIL');
    assert.equal(result.reason, 'ROOT_CHANGED');
    assert.equal(result.worker_launches, 0);
    assert.equal(changed, true);
    assert.equal(fs.existsSync(f.request.runRoot + '.original'), true);
    assert.equal(fs.readFileSync(path.join(f.request.runRoot, 'external-sentinel'), 'utf8'), 'replacement root');
});

test('real second-worker failure retains the first verified output and diagnostic prefix without persistence claims', async t => {
    const f = await setup(t, {modes: ['pass', 'malformed']});
    const result = realCli(argumentsFor(f.request));
    assert.equal(result.status, 'FAIL');
    assert.equal(result.reason, 'WORKER_RESULT_PARSE_FAILED');
    assert.equal(result.worker_launches, 2);
    assert.equal(result.workers.length, 1);
    assert.equal(result.workers[0].outputs[0].sha256, h1.sha256(fs.readFileSync(path.join(f.request.runRoot, 'a.json'))));
    assert.equal(Buffer.from(result.last_dispatch.stdout.base64, 'base64').toString(), 'not JSON\n');
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
    assert.equal(Object.hasOwn(result, 'ledger_rows'), false);
});
