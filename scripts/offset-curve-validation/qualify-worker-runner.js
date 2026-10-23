/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const {randomUUID} = require('node:crypto');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const h4 = require('./formal-ledger');
const h5 = require('./p0f-evidence');
const registry = require('./formal-registry');
const suiteApi = require('./suite-inventory');
const {generateAndPublish, writeGenerated} = require('./generate-suite-inventory');
const {capturePreflight} = require('./preflight-capture');
const {runWorkerPipeline} = require('./run-worker-pipeline');

const check = (ok, message) => { if (!ok) throw new TypeError(message); };
const equal = (a, b) => h1.canonicalJson(a).equals(h1.canonicalJson(b));
const source = name => path.join(__dirname, name);
function directory(p) { fs.mkdirSync(p, {mode: 0o700}); h2.directoryGuards(p); return p; }
function persistBytes(p, bytes, cap) {
    check(Buffer.isBuffer(bytes), 'qualification publication requires bounded bytes');
    check(bytes.length <= cap, 'pipeline diagnostics exceed qualification cap');
    const guards = h2.directoryGuards(path.dirname(p));
    const fd = fs.openSync(p, fs.constants.O_CREAT | fs.constants.O_EXCL | fs.constants.O_NOFOLLOW | fs.constants.O_WRONLY, 0o600);
    let identity;
    try {
        const before = fs.fstatSync(fd, {bigint: true});
        check(before.isFile() && before.nlink === 1n && before.size === 0n &&
            h2.statIdentity(fs.lstatSync(p, {bigint: true})) === h2.statIdentity(before), 'new qualification result descriptor differs');
        for (let offset = 0; offset < bytes.length;) {
            const n = fs.writeSync(fd, bytes, offset, bytes.length - offset);
            check(n > 0, 'short qualification result write'); offset += n;
        }
        fs.fchmodSync(fd, 0o400); fs.fsyncSync(fd);
        const after = fs.fstatSync(fd, {bigint: true});
        check(after.dev === before.dev && after.ino === before.ino && after.nlink === 1n &&
            Number(after.mode & 0o7777n) === 0o400 && after.size === BigInt(bytes.length) &&
            h2.statIdentity(fs.lstatSync(p, {bigint: true})) === h2.statIdentity(after), 'qualification result changed during write');
        identity = h2.statIdentity(after);
    } finally { fs.closeSync(fd); }
    h2.checkDirectoryGuards(guards);
    const observed = registry.readStableFile(p, cap);
    check(observed.identity === identity && observed.mode === 0o400 && observed.sha256 === h1.sha256(bytes) &&
        observed.byte_length === bytes.length, 'qualification result readback differs');
    h2.checkDirectoryGuards(guards);
    return observed;
}
function persistResult(p, result) { return persistBytes(p, h1.canonicalJson(result), 268435456); }
function captureFailure(error) {
    const d = error && error.details || {};
    return {schema_version: 1, status: 'FAIL', kind: 'preflight-capture-failure',
        stage: error.stage || 'CAPTURE', message: String(error.message || error).slice(0, 4096),
        child: d.child || null, stdin: d.stdin || null, stdout: d.stdout || null, stderr: d.stderr || null,
        observed: d.observed || null, capture_failure: d.captureFailure || null,
        inputs: d.inputs ? d.inputs.manifest : null, source_manifest_sha256: d.inputs ? d.inputs.sourceManifestSha256 : null,
        receipt_directory: d.receiptDirectory || null, verification_failures: d.verificationFailures || [],
        durability_limitations: d.durabilityLimitations || []};
}

function validateQualificationOptions(input) {
    check(input && typeof input === 'object' && [Object.prototype, null].includes(Object.getPrototypeOf(input)), 'plain qualification options required');
    const descriptors = Object.getOwnPropertyDescriptors(input);
    const names = ['directory', 'basePath', 'productPath', 'candidatePath', 'roadmapPath'];
    check(equal(Reflect.ownKeys(descriptors), names) && names.every(key => Object.hasOwn(descriptors[key], 'value')),
        'qualification options keys/order differ');
    const result = {};
    for (const key of names) result[key] = h2.canonicalPath(descriptors[key].value);
    return Object.freeze(result);
}

function revalidateCompletedRuns(runs) {
    for (const run of runs) {
        h5.revalidateEvidenceManifest(run.result.evidence);
        h4.revalidateJoinedLedger(run.result.ledger);
        for (const capture of run.captures) h2.revalidateInputs(capture.inputs);
        registry.recheckFile(run.resultFile);
    }
    // End cross-run reads with the ORIGINAL metadata, not a freshly blessed
    // baseline sampled after the second run.
    for (const run of runs) {
        registry.recheckFile(run.resultFile);
        for (const capture of run.captures) for (const file of capture.inputs.snapshots)
            check(h2.statIdentity(fs.lstatSync(file.path, {bigint: true})) === file.identity, 'final original qualification input changed');
    }
}

async function qualifyWorkerRunner(input, progress = () => {}) {
    const options = validateQualificationOptions(input);
    const original = [options.basePath, options.productPath, options.candidatePath, options.roadmapPath].map(p => suiteApi.pinFile(p));
    const parent = path.dirname(options.directory); h2.directoryGuards(parent);
    for (const row of original) check(!row.path.startsWith(`${options.directory}${path.sep}`) && row.path !== options.directory,
        'qualification directory overlaps original input');
    directory(options.directory);
    const snapshotPath = path.join(options.directory, 'roadmap-snapshot.md');
    const plan = registry.readStableFile(options.roadmapPath, 8388608);
    check(plan.sha256 === original[3].sha256 && plan.byte_length === original[3].bytes, 'original roadmap changed before copy');
    persistBytes(snapshotPath, Buffer.from(plan.base64, 'base64'), 8388608);
    const snapshot = suiteApi.pinFile(snapshotPath);
    check(snapshot.sha256 === original[3].sha256 && snapshot.bytes === original[3].bytes, 'roadmap copy differs from actual plan');
    const environment = {LC_ALL: 'C', LC_OFFSET_ROADMAP_SNAPSHOT: snapshotPath,
        ...(process.platform === 'darwin' ? {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})};
    const attempts = [], completedRuns = [];
    let expectedSemantics = null, expectedSources = null, expectedRuntime = null;
    for (let ordinal = 1; ordinal <= 2; ordinal++) {
        const root = directory(path.join(options.directory, `attempt-${ordinal}`));
        const ids = [randomUUID(), randomUUID()];
        progress({phase: 'generate', ordinal, directory: root});
        const definitionsPath = path.join(root, 'suite-definitions.json'), registryPath = path.join(root, 'registry.json'), inventoryPath = path.join(root, 'cases.json');
        let generated;
        try { generated = await generateAndPublish({purpose: 'validation', definitionsPath, registryPath, inventoryPath, groups: null, environment}); }
        catch (error) {
            try { persistResult(path.join(root, 'generation-failure.json'), {schema_version: 1, status: 'FAIL',
                kind: 'suite-generation-failure', message: String(error.message || error).slice(0, 4096), discovery: error.discovery || null}); }
            catch (publication) { process.stderr.write(`Generation diagnostic publication also failed: ${publication.message}\n`); }
            throw error;
        }
        check(generated.definitions.h6.pending_ids.length === 0 && generated.definitions.h6.original_ids.length === 71, 'H6 inventory is incomplete');
        const sources = generated.definitions.sources;
        if (expectedSources) check(equal(sources, expectedSources), 'actual source/runtime input set changed between qualification runs');
        else expectedSources = sources;
        if (expectedRuntime) check(equal(generated.definitions.runtime, expectedRuntime), 'actual runtime changed between qualification runs');
        else expectedRuntime = generated.definitions.runtime;
        const formalPath = path.join(root, 'formal-config.json'), qualificationPath = path.join(root, 'qualification-config.json'), checkerPath = path.join(root, 'checker-config.json');
        const workerParent = directory(path.join(root, 'worker-parent')), ledgerParent = directory(path.join(root, 'ledger-parent')),
            evidenceParent = directory(path.join(root, 'evidence-parent')), diagnosticParent = directory(path.join(root, 'preflight-diagnostics'));
        const receiptRoots = [], receiptDirectories = [];
        for (let i = 0; i < 2; i++) {
            receiptRoots.push(path.join(directory(path.join(root, `reserved-parent-${i}`)), 'reserved'));
            receiptDirectories.push(path.join(directory(path.join(root, `receipt-parent-${i}`)), 'receipt'));
        }
        const inputPaths = [...new Set([...generated.capture.inputPaths, options.basePath, options.productPath])].sort();
        const preflight = {purpose: 'validation', candidatePath: options.candidatePath, runnerPath: source('genuine-preflight.js'),
            sourcePaths: generated.capture.sourcePaths, inputPaths, fixturePaths: generated.capture.fixturePaths,
            configPaths: [...generated.capture.configPaths, formalPath, qualificationPath, checkerPath],
            caseInventoryPath: inventoryPath, runnerArgs: [qualificationPath], cwd: root, environment};
        const formal = {schema_version: 1, purpose: 'validation', registry_path: registryPath, run_root: path.join(workerParent, 'outputs'),
            receipt_ids: ids, receipt_roots: receiptRoots, preflight};
        writeGenerated(formalPath, formal);
        writeGenerated(qualificationPath, {schema_version: 1, purpose: 'validation', formal_config_path: formalPath,
            suite_definitions_path: definitionsPath, base_path: options.basePath, product_path: options.productPath,
            candidate_path: options.candidatePath, diagnostic_parent: diagnosticParent});
        writeGenerated(checkerPath, {schema_version: 1, purpose: 'validation', module_path: source('independent-evidence-checker.js'),
            args: [], cwd: root, environment, timeout_ms: 300000, max_stream_bytes: 1048576});
        const captures = [];
        for (let i = 0; i < 2; i++) {
            progress({phase: 'preflight', ordinal, capture: i + 1});
            try {
                captures.push(await capturePreflight({...preflight, receiptId: ids[i], runRoot: receiptRoots[i], receiptDirectory: receiptDirectories[i]}));
            } catch (error) {
                try { persistResult(path.join(root, `capture-${i + 1}-failure.json`), captureFailure(error)); }
                catch (publication) { process.stderr.write(`Capture diagnostic publication also failed: ${publication.message}\n`); }
                throw error;
            }
        }
        const receiptPaths = captures.map(row => row.receiptPath);
        progress({phase: 'pipeline', ordinal});
        const result = await runWorkerPipeline({configPath: formalPath, runRoot: formal.run_root, receiptPaths,
            ledgerPath: path.join(ledgerParent, 'joined', 'events.json'), checkerConfigPath: checkerPath,
            manifestPath: path.join(evidenceParent, 'evidence', 'manifest.json'), preflightCaptures: captures});
        // Persist real refusal diagnostics before testing terminal success.
        const resultFile = persistResult(path.join(root, 'pipeline-result.json'), result);
        check(result.status === 'PASS', `qualification pipeline ${ordinal} refused: ${result.stage}/${result.reason}: ${result.message}`);
        const semantics = generated.definitions.suites.map(suite => {
            const reportPath = path.join(formal.run_root, `${suite.name}.report.json`);
            const binding = registry.readStableFile(reportPath, 1048576);
            const report = h1.parseCanonicalJson(Buffer.from(binding.base64, 'base64'), {maxBytes: 1048576});
            check(report.status === 'PASS' && report.assertions === suite.cases.length && report.assertions > 0, 'worker semantic report did not execute its exact inventory');
            return {suite: suite.name, bytes: binding.byte_length, sha256: binding.sha256, assertions: report.assertions,
                assertion_unit: report.assertion_unit, case_ids: report.case_ids};
        });
        if (expectedSemantics) check(equal(semantics, expectedSemantics), 'complete qualification semantic outcomes differ');
        else expectedSemantics = semantics;
        for (const row of captures) h2.revalidateInputs(row.inputs);
        for (const row of sources) check(equal(row, suiteApi.pinFile(row.path)), 'source changed after qualification');
        check(equal(snapshot, suiteApi.pinFile(snapshotPath)), 'original roadmap snapshot changed');
        attempts.push({ordinal, status: 'PASS', purpose: 'validation', case_count: generated.inventory.cases.length,
            source_manifest_sha256: captures[0].inputs.sourceManifestSha256,
            receipts: receiptPaths.map(p => suiteApi.pinFile(p)), result_path: path.join(root, 'pipeline-result.json'), semantics});
        completedRuns.push({captures, result, resultFile});
        progress({phase: 'attempt-complete', ordinal, case_count: generated.inventory.cases.length});
    }
    for (const row of original) check(equal(row, suiteApi.pinFile(row.path)), 'original input changed during qualification');
    for (const row of expectedSources) check(equal(row, suiteApi.pinFile(row.path)), 'source changed at final qualification boundary');
    revalidateCompletedRuns(completedRuns);
    const report = {schema_version: 1, purpose: 'validation', status: 'RECORDED', kind: 'worker-runner-qualification',
        complete_runs: attempts.length, original_inputs: original, runtime: expectedRuntime, roadmap_snapshot: snapshot, attempts,
        identical_semantic_outcomes: true, completion_scope: 'automated-worker-evidence-only',
        verification_scope: 'before-report-publication; final readback requires successful CLI/API return',
        limitations: ['Each full run has isolated, explicitly pinned configuration/output paths; raw input-manifest hashes are not claimed equal across those namespaces.',
            'Repeated suite semantic reports exclude only declared timing/diagnostic/run-namespace fields; raw diagnostics retain actual bindings.',
            'TAP top-level test cases are counted, not JavaScript assertion calls.',
            'No human approval, native feature completion, process-tree containment or filesystem race elimination is asserted.']};
    const savedReport = persistResult(path.join(options.directory, 'qualification.json'), report);
    revalidateCompletedRuns(completedRuns);
    registry.recheckFile(savedReport);
    return {...report, status: 'PASS', recorded_report: {path: savedReport.path, bytes: savedReport.byte_length, sha256: savedReport.sha256}};
}

async function cli(argv) {
    const flags = ['--directory', '--base', '--product', '--candidate', '--roadmap'];
    check(argv.length === 10 && flags.every((flag, i) => argv[i * 2] === flag), 'expected --directory ABS --base ABS --product ABS --candidate ABS --roadmap ABS');
    const report = await qualifyWorkerRunner(Object.fromEntries(['directory', 'basePath', 'productPath', 'candidatePath', 'roadmapPath'].map((key, i) => [key, argv[i * 2 + 1]])),
        progress => process.stderr.write(h1.canonicalJson(progress)));
    process.stdout.write(h1.canonicalJson(report));
}
if (require.main === module) cli(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.stack || error}\n`); process.exitCode = 1; });

module.exports = Object.freeze({validateQualificationOptions, persistResult, revalidateCompletedRuns, qualifyWorkerRunner, cli});
