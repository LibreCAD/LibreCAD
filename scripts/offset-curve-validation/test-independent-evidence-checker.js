/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const vm = require('node:vm');
const {spawnSync} = require('node:child_process');
const test = require('node:test');
const {setup} = require('./fixtures/evidence-transaction');
const h2 = require('./input-capture');
const {capturePreflight} = require('./preflight-capture');
const {runFormalTransaction} = require('./formal-runner');
const h4 = require('./formal-ledger');
const h5 = require('./p0f-evidence');
const checker = require('./independent-evidence-checker');
const {SCHEMA, CHECKS, LIMITS} = require('./evidence-descriptors');
const {generateSuiteInventory, SUITES, NORMALIZATION} = require('./suite-inventory');
const {ORIGINAL_IDS} = require('./refusal-inventory');

const checkerPath = path.join(__dirname, 'independent-evidence-checker.js');
const canonical = value => Buffer.from(`${JSON.stringify(value)}\n`);
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const readJson = p => JSON.parse(fs.readFileSync(p));
const identity = s => [s.dev, s.ino, s.mode, s.nlink, s.size, s.mtimeNs, s.ctimeNs].join(':');
function rewrite(p, bytes) { fs.chmodSync(p, 0o600); fs.writeFileSync(p, bytes); fs.chmodSync(p, 0o400); }
function binding(p, metadata = false) {
    const bytes = fs.readFileSync(p), stat = fs.lstatSync(p, {bigint: true});
    return {path: p, bytes: bytes.length, sha256: hash(bytes), ...(metadata ? {identity: identity(stat), mode: Number(stat.mode & 0o7777n)} : {})};
}
async function prepare(t, publish = true) {
    const f = await setup(t, {execute: false, extraSources: [checkerPath]});
    const config = readJson(f.checkerConfigPath);
    config.module_path = checkerPath; config.args = []; config.timeout_ms = 10000;
    fs.writeFileSync(f.checkerConfigPath, canonical(config));
    for (let i = 0; i < 2; i++) f.captures.push(await capturePreflight({...f.config.preflight,
        receiptId: f.config.receipt_ids[i], runRoot: f.receiptRoots[i], receiptDirectory: f.receiptDirs[i]}));
    f.h3 = await runFormalTransaction(f.request); assert.equal(f.h3.status, 'PASS', f.h3.message);
    f.ledgerContext = h4.captureLedgerContext({configPath: f.configPath, receiptPaths: f.request.receiptPaths});
    f.ledgerDocument = h4.buildJoinedLedger(f.h3, f.ledgerContext);
    f.ledgerReadback = h4.publishJoinedLedger(f.ledgerDocument, f.ledgerPath);
    Object.assign(f.evidenceRequest, {h3Result: f.h3, ledgerReadback: f.ledgerReadback, preflightCaptures: f.captures});
    if (publish) {
        f.evidenceContext = h5.captureEvidenceContext(f.evidenceRequest);
        f.previous = h5.publishEvidenceManifest(f.evidenceContext);
        f.context = contextFor(f.previous, f.ledgerReadback, f.captures[0].inputs);
    }
    return f;
}
function contextFor(previous, ledger, inputs) {
    return {schema_version: 1, revision: 'offset-2961', purpose: previous.purpose,
        manifest: binding(previous.manifest_path), ledger: binding(ledger.ledger_path), source_manifest_sha256: inputs.sourceManifestSha256,
        case_ids: inputs.caseIds, output_count: previous.manifest.worker_outputs.length, required_checks: CHECKS};
}
function invoke(f, context = f.context) {
    return spawnSync(process.execPath, [checkerPath], {input: Buffer.isBuffer(context) ? context : canonical(context),
        cwd: f.base, env: f.config.preflight.environment, timeout: 15000, maxBuffer: 1048576});
}
function refused(child, reason) {
    assert.equal(child.status, 1, String(child.stderr)); assert.equal(child.signal, null);
    const report = JSON.parse(child.stderr); assert.equal(report.status, 'FAIL');
    if (reason) assert.equal(report.reason, reason, String(child.stderr));
    if (child.stdout.length) { const result = JSON.parse(child.stdout); assert.deepEqual(Object.keys(result), SCHEMA.result_keys); assert.equal(result.status, 'FAIL'); }
    return report;
}
function updateManifest(f, fn) {
    const value = readJson(f.context.manifest.path); fn(value); rewrite(f.context.manifest.path, canonical(value));
    f.context.manifest = binding(f.context.manifest.path);
}
function rechain(value) {
    value.genesis_sha256 = hash(canonical(value.genesis)); let previous = value.genesis_sha256;
    for (const row of value.rows) {
        row.event.genesis_sha256 = value.genesis_sha256; row.event.previous_sha256 = previous;
        row.sha256 = hash(canonical(row.event)); previous = row.sha256;
    }
    value.row_count = value.rows.length; value.tail_sha256 = previous;
}
async function genuine(t) {
    const f = await setup(t, {execute: false, extraSources: [checkerPath]});
    const groups = SUITES.map((suite, i) => ({name: suite.name, files: suite.files.map((name, j) => {
        const file = path.join(f.base, `real-case-${i}-${j}.js`);
        fs.writeFileSync(file, "const test=require('node:test');const assert=require('node:assert/strict');\ntest('actual arithmetic',()=>{assert.equal(6*7,42);});\n");
        return file;
    })}));
    const definitionsPath = path.join(f.base, 'real-definitions.json'), registryPath = path.join(f.base, 'real-registry.json');
    const inventoryPath = path.join(f.base, 'real-inventory.json'), qualificationPath = path.join(f.base, 'qualification.json');
    const generated = await generateSuiteInventory({purpose: 'synthetic', definitionsPath, registryPath, inventoryPath, groups, environment: f.config.preflight.environment});
    for (const [p, value] of [[definitionsPath, generated.definitions], [registryPath, generated.registry], [inventoryPath, generated.inventory]]) fs.writeFileSync(p, canonical(value));
    const basePath = path.join(f.base, 'base.cpp'), productPath = path.join(f.base, 'product.cpp'), diagnostics = path.join(f.base, 'diagnostics');
    fs.writeFileSync(basePath, 'int original = 1;\n'); fs.writeFileSync(productPath, 'int revised = 2;\n'); fs.mkdirSync(diagnostics, {mode: 0o700});
    f.config.registry_path = registryPath;
    Object.assign(f.config.preflight, {runnerPath: path.join(__dirname, 'genuine-preflight.js'), runnerArgs: [qualificationPath],
        sourcePaths: [...new Set([...f.config.preflight.sourcePaths, ...generated.capture.sourcePaths, ...['genuine-preflight.js', 'trace-probe.js',
            'independent-evidence-checker.js'].map(name => path.join(__dirname, name))])], inputPaths: [basePath, productPath],
        configPaths: [f.configPath, registryPath, definitionsPath, f.checkerConfigPath, qualificationPath], caseInventoryPath: inventoryPath});
    fs.writeFileSync(f.configPath, canonical(f.config));
    fs.writeFileSync(qualificationPath, canonical({schema_version: 1, purpose: 'synthetic', formal_config_path: f.configPath,
        suite_definitions_path: definitionsPath, base_path: basePath, product_path: productPath, candidate_path: f.candidatePath, diagnostic_parent: diagnostics}));
    const cc = readJson(f.checkerConfigPath); cc.module_path = checkerPath; cc.args = []; cc.timeout_ms = 30000; fs.writeFileSync(f.checkerConfigPath, canonical(cc));
    for (let i = 0; i < 2; i++) f.captures.push(await capturePreflight({...f.config.preflight,
        receiptId: f.config.receipt_ids[i], runRoot: f.receiptRoots[i], receiptDirectory: f.receiptDirs[i]}));
    f.h3 = await runFormalTransaction(f.request); assert.equal(f.h3.status, 'PASS', f.h3.message);
    const lc = h4.captureLedgerContext({configPath: f.configPath, receiptPaths: f.request.receiptPaths});
    f.ledgerReadback = h4.publishJoinedLedger(h4.buildJoinedLedger(f.h3, lc), f.ledgerPath);
    Object.assign(f.evidenceRequest, {h3Result: f.h3, ledgerReadback: f.ledgerReadback, preflightCaptures: f.captures});
    const ec = h5.captureEvidenceContext(f.evidenceRequest); f.previous = h5.publishEvidenceManifest(ec);
    f.context = contextFor(f.previous, f.ledgerReadback, f.captures[0].inputs);
    f.definitions = generated.definitions; return f;
}
function refreshEnvelope(f, ordinal, envelope) {
    const manifest = readJson(f.context.manifest.path), row = manifest.receipts[ordinal];
    rewrite(row.stderr.path, canonical(envelope));
    const receipt = readJson(row.report.path), stderr = binding(row.stderr.path);
    receipt.stderr = {...stderr, file_sha256: stderr.sha256}; rewrite(row.report.path, canonical(receipt));
    const report = binding(row.report.path), ledger = readJson(f.context.ledger.path);
    Object.assign(ledger.genesis.receipts[ordinal], {report, stderr}); rechain(ledger); rewrite(f.context.ledger.path, canonical(ledger));
    f.context.ledger = binding(f.context.ledger.path);
    updateManifest(f, m => { Object.assign(m.receipts[ordinal], {report, stderr}); m.ledger = binding(f.context.ledger.path, true); });
}
function definitionProbe() {
    const module = {exports: {}};
    const load = vm.runInThisContext(`(function(require, module, exports, __dirname, __filename) {\n${fs.readFileSync(checkerPath, 'utf8')}\nreturn {suiteDefinitions, Disk};\n})`);
    return load(require, module, module.exports, __dirname, checkerPath);
}
function pin(p) {
    const bytes = fs.readFileSync(p);
    return {path: p, bytes: bytes.length, sha256: hash(bytes), git_blob: crypto.createHash('sha1').update(`blob ${bytes.length}\0`).update(bytes).digest('hex')};
}
async function baselineDefinition(t, omitted, additions = []) {
    const files = [['test-preflight-receipt.js'], ['test-input-capture.js', 'test-preflight-capture.js'],
        ['test-formal-runner.js', 'test-refusal-inventory.js'], ['test-formal-ledger.js', 'test-p0f-evidence.js']].map(group => group.map(n => path.join(__dirname, n)));
    const workerPath = path.join(__dirname, 'suite-worker.js');
    const f = await setup(t, {execute: false, purpose: 'validation', extraSources: [...new Set([checkerPath, workerPath, ...files.flat()]) ]});
    const definitionPath = path.join(f.base, 'negative-definitions.json');
    f.config.preflight.configPaths.push(definitionPath);
    f.config.preflight.environment.LC_OFFSET_ROADMAP_SNAPSHOT = f.raw;
    const extras = ['H6-ID-02g', 'H6-ID-02h', 'H6-ID-03d', 'H6-ID-03e',
        ...Array.from({length: 8}, (_, i) => `H6-WORKER-${String(i + 1).padStart(2, '0')}`),
        ...Array.from({length: 4}, (_, i) => `H6-EVIDENCE-${String(i + 1).padStart(2, '0')}`),
        ...Array.from({length: 3}, (_, i) => `H6-CONTROL-${String(i + 1).padStart(2, '0')}`),
        ...Array.from({length: 3}, (_, i) => `H6-POST-${String(i + 1).padStart(2, '0')}`)];
    const baseline = [...ORIGINAL_IDS, ...extras].sort(); assert.equal(baseline.length, 93); assert.equal(new Set(baseline).size, 93);
    const ids = [...baseline.filter(id => id !== omitted), ...additions].sort();
    // Configuration-level negatives only: no validation receipts, raw execution
    // events or successful execution results are fabricated for these maps.
    const entry = (id, file, ordinal) => ({id, file, name: `${id}: negative definition metadata`, declaration_ordinal: ordinal});
    const cases = [[entry('H1-001', files[0][0], 1)], [entry('H2I-001', files[1][0], 1), entry('H2P-001', files[1][1], 1)],
        [entry('H3-001', files[2][0], 1), entry('H6-000-inventory', files[2][1], 1), ...ids.map((id, i) => entry(id, files[2][1], i + 2))],
        [entry('H4-001', files[3][0], 1), entry('H5-001', files[3][1], 1)]];
    const environment = Object.fromEntries(Object.entries(f.config.preflight.environment).sort(([a], [b]) => a < b ? -1 : a > b ? 1 : 0));
    const defs = {schema_version: 1, purpose: 'validation', kind: 'canonical-node-suites', worker: pin(workerPath),
        runtime: {exec_path: process.execPath, version: process.version, executable: pin(fs.realpathSync.native(process.execPath))},
        cwd: __dirname, environment, sources: [workerPath, ...files.flat()].sort().map(pin), inputs: [pin(f.raw)],
        h6: {required_ids: ids, original_ids: [...ORIGINAL_IDS], pending_ids: []}, normalization: NORMALIZATION,
        suites: SUITES.map((suite, i) => ({name: suite.name, files: files[i].map(pin), cases: cases[i],
            outputs: [`${suite.name}.diagnostics.json`, `${suite.name}.events.jsonl`, `${suite.name}.report.json`],
            timeout_ms: 10000, max_event_bytes: 4194304, max_stderr_bytes: 1048576}))};
    const inventory = {schema_version: 1, purpose: 'validation', cases: cases.flat().map(c => ({id: c.id, source_path: workerPath})).sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0)};
    fs.writeFileSync(f.inventoryPath, canonical(inventory)); fs.writeFileSync(f.configPath, canonical(f.config)); fs.writeFileSync(definitionPath, canonical(defs));
    const inputs = checker.deriveInputManifest(f.config.preflight);
    assert.deepEqual(defs.h6.required_ids, cases[2].filter(c => c.id.startsWith('H6-') && c.id !== 'H6-000-inventory').map(c => c.id));
    return {f, defs, inputs, definitionPath};
}

test('the checker imports only built-ins and inert descriptors, even with production imports forbidden', () => {
    const allowed = new Set(['./ledger-descriptors', './evidence-descriptors', './schema-descriptors']);
    const seen = [], module = {exports: {}};
    const load = vm.runInThisContext(`(function(require, module, exports, __dirname, __filename) {\n${fs.readFileSync(checkerPath, 'utf8')}\n})`);
    const guarded = name => { assert.ok(name.startsWith('node:') || allowed.has(name), name); seen.push(name); return require(name); };
    load(guarded, module, module.exports, __dirname, checkerPath);
    assert.equal(typeof module.exports.deriveInputManifest, 'function');
    assert.equal(typeof module.exports.checkEvidence, 'function');
    assert.ok(seen.includes('node:crypto'));
    for (const name of allowed) {
        const visit = value => { assert.notEqual(typeof value, 'function'); if (value && typeof value === 'object') Object.values(value).forEach(visit); };
        visit(require(name));
    }
});

test('independent input audit reconstructs exact H2 bytes, roles, candidate, runtime and inventory', async t => {
    const f = await setup(t, {execute: false, extraSources: [checkerPath]});
    const primary = h2.captureInputs(f.config.preflight);
    const independent = checker.deriveInputManifest(f.config.preflight);
    const primaryBytes = canonical(primary.manifest), independentlyDerivedBytes = canonical(independent);
    assert.deepEqual(independentlyDerivedBytes, primaryBytes);
    assert.equal(hash(independentlyDerivedBytes), hash(primaryBytes));
    assert.equal(independent.files.some(row => row.path === checkerPath && row.roles.includes('source')), true);
    assert.equal(Object.getPrototypeOf(independent), Object.prototype);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('independent input derivation rejects accessors without executing them and rejects changed actual inventory', async t => {
    const f = await setup(t, {execute: false, extraSources: [checkerPath]}); let calls = 0;
    const cfg = {...f.config.preflight}; Object.defineProperty(cfg, 'sourcePaths', {get() { calls++; throw new Error('getter'); }});
    assert.throws(() => checker.deriveInputManifest(cfg), /DATA_ACCESSOR/); assert.equal(calls, 0);
    const inventory = readJson(f.inventoryPath); inventory.cases.reverse(); fs.writeFileSync(f.inventoryPath, canonical(inventory));
    assert.throws(() => checker.deriveInputManifest(f.config.preflight), /SORTED_UNIQUE/);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const id of ['H6-WORKER-08', 'H6-CONTROL-01', 'H6-CONTROL-02', 'H6-CONTROL-03', 'H6-POST-01', 'H6-POST-02', 'H6-POST-03'])
    test(`validation definition cannot omit mandated ${id} with a consistent remaining map`, async t => {
        const {f, inputs, definitionPath} = await baselineDefinition(t, id), probe = definitionProbe(), disk = new probe.Disk();
        assert.throws(() => probe.suiteDefinitions(definitionPath, inputs, disk), error => error.code === 'H6_REQUIRED_IDENTITIES');
        disk.finish(); fs.rmSync(f.base, {recursive: true, force: true});
    });

test('validation definition baseline accepts all 93 configured obligations and permits future configured extras, without claiming execution', async t => {
    for (const additions of [[], ['H6-FUTURE-01']]) {
        const {f, inputs, definitionPath} = await baselineDefinition(t, undefined, additions), probe = definitionProbe(), disk = new probe.Disk();
        const defs = probe.suiteDefinitions(definitionPath, inputs, disk);
        assert.equal(defs.h6.required_ids.length, 93 + additions.length);
        assert.deepEqual(defs.h6.pending_ids, []); disk.finish(); fs.rmSync(f.base, {recursive: true, force: true});
    }
});

test('real invocation derives PASS from actual H2/H3/H4/H5 artifacts and yields deterministic semantic bytes', async t => {
    const f = await prepare(t), first = invoke(f), second = invoke(f);
    assert.equal(first.status, 0, String(first.stderr)); assert.equal(second.status, 0, String(second.stderr));
    assert.deepEqual(first.stdout, second.stdout); assert.deepEqual(first.stderr, second.stderr);
    const result = JSON.parse(first.stdout), report = JSON.parse(first.stderr);
    assert.deepEqual(Object.keys(result), SCHEMA.result_keys); assert.equal(result.status, 'PASS');
    assert.equal(result.manifest_sha256, f.context.manifest.sha256); assert.equal(result.ledger_sha256, f.context.ledger.sha256);
    assert.deepEqual(result.case_ids, ['case-a-01', 'case-b-01']);
    assert.deepEqual(result.checks, CHECKS.map(id => ({id, status: 'PASS'})));
    assert.equal(report.worker_count, 2); assert.equal(report.output_count, 2); assert.ok(report.limitations.length > 0);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('real H5 dispatch accepts the exact independent result and recorded review can be independently replayed', async t => {
    const f = await prepare(t, false), result = await h5.runP0fEvidence(f.evidenceRequest);
    assert.equal(result.status, 'PASS', result.message); assert.equal(result.dispatch.mode, 'process');
    assert.equal(result.dispatch.child.exit_code, 0); assert.equal(result.dispatch.stdin.write_completed, true);
    f.context = JSON.parse(Buffer.from(result.dispatch.stdin.base64, 'base64'));
    const replay = invoke(f); assert.equal(replay.status, 0, String(replay.stderr));
    assert.deepEqual(replay.stdout, Buffer.from(result.dispatch.stdout.base64, 'base64'));
    assert.deepEqual(replay.stderr, Buffer.from(result.dispatch.stderr.base64, 'base64'));
    assert.equal(h5.revalidateEvidenceManifest(result), result);
    fs.rmSync(f.base, {recursive: true, force: true});
});

for (const kind of ['missing-input', 'wrong-input-bytes', 'input-identity', 'output-bytes', 'output-symlink', 'input-hardlink'])
    test(`actual ${kind} cannot be replaced by matching digest claims`, async t => {
        const f = await prepare(t), output = path.join(f.config.run_root, 'a.json');
        if (kind === 'missing-input') fs.unlinkSync(f.raw);
        if (kind === 'wrong-input-bytes') fs.writeFileSync(f.raw, 'different actual bytes');
        if (kind === 'input-identity') { const bytes = fs.readFileSync(f.raw); fs.renameSync(f.raw, path.join(f.base, 'original-input')); fs.writeFileSync(f.raw, bytes); }
        if (kind === 'output-bytes') rewrite(output, Buffer.from('substituted output'));
        if (kind === 'output-symlink') { fs.renameSync(output, path.join(f.base, 'original-output')); fs.symlinkSync(path.join(f.base, 'original-output'), output); }
        if (kind === 'input-hardlink') fs.linkSync(f.raw, path.join(f.base, 'input-alias'));
        refused(invoke(f)); fs.rmSync(f.base, {recursive: true, force: true});
    });

for (const kind of ['malformed-json', 'duplicate-key', 'invalid-utf8', 'extra-key', 'wrong-digest', 'relabel-validation'])
    test(`context ${kind} is fail-closed`, async t => {
        const f = await prepare(t); let context = {...f.context};
        if (kind === 'malformed-json') context = Buffer.from('not JSON\n');
        if (kind === 'duplicate-key') context = Buffer.from(canonical(context).toString().replace('{', '{"schema_version":1,'));
        if (kind === 'invalid-utf8') context = Buffer.from([0xff, 10]);
        if (kind === 'extra-key') context.extra = true;
        if (kind === 'wrong-digest') context.manifest = {...context.manifest, sha256: '0'.repeat(64)};
        if (kind === 'relabel-validation') context.purpose = 'validation';
        refused(invoke(f, context)); fs.rmSync(f.base, {recursive: true, force: true});
    });

for (const kind of ['case', 'missing', 'duplicate', 'genesis']) test(`fully rehashed ledger ${kind} mutation is independently rejected`, async t => {
    const f = await prepare(t), ledger = readJson(f.context.ledger.path);
    if (kind === 'case') ledger.rows[0].event.case_id = 'invented-case';
    if (kind === 'missing') ledger.rows.pop();
    if (kind === 'duplicate') ledger.rows.push(JSON.parse(JSON.stringify(ledger.rows[0])));
    if (kind === 'genesis') ledger.genesis.candidate.sha256 = 'a'.repeat(64);
    rechain(ledger); rewrite(f.context.ledger.path, canonical(ledger)); f.context.ledger = binding(f.context.ledger.path);
    updateManifest(f, m => { m.ledger = binding(f.context.ledger.path, true); });
    refused(invoke(f), 'DERIVED_LEDGER'); fs.rmSync(f.base, {recursive: true, force: true});
});

test('self-consistent H3 stdout/hash with a false event is not an independent oracle', async t => {
    const f = await prepare(t), manifest = readJson(f.context.manifest.path), diagnosticPath = manifest.h3_diagnostics.path;
    const h3 = readJson(diagnosticPath), row = h3.workers[0]; row.result.events[0].status = 'FAIL';
    const stdout = canonical(row.result);
    row.dispatch.stdout = {base64: stdout.toString('base64'), bytes: stdout.length, sha256: hash(stdout)};
    row.dispatch.observed.stdout = stdout.length;
    rewrite(diagnosticPath, canonical(h3)); updateManifest(f, m => { m.h3_diagnostics = binding(diagnosticPath); });
    refused(invoke(f), 'H3_EVENTS'); fs.rmSync(f.base, {recursive: true, force: true});
});

test('malformed canonical persisted JSON cannot be rescued by rewriting its outer digest', async t => {
    const f = await prepare(t), p = f.context.manifest.path;
    rewrite(p, Buffer.from('{"schema_version":1,"schema_version":1}\n')); f.context.manifest = binding(p);
    refused(invoke(f), 'JSON_CANONICAL'); fs.rmSync(f.base, {recursive: true, force: true});
});

test('metadata revalidation notices an output changed during later cross-input readback', async t => {
    const f = await prepare(t), output = path.join(f.config.run_root, 'a.json'), descriptors = new Map();
    let changed = false, outputRead = false;
    const adapter = {...fs,
        openSync(p, ...args) { const fd = fs.openSync(p, ...args); descriptors.set(fd, p); return fd; },
        closeSync(fd) { descriptors.delete(fd); return fs.closeSync(fd); },
        readSync(fd, ...args) {
            const n = fs.readSync(fd, ...args), p = descriptors.get(fd);
            if (p === output && n) outputRead = true;
            if (!changed && outputRead && p === f.context.manifest.path && n) { changed = true; rewrite(output, Buffer.from('late cross-read substitution')); }
            return n;
        }};
    const module = {exports: {}}, processView = {...process, cwd: () => f.base, env: f.config.preflight.environment};
    const load = vm.runInNewContext(`(function(require, module, exports, __dirname, __filename) {\n${fs.readFileSync(checkerPath, 'utf8')}\n})`, {Buffer, process: processView});
    load(name => name === 'node:fs' ? adapter : require(name), module, module.exports, __dirname, checkerPath);
    const result = module.exports.checkEvidence(canonical(f.context));
    assert.equal(changed, true); assert.equal(result.result.status, 'FAIL'); assert.equal(result.report.readback_failure, 'READBACK_CHANGED');
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('stdin size/depth caps fail before accepting any completion result', async t => {
    const f = await setup(t, {execute: false, extraSources: [checkerPath]});
    refused(invoke(f, Buffer.alloc(LIMITS.contextBytes + 1, 32)), 'STDIN_LIMIT');
    refused(invoke(f, Buffer.from(`${'['.repeat(65)}0${']'.repeat(65)}\n`)), 'JSON_DEPTH');
    const oversizedResult = {schema_version: 1, revision: 'offset-2961', purpose: 'synthetic',
        manifest: {path: path.join(f.base, 'absent-manifest.json'), bytes: 1, sha256: '0'.repeat(64)},
        ledger: {path: path.join(f.base, 'absent-ledger.json'), bytes: 1, sha256: '0'.repeat(64)}, source_manifest_sha256: '0'.repeat(64),
        case_ids: Array.from({length: 1000}, (_, i) => `case-${String(i).padStart(4, '0')}-${'x'.repeat(96)}`), output_count: 1, required_checks: CHECKS};
    assert.ok(canonical(oversizedResult).length < LIMITS.contextBytes);
    refused(invoke(f, oversizedResult), 'RESULT_LIMIT');
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('genuine preflight raw events, independent audits, actual byte replays, syntax and traces derive a real synthetic PASS', async t => {
    const f = await genuine(t), child = invoke(f);
    assert.equal(child.status, 0, String(child.stderr));
    assert.equal(JSON.parse(child.stdout).status, 'PASS');
    assert.deepEqual(JSON.parse(child.stdout).case_ids, f.definitions.suites.flatMap(s => s.cases.map(c => c.id)).sort());
    assert.equal(JSON.parse(child.stderr).purpose, 'synthetic', 'small actual child tests are not canonical product validation');
    const envelope = readJson(f.captures[0].receipt.stderr.path), artifact = envelope.artifacts.find(a => a.role === 'trace-probe');
    const traceBytes = fs.readFileSync(artifact.path);
    rewrite(artifact.path, Buffer.from('changed actual trace bytes\n'));
    refused(invoke(f), 'DISK_BINDING');
    rewrite(artifact.path, traceBytes);
    const patchRow = envelope.artifacts.find(a => a.role === 'patch-replays'), patches = readJson(patchRow.path);
    patches.directions[0].patch.target.base64 = Buffer.from('not the actual product bytes').toString('base64');
    rewrite(patchRow.path, canonical(patches)); Object.assign(patchRow, binding(patchRow.path)); refreshEnvelope(f, 0, envelope);
    refused(invoke(f), 'GENUINE_PATCH_REPLAY');
    envelope.kind = 'opaque-synthetic-diagnostic'; refreshEnvelope(f, 0, envelope);
    refused(invoke(f), 'GENUINE_PREFLIGHT_REQUIRED');
    fs.rmSync(f.base, {recursive: true, force: true});
});

test('synthetic negative raw-event mutations cannot promote a finished prefix or unknown testcase to PASS after full rebinding', async t => {
    const f = await genuine(t), originalEnvelope = readJson(f.captures[0].receipt.stderr.path), prefix = 'suite:receipt-schema:attempt:1:';
    const originalEventsRow = originalEnvelope.artifacts.find(a => a.role === `${prefix}events`);
    const originalDiagRow = originalEnvelope.artifacts.find(a => a.role === `${prefix}diagnostics`);
    const originalEvents = fs.readFileSync(originalEventsRow.path, 'utf8').trimEnd().split('\n').map(line => JSON.parse(line));
    const originalDiagnostic = readJson(originalDiagRow.path);
    assert.equal(invoke(f).status, 0, 'real synthetic capture is the positive control, not fabricated validation evidence');
    for (const [kind, reason] of [['skip', 'NODE_TEST_SKIPPED'], ['missing-summary', 'NODE_SUMMARY_COUNT'],
        ['duplicate-summary', 'NODE_SUMMARY_COUNT'], ['cancelled-summary', 'NODE_SUMMARY_STATUS'],
        ['unsuccessful-summary', 'NODE_SUMMARY_STATUS'], ['nonfinal-summary', 'NODE_SUMMARY_FINAL'],
        ['unknown-entry', 'NODE_UNEXPECTED_TOP_LEVEL'], ['extra-top-level', 'NODE_UNEXPECTED_TOP_LEVEL']]) {
        t.diagnostic(`synthetic negative only: ${kind}`);
        const envelope = JSON.parse(JSON.stringify(originalEnvelope)), diagnostic = JSON.parse(JSON.stringify(originalDiagnostic));
        let events = JSON.parse(JSON.stringify(originalEvents));
        const terminal = events.find(e => e.type === 'test:pass' && e.data.details && e.data.details.type === 'test' && e.data.nesting === 0);
        const summary = events.find(e => e.type === 'test:summary' && e.data.file === null);
        assert.ok(terminal); assert.ok(summary);
        if (kind === 'skip') terminal.data.skip = true;
        if (kind === 'missing-summary') events = events.filter(e => !(e.type === 'test:summary' && e.data.file === null));
        if (kind === 'duplicate-summary') events.push(JSON.parse(JSON.stringify(summary)));
        if (kind === 'cancelled-summary') summary.data.counts.cancelled = 1;
        if (kind === 'unsuccessful-summary') summary.data.success = false;
        if (kind === 'nonfinal-summary') events.push({type: 'test:diagnostic', data: {message: 'synthetic trailing event'}});
        if (kind === 'unknown-entry') terminal.data.entryFile = path.join(f.base, 'unexpected-case.js');
        if (kind === 'extra-top-level') {
            const extra = JSON.parse(JSON.stringify(terminal)); extra.data.entryFile = path.join(f.base, 'unexpected-case.js');
            events.splice(events.indexOf(summary), 0, extra);
        }
        const eventsRow = envelope.artifacts.find(a => a.role === `${prefix}events`), diagRow = envelope.artifacts.find(a => a.role === `${prefix}diagnostics`);
        const raw = Buffer.concat(events.map(canonical)); rewrite(eventsRow.path, raw);
        diagnostic.capture.stdout = {base64: raw.toString('base64'), bytes: raw.length, sha256: hash(raw), observed_bytes: raw.length, truncated: false};
        rewrite(diagRow.path, canonical(diagnostic));
        for (const row of [eventsRow, diagRow]) Object.assign(row, binding(row.path));
        refreshEnvelope(f, 0, envelope); refused(invoke(f), reason);
    }
    fs.rmSync(f.base, {recursive: true, force: true});
});
