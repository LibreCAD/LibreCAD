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
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {capturePreflight} = require('./preflight-capture');
const suiteApi = require('./suite-inventory');
const {generateAndPublish} = require('./generate-suite-inventory');
const api = require('./genuine-preflight');

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

async function fixture(t) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-real-preflight-'));
    t.diagnostic(`attempt ${base}; retained on unexpected failure`);
    const environment = {LC_ALL: 'C', ...(process.platform === 'darwin' ? {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})};
    const groups = suiteApi.SUITES.map((suite, i) => {
        const file = path.join(base, `actual-case-${i}.js`);
        fs.writeFileSync(file, "const test=require('node:test');const assert=require('node:assert/strict');\ntest('actual arithmetic',()=>{assert.equal(2+3,5);});\n");
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
