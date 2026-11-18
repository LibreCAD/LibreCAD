/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const path = require('node:path');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {normalizedEnvironment} = require('./preflight-capture');
const registryApi = require('./formal-registry');
const {LIMITS, SCHEMA} = require('./ledger-descriptors');

// Production H4 join/persistence, not an independent checker or H5 manifest.
// H7 must derive its own expectations; it may import ledger-descriptors only.
// Filesystem checks are bounded open/fstat/readback observations, not a sandbox.
// A context is independently captured AFTER H3 completes; no H3 row supplies
// expected registry, receipt, source, runtime, candidate or actual output bytes.
const H3_KEYS = Object.freeze(['schema_version', 'purpose', 'status', 'stage', 'reason', 'message', 'exit_code',
    'run_root', 'root_created', 'worker_launches', 'source_manifest_sha256', 'registry_sha256', 'workers',
    'last_dispatch', 'durability_limitations', 'verification_failures']);
const contexts = new WeakMap();
const documents = new WeakMap();
const readbacks = new WeakMap();

class LedgerError extends Error {
    constructor(stage, reason, message, details = {}) {
        super(message);
        this.name = 'LedgerError';
        this.stage = stage;
        this.reason = reason;
        this.details = h2.freeze(details);
    }
}

function ensure(condition, stage, reason, message) {
    if (!condition) throw new LedgerError(stage, reason, message);
}

function equal(a, b) { return h1.canonicalJson(a).equals(h1.canonicalJson(b)); }

function exactKeys(value, expected, label) {
    ensure(value && typeof value === 'object' && !Array.isArray(value) &&
        equal(Object.keys(value), expected), 'JOIN', 'SCHEMA_INVALID', `${label} keys/order mismatch`);
}

// Do not execute getters/toJSON or freeze caller objects. Bound the entire
// copied data tree before canonicalization; shared aliases are data, cycles are not.
function snapshotData(value) {
    const active = new WeakSet();
    let nodes = 0, bytes = 0;
    const copy = (item, depth) => {
        ensure(++nodes <= LIMITS.dataNodes && depth <= LIMITS.dataDepth,
            'JOIN', 'LIMIT_EXCEEDED', 'data tree exceeds node/depth cap');
        if (typeof item === 'string') {
            const length = Buffer.byteLength(item);
            bytes += length;
            ensure(length <= LIMITS.stringBytes && bytes <= LIMITS.h3Bytes && !item.includes('\0') &&
                Buffer.from(item).toString('utf8') === item, 'JOIN', 'LIMIT_EXCEEDED', 'data string is invalid or exceeds byte cap');
            return item;
        }
        if (item === null || typeof item === 'boolean') return item;
        if (typeof item === 'number') {
            ensure(Number.isFinite(item), 'JOIN', 'SCHEMA_INVALID', 'nonfinite data number');
            return item;
        }
        ensure(item && typeof item === 'object' && !active.has(item), 'JOIN', 'SCHEMA_INVALID', 'data must be acyclic JSON values');
        const prototype = Object.getPrototypeOf(item);
        ensure(Array.isArray(item) || prototype === Object.prototype || prototype === null,
            'JOIN', 'SCHEMA_INVALID', 'data must have a plain prototype');
        const descriptors = Object.getOwnPropertyDescriptors(item);
        const keys = Reflect.ownKeys(descriptors);
        ensure(keys.every(key => typeof key === 'string' && Object.hasOwn(descriptors[key], 'value')),
            'JOIN', 'SCHEMA_INVALID', 'data accessors/symbols are not permitted');
        active.add(item);
        let out;
        if (Array.isArray(item)) {
            const length = descriptors.length.value;
            ensure(length <= LIMITS.arrayItems && keys.length === length + 1,
                'JOIN', 'LIMIT_EXCEEDED', 'array is sparse, extended or oversized');
            out = [];
            for (let i = 0; i < length; i++) {
                ensure(Object.hasOwn(descriptors, String(i)), 'JOIN', 'SCHEMA_INVALID', 'array has missing entries');
                out.push(copy(descriptors[i].value, depth + 1));
            }
        } else {
            ensure(keys.length <= 128, 'JOIN', 'LIMIT_EXCEEDED', 'object has too many keys');
            out = Object.create(null);
            for (const key of keys) {
                const keyBytes = Buffer.byteLength(key);
                bytes += keyBytes;
                ensure(keyBytes <= 4096 && bytes <= LIMITS.h3Bytes && !key.includes('\0') &&
                    Buffer.from(key).toString('utf8') === key, 'JOIN', 'SCHEMA_INVALID', 'invalid object key');
                out[key] = copy(descriptors[key].value, depth + 1);
            }
        }
        active.delete(item);
        return Object.freeze(out);
    };
    return copy(value, 0);
}

function selectIo(dependencies, purpose, fallback) {
    ensure(dependencies && typeof dependencies === 'object' && !Array.isArray(dependencies),
        'CONTEXT', 'SCHEMA_INVALID', 'dependencies must be a plain object');
    const prototype = Object.getPrototypeOf(dependencies);
    const descriptors = Object.getOwnPropertyDescriptors(dependencies);
    const keys = Reflect.ownKeys(descriptors);
    const plain = prototype === Object.prototype || prototype === null;
    const inherited = !Object.hasOwn(descriptors, 'fs') && 'fs' in dependencies;
    ensure(purpose === 'synthetic' || (plain && !inherited && keys.length === 0),
        'CONTEXT', 'SYNTHETIC_INJECTION_FORBIDDEN', 'I/O injection is restricted to synthetic controls');
    ensure(plain && !inherited && keys.every(key => key === 'fs' && Object.hasOwn(descriptors[key], 'value')),
        'CONTEXT', 'SCHEMA_INVALID', 'synthetic fs must be an own data property');
    if (keys.length === 0) return fallback;
    const io = descriptors.fs.value;
    ensure(io && typeof io === 'object' && !Array.isArray(io), 'CONTEXT', 'SCHEMA_INVALID', 'invalid fs adapter');
    return io;
}

function contextData(context) {
    ensure(contexts.has(context), 'CONTEXT', 'CONTEXT_REQUIRED', 'original captured ledger context is required');
    return contexts.get(context);
}

function documentData(document) {
    ensure(documents.has(document), 'JOIN', 'DOCUMENT_REQUIRED', 'original built ledger is required');
    return documents.get(document);
}

function absent(p, io) {
    try { io.lstatSync(p); }
    catch (error) { if (error.code === 'ENOENT') return; throw error; }
    throw new LedgerError('PUBLISH', 'PATH_EXISTS', `path already exists: ${p}`);
}

function related(a, b) {
    const relative = path.relative(a, b);
    return relative === '' || (relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
}

function physicalPath(p, io) {
    const tail = [];
    let existing = p;
    for (;;) {
        try { return path.join(io.realpathSync(existing), ...tail); }
        catch (error) {
            if (error.code !== 'ENOENT') throw error;
            try {
                io.lstatSync(existing);
                throw new LedgerError('PUBLISH', 'PATH_POLICY', 'unresolvable protected symlink');
            } catch (entryError) { if (entryError.code !== 'ENOENT') throw entryError; }
            const parent = path.dirname(existing);
            ensure(parent !== existing, 'PUBLISH', 'PATH_POLICY', 'filesystem root is missing');
            tail.unshift(path.basename(existing)); existing = parent;
        }
    }
}

function directorySnapshot(p, expectedNames, io, mode) {
    const guards = h2.directoryGuards(p, io);
    const before = io.lstatSync(p);
    ensure(Number(before.mode & 0o7777n) === mode, 'READBACK', 'DIRECTORY_MODE', 'directory mode mismatch');
    const fd = io.openSync(p, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
    try {
        ensure(h2.statIdentity(io.fstatSync(fd)) === h2.statIdentity(before), 'READBACK', 'DIRECTORY_CHANGED', 'directory changed while opening');
        const directory = io.opendirSync(p);
        const names = [];
        try {
            for (;;) {
                const entry = directory.readSync();
                if (entry === null) break;
                ensure(names.length < expectedNames.length, 'READBACK', 'MEMBERSHIP_MISMATCH', 'directory has extra entries');
                names.push(entry.name);
            }
        } finally { directory.closeSync(); }
        ensure(equal(names.sort(), [...expectedNames].sort()), 'READBACK', 'MEMBERSHIP_MISMATCH', 'directory has missing/extra entries');
        ensure(h2.statIdentity(io.fstatSync(fd)) === h2.statIdentity(before) &&
            h2.statIdentity(io.lstatSync(p)) === h2.statIdentity(before), 'READBACK', 'DIRECTORY_CHANGED', 'directory changed during enumeration');
        h2.checkDirectoryGuards(guards, io);
        return h2.freeze({path: p, guards, identity: h2.statIdentity(before)});
    } finally { io.closeSync(fd); }
}

function readOutputs(data, io) {
    const names = data.registry.workers.flatMap(row => row.outputs);
    ensure(names.length <= LIMITS.directoryEntries, 'CONTEXT', 'LIMIT_EXCEEDED', 'too many output files');
    const root = directorySnapshot(data.config.run_root, names, io, 0o700);
    let total = 0;
    const files = [...names].sort().map(name => {
        const file = registryApi.readStableFile(path.join(root.path, name), registryApi.LIMITS.outputBytes, io);
        ensure(file.mode === 0o400 && Number(io.lstatSync(file.path).mode & 0o7777n) === 0o400,
            'READBACK', 'FILE_MODE', 'worker output must have mode 0400');
        total += file.byte_length;
        ensure(total <= registryApi.LIMITS.totalOutputBytes, 'READBACK', 'LIMIT_EXCEEDED', 'total output bytes exceed cap');
        return h2.freeze({name, ...file});
    });
    for (const file of files) registryApi.recheckFile(file, io);
    ensure(h2.statIdentity(io.lstatSync(root.path)) === root.identity, 'READBACK', 'DIRECTORY_CHANGED', 'output root changed during readback');
    return {root, files: Object.freeze(files)};
}

function checkReceipts(data, io) {
    const {config, inputs, receipts} = data;
    ensure(equal(receipts.receipts.map(row => row.receipt.receipt_id), config.receipt_ids),
        'CONTEXT', 'RECEIPT_MISMATCH', 'receipt IDs/order differ from pinned configuration');
    for (let i = 0; i < receipts.receipts.length; i++) {
        const receipt = receipts.receipts[i].receipt;
        const gate = receipt.preflight_result.inventory_gate;
        ensure(receipt.run_root.path === config.receipt_roots[i] &&
            gate.expected_count === inputs.caseIds.length && gate.actual_count === inputs.caseIds.length &&
            gate.expected_ids_sha256 === inputs.manifest.case_ids_sha256 && gate.actual_ids_sha256 === inputs.manifest.case_ids_sha256,
        'CONTEXT', 'RECEIPT_MISMATCH', 'receipt root/case inventory differs from pinned expectations');
        const guards = h2.directoryGuards(receipt.run_root.parent.path, io);
        const parent = io.lstatSync(receipt.run_root.parent.path);
        ensure(String(parent.dev) === receipt.run_root.parent.device && String(parent.ino) === receipt.run_root.parent.inode,
            'CONTEXT', 'RECEIPT_MISMATCH', 'receipt root parent identity changed');
        absent(receipt.run_root.path, io);
        h2.checkDirectoryGuards(guards, io);
    }
}

function revalidateContext(data, io) {
    h2.revalidateInputs(data.inputs, {io});
    registryApi.recheckFile(data.configFile, io);
    registryApi.recheckFile(data.registryFile, io);
    h1.revalidateReceiptSet(data.receipts, {...data.receiptOptions, io});
    for (const guards of data.receiptGuards) h2.checkDirectoryGuards(guards, io);
    h2.checkDirectoryGuards(data.cwdGuards, io);
    checkReceipts(data, io);
    const current = readOutputs(data, io);
    ensure(current.root.identity === data.outputs.root.identity && equal(current.files, data.outputs.files),
        'REVALIDATION', 'OUTPUT_CHANGED', 'worker output bytes/identity changed after context capture');
    for (const saved of data.outputs.files) registryApi.recheckFile(saved, io);
    h2.revalidateInputs(data.inputs, {io});
    h1.revalidateReceiptSet(data.receipts, {...data.receiptOptions, io});
    // The cross-input readback above must not leave earlier output/input reads as
    // their own last observation. Finish with original metadata, not new baselines.
    for (const saved of data.inputs.snapshots) {
        h2.checkDirectoryGuards(saved.guards, io);
        ensure(h2.statIdentity(io.lstatSync(saved.path)) === saved.identity,
            'REVALIDATION', 'INPUT_CHANGED', 'captured input changed at final context boundary');
    }
    for (const receipt of data.receipts.receipts) {
        ensure(h2.statIdentity(io.lstatSync(receipt.receiptDirectory)) === receipt.receiptDirectoryIdentity,
            'REVALIDATION', 'RECEIPT_CHANGED', 'receipt directory changed at final context boundary');
        const paths = [receipt.receiptPath, receipt.receipt.stdout.path, receipt.receipt.stderr.path];
        paths.forEach((p, i) => ensure(h2.statIdentity(io.lstatSync(p)) === receipt.fileIdentities[i],
            'REVALIDATION', 'RECEIPT_CHANGED', 'receipt file changed at final context boundary'));
    }
    for (const saved of data.outputs.files) {
        h2.checkDirectoryGuards(saved.guards, io);
        ensure(h2.statIdentity(io.lstatSync(saved.path)) === saved.identity,
            'REVALIDATION', 'OUTPUT_CHANGED', 'worker output changed at final context boundary');
    }
    for (const guards of data.receiptGuards) h2.checkDirectoryGuards(guards, io);
    h2.checkDirectoryGuards(data.cwdGuards, io);
    h2.checkDirectoryGuards(data.outputs.root.guards, io);
    ensure(h2.statIdentity(io.lstatSync(data.outputs.root.path)) === data.outputs.root.identity,
        'REVALIDATION', 'OUTPUT_CHANGED', 'worker output membership changed at final context boundary');
}

/** Capture actual expectations independently of the H3 result; call after H3 finishes. */
function captureLedgerContext(requestInput, dependencies = {}) {
    ensure(SCHEMA.revision === h1.REVISION, 'CONTEXT', 'REVISION_MISMATCH', 'ledger/receipt descriptor revisions differ');
    const request = snapshotData(requestInput);
    exactKeys(request, ['configPath', 'receiptPaths'], 'context request');
    h2.canonicalPath(request.configPath);
    ensure(Array.isArray(request.receiptPaths) && request.receiptPaths.length === 2,
        'CONTEXT', 'RECEIPT_MISMATCH', 'exactly two receipt paths are required');
    request.receiptPaths.forEach(p => h2.canonicalPath(p));
    const configFile = registryApi.readStableFile(request.configPath, registryApi.LIMITS.configBytes, h2.nodeFsAdapter);
    const config = registryApi.validateConfiguration(h1.parseCanonicalJson(Buffer.from(configFile.base64, 'base64'), {maxBytes: registryApi.LIMITS.configBytes}));
    const io = selectIo(dependencies, config.purpose, h2.nodeFsAdapter);
    registryApi.recheckFile(configFile, io);
    const inputs = h2.captureInputs(config.preflight, {io});
    const source = p => inputs.manifest.files.some(row => row.path === p && row.roles.includes('source'));
    for (const name of ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js', 'formal-ledger.js', 'ledger-descriptors.js'])
        ensure(source(path.join(__dirname, name)), 'CONTEXT', 'SOURCE_UNPINNED', 'H3/H4 implementation source must be pinned');
    for (const p of [request.configPath, config.registry_path])
        ensure(inputs.manifest.files.some(row => row.path === p && row.roles.includes('config')),
            'CONTEXT', 'CONFIG_UNPINNED', 'configuration/registry must be actual captured config inputs');
    const registryFile = registryApi.readStableFile(config.registry_path, registryApi.LIMITS.registryBytes, io);
    const registry = registryApi.validateRegistry(h1.parseCanonicalJson(Buffer.from(registryFile.base64, 'base64'), {maxBytes: registryApi.LIMITS.registryBytes}), inputs);
    const environment = normalizedEnvironment(config.preflight.environment);
    if (process.platform === 'darwin') ensure(typeof process.env.__CF_USER_TEXT_ENCODING === 'string' &&
        environment.__CF_USER_TEXT_ENCODING === process.env.__CF_USER_TEXT_ENCODING,
    'CONTEXT', 'ENVIRONMENT_MISMATCH', 'macOS runtime encoding environment must be pinned');
    const environmentSha256 = h1.sha256(h1.canonicalJson(environment));
    const receiptOptions = {io, requirePass: true, forbiddenRoots: [config.run_root, __dirname,
        ...inputs.manifest.files.map(row => row.path)], expected: {purpose: config.purpose,
        candidate: inputs.candidate, sourceManifestSha256: inputs.sourceManifestSha256,
        argv: [inputs.runtime.executable_path, config.preflight.runnerPath, ...config.preflight.runnerArgs],
        cwd: config.preflight.cwd, environmentSha256}};
    const receipts = h1.validateReceiptSet(request.receiptPaths, receiptOptions);
    const receiptGuards = receipts.receipts.flatMap(row => [h2.directoryGuards(row.receipt.run_root.parent.path, io),
        h2.directoryGuards(path.dirname(row.receiptDirectory), io)]);
    const data = {configFile, config, inputs, registryFile, registry, receipts, receiptOptions, receiptGuards,
        environmentSha256, cwdGuards: h2.directoryGuards(config.preflight.cwd, io), io};
    for (const row of inputs.manifest.files)
        ensure(!related(config.run_root, row.path) && !related(row.path, config.run_root),
            'CONTEXT', 'PATH_POLICY', 'worker output root overlaps a captured input');
    checkReceipts(data, io);
    data.outputs = readOutputs(data, io);
    revalidateContext(data, io);
    const context = h2.freeze({schema_version: 1, purpose: config.purpose, config_path: request.configPath,
        registry_sha256: registryFile.sha256, source_manifest_sha256: inputs.sourceManifestSha256, run_root: config.run_root});
    contexts.set(context, data);
    return context;
}

function streamBytes(stream, maxBytes, stdin = false) {
    exactKeys(stream, stdin ? ['base64', 'bytes', 'sha256', 'write_completed'] : ['base64', 'bytes', 'sha256'], 'dispatch stream');
    ensure(typeof stream.base64 === 'string' && Number.isSafeInteger(stream.bytes) && stream.bytes >= 0 && stream.bytes <= maxBytes,
        'JOIN', 'STREAM_MISMATCH', 'invalid/big stream byte count');
    const bytes = Buffer.from(stream.base64, 'base64');
    ensure(bytes.length === stream.bytes && bytes.toString('base64') === stream.base64 && h1.sha256(bytes) === stream.sha256,
        'JOIN', 'STREAM_MISMATCH', 'dispatch stream bytes/digest differ');
    if (stdin) ensure(stream.write_completed === true, 'JOIN', 'STREAM_MISMATCH', 'worker stdin was not completed');
    return bytes;
}

function checkDispatch(dispatch, result, worker, ordinal, data) {
    const processMode = dispatch.mode === 'process';
    exactKeys(dispatch, processMode ? ['worker_id', 'ordinal', 'worker_source', 'runtime', 'argv', 'environment_sha256',
        'mode', 'child', 'failure', 'stdin', 'observed', 'stdout', 'stderr'] :
        ['mode', 'worker_id', 'ordinal', 'worker_source', 'runtime', 'stdin', 'child', 'stdout', 'stderr'], 'dispatch');
    ensure(processMode || (data.config.purpose === 'synthetic' && dispatch.mode === 'synthetic-callback'),
        'JOIN', 'DISPATCH_MISMATCH', 'validation requires a successful process dispatch');
    const source = data.inputs.manifest.files.find(row => row.path === worker.module_path);
    const executable = data.inputs.manifest.files.find(row => row.roles.includes('runtime-executable'));
    ensure(dispatch.worker_id === worker.id && dispatch.ordinal === ordinal && equal(dispatch.worker_source, source) &&
        equal(dispatch.runtime, {...data.inputs.runtime, executable}), 'JOIN', 'DISPATCH_MISMATCH', 'dispatch source/runtime/identity mismatch');
    const context = {schema_version: 1, purpose: data.config.purpose, worker_id: worker.id, ordinal,
        case_ids: worker.case_ids, outputs: worker.outputs, run_root: data.config.run_root,
        candidate: data.inputs.candidate, source_manifest_sha256: data.inputs.sourceManifestSha256, registry_sha256: data.registryFile.sha256};
    const stdin = streamBytes(dispatch.stdin, registryApi.LIMITS.contextBytes, processMode);
    ensure(stdin.equals(h1.canonicalJson(context)), 'JOIN', 'DISPATCH_MISMATCH', 'worker stdin differs from pinned dispatch context');
    const stdout = streamBytes(dispatch.stdout, worker.max_stream_bytes);
    const stderr = streamBytes(dispatch.stderr, worker.max_stream_bytes);
    if (processMode) {
        ensure(equal(dispatch.argv, [data.inputs.runtime.executable_path, worker.module_path, ...worker.args]) &&
            dispatch.environment_sha256 === data.environmentSha256, 'JOIN', 'DISPATCH_MISMATCH', 'worker argv/environment differ');
        exactKeys(dispatch.child, ['exit_code', 'signal', 'error'], 'child');
        exactKeys(dispatch.observed, ['stdout', 'stderr'], 'observed streams');
        ensure(dispatch.child.exit_code === 0 && dispatch.child.signal === null && dispatch.child.error === null &&
            dispatch.failure === null && dispatch.observed.stdout === stdout.length && dispatch.observed.stderr === stderr.length,
        'JOIN', 'DISPATCH_MISMATCH', 'worker process did not complete successfully');
        const parsed = h1.parseCanonicalJson(stdout, {maxBytes: registryApi.LIMITS.resultBytes});
        ensure(equal(parsed, result), 'JOIN', 'RESULT_MISMATCH', 'actual worker stdout differs from its returned result');
    } else ensure(dispatch.child === null && stdout.length === 0 && stderr.length === 0,
        'JOIN', 'DISPATCH_MISMATCH', 'synthetic callback cannot claim process diagnostics');
}

function binding(filePath, snapshot) { return {path: filePath, bytes: snapshot.byte_length, sha256: snapshot.sha256}; }

/** Pure derivation from H3 data and an independently captured, immutable context. No filesystem calls. */
function buildJoinedLedger(h3Input, context) {
    const data = contextData(context);
    const h3 = snapshotData(h3Input);
    exactKeys(h3, H3_KEYS, 'H3 result');
    ensure(h3.schema_version === 1 && h3.status === 'PASS' && h3.stage === 'COMPLETE' && h3.reason === 'H3_DISPATCH_COMPLETE' &&
        h3.exit_code === 0 && h3.root_created === true && h3.purpose === data.config.purpose && h3.run_root === data.config.run_root &&
        h3.worker_launches === data.registry.workers.length && Array.isArray(h3.workers) && h3.workers.length === data.registry.workers.length &&
        Array.isArray(h3.verification_failures) && h3.verification_failures.length === 0,
    'JOIN', 'H3_NOT_COMPLETE', 'only a complete, successful H3 result can produce a PASS ledger');
    ensure(h3.source_manifest_sha256 === data.inputs.sourceManifestSha256 && h3.registry_sha256 === data.registryFile.sha256,
        'JOIN', 'IDENTITY_MISMATCH', 'H3 source manifest/registry differ from pinned expectations');
    ensure(typeof h3.message === 'string' && Buffer.byteLength(h3.message) <= 4096 &&
        Array.isArray(h3.durability_limitations) && h3.durability_limitations.length <= 64 &&
        h3.durability_limitations.every(value => typeof value === 'string' && Buffer.byteLength(value) <= 8192),
    'JOIN', 'SCHEMA_INVALID', 'invalid H3 diagnostics');
    const executable = data.inputs.manifest.files.find(row => row.roles.includes('runtime-executable'));
    const genesis = {schema_version: 1, revision: h1.REVISION, purpose: data.config.purpose,
        candidate: data.inputs.candidate, runtime: {...data.inputs.runtime, executable},
        source_manifest_sha256: data.inputs.sourceManifestSha256,
        config: binding(data.configFile.path, data.configFile), registry: binding(data.registryFile.path, data.registryFile),
        run_root: data.config.run_root, receipts: data.receipts.receipts.map((row, i) => ({ordinal: i + 1,
            receipt_id: row.receipt.receipt_id, report: binding(row.receiptPath, row.snapshot.receipt),
            stdout: binding(row.receipt.stdout.path, row.snapshot.stdout), stderr: binding(row.receipt.stderr.path, row.snapshot.stderr)})),
        case_inventory_sha256: data.inputs.manifest.case_ids_sha256, worker_count: data.registry.workers.length,
        case_count: data.inputs.caseIds.length, output_count: data.outputs.files.length};
    const genesisSha256 = h1.sha256(h1.canonicalJson(genesis));
    let previous = genesisSha256;
    const rows = [];
    for (let k = 0; k < data.registry.workers.length; k++) {
        const worker = data.registry.workers[k], actual = h3.workers[k];
        exactKeys(actual, ['ordinal', 'worker_id', 'result', 'dispatch', 'outputs'], 'H3 worker row');
        ensure(actual.ordinal === k + 1 && actual.worker_id === worker.id, 'JOIN', 'EVENT_MISMATCH', 'worker identity/order mismatch');
        registryApi.validateWorkerResult(actual.result, worker, k + 1, data.config.purpose);
        checkDispatch(actual.dispatch, actual.result, worker, k + 1, data);
        const outputs = data.outputs.files.filter(row => worker.outputs.includes(row.name));
        ensure(equal(actual.outputs, outputs), 'JOIN', 'OUTPUT_MISMATCH', 'worker output bytes/identity/hash differ from actual files');
        const source = data.inputs.manifest.files.find(row => row.path === worker.module_path);
        for (let j = 0; j < worker.case_ids.length; j++) {
            const event = {ordinal: rows.length + 1, previous_sha256: previous, genesis_sha256: genesisSha256,
                worker_ordinal: k + 1, worker_id: worker.id, event_ordinal: j + 1, case_id: worker.case_ids[j],
                status: actual.result.events[j].status,
                source: {path: source.path, bytes: source.bytes, sha256: source.sha256},
                result_sha256: h1.sha256(h1.canonicalJson(actual.result)), dispatch_mode: actual.dispatch.mode,
                stdin_sha256: actual.dispatch.stdin.sha256, stdout_sha256: actual.dispatch.stdout.sha256,
                stderr_sha256: actual.dispatch.stderr.sha256,
                outputs: outputs.map(row => ({name: row.name, bytes: row.byte_length, sha256: row.sha256}))};
            previous = h1.sha256(h1.canonicalJson(event));
            rows.push({event, sha256: previous});
        }
    }
    ensure(equal(h3.last_dispatch, h3.workers[h3.workers.length - 1].dispatch),
        'JOIN', 'DISPATCH_MISMATCH', 'last dispatch is not the final completed worker');
    ensure(rows.length === data.inputs.caseIds.length, 'JOIN', 'EVENT_MISMATCH', 'joined event inventory is incomplete');
    const document = h2.freeze({schema_version: 1, revision: h1.REVISION, purpose: data.config.purpose, status: 'PASS',
        genesis, genesis_sha256: genesisSha256, rows, row_count: rows.length, tail_sha256: previous});
    const bytes = h1.canonicalJson(document);
    ensure(bytes.length <= LIMITS.ledgerBytes, 'JOIN', 'LIMIT_EXCEEDED', 'ledger exceeds byte cap');
    documents.set(document, {context, data, bytes: h2.bytesSnapshot(bytes)});
    return document;
}

function ledgerLocation(ledgerPath, data, io) {
    h2.canonicalPath(ledgerPath);
    ensure(/^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(path.basename(ledgerPath)), 'PUBLISH', 'PATH_POLICY', 'invalid ledger filename');
    const directory = path.dirname(ledgerPath), parent = path.dirname(directory);
    const guards = h2.directoryGuards(parent, io);
    const physical = physicalPath(directory, io);
    const protectedPaths = [data.config.run_root, ...data.inputs.manifest.files.map(row => row.path),
        ...data.receipts.receipts.flatMap(row => [row.receiptDirectory, row.receipt.run_root.path])];
    for (const p of protectedPaths) {
        const resolved = physicalPath(p, io);
        ensure(!related(physical, resolved) && !related(resolved, physical),
            'PUBLISH', 'PATH_POLICY', 'ledger directory overlaps worker outputs or protected inputs/receipts');
    }
    h2.checkDirectoryGuards(guards, io);
    return {ledgerPath, directory, parent, guards, physical};
}

function syncDirectory(p, io, limitations) {
    const guards = h2.directoryGuards(p, io);
    const fd = io.openSync(p, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
    try {
        ensure(h2.directoryIdentity(io.fstatSync(fd)) === guards[guards.length - 1].identity,
            'PUBLISH', 'DIRECTORY_CHANGED', 'directory descriptor changed');
        try { io.fsyncSync(fd); }
        catch (error) {
            if (!['EINVAL', 'ENOTSUP', 'EOPNOTSUPP', 'ENOSYS'].includes(error.code)) throw error;
            const text = `${p}: directory fsync unavailable (${error.code})`;
            if (!limitations.includes(text)) limitations.push(text);
        }
        h2.checkDirectoryGuards(guards, io);
    } finally { io.closeSync(fd); }
}

function readDocument(ledgerPath, document, io, limitations) {
    const expected = documentData(document);
    const location = ledgerLocation(ledgerPath, expected.data, io);
    revalidateContext(expected.data, io);
    const root = directorySnapshot(location.directory, [path.basename(ledgerPath)], io, 0o700);
    const file = registryApi.readStableFile(ledgerPath, LIMITS.ledgerBytes, io);
    ensure(file.mode === 0o400 && Number(io.lstatSync(ledgerPath).mode & 0o7777n) === 0o400,
        'READBACK', 'FILE_MODE', 'ledger mode must be 0400');
    const parsed = h1.parseCanonicalJson(Buffer.from(file.base64, 'base64'), {maxBytes: LIMITS.ledgerBytes});
    ensure(file.base64 === expected.bytes.base64 && equal(parsed, document),
        'READBACK', 'LEDGER_MISMATCH', 'persisted genesis/event ledger differs from independently joined expectations');
    registryApi.recheckFile(file, io);
    revalidateContext(expected.data, io);
    registryApi.recheckFile(file, io);
    ensure(h2.statIdentity(io.lstatSync(location.directory)) === root.identity,
        'READBACK', 'DIRECTORY_CHANGED', 'ledger directory changed during readback');
    h2.checkDirectoryGuards(location.guards, io);
    ensure(h2.statIdentity(io.lstatSync(ledgerPath)) === file.identity,
        'READBACK', 'FILE_CHANGED', 'ledger changed at final readback boundary');
    const snapshot = h2.freeze({schema_version: 1, purpose: document.purpose, status: 'PASS', ledger_path: ledgerPath,
        ledger: parsed, bytes: file.byte_length, sha256: file.sha256, durability_limitations: [...limitations]});
    readbacks.set(snapshot, {document, location, root, file, io});
    return snapshot;
}

/** Verify a persisted ledger against a freshly built expectation, not against its own claimed hashes. */
function readJoinedLedger(ledgerPath, document, dependencies = {}) {
    const saved = documentData(document);
    const io = selectIo(dependencies, document.purpose, saved.data.io);
    return readDocument(ledgerPath, document, io, []);
}

/** Create one absent evidence directory, outside H3's exact flat output root. Never remove failed artifacts. */
function publishJoinedLedger(document, ledgerPath, dependencies = {}) {
    const saved = documentData(document);
    const io = selectIo(dependencies, document.purpose, saved.data.io);
    const location = ledgerLocation(ledgerPath, saved.data, io);
    const limitations = [];
    let directoryCreated = false, fileCreated = false;
    try {
        revalidateContext(saved.data, io);
        absent(location.directory, io);
        h2.checkDirectoryGuards(location.guards, io);
        try { io.mkdirSync(location.directory, {mode: 0o700}); }
        catch (error) {
            if (error.code === 'EEXIST') throw new LedgerError('PUBLISH', 'PATH_EXISTS', 'external evidence directory won exclusive creation');
            throw error;
        }
        directoryCreated = true;
        h2.checkDirectoryGuards(location.guards, io);
        const before = io.lstatSync(location.directory);
        ensure(before.isDirectory() && !before.isSymbolicLink(), 'PUBLISH', 'DIRECTORY_CHANGED', 'new evidence directory is not real');
        const dirFd = io.openSync(location.directory, io.constants.O_RDONLY | io.constants.O_DIRECTORY | io.constants.O_NOFOLLOW);
        let owned;
        try {
            ensure(h2.directoryIdentity(io.fstatSync(dirFd)) === h2.directoryIdentity(before),
                'PUBLISH', 'DIRECTORY_CHANGED', 'evidence directory changed while opening');
            io.fchmodSync(dirFd, 0o700);
            owned = h2.directoryIdentity(io.fstatSync(dirFd));
        } finally { io.closeSync(dirFd); }
        const ownedGuards = h2.directoryGuards(location.directory, io);
        ensure(ownedGuards[ownedGuards.length - 1].identity === owned, 'PUBLISH', 'DIRECTORY_CHANGED', 'new evidence directory replaced');
        directorySnapshot(location.directory, [], io, 0o700);
        syncDirectory(location.parent, io, limitations);
        revalidateContext(saved.data, io);
        h2.checkDirectoryGuards(ownedGuards, io);
        const bytes = Buffer.from(saved.bytes.base64, 'base64');
        const fd = io.openSync(ledgerPath, io.constants.O_CREAT | io.constants.O_EXCL | io.constants.O_WRONLY | io.constants.O_NOFOLLOW, 0o600);
        fileCreated = true;
        let writtenIdentity;
        try {
            const opened = io.fstatSync(fd);
            ensure(opened.isFile() && opened.nlink === 1n && opened.size === 0n &&
                h2.statIdentity(io.lstatSync(ledgerPath)) === h2.statIdentity(opened),
            'PUBLISH', 'FILE_CHANGED', 'new ledger file identity differs');
            for (let offset = 0; offset < bytes.length;) {
                const count = io.writeSync(fd, bytes, offset, bytes.length - offset);
                ensure(Number.isSafeInteger(count) && count > 0 && count <= bytes.length - offset,
                    'PUBLISH', 'SHORT_WRITE', 'invalid/zero ledger write');
                offset += count;
            }
            io.fchmodSync(fd, 0o400);
            io.fsyncSync(fd);
            const after = io.fstatSync(fd);
            ensure(after.dev === opened.dev && after.ino === opened.ino && after.nlink === 1n &&
                Number(after.mode & 0o7777n) === 0o400 && after.size === BigInt(bytes.length) &&
                h2.statIdentity(io.lstatSync(ledgerPath)) === h2.statIdentity(after),
            'PUBLISH', 'FILE_CHANGED', 'ledger file changed during write');
            writtenIdentity = h2.statIdentity(after);
        } finally { io.closeSync(fd); }
        syncDirectory(location.directory, io, limitations);
        syncDirectory(location.parent, io, limitations);
        h2.checkDirectoryGuards(ownedGuards, io);
        const readback = readDocument(ledgerPath, document, io, limitations);
        ensure(h2.statIdentity(io.lstatSync(ledgerPath)) === writtenIdentity,
            'READBACK', 'FILE_CHANGED', 'published ledger replaced after write/readback');
        h2.checkDirectoryGuards(ownedGuards, io);
        const observed = readbacks.get(readback);
        ensure(h2.statIdentity(io.lstatSync(location.directory)) === observed.root.identity &&
            h2.statIdentity(io.lstatSync(ledgerPath)) === writtenIdentity,
        'READBACK', 'FILE_CHANGED', 'published ledger/directory changed at final boundary');
        return readback;
    } catch (error) {
        throw new LedgerError(error.stage || 'PUBLISH', error.reason || 'IO_FAILURE', String(error.message || error),
            {ledger_path: ledgerPath, evidence_directory: location.directory, directory_created: directoryCreated,
                file_created: fileCreated, cause_code: error.code || null, durability_limitations: limitations});
    }
}

/** Compare with the ORIGINAL file/directory identities and bytes, not a newly sampled baseline. */
function revalidateJoinedLedger(previous, dependencies = {}) {
    ensure(readbacks.has(previous), 'REVALIDATION', 'READBACK_REQUIRED', 'original ledger readback is required');
    const original = readbacks.get(previous);
    const io = selectIo(dependencies, previous.purpose, original.io);
    h2.checkDirectoryGuards(original.location.guards, io);
    h2.checkDirectoryGuards(original.root.guards, io);
    const current = readDocument(previous.ledger_path, original.document, io, previous.durability_limitations);
    const after = readbacks.get(current);
    ensure(after.root.identity === original.root.identity && after.file.identity === original.file.identity &&
        after.file.base64 === original.file.base64, 'REVALIDATION', 'LEDGER_CHANGED', 'persisted ledger bytes/identity changed');
    h2.checkDirectoryGuards(original.root.guards, io);
    ensure(h2.statIdentity(io.lstatSync(original.location.directory)) === original.root.identity &&
        h2.statIdentity(io.lstatSync(original.file.path)) === original.file.identity,
    'REVALIDATION', 'LEDGER_CHANGED', 'ledger/directory changed at final revalidation boundary');
    return previous;
}

module.exports = Object.freeze({LedgerError, captureLedgerContext, buildJoinedLedger,
    publishJoinedLedger, readJoinedLedger, revalidateJoinedLedger});
