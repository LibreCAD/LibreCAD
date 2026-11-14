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
const vm = require('node:vm');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {capturePreflight} = require('./preflight-capture');
const suiteApi = require('./suite-inventory');
const {generateAndPublish} = require('./generate-suite-inventory');
const api = require('./genuine-preflight');
const worker = require('./suite-worker');

test('byte patch replay verifies source and target rather than trusting matching digest strings', () => {
    const base = Buffer.from('actual original\n'), product = Buffer.from('actual revised\n');
    const patch = api.replacementPatch(base, product);
    assert.deepEqual(api.replayPatch(base, patch), product);
    assert.throws(() => api.replayPatch(product, patch), /source differs/);
    assert.throws(() => api.replayPatch(base, {...patch, target: {...patch.target, base64: base.toString('base64')}}), /binding differs/);
    assert.throws(() => api.replayPatch(base, {...patch, from: {...patch.from, bytes: 0}}), /source differs/);
    assert.throws(() => api.replayPatch(base, {...patch, target: {...patch.target, bytes: 0}}), /binding differs/);
    assert.throws(() => api.replayPatch(base, {...patch, format: 'git'}), /source differs/);
    assert.throws(() => api.replayPatch(base, {...patch, extra: true}), /keys\/order/);
    assert.throws(() => api.replacementPatch(Buffer.alloc(8388609), product), /exceeds cap/);
});

test('byte patch identity and binary payload round trips remain explicitly byte-level', () => {
    const bytes = Buffer.from([0, 255, 13, 10, 128]);
    const patch = api.replacementPatch(bytes, bytes);
    assert.equal(patch.format, 'bounded-whole-file-replacement-v1');
    assert.deepEqual(api.replayPatch(bytes, patch), bytes);
    assert.deepEqual(api.replayPatch(Buffer.alloc(0), api.replacementPatch(Buffer.alloc(0), bytes)), bytes);
    assert.deepEqual(api.replayPatch(bytes, api.replacementPatch(bytes, Buffer.alloc(0))), Buffer.alloc(0));
});

async function fixture(t, {failSuite = -1} = {}) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-real-preflight-'));
    t.diagnostic(`attempt ${base}; retained on unexpected failure`);
    const environment = {LC_ALL: 'C', ...(process.platform === 'darwin' ? {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})};
    const groups = suiteApi.SUITES.map((suite, i) => {
        const file = path.join(base, `actual-case-${i}.js`);
        fs.writeFileSync(file, `const test=require('node:test');const assert=require('node:assert/strict');\ntest('actual arithmetic',()=>{assert.equal(2+3,${i === failSuite ? 6 : 5});});\n`);
        return {name: suite.name, files: [file]};
    });
    const options = {purpose: 'synthetic', definitionsPath: path.join(base, 'definitions.json'), registryPath: path.join(base, 'registry.json'),
        inventoryPath: path.join(base, 'cases.json'), groups, environment};
    const generated = await generateAndPublish(options);
    const configPath = path.join(base, 'formal.json'), qualificationPath = path.join(base, 'qualification.json');
    const basePath = path.join(base, 'base.cpp'), productPath = path.join(base, 'product.cpp'), candidatePath = path.join(base, 'candidate.cpp');
    fs.writeFileSync(basePath, 'int original = 1;\n'); fs.writeFileSync(productPath, 'int revised = 2;\n');
    fs.writeFileSync(candidatePath, 'int candidate = 3;\n');
    const diagnosticParent = path.join(base, 'diagnostics'), rootParent = path.join(base, 'root-parent'), receiptParent = path.join(base, 'receipt-parent');
    for (const p of [diagnosticParent, rootParent, receiptParent]) fs.mkdirSync(p, {mode: 0o700});
    const preflight = {purpose: 'synthetic', candidatePath, runnerPath: path.join(__dirname, 'genuine-preflight.js'),
        sourcePaths: [...new Set([...generated.capture.sourcePaths, ...['genuine-preflight.js', 'formal-registry.js', 'trace-probe.js',
            'independent-evidence-checker.js'].map(name => path.join(__dirname, name))])],
        inputPaths: [basePath, productPath], fixturePaths: [],
        configPaths: [configPath, qualificationPath, options.definitionsPath, options.registryPath], caseInventoryPath: options.inventoryPath,
        runnerArgs: [qualificationPath], cwd: base, environment};
    const formal = {schema_version: 1, purpose: 'synthetic', registry_path: options.registryPath,
        run_root: path.join(base, 'unused-worker-root'), receipt_ids: ['10000000-0000-4000-8000-000000000001', '20000000-0000-4000-8000-000000000002'],
        receipt_roots: [path.join(rootParent, 'reserved'), path.join(rootParent, 'other-reserved')], preflight};
    fs.writeFileSync(configPath, h1.canonicalJson(formal));
    const qualification = {schema_version: 1, purpose: 'synthetic', formal_config_path: configPath, suite_definitions_path: options.definitionsPath,
        base_path: basePath, product_path: productPath, candidate_path: candidatePath, diagnostic_parent: diagnosticParent};
    fs.writeFileSync(qualificationPath, h1.canonicalJson(qualification));
    return {base, preflight, formal, qualification, configPath, qualificationPath, diagnosticParent, rootParent, receiptParent, generated};
}

test('genuine runner executes two real attempts per suite and publishes a valid H1 capture', async t => {
    const f = await fixture(t);
    const result = await capturePreflight({...f.preflight, receiptId: f.formal.receipt_ids[0], runRoot: f.formal.receipt_roots[0],
        receiptDirectory: path.join(f.receiptParent, 'receipt')});
    const report = result.receipt.preflight_result;
    h1.validatePreflightResult(report);
    assert.equal(report.status, 'PASS');
    assert.equal(report.purpose, 'synthetic', 'tiny real child controls are not canonical validation qualification');
    assert.equal(report.inventory_gate.actual_count, 4);
    assert.equal(report.syntax.checked_file_count, result.inputs.manifest.files.filter(row => row.path.endsWith('.js')).length);
    assert.equal(report.audit_parity.primary_sha256, report.audit_parity.independent_sha256);
    assert.equal(report.audit_parity.primary_sha256, result.inputs.sourceManifestSha256);
    const evidence = h1.parseCanonicalJson(fs.readFileSync(result.validated.receipt.stderr.path), {maxBytes: 65536});
    assert.equal(evidence.kind, 'genuine-preflight-evidence');
    assert.equal(evidence.context_sha256, result.stdin.sha256);
    assert.equal(evidence.artifacts.length, 28);
    assert.deepEqual(evidence.artifacts.map(row => row.role), ['input-audit', 'patch-replays', 'syntax',
        ...suiteApi.SUITES.flatMap(suite => [1, 2].flatMap(ordinal => ['report', 'diagnostics', 'events']
            .map(kind => `suite:${suite.name}:attempt:${ordinal}:${kind}`))), 'trace-probe']);
    for (const binding of evidence.artifacts) {
        assert.equal(fs.statSync(binding.path).mode & 0o7777, 0o400);
        const bytes = fs.readFileSync(binding.path);
        assert.equal(bytes.length, binding.bytes); assert.equal(h1.sha256(bytes), binding.sha256);
    }
    for (const suite of report.suites) {
        assert.equal(suite.attempts.length, 2);
        assert.equal(suite.attempts[0].assertions, 1);
        assert.equal(suite.attempts[0].report_sha256, suite.attempts[1].report_sha256);
        for (const attempt of suite.attempts) {
            const artifact = evidence.artifacts.find(row => row.role === `suite:${suite.name}:attempt:${attempt.ordinal}:report`);
            assert.equal(artifact.sha256, attempt.report_sha256);
            const semantic = h1.parseCanonicalJson(fs.readFileSync(artifact.path));
            assert.equal(semantic.status, 'PASS');
            assert.equal(semantic.assertions, 1);
            assert.ok(semantic.assertion_unit.includes('not JavaScript assertion calls'));
        }
    }
    assert.ok(!fs.existsSync(f.formal.receipt_roots[0]));
    h2.revalidateInputs(result.inputs);
    fs.rmSync(f.base, {recursive: true});
});

function contextFor(f) {
    const inputs = h2.captureInputs(f.preflight), parent = fs.lstatSync(f.rootParent, {bigint: true});
    return {purpose: 'synthetic', candidate: inputs.candidate, source_manifest_sha256: inputs.sourceManifestSha256,
        case_inventory_path: f.preflight.caseInventoryPath, case_count: inputs.caseIds.length,
        case_ids_sha256: inputs.manifest.case_ids_sha256, runtime: inputs.runtime,
        run_root: {path: f.formal.receipt_roots[0], parent: {path: f.rootParent, device: String(parent.dev), inode: String(parent.ino)}}};
}

function loadPreflight({execute = worker.executeSuite, input = h2, filesystem = fs} = {}) {
    const file = path.join(__dirname, 'genuine-preflight.js'), module = {exports: {}};
    vm.runInThisContext(`(function(require,module,exports,__dirname,__filename){\n${fs.readFileSync(file, 'utf8')}\n})`)(name => {
        if (name === './suite-worker') return {...worker, executeSuite: execute};
        if (name === './input-capture') return input;
        if (name === 'node:fs') return filesystem;
        return require(name);
    }, module, module.exports, __dirname, file);
    return module.exports;
}

function diagnosticFiles(f) {
    const directories = fs.readdirSync(f.diagnosticParent);
    assert.equal(directories.length, 1);
    const directory = path.join(f.diagnosticParent, directories[0]);
    return {directory, names: fs.readdirSync(directory).sort()};
}

// The real Node executions are unmodified. Delay only their return boundaries
// until four settle, then release them in reverse order to test the scheduler.
function reverseWaveTracker({rejectIndex = -1, afterWave = () => {}} = {}) {
    const state = {started: [], completed: [], observed: [], active: 0, peak: 0};
    const waves = [];
    state.execute = async (...args) => {
        const index = state.started.length, waveIndex = Math.floor(index / 4);
        state.started.push(args[1]); state.active++; state.peak = Math.max(state.peak, state.active);
        if (!waves[waveIndex]) {
            const wave = {ready: 0, error: null, slots: Array.from({length: 4}, () => {
                let resolve; const promise = new Promise(done => { resolve = done; }); return {promise, resolve};
            })};
            wave.timer = setTimeout(() => {
                wave.error = new Error('scheduler did not settle a four-execution wave');
                wave.slots.forEach(slot => slot.resolve());
            }, 30000);
            waves[waveIndex] = wave;
        }
        const wave = waves[waveIndex];
        let actual, failure;
        try { actual = await worker.executeSuite(...args); state.observed[index] = actual; }
        catch (error) { failure = error; }
        if (++wave.ready === 4) {
            clearTimeout(wave.timer);
            try { afterWave(waveIndex); } catch (error) { wave.error = error; }
            setImmediate(() => [...wave.slots].reverse().forEach(slot => slot.resolve()));
        }
        try {
            await wave.slots[index % 4].promise;
            state.completed.push(index);
            if (wave.error) throw wave.error;
            if (failure) throw failure;
            if (index === rejectIndex) throw new Error('intentional post-execution rejection');
            return actual;
        } finally { state.active--; }
    };
    return state;
}

test('two suite pairs overlap within four real executions with canonical publication and all original guards', async t => {
    const f = await fixture(t), context = contextFor(f), tracker = reverseWaveTracker();
    let inputChecks = 0, rootChecks = 0;
    const instrumented = loadPreflight({execute: tracker.execute,
        input: {...h2, revalidateInputs(...args) { inputChecks++; return h2.revalidateInputs(...args); }},
        filesystem: {...fs, lstatSync(p, ...args) { if (p === context.run_root.path) rootChecks++; return fs.lstatSync(p, ...args); }}});
    const out = await instrumented.runPreflight(context, f.qualificationPath);
    h1.validatePreflightResult(out.result);
    assert.equal(out.result.status, 'PASS'); assert.equal(out.result.purpose, 'synthetic');
    assert.equal(tracker.peak, 4); assert.equal(tracker.active, 0);
    assert.deepEqual(tracker.started, suiteApi.SUITES.flatMap(suite => [suite.name, suite.name]));
    assert.deepEqual(tracker.completed, [3, 2, 1, 0, 7, 6, 5, 4]);
    assert.equal(inputChecks, 9, 'four before, four after, and final original-input revalidations');
    assert.equal(rootChecks, 10, 'initial, four before, four after, and final reserved-root guards');
    assert.deepEqual(out.result.suites.map(row => row.name), h1.PREFLIGHT_SUITES);
    assert.deepEqual(out.evidence.artifacts.map(row => row.role), ['input-audit', 'patch-replays', 'syntax',
        ...suiteApi.SUITES.flatMap(suite => [1, 2].flatMap(ordinal => ['report', 'diagnostics', 'events']
            .map(kind => `suite:${suite.name}:attempt:${ordinal}:${kind}`))), 'trace-probe']);
    assert.equal(out.result.caps.checks.find(row => row.name === 'actual-suite-execution-count').measured, 8);
    for (const actual of tracker.observed) {
        assert.equal(actual.report.status, 'PASS'); assert.equal(actual.assertions, 1);
        assert.equal(actual.diagnostics.capture.mode, 'execution');
        assert.ok(actual.rawEvents.length > 0);
    }
    fs.rmSync(f.base, {recursive: true});
});

test('a completed pair receives its original post-guards while the other real pair is still pending', async t => {
    const f = await fixture(t), completed = [];
    let release, checks = 0, guardedWhilePending = false;
    const gate = new Promise(done => { release = done; });
    const timer = setTimeout(() => release(), 30000);
    const instrumented = loadPreflight({execute: async (...args) => {
        const actual = await worker.executeSuite(...args);
        if (args[1] === h1.PREFLIGHT_SUITES[0]) await gate;
        completed.push(args[1]); return actual;
    }, input: {...h2, revalidateInputs(...args) {
        const actual = h2.revalidateInputs(...args);
        if (++checks === 3) {
            assert.deepEqual(completed, [h1.PREFLIGHT_SUITES[1], h1.PREFLIGHT_SUITES[1]],
                'the second pair must be guarded before awaiting the first pair');
            guardedWhilePending = true; release();
        }
        return actual;
    }}});
    try {
        const out = await instrumented.runPreflight(contextFor(f), f.qualificationPath);
        h1.validatePreflightResult(out.result);
        assert.equal(guardedWhilePending, true); assert.equal(checks, 9); assert.equal(completed.length, 8);
    } finally { clearTimeout(timer); release(); }
    fs.rmSync(f.base, {recursive: true});
});

test('a completed canonical pair is durably published before the later real pair finishes', async t => {
    const f = await fixture(t), context = contextFor(f), completed = [];
    const directory = path.join(f.diagnosticParent, h1.sha256(h1.canonicalJson(context.run_root)).slice(0, 32));
    let release, checks = 0, publishedWhilePending = false;
    const gate = new Promise(done => { release = done; });
    const timer = setTimeout(() => release(), 30000);
    const instrumented = loadPreflight({execute: async (...args) => {
        const actual = await worker.executeSuite(...args);
        if (args[1] === h1.PREFLIGHT_SUITES[1]) await gate;
        completed.push(args[1]); return actual;
    }, input: {...h2, revalidateInputs(...args) {
        const actual = h2.revalidateInputs(...args);
        if (++checks === 3) setImmediate(() => {
            const last = path.join(directory, `${h1.PREFLIGHT_SUITES[0]}-2.events.jsonl`);
            publishedWhilePending = fs.existsSync(last) && (fs.lstatSync(last).mode & 0o7777) === 0o400 &&
                !completed.includes(h1.PREFLIGHT_SUITES[1]);
            release();
        });
        return actual;
    }}});
    try {
        const out = await instrumented.runPreflight(context, f.qualificationPath);
        h1.validatePreflightResult(out.result);
        assert.equal(publishedWhilePending, true); assert.equal(checks, 9); assert.equal(completed.length, 8);
    } finally { clearTimeout(timer); release(); }
    fs.rmSync(f.base, {recursive: true});
});

test('a rejected execution waits for every launched sibling and retains their actual diagnostics', async t => {
    const f = await fixture(t), tracker = reverseWaveTracker({rejectIndex: 0});
    await assert.rejects(loadPreflight({execute: tracker.execute}).runPreflight(contextFor(f), f.qualificationPath),
        /intentional post-execution rejection/);
    assert.equal(tracker.started.length, 4); assert.equal(tracker.completed.length, 4); assert.equal(tracker.active, 0);
    const {directory, names} = diagnosticFiles(f);
    assert.equal(names.length, 12); // Three initial artifacts and three available executions, three artifacts each.
    for (let index = 1; index < 4; index++) {
        const prefix = `${tracker.started[index]}-${index % 2 + 1}`, actual = tracker.observed[index];
        assert.deepEqual(fs.readFileSync(path.join(directory, `${prefix}.report.json`)), actual.semantic);
        assert.deepEqual(fs.readFileSync(path.join(directory, `${prefix}.events.jsonl`)), actual.rawEvents);
        assert.deepEqual(fs.readFileSync(path.join(directory, `${prefix}.diagnostics.json`)), h1.canonicalJson(actual.diagnostics));
    }
    assert.ok(!names.includes(`${tracker.started[0]}-1.report.json`));
    assert.ok(!fs.existsSync(f.formal.receipt_roots[0]));
    fs.rmSync(f.base, {recursive: true});
});

test('publication failure still waits for the concurrently running real sibling pair', async t => {
    const f = await fixture(t), completed = [];
    let release;
    const gate = new Promise(done => { release = done; });
    const instrumented = loadPreflight({execute: async (...args) => {
        const actual = await worker.executeSuite(...args);
        if (args[1] === h1.PREFLIGHT_SUITES[1]) await gate;
        completed.push(args[1]); return actual;
    }, filesystem: {...fs, openSync(p, ...args) {
        if (p.endsWith(`${h1.PREFLIGHT_SUITES[0]}-1.report.json`)) {
            setImmediate(() => release());
            throw new Error('intentional publication refusal');
        }
        return fs.openSync(p, ...args);
    }}});
    const timer = setTimeout(() => release(), 30000);
    try {
        await assert.rejects(instrumented.runPreflight(contextFor(f), f.qualificationPath), /intentional publication refusal/);
        assert.equal(completed.length, 4, 'publication failure must not abandon a launched sibling pair');
        assert.deepEqual(diagnosticFiles(f).names, ['input-audit.json', 'patch-replays.json', 'syntax.json']);
        assert.ok(!fs.existsSync(f.formal.receipt_roots[0]));
    } finally { clearTimeout(timer); release(); }
    fs.rmSync(f.base, {recursive: true});
});

test('a later launch guard refuses without abandoning the already launched real pair', async t => {
    const f = await fixture(t), started = [], completed = [];
    let checks = 0;
    const instrumented = loadPreflight({execute: async (...args) => {
        started.push(args[1]); const actual = await worker.executeSuite(...args); completed.push(actual); return actual;
    }, input: {...h2, revalidateInputs(...args) {
        if (++checks === 2) throw new Error('intentional second-pair launch refusal');
        return h2.revalidateInputs(...args);
    }}});
    await assert.rejects(instrumented.runPreflight(contextFor(f), f.qualificationPath), /intentional second-pair launch refusal/);
    assert.deepEqual(started, [h1.PREFLIGHT_SUITES[0], h1.PREFLIGHT_SUITES[0]]);
    assert.equal(completed.length, 2); assert.equal(checks, 3, 'the launched pair still receives its post-boundary check');
    const {directory, names} = diagnosticFiles(f);
    assert.equal(names.length, 9);
    for (let ordinal = 1; ordinal <= 2; ordinal++) {
        const prefix = `${h1.PREFLIGHT_SUITES[0]}-${ordinal}`;
        const report = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, `${prefix}.report.json`)));
        assert.equal(report.status, 'PASS'); assert.equal(report.assertions, 1);
        assert.ok(fs.statSync(path.join(directory, `${prefix}.events.jsonl`)).size > 0);
    }
    fs.rmSync(f.base, {recursive: true});
});

test('original input mutation after overlapping real executions retains both pairs before refusing', async t => {
    const f = await fixture(t), original = fs.readFileSync(f.qualification.product_path);
    const tracker = reverseWaveTracker({afterWave() { fs.writeFileSync(f.qualification.product_path, 'changed actual pinned product\n'); }});
    await assert.rejects(loadPreflight({execute: tracker.execute}).runPreflight(contextFor(f), f.qualificationPath), /captured input changed/);
    assert.equal(tracker.started.length, 4); assert.equal(tracker.completed.length, 4); assert.equal(tracker.active, 0);
    const {directory, names} = diagnosticFiles(f);
    assert.equal(names.length, 15);
    for (const suite of h1.PREFLIGHT_SUITES.slice(0, 2)) for (const ordinal of [1, 2]) {
        const report = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, `${suite}-${ordinal}.report.json`)));
        assert.equal(report.status, 'PASS'); assert.equal(report.assertions, 1);
        assert.ok(fs.statSync(path.join(directory, `${suite}-${ordinal}.events.jsonl`)).size > 0);
    }
    assert.notDeepEqual(fs.readFileSync(f.qualification.product_path), original);
    assert.ok(!names.includes('trace-probe.json'));
    fs.rmSync(f.base, {recursive: true});
});

test('real failed testcases cannot qualify or discard a concurrently completed suite pair', async t => {
    const f = await fixture(t, {failSuite: 0}), tracker = reverseWaveTracker();
    await assert.rejects(loadPreflight({execute: tracker.execute}).runPreflight(contextFor(f), f.qualificationPath), /actual suite failed/);
    assert.equal(tracker.started.length, 4); assert.equal(tracker.completed.length, 4); assert.equal(tracker.active, 0);
    const {directory, names} = diagnosticFiles(f);
    assert.equal(names.length, 15);
    for (let index = 0; index < 4; index++) {
        const prefix = `${tracker.started[index]}-${index % 2 + 1}`;
        const report = h1.parseCanonicalJson(fs.readFileSync(path.join(directory, `${prefix}.report.json`)));
        assert.equal(report.status, index < 2 ? 'FAIL' : 'PASS');
        assert.equal(report.cases[0].outcome, index < 2 ? 'FAIL' : 'PASS');
        assert.deepEqual(fs.readFileSync(path.join(directory, `${prefix}.events.jsonl`)), tracker.observed[index].rawEvents);
    }
    fs.rmSync(f.base, {recursive: true});
});

test('genuine preflight rejects forged context before creating any artifact namespace', async t => {
    const f = await fixture(t);
    const inputs = h2.captureInputs(f.preflight), parent = fs.lstatSync(f.rootParent, {bigint: true});
    const context = {purpose: 'synthetic', candidate: inputs.candidate, source_manifest_sha256: '0'.repeat(64),
        case_inventory_path: f.preflight.caseInventoryPath, case_count: inputs.caseIds.length,
        case_ids_sha256: inputs.manifest.case_ids_sha256, runtime: inputs.runtime,
        run_root: {path: f.formal.receipt_roots[0], parent: {path: f.rootParent, device: String(parent.dev), inode: String(parent.ino)}}};
    await assert.rejects(api.runPreflight(context, f.qualificationPath), /stdin differs/);
    assert.deepEqual(fs.readdirSync(f.diagnosticParent), []);
    assert.ok(!fs.existsSync(f.formal.receipt_roots[0]));
    h2.revalidateInputs(inputs);
    fs.rmSync(f.base, {recursive: true});
});
