/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const {capturePreflight} = require('../preflight-capture');

const directory = path.dirname(__dirname);
const cliPath = path.join(directory, 'run-formal-gates.js');
const workerPath = path.join(__dirname, 'formal-child.js');
const RECEIPT_IDS = Object.freeze(['10000000-0000-4000-8000-000000000001', '20000000-0000-4000-8000-000000000002']);
const bytes = value => Buffer.from(`${JSON.stringify(value)}\n`);
const hash = value => crypto.createHash('sha256').update(value).digest('hex');
const environment = () => Object.keys(process.env).sort().map(key => [key, process.env[key]]);

function statRecord(p) {
    const s = fs.lstatSync(p, {bigint: true});
    return {path: p, metadata: ['dev', 'ino', 'mode', 'nlink', 'size', 'mtimeNs', 'ctimeNs'].map(key => String(s[key])),
        sha256: s.isFile() ? hash(fs.readFileSync(p)) : null,
        link: s.isSymbolicLink() ? fs.readlinkSync(p) : null,
        entries: s.isDirectory() ? fs.readdirSync(p).sort() : null};
}

function tree(root) {
    const rows = [];
    const visit = p => {
        const record = statRecord(p);
        rows.push(record);
        if (record.entries) for (const name of record.entries) visit(path.join(p, name));
    };
    visit(root);
    return rows;
}

function replace(p, value) {
    fs.chmodSync(p, 0o600);
    fs.writeFileSync(p, value);
    fs.chmodSync(p, 0o400);
}

function readReceipt(f, ordinal = 0) {
    return JSON.parse(fs.readFileSync(f.request.receiptPaths[ordinal], 'utf8'));
}

function rewriteReceipt(f, ordinal, mutate, {syncStdout = true} = {}) {
    const receipt = readReceipt(f, ordinal);
    mutate(receipt);
    if (syncStdout) {
        const stdout = bytes(receipt.preflight_result);
        replace(receipt.stdout.path, stdout);
        receipt.stdout.bytes = stdout.length;
        receipt.stdout.sha256 = receipt.stdout.file_sha256 = hash(stdout);
    }
    replace(f.request.receiptPaths[ordinal], bytes(receipt));
}

function setStdout(f, value) {
    const receipt = readReceipt(f);
    replace(receipt.stdout.path, value);
    rewriteReceipt(f, 0, row => {
        row.stdout.bytes = value.length;
        row.stdout.sha256 = row.stdout.file_sha256 = hash(value);
    }, {syncStdout: false});
}

function setReservedRoot(f, ordinal, target) {
    const parent = path.dirname(target);
    const stat = fs.lstatSync(parent, {bigint: true});
    rewriteReceipt(f, ordinal, row => {
        row.run_root.path = target;
        row.run_root.parent = {path: parent, device: String(stat.dev), inode: String(stat.ino)};
        row.preflight_result.run_root = row.run_root;
    });
}

async function setup({capture = true, modes = ['pass', 'pass', 'pass'], duplicateId = false} = {}) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-h6-'));
    const original = path.join(base, 'original-captures');
    const derived = path.join(base, 'derived-attempt');
    const inputDirectory = path.join(base, 'inputs');
    for (const p of [original, derived, inputDirectory]) fs.mkdirSync(p, {mode: 0o700});
    const configPath = path.join(inputDirectory, 'formal-config.json');
    const registryPath = path.join(inputDirectory, 'registry.json');
    const candidatePath = path.join(inputDirectory, 'candidate.md');
    const raw = path.join(inputDirectory, 'raw.txt');
    const inventoryPath = path.join(inputDirectory, 'cases.json');
    const runParent = path.join(base, 'formal-parent');
    fs.mkdirSync(runParent, {mode: 0o700});
    const runRoot = path.join(runParent, 'run');
    fs.writeFileSync(candidatePath, 'actual H6 synthetic candidate source\n');
    fs.writeFileSync(raw, 'actual H6 synthetic raw input\n');
    const workers = ['a', 'b', 'c'].map((name, i) => ({id: `worker-${name}`, module_path: workerPath,
        args: [modes[i]], case_ids: [`case-${name}-01`], outputs: [`${name}.json`],
        timeout_ms: 3000, max_stream_bytes: 65536}));
    fs.writeFileSync(registryPath, bytes({schema_version: 1, purpose: 'synthetic', workers}));
    fs.writeFileSync(inventoryPath, bytes({schema_version: 1, purpose: 'synthetic',
        cases: workers.flatMap(worker => worker.case_ids.map(id => ({id, source_path: workerPath})))}));
    const receiptRoots = [];
    const originalDirectories = [];
    for (let i = 0; i < 2; i++) {
        const rootParent = path.join(original, `root-parent-${i}`);
        const receiptParent = path.join(original, `receipt-parent-${i}`);
        fs.mkdirSync(rootParent, {mode: 0o700});
        fs.mkdirSync(receiptParent, {mode: 0o700});
        receiptRoots.push(path.join(rootParent, 'reserved-root'));
        originalDirectories.push(path.join(receiptParent, 'receipt'));
        fs.mkdirSync(path.join(derived, `receipt-parent-${i}`), {mode: 0o700});
    }
    const preflight = {purpose: 'synthetic', candidatePath, runnerPath: path.join(__dirname, 'capture-child.js'),
        sourcePaths: ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js'].map(name => path.join(directory, name)).concat(workerPath),
        inputPaths: [raw], fixturePaths: [], configPaths: [configPath, registryPath], caseInventoryPath: inventoryPath,
        runnerArgs: ['pass'], cwd: inputDirectory, environment: {LC_ALL: 'C', ...(process.platform === 'darwin' ?
            {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})}};
    const config = {schema_version: 1, purpose: 'synthetic', registry_path: registryPath, run_root: runRoot,
        receipt_ids: [...RECEIPT_IDS], receipt_roots: receiptRoots, preflight};
    fs.writeFileSync(configPath, bytes(config));
    const receiptPaths = [];
    let pinnedPaths = [];
    for (let i = 0; i < 2; i++) {
        const id = duplicateId ? RECEIPT_IDS[0] : RECEIPT_IDS[i];
        const receiptDirectory = path.join(derived, `receipt-parent-${i}`, 'receipt');
        const receiptPath = path.join(receiptDirectory, `${id}.json`);
        receiptPaths.push(receiptPath);
        if (!capture) continue;
        const captured = await capturePreflight({...preflight, receiptId: id, runRoot: receiptRoots[i],
            receiptDirectory: originalDirectories[i], timeoutMs: 10000});
        pinnedPaths = captured.inputs.manifest.files.map(row => row.path);
        fs.mkdirSync(receiptDirectory, {mode: 0o700});
        for (const name of fs.readdirSync(captured.receiptDirectory))
            fs.copyFileSync(path.join(captured.receiptDirectory, name), path.join(receiptDirectory, name), fs.constants.COPYFILE_EXCL);
        // Only the derived storage paths differ; actual captured inputs stay pinned.
        const receipt = JSON.parse(fs.readFileSync(receiptPath, 'utf8'));
        receipt.stdout.path = path.join(receiptDirectory, `${id}.stdout`);
        receipt.stderr.path = path.join(receiptDirectory, `${id}.stderr`);
        replace(receiptPath, bytes(receipt));
    }
    return {base, original, derived, inputDirectory, raw, runParent, config, pinnedPaths,
        request: {configPath, runRoot, receiptPaths}};
}

function immutableSnapshot(f) {
    return {original: tree(f.original), inputs: tree(f.inputDirectory),
        pinned: f.pinnedPaths.filter(p => !p.startsWith(`${f.inputDirectory}${path.sep}`)).sort().map(statRecord),
        environment: environment()};
}

function argumentsFor(request) {
    return ['--run', '--config', request.configPath, '--run-dir', request.runRoot,
        ...request.receiptPaths.flatMap(p => ['--preflight-receipt', p])];
}

function realCli(argv) {
    const child = spawnSync(process.execPath, [cliPath, ...argv], {maxBuffer: 8388608, timeout: 30000});
    assert.equal(child.error, undefined);
    assert.equal(child.signal, null);
    const result = JSON.parse(child.stdout.toString('utf8'));
    assert.deepEqual(child.stdout, bytes(result), 'CLI must emit canonical JSON+LF');
    assert.equal(child.status, result.exit_code);
    return result;
}

module.exports = Object.freeze({setup, tree, statRecord, immutableSnapshot, environment, bytes, hash, replace,
    readReceipt, rewriteReceipt, setStdout, setReservedRoot, argumentsFor, realCli, cliPath, workerPath, RECEIPT_IDS});
