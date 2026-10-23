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
const h1 = require('../preflight-receipt');
const h2 = require('../input-capture');
const {capturePreflight} = require('../preflight-capture');
const {runFormalTransaction} = require('../formal-runner');
const h4 = require('../formal-ledger');

const directory = path.dirname(__dirname);
const workerPath = path.join(__dirname, 'formal-child.js');
const checkerPath = path.join(__dirname, 'evidence-checker.js');
const ids = Object.freeze(['10000000-0000-4000-8000-000000000001', '20000000-0000-4000-8000-000000000002']);

// No automatic teardown: callers remove successful attempts after all their
// assertions. Unexpected failures retain actual captured inputs and prefixes.
async function setup(t, {checkerMode = 'pass', execute = true, purpose = 'synthetic', omitSource,
    omitCaptures = false, omitCheckerConfig = false, timeout = 3000, streamCap = 65536, emptySource = false,
    captureOnly = false, extraSources = [], checkerModule = checkerPath, workerMode = 'pass'} = {}) {
    assert.ok(Array.isArray(extraSources), 'extraSources must be an explicit array');
    const pinnedExtras = extraSources.map(p => h2.canonicalPath(p));
    const selectedChecker = h2.canonicalPath(checkerModule, 'checkerModule');
    h2.string(workerMode, 'workerMode', 128);
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
    const checker = {schema_version: 1, purpose, module_path: selectedChecker, args: [checkerMode], cwd: base,
        environment: env, timeout_ms: timeout, max_stream_bytes: streamCap};
    fs.writeFileSync(checkerConfigPath, h1.canonicalJson(checker));
    const workers = ['a', 'b'].map(name => ({id: `worker-${name}`, module_path: workerPath, args: [workerMode],
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
    const sourcePaths = [...new Set(sourceNames.map(name => path.join(directory, name)).concat(workerPath, selectedChecker, pinnedExtras))];
    if (emptySource) {
        const p = path.join(base, 'empty-source.js'); fs.writeFileSync(p, ''); sourcePaths.push(p);
    }
    const preflight = {purpose, candidatePath, runnerPath: path.join(__dirname, 'capture-child.js'), sourcePaths,
        inputPaths: [raw], fixturePaths: [], configPaths: [configPath, registryPath, ...(omitCheckerConfig ? [] : [checkerConfigPath])],
        caseInventoryPath: inventoryPath, runnerArgs: ['pass'], cwd: base, environment: env};
    const config = {schema_version: 1, purpose, registry_path: registryPath, run_root: path.join(runParent, 'run'),
        receipt_ids: [...ids], receipt_roots: roots, preflight};
    fs.writeFileSync(configPath, h1.canonicalJson(config));
    const captures = [];
    let h3 = null, ledgerReadback = null, ledgerContext = null, ledgerDocument = null;
    const request = {configPath, runRoot: config.run_root, receiptPaths: receiptDirs.map((p, i) => path.join(p, `${ids[i]}.json`))};
    if (execute || captureOnly) {
        for (let i = 0; i < 2; i++) captures.push(await capturePreflight({...preflight,
            receiptId: ids[i], runRoot: roots[i], receiptDirectory: receiptDirs[i]}));
    }
    const ledgerPath = path.join(ledgerParent, 'ledger', 'joined-events.json');
    if (execute && !captureOnly) {
        h3 = await runFormalTransaction(request); assert.equal(h3.status, 'PASS', h3.message);
        ledgerContext = h4.captureLedgerContext({configPath, receiptPaths: request.receiptPaths});
        ledgerDocument = h4.buildJoinedLedger(h3, ledgerContext);
        ledgerReadback = h4.publishJoinedLedger(ledgerDocument, ledgerPath);
    }
    const evidenceRequest = {configPath, receiptPaths: request.receiptPaths, h3Result: h3, ledgerReadback,
        preflightCaptures: omitCaptures ? null : captures, checkerConfigPath,
        manifestPath: path.join(evidenceParent, 'attempt', 'manifest.json')};
    return {base, config, request, h3, captures, raw, ledgerReadback, evidenceParent, checkerConfigPath, evidenceRequest,
        configPath, registryPath, candidatePath, inventoryPath, runParent, ledgerParent, ledgerPath,
        ledgerContext, ledgerDocument, receiptDirs, receiptRoots: roots, sourcePaths,
        checkerModule: selectedChecker, workerMode,
        phase: captureOnly ? 'captured' : execute ? 'ledger' : 'configured'};
}

module.exports = Object.freeze({setup, workerPath, checkerPath, ids});
