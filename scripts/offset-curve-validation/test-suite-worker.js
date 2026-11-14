/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const test = require('node:test');
const inventory = require('./suite-inventory');
const worker = require('./suite-worker');
const generator = require('./generate-suite-inventory');
const registryApi = require('./formal-registry');
const {captureInputs} = require('./input-capture');

const env = {LC_ALL: 'C', ...(process.platform === 'darwin' ? {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})};
const clone = value => JSON.parse(inventory.canonical(value));
const tinyPass = "const test = require('node:test'); const assert = require('node:assert/strict');\ntest('real first case', () => { assert.equal(2 + 2, 4); });\ntest('real second case', () => { assert.ok(true); });\n";

function fixture(t, source = tinyPass) {
    const root = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-h7-worker-'));
    t.diagnostic(`attempt artifacts: ${root}; retained on unexpected failure`);
    const groups = inventory.SUITES.map((suite, i) => {
        const files = suite.files.map((name, j) => {
            const file = path.join(root, `${i}-${j}-${name}`); fs.writeFileSync(file, source); return file;
        });
        return {name: suite.name, files};
    });
    const options = {purpose: 'synthetic', definitionsPath: path.join(root, 'definitions.json'), registryPath: path.join(root, 'registry.json'),
        inventoryPath: path.join(root, 'cases.json'), groups, environment: env};
    return {root, groups, options, cleanup: () => fs.rmSync(root, {recursive: true, force: true})};
}

let generated, sharedRoot;
let declared = 0, passed = 0;
function checkedTest(name, fn) {
    declared++;
    test(name, async t => { await fn(t); passed++; });
}
test.before(async () => {
    // Shared tiny real modules keep these controls bounded; they never qualify the canonical suites.
    const root = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-h7-shared-'));
    sharedRoot = root;
    const groups = inventory.SUITES.map((suite, i) => ({name: suite.name, files: suite.files.map((name, j) => {
        const file = path.join(root, `${i}-${j}-${name}`); fs.writeFileSync(file, tinyPass); return file;
    })}));
    const options = {purpose: 'synthetic', definitionsPath: path.join(root, 'definitions.json'), registryPath: path.join(root, 'registry.json'),
        inventoryPath: path.join(root, 'cases.json'), groups, environment: env};
    generated = {root, options, ...await generator.generateAndPublish(options)};
});
test.after(() => {
    if (generated && passed === declared) fs.rmSync(generated.root, {recursive: true, force: true});
    else if (sharedRoot) process.stderr.write(`Retained unexpected failed suite-worker controls: ${sharedRoot}\n`);
});

checkedTest('canonical groups name the actual seven existing test modules in order', () => {
    assert.deepEqual(inventory.SUITES.map(suite => suite.name), ['receipt-schema', 'capture-readback', 'formal-runner-boundary', 'genesis-ledger-p0f-joins']);
    assert.equal(inventory.SUITES.flatMap(suite => suite.files).length, 7);
    for (const file of inventory.SUITES.flatMap(suite => suite.files)) assert.ok(fs.statSync(path.join(__dirname, file)).isFile());
});

checkedTest('synthetic option snapshots retain supported one-module control groups', () => {
    const options = {...generated.options, groups: generated.options.groups.map(group => ({...group, files: group.files.slice(0, 1)}))};
    const saved = inventory.snapshotGeneratorOptions(options);
    assert.deepEqual(saved.groups.map(group => group.files.length), [1, 1, 1, 1]);
    assert.equal(saved.purpose, 'synthetic');
    assert.throws(() => inventory.snapshotGeneratorOptions({...options, purpose: 'validation'}), /cannot replace/);
});

checkedTest('real discovery reports skipped definitions without qualifying execution', async () => {
    const observed = await worker.discoverTests(generated.options.groups[0].files, {environment: env});
    assert.equal(observed.status, 'DISCOVERED', observed.reason);
    assert.deepEqual(observed.tests.map(row => row.name), ['real first case', 'real second case']);
    assert.ok(observed.tests.every(row => row.outcome === 'SKIP'));
    const suite = generated.definitions.suites[0];
    const report = worker.semanticReport(suite, 'synthetic', observed.capture);
    assert.equal(report.status, 'FAIL'); assert.equal(report.assertions, 0);
});

checkedTest('generator pins real worker/runtime/test definitions and preserves strict H3 source mapping', () => {
    const {definitions, registry, inventory: cases} = generated;
    assert.equal(registry.workers.length, 4);
    assert.ok(cases.cases.every(row => row.source_path === inventory.WORKER_PATH));
    assert.ok(definitions.suites.every(suite => suite.files.every(file => definitions.sources.some(source => source.sha256 === file.sha256 && source.path === file.path))));
    assert.equal(definitions.runtime.version, process.version);
    assert.deepEqual(definitions.worker, inventory.pinFile(inventory.WORKER_PATH));
    assert.equal(new Set(cases.cases.map(row => row.id)).size, cases.cases.length);
    assert.deepEqual(cases.cases.map(row => row.id), cases.cases.map(row => row.id).sort());
    const inputs = captureInputs({purpose: 'synthetic', candidatePath: inventory.WORKER_PATH, runnerPath: inventory.WORKER_PATH,
        ...generated.capture, caseInventoryPath: generated.options.inventoryPath});
    registryApi.validateRegistry(registry, inputs);
});

checkedTest('real suite execution yields deterministic semantics and retains separate actual diagnostics', async () => {
    const one = await worker.executeSuite(generated.definitions, 'receipt-schema', {environment: env});
    const two = await worker.executeSuite(generated.definitions, 'receipt-schema', {environment: env});
    assert.equal(one.report.status, 'PASS', one.report.reason); assert.equal(two.report.status, 'PASS', two.report.reason);
    assert.deepEqual(one.semantic, two.semantic);
    assert.equal(one.assertions, 2); assert.equal(one.assertion_unit, inventory.ASSERTION_UNIT);
    assert.equal(one.report.cases.length, 2);
    assert.equal(one.diagnostics.capture.mode, 'execution');
    assert.equal(one.diagnostics.capture.child.exit_code, 0);
    assert.deepEqual(one.diagnostics.capture.environment, env);
    assert.equal(inventory.hash(one.rawEvents), one.diagnostics.capture.stdout.sha256);
    assert.equal(inventory.hash(one.semantic), one.diagnostics.semantic.sha256);
    assert.ok(worker.decodeEvents(one.diagnostics.capture).some(event => event.type === 'test:pass'));
    assert.equal(one.semantic.includes(Buffer.from('duration_ms')), false);
    assert.equal(one.semantic.includes(Buffer.from(generated.root)), false);
});

checkedTest('CLI executes real tests and emits the exact H3 result/output inventory', () => {
    const suite = generated.definitions.suites[0], runRoot = path.join(generated.root, 'worker-output'); fs.mkdirSync(runRoot, {mode: 0o700});
    const context = {schema_version: 1, purpose: 'synthetic', worker_id: suite.name, ordinal: 1,
        case_ids: suite.cases.map(row => row.id), outputs: suite.outputs, run_root: runRoot,
        candidate: generated.definitions.worker, source_manifest_sha256: 'a'.repeat(64), registry_sha256: 'b'.repeat(64)};
    const child = spawnSync(process.execPath, [inventory.WORKER_PATH, '--definitions', generated.options.definitionsPath, '--suite', suite.name],
        {env, input: inventory.canonical(context), maxBuffer: inventory.LIMITS.stderrBytes, timeout: 10000});
    assert.equal(child.status, 0, String(child.stderr));
    const result = JSON.parse(child.stdout);
    registryApi.validateWorkerResult(result, generated.registry.workers[0], 1, 'synthetic');
    assert.deepEqual(fs.readdirSync(runRoot).sort(), suite.outputs);
    for (const name of suite.outputs) assert.equal(fs.statSync(path.join(runRoot, name)).mode & 0o7777, 0o400);
    const diagnostics = JSON.parse(fs.readFileSync(path.join(runRoot, suite.outputs[0])));
    assert.deepEqual(diagnostics.context, context);
    assert.deepEqual(diagnostics.capture.environment, env);
    assert.equal(diagnostics.stdin.sha256, inventory.hash(inventory.canonical(context)));
});

checkedTest('skipped TODO failed and missing tests cannot produce a semantic PASS', async t => {
    let complete = 0;
    for (const [name, source] of [
        ['skip', "const t=require('node:test');t('real first case',{skip:true},()=>{});t('real second case',()=>{});"],
        ['todo', "const t=require('node:test');t('real first case',{todo:true},()=>{});t('real second case',()=>{});"],
        ['fail', "const t=require('node:test');t('real first case',()=>{throw Error('deliberate');});t('real second case',()=>{});"],
        ['missing', "const t=require('node:test');t('real first case',()=>{});"]]) {
        await t.test(name, async () => {
            const f = fixture(t); const file = f.groups[0].files[0];
            const capture = await worker.captureNode([file], {environment: env});
            assert.equal(capture.child.exit_code, 0);
            fs.writeFileSync(file, source);
            const changed = await worker.captureNode([file], {environment: env});
            const report = worker.semanticReport({...generated.definitions.suites[0], files: [{path: file}],
                cases: generated.definitions.suites[0].cases.map(row => ({...row, file}))}, 'synthetic', changed);
            assert.equal(report.status, 'FAIL');
            f.cleanup(); complete++;
        });
    }
    assert.equal(complete, 4, 'each negative control must complete every assertion');
});

checkedTest('wrong extra reordered and duplicate execution identities are refusals', async () => {
    const execution = await worker.executeSuite(generated.definitions, 'receipt-schema', {environment: env});
    for (const mutate of [rows => rows.reverse(), rows => rows.slice(0, 1), rows => [...rows, rows[0]],
        rows => rows.map((row, i) => i ? row : {...row, name: 'wrong name'})]) {
        const suite = {...generated.definitions.suites[0], cases: mutate(clone(generated.definitions.suites[0].cases))};
        assert.equal(worker.semanticReport(suite, 'synthetic', execution.diagnostics.capture).status, 'FAIL');
    }
});

checkedTest('source/runtime/environment substitutions refuse before genuine suite execution', async () => {
    for (const mutate of [value => { value.runtime.version = 'v0.0.0'; }, value => { value.sources[0].sha256 = '0'.repeat(64); },
        value => { value.worker.sha256 = '0'.repeat(64); }]) {
        const changed = clone(generated.definitions); mutate(changed);
        await assert.rejects(worker.executeSuite(changed, 'receipt-schema', {environment: env}));
    }
    await assert.rejects(worker.executeSuite(generated.definitions, 'receipt-schema', {environment: {...env, EXPLICIT: 'different'}}), /environment differs/);
    await assert.rejects(worker.executeSuite(generated.definitions, 'receipt-schema', {timeoutMs: inventory.LIMITS.timeoutMs + 1}), /raise pinned caps/);
});

checkedTest('pending H6 duplicate case IDs and registration-order mismatches fail closed', () => {
    for (const mutate of [value => { value.h6.pending_ids = ['H6-POST-01']; },
        value => { value.suites[0].cases[1].id = value.suites[0].cases[0].id; },
        value => { value.suites[0].cases.reverse(); }, value => { value.h6.required_ids = ['H6-CLI-01']; }]) {
        const changed = clone(generated.definitions); mutate(changed);
        assert.throws(() => inventory.validateDefinitions(changed));
    }
});

checkedTest('validation groups cannot use synthetic file substitutions or an unpinned roadmap', async () => {
    const changed = {...generated.options, purpose: 'validation'};
    await assert.rejects(inventory.generateSuiteInventory(changed), /cannot replace/);
    const definitions = clone(generated.definitions); definitions.purpose = 'validation';
    assert.throws(() => inventory.validateDefinitions(definitions), /canonical|absolute|snapshot/);
});

checkedTest('bounded real process timeout overflow syntax errors and truncated events are failures', async t => {
    const f = fixture(t); const file = f.groups[0].files[0];
    fs.writeFileSync(file, "const t=require('node:test');t('wait',()=>new Promise(()=>setInterval(()=>{},1000)));\n");
    const timeout = await worker.captureNode([file], {environment: env, timeoutMs: 100});
    assert.equal(timeout.failure, 'timeout');
    fs.writeFileSync(file, "const t=require('node:test');t('loud',()=>process.stdout.write('x'.repeat(65536)));\n");
    const overflow = await worker.captureNode([file], {environment: env, maxEventBytes: 2048});
    assert.equal(overflow.failure, 'stdout-overflow'); assert.ok(overflow.stdout.truncated); assert.equal(overflow.stdout.bytes, 2048);
    assert.throws(() => worker.decodeEvents(overflow), /binding differs|truncated/);
    fs.writeFileSync(file, 'not valid JavaScript !!!');
    const syntax = await worker.discoverTests([file], {environment: env});
    assert.equal(syntax.status, 'REFUSED');
    f.cleanup();
});

checkedTest('exclusive generated publications retain existing artifacts rather than overwrite them', () => {
    const before = fs.readFileSync(generated.options.definitionsPath);
    assert.throws(() => generator.writeGenerated(generated.options.definitionsPath, {wrong: true}), /EEXIST/);
    assert.deepEqual(fs.readFileSync(generated.options.definitionsPath), before);
});

checkedTest('H6 identities survive discovery in actual registration order with reproducible generated cases', async t => {
    const f = fixture(t);
    fs.writeFileSync(f.groups[2].files[1], "const t=require('node:test');\nt('inventory meta',()=>{});\n" +
        "t('H6-CLI-01: first retained identity',()=>{});\nt('H6-CLI-02: second retained identity',()=>{});\n");
    const first = await inventory.generateSuiteInventory(f.options);
    const second = await inventory.generateSuiteInventory(f.options);
    assert.deepEqual(first.definitions, second.definitions);
    assert.deepEqual(first.registry, second.registry); assert.deepEqual(first.inventory, second.inventory);
    assert.deepEqual(first.definitions.h6.required_ids, ['H6-CLI-01', 'H6-CLI-02']);
    assert.deepEqual(first.definitions.suites[2].cases.filter(row => row.file === f.groups[2].files[1]).map(row => row.id),
        ['H6-000-inventory', 'H6-CLI-01', 'H6-CLI-02']);
    f.cleanup();
});

checkedTest('discovery never executes test bodies and post-execution source mutation cannot qualify', async t => {
    const f = fixture(t);
    const file = f.groups[0].files[0];
    fs.writeFileSync(file, "const t=require('node:test'); const fs=require('node:fs');\n" +
        "t('mutating real testcase',()=>fs.appendFileSync(__filename,'\\n// actual mutation\\n'));\n");
    const before = fs.readFileSync(file);
    const definitions = (await inventory.generateSuiteInventory(f.options)).definitions;
    assert.deepEqual(fs.readFileSync(file), before, 'discovery must not execute the mutating body');
    const result = await worker.executeSuite(definitions, 'receipt-schema', {environment: env});
    assert.equal(result.diagnostics.capture.child.exit_code, 0, 'the real testcase itself passed');
    assert.equal(result.report.status, 'FAIL'); assert.equal(result.report.reason, 'SOURCE_REVALIDATION_FAILED');
    assert.match(result.diagnostics.verification_error, /pinned source\/input changed/);
    assert.notDeepEqual(fs.readFileSync(file), before);
    f.cleanup();
});

checkedTest('unbound corrupted or malformed raw event streams never default to PASS', async () => {
    const execution = await worker.executeSuite(generated.definitions, 'receipt-schema', {environment: env});
    const original = execution.diagnostics.capture;
    for (const change of [value => { value.stdout.sha256 = '0'.repeat(64); }, value => { value.stdout.bytes++; },
        value => { value.stdout.base64 += '!'; }]) {
        const altered = clone(original); change(altered);
        assert.equal(worker.semanticReport(generated.definitions.suites[0], 'synthetic', altered).status, 'FAIL');
    }
    const altered = clone(original), malformed = Buffer.from([0xff, 0x0a]);
    altered.stdout = {base64: malformed.toString('base64'), bytes: malformed.length, sha256: inventory.hash(malformed),
        observed_bytes: malformed.length, truncated: false};
    assert.equal(worker.semanticReport(generated.definitions.suites[0], 'synthetic', altered).status, 'FAIL');
});

checkedTest('generator discovery refusal retains actual bounded child diagnostics without publishing PASS', async t => {
    const f = fixture(t);
    fs.writeFileSync(f.groups[1].files[1], "process.stderr.write('genuine-discovery-refusal\\n');\n" +
        "throw new Error('genuine-discovery-refusal');\n");
    let refusal;
    await assert.rejects(generator.generateAndPublish(f.options), error => {
        refusal = error;
        assert.ok(error instanceof TypeError);
        assert.match(error.message, /^discovery refused capture-readback: discovery process did not complete$/);
        assert.equal(error.discovery.status, 'REFUSED');
        assert.equal(error.discovery.reason, 'discovery process did not complete');
        return true;
    });
    const {capture, tests} = refusal.discovery;
    assert.equal(capture.mode, 'discovery');
    assert.equal(capture.child.exit_code, 1); assert.equal(capture.child.signal, null); assert.equal(capture.child.error, null);
    assert.deepEqual(capture.argv.slice(-2), f.groups[1].files);
    assert.deepEqual(capture.environment, env);
    assert.deepEqual(tests.map(row => row.name), ['real first case', 'real second case']);
    assert.ok(tests.every(row => row.outcome === 'SKIP'), 'a completed discovery prefix is not executed PASS');
    for (const name of ['stdout', 'stderr']) {
        const stream = capture[name], bytes = Buffer.from(stream.base64, 'base64');
        assert.equal(bytes.length, stream.bytes); assert.equal(inventory.hash(bytes), stream.sha256);
        assert.equal(stream.observed_bytes, stream.bytes); assert.equal(stream.truncated, false);
        assert.ok(stream.bytes <= (name === 'stdout' ? inventory.LIMITS.eventBytes : inventory.LIMITS.stderrBytes));
    }
    const events = worker.decodeEvents(capture);
    assert.ok(events.some(event => event.type === 'test:fail'));
    assert.match(Buffer.from(capture.stdout.base64, 'base64').toString('utf8'), /genuine-discovery-refusal/);
    for (const key of ['definitionsPath', 'registryPath', 'inventoryPath']) assert.equal(fs.existsSync(f.options[key]), false);
    f.cleanup();
});

checkedTest('both generator entrypoints snapshot bounded own data without evaluating getters', async () => {
    let getterCalls = 0;
    const getter = () => { getterCalls++; throw new Error('caller getter must never execute'); };
    const mutations = [
        options => { Object.defineProperty(options, 'purpose', {get: getter}); },
        options => { Object.defineProperty(options, 'environment', {get: getter, enumerable: false}); },
        options => { Object.defineProperty(options.environment, 'secret', {get: getter, enumerable: true}); },
        options => { Object.defineProperty(options.groups[0], 'files', {get: getter}); },
        options => { Object.defineProperty(options.groups[0].files, '0', {get: getter}); },
        options => { options[Symbol('hidden')] = 'not JSON'; },
        options => { options.environment[Symbol('hidden')] = 'not JSON'; },
        options => { Object.setPrototypeOf(options, {get inherited() { return getter(); }}); },
        options => { Object.setPrototypeOf(options.environment, {get inherited() { return getter(); }}); },
        options => { delete options.purpose; Object.setPrototypeOf(options, {get purpose() { return getter(); }}); },
        options => { delete options.groups[0].files[0]; },
        options => { options.environment.self = options.environment; },
        options => { options.environment.toJSON = getter; },
        options => { options.extra = 'not one of the six fields'; },
        options => { options.environment.large = 'x'.repeat(inventory.LIMITS.generatorBytes + 1); }
    ];
    for (const entrypoint of [inventory.generateSuiteInventory, generator.generateAndPublish]) {
        for (const mutate of mutations) {
            const options = clone(generated.options); mutate(options);
            await assert.rejects(entrypoint(options), TypeError);
        }
    }
    assert.equal(getterCalls, 0);
    const options = clone(generated.options), saved = inventory.snapshotGeneratorOptions(options);
    assert.notEqual(saved, options); assert.notEqual(saved.environment, options.environment);
    assert.notEqual(saved.groups, options.groups); assert.notEqual(saved.groups[0].files, options.groups[0].files);
    assert.ok(Object.isFrozen(saved)); assert.ok(Object.isFrozen(saved.environment));
    assert.ok(Object.isFrozen(saved.groups)); assert.ok(Object.isFrozen(saved.groups[0])); assert.ok(Object.isFrozen(saved.groups[0].files));
    assert.equal(Object.isFrozen(options), false); assert.equal(Object.isFrozen(options.environment), false);
    assert.equal(Object.isFrozen(options.groups[0].files), false);
});

checkedTest('caller mutation during discovery cannot redirect publication or promote captured purpose', async t => {
    const f = fixture(t), options = f.options;
    options.environment = {...env, LC_H7_SNAPSHOT_CONTROL: 'original'};
    const paths = {definitionsPath: options.definitionsPath, registryPath: options.registryPath, inventoryPath: options.inventoryPath};
    const files = f.groups.map(group => [...group.files]);
    const pending = generator.generateAndPublish(options);
    const redirected = [];
    for (const key of Object.keys(paths)) {
        options[key] = path.join(f.root, `redirected-${key}.json`); redirected.push(options[key]);
    }
    options.purpose = 'validation';
    options.environment.LC_H7_SNAPSHOT_CONTROL = 'mutated';
    options.environment.NEW_CALLER_VALUE = 'must not leak';
    options.groups[0].name = 'caller-mutated-suite';
    options.groups[0].files[0] = path.join(f.root, 'caller-missing-module.js');
    options.groups.push({name: 'extra', files: []});
    options.groups = null;
    const result = await pending;
    assert.equal(options.purpose, 'validation', 'the implementation must not freeze or restore the caller');
    assert.equal(Object.isFrozen(options), false);
    assert.equal(result.definitions.purpose, 'synthetic'); assert.equal(result.registry.purpose, 'synthetic'); assert.equal(result.inventory.purpose, 'synthetic');
    assert.deepEqual(result.definitions.environment, {...env, LC_H7_SNAPSHOT_CONTROL: 'original'});
    assert.deepEqual(result.definitions.suites.map(suite => suite.files.map(file => file.path)), files);
    for (const observed of result.discovery) assert.deepEqual(observed.capture.environment, {...env, LC_H7_SNAPSHOT_CONTROL: 'original'});
    assert.deepEqual(JSON.parse(fs.readFileSync(paths.definitionsPath)), result.definitions);
    assert.deepEqual(JSON.parse(fs.readFileSync(paths.registryPath)), result.registry);
    assert.deepEqual(JSON.parse(fs.readFileSync(paths.inventoryPath)), result.inventory);
    assert.ok(result.registry.workers.every(row => row.args[1] === paths.definitionsPath));
    assert.deepEqual(result.capture.configPaths, Object.values(paths));
    for (const file of redirected) assert.equal(fs.existsSync(file), false);
    f.cleanup();
});
