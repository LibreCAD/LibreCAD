/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const path = require('node:path');
const {spawn} = require('node:child_process');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {normalizedEnvironment} = require('./preflight-capture');
const registry = require('./formal-registry');
const ledger = require('./formal-ledger');
const {LIMITS: LEDGER_LIMITS} = require('./ledger-descriptors');
const {LIMITS, FILES, CHECKS, SCHEMA} = require('./evidence-descriptors');

// H5 binds production evidence and an automated review, not an H7-independent
// qualification or human approval. Failed artifacts are never removed. An on-disk
// Recorded review alone is not a completion token: revalidation requires its issuer.
// No descendant-process containment or adversarial filesystem race elimination.
const contexts = new WeakMap();
const readbacks = new WeakMap();
const BASE_LIMITATIONS = Object.freeze([
    'Automated checker execution is not human review or independent H7 qualification.',
    'Preflight stdin origin/delivery cannot be independently proven from receipts or caller-supplied capture snapshots.',
    'Filesystem checks are bounded open/fstat/readback observations, not a filesystem sandbox.',
    'Checker deadlines terminate the direct child; descendant containment is not provided.',
    'Synthetic callback controls cannot be forcibly cancelled and are not process transport evidence.'
]);

class EvidenceError extends Error {
    constructor(stage, reason, message, details = {}) {
        super(message);
        this.name = 'EvidenceError'; this.stage = stage; this.reason = reason;
        this.details = h2.freeze(details);
    }
}
function ensure(ok, stage, reason, message) { if (!ok) throw new EvidenceError(stage, reason, message); }
function equal(a, b) { return h1.canonicalJson(a).equals(h1.canonicalJson(b)); }
function keys(value, expected, label) {
    ensure(value && typeof value === 'object' && !Array.isArray(value) && equal(Object.keys(value), expected),
        'SCHEMA', 'SCHEMA_INVALID', `${label} keys/order mismatch`);
}
function data(value) {
    let nodes = 0, size = 0;
    const active = new WeakSet();
    const copy = (v, depth) => {
        ensure(++nodes <= LIMITS.dataNodes && depth <= LIMITS.dataDepth, 'SCHEMA', 'LIMIT_EXCEEDED', 'data tree exceeds cap');
        if (v === null || typeof v === 'boolean') return v;
        if (typeof v === 'number') { ensure(Number.isFinite(v), 'SCHEMA', 'SCHEMA_INVALID', 'nonfinite number'); return v; }
        if (typeof v === 'string') {
            size += Buffer.byteLength(v);
            ensure(Buffer.byteLength(v) <= LIMITS.stringBytes && size <= LIMITS.diagnosticsBytes && !v.includes('\0') &&
                Buffer.from(v).toString('utf8') === v, 'SCHEMA', 'LIMIT_EXCEEDED', 'invalid/oversized data string');
            return v;
        }
        ensure(v && typeof v === 'object' && !active.has(v), 'SCHEMA', 'SCHEMA_INVALID', 'acyclic JSON data required');
        const descriptors = ownData(v);
        active.add(v);
        let out;
        if (Array.isArray(v)) {
            const n = descriptors.length.value;
            ensure(n <= LIMITS.arrayItems && Object.keys(descriptors).length === n + 1,
                'SCHEMA', 'LIMIT_EXCEEDED', 'sparse/extended/oversized array');
            out = [];
            for (let i = 0; i < n; ++i) {
                ensure(Object.hasOwn(descriptors, i), 'SCHEMA', 'SCHEMA_INVALID', 'sparse array');
                out.push(copy(descriptors[i].value, depth + 1));
            }
        } else {
            ensure(Object.keys(descriptors).length <= 128, 'SCHEMA', 'LIMIT_EXCEEDED', 'too many object keys');
            out = Object.create(null);
            for (const [key, descriptor] of Object.entries(descriptors)) {
                h2.string(key, 'data key'); size += Buffer.byteLength(key);
                ensure(size <= LIMITS.diagnosticsBytes, 'SCHEMA', 'LIMIT_EXCEEDED', 'data keys exceed byte cap');
                out[key] = copy(descriptor.value, depth + 1);
            }
        }
        active.delete(v); return h2.freeze(out);
    };
    return copy(value, 0);
}
function ownData(value) {
    ensure(value && typeof value === 'object' && (Array.isArray(value) ||
        [Object.prototype, null].includes(Object.getPrototypeOf(value))), 'SCHEMA', 'SCHEMA_INVALID', 'plain data required');
    const descriptors = Object.getOwnPropertyDescriptors(value);
    ensure(Reflect.ownKeys(descriptors).every(key => typeof key === 'string' && Object.hasOwn(descriptors[key], 'value')),
        'SCHEMA', 'SCHEMA_INVALID', 'accessors/symbols are forbidden');
    return descriptors;
}
function hooksFor(dependencies, purpose) {
    ensure(dependencies && typeof dependencies === 'object' && !Array.isArray(dependencies), 'CONFIG', 'SCHEMA_INVALID', 'plain dependencies required');
    const plain = [Object.prototype, null].includes(Object.getPrototypeOf(dependencies));
    const descriptors = Object.getOwnPropertyDescriptors(dependencies);
    const names = Reflect.ownKeys(descriptors), allowed = ['fs', 'spawn', 'dispatch'];
    const inherited = allowed.some(key => !Object.hasOwn(descriptors, key) && key in dependencies);
    ensure(purpose === 'synthetic' || (plain && !inherited && names.length === 0),
        'CONFIG', 'SYNTHETIC_INJECTION_FORBIDDEN', 'validation forbids dependency injection');
    ensure(plain && !inherited && names.every(key => allowed.includes(key) && Object.hasOwn(descriptors[key], 'value')),
        'CONFIG', 'SCHEMA_INVALID', 'synthetic hooks must be own data properties');
    const selected = Object.create(null);
    for (const key of names) {
        const v = descriptors[key].value;
        ensure(key === 'fs' ? v && typeof v === 'object' && !Array.isArray(v) : typeof v === 'function',
            'CONFIG', 'SCHEMA_INVALID', `invalid hook ${key}`);
        selected[key] = v;
    }
    return Object.freeze(selected);
}
function requiredApis() {
    for (const name of ['captureLedgerContext', 'buildJoinedLedger', 'readJoinedLedger', 'revalidateJoinedLedger'])
        ensure(typeof ledger[name] === 'function', 'CONFIG', 'DEPENDENCY_MISSING', `missing H4 export: ${name}`);
    for (const name of ['captureInputs', 'revalidateInputs', 'directoryGuards', 'checkDirectoryGuards'])
        ensure(typeof h2[name] === 'function', 'CONFIG', 'DEPENDENCY_MISSING', `missing H2 export: ${name}`);
}
function envSnapshot() { return JSON.stringify(Object.keys(process.env).sort().map(k => [k, process.env[k]])); }
function ioDeps(d, io) { return d.config.purpose === 'synthetic' ? {fs: io} : {}; }
function binding(p, snapshot) {
    return {path: p, bytes: snapshot.byte_length, sha256: snapshot.sha256, identity: snapshot.identity, mode: snapshot.mode};
}
function related(a, b) { const r = path.relative(a, b); return r === '' || (r !== '..' && !r.startsWith(`..${path.sep}`) && !path.isAbsolute(r)); }
function physical(p, io) {
    const tail = [];
    for (;;) {
        try { return path.join(io.realpathSync(p), ...tail); }
        catch (error) {
            if (error.code !== 'ENOENT') throw error;
            try { io.lstatSync(p); throw new EvidenceError('PATH', 'PATH_POLICY', 'unresolvable existing path'); }
            catch (cause) { if (cause.code !== 'ENOENT') throw cause; }
            const parent = path.dirname(p);
            ensure(parent !== p, 'PATH', 'PATH_POLICY', 'missing filesystem root');
            tail.unshift(path.basename(p)); p = parent;
        }
    }
}
function absent(p, io) {
    try { io.lstatSync(p); }
    catch (error) { if (error.code === 'ENOENT') return; throw error; }
    throw new EvidenceError('PUBLISH', 'PATH_EXISTS', `path already exists: ${p}`);
}
function read(p, cap, io, mode) {
    const saved = registry.readStableFile(p, cap, io);
    ensure(mode === undefined || saved.mode === mode && Number(io.lstatSync(p).mode & 0o7777n) === mode,
        'READBACK', 'FILE_METADATA', `unexpected mode: ${p}`);
    return saved;
}
function contextData(context) {
    ensure(contexts.has(context), 'CONTEXT', 'CONTEXT_REQUIRED', 'original evidence context required');
    return contexts.get(context);
}
function issued(previous) {
    ensure(readbacks.has(previous), 'REVALIDATION', 'READBACK_REQUIRED', 'original evidence readback required');
    return readbacks.get(previous);
}
function checkTransport(captures, d) {
    ensure(captures === null || Array.isArray(captures) && captures.length === 2,
        'TRANSPORT', 'CAPTURE_MISMATCH', 'explicit null or two actual capture snapshots required');
    return d.receipts.receipts.map((row, i) => {
        const receipt = row.receipt;
        const expected = h2.bytesSnapshot(h1.canonicalJson({purpose: d.config.purpose, candidate: d.inputs.candidate,
            source_manifest_sha256: d.inputs.sourceManifestSha256, case_inventory_path: d.inputs.manifest.case_inventory_path,
            case_count: d.inputs.caseIds.length, case_ids_sha256: d.inputs.manifest.case_ids_sha256,
            runtime: d.inputs.runtime, run_root: receipt.run_root}));
        let observed = null;
        if (captures !== null) {
            const descriptors = ownData(captures[i]);
            ensure(Object.hasOwn(descriptors, 'receiptPath') && Object.hasOwn(descriptors, 'stdin') &&
                descriptors.receiptPath.value === row.receiptPath, 'TRANSPORT', 'CAPTURE_MISMATCH', 'capture receipt/order differs');
            observed = data(descriptors.stdin.value);
            keys(observed, ['base64', 'bytes', 'sha256', 'write_completed'], 'capture stdin');
            ensure(observed.write_completed === true && equal({...expected, write_completed: true}, observed),
                'TRANSPORT', 'CAPTURE_MISMATCH', 'capture stdin differs from actual receipt/input/runtime/root context');
        }
        return {ordinal: i + 1, receipt_id: receipt.receipt_id, expected, observed,
            origin: observed === null ? 'unavailable' : 'caller-supplied-capture-snapshot'};
    });
}

function captureEvidenceContext(requestInput, dependencies = {}) {
    ensure(SCHEMA.revision === h1.REVISION, 'CONFIG', 'REVISION_MISMATCH', 'evidence/receipt revisions differ');
    const descriptors = ownData(requestInput);
    ensure(Object.hasOwn(descriptors, 'configPath'), 'CONFIG', 'SCHEMA_INVALID', 'configPath required');
    const configPath = h2.canonicalPath(descriptors.configPath.value);
    // Only trusted I/O establishes purpose, before looking at any hook values.
    const configFile = read(configPath, registry.LIMITS.configBytes, h2.nodeFsAdapter);
    const config = registry.validateConfiguration(h1.parseCanonicalJson(Buffer.from(configFile.base64, 'base64'), {maxBytes: registry.LIMITS.configBytes}));
    const hooks = hooksFor(dependencies, config.purpose);
    requiredApis();
    ensure(equal(Object.keys(descriptors), SCHEMA.request_keys), 'CONFIG', 'SCHEMA_INVALID', 'evidence request keys/order mismatch');
    const io = hooks.fs || h2.nodeFsAdapter;
    const receiptPaths = data(descriptors.receiptPaths.value);
    const checkerConfigPath = h2.canonicalPath(descriptors.checkerConfigPath.value);
    const manifestPath = h2.canonicalPath(descriptors.manifestPath.value);
    const directory = path.dirname(manifestPath), parent = path.dirname(directory);
    ensure(/^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(path.basename(manifestPath)) &&
        !Object.values(FILES).includes(path.basename(manifestPath)), 'PATH', 'PATH_POLICY', 'invalid/reserved manifest basename');
    const d = {configFile, config, configPath, io, hooks, receiptPaths, checkerConfigPath, manifestPath, directory, parent,
        parentGuards: h2.directoryGuards(parent, io), environment: envSnapshot(), failures: [], limitations: [...BASE_LIMITATIONS],
        durability: [], publication: {directory_created: false, paths: []}, originalLedger: descriptors.ledgerReadback.value};
    registry.recheckFile(configFile, io);
    d.inputs = h2.captureInputs(config.preflight, {io});
    const actual = (p, role) => d.inputs.manifest.files.find(row => row.path === p && row.roles.includes(role) && row.bytes > 0);
    for (const name of ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js', 'formal-ledger.js',
        'ledger-descriptors.js', 'p0f-evidence.js', 'evidence-descriptors.js'])
        ensure(actual(path.join(__dirname, name), 'source'), 'CONTEXT', 'SOURCE_UNPINNED', `missing actual source: ${name}`);
    for (const p of [configPath, config.registry_path, checkerConfigPath])
        ensure(actual(p, 'config'), 'CONTEXT', 'CONFIG_UNPINNED', 'formal/registry/checker config must be real H2 inputs');
    d.checkerFile = read(checkerConfigPath, LIMITS.checkerConfigBytes, io);
    d.checker = h1.parseCanonicalJson(Buffer.from(d.checkerFile.base64, 'base64'), {maxBytes: LIMITS.checkerConfigBytes});
    keys(d.checker, SCHEMA.checker_keys, 'checker config');
    const checker = d.checker;
    ensure(checker.schema_version === 1 && checker.purpose === config.purpose, 'CONFIG', 'PURPOSE_MISMATCH', 'checker version/purpose differs');
    h2.canonicalPath(checker.module_path); h2.canonicalPath(checker.cwd);
    ensure(actual(checker.module_path, 'source') || actual(checker.module_path, 'fixture'),
        'CONTEXT', 'CHECKER_UNPINNED', 'checker source must be a nonempty actual H2 source/fixture');
    ensure(config.purpose === 'synthetic' || !related(path.join(__dirname, 'fixtures'), checker.module_path),
        'CONFIG', 'SYNTHETIC_CHECKER_FORBIDDEN', 'validation cannot use a synthetic fixture checker');
    ensure(Array.isArray(checker.args) && checker.args.length <= 32, 'CONFIG', 'SCHEMA_INVALID', 'bounded explicit checker args required');
    for (const arg of checker.args) if (arg !== '') h2.string(arg, 'checker arg', 8192);
    ensure(h1.canonicalJson(checker.args).length <= 32768, 'CONFIG', 'LIMIT_EXCEEDED', 'checker argv exceeds cap');
    for (const [key, max] of [['timeout_ms', LIMITS.timeoutMs], ['max_stream_bytes', LIMITS.streamBytes]])
        ensure(Number.isSafeInteger(checker[key]) && checker[key] > 0 && checker[key] <= max, 'CONFIG', 'LIMIT_EXCEEDED', `invalid ${key}`);
    d.checkerEnvironment = normalizedEnvironment(checker.environment);
    if (process.platform === 'darwin') ensure(typeof process.env.__CF_USER_TEXT_ENCODING === 'string' &&
        d.checkerEnvironment.__CF_USER_TEXT_ENCODING === process.env.__CF_USER_TEXT_ENCODING,
    'CONFIG', 'ENVIRONMENT_MISMATCH', 'macOS encoding must be explicitly pinned');
    d.cwdGuards = h2.directoryGuards(checker.cwd, io);
    d.receiptOptions = {io, expected: {purpose: config.purpose, candidate: d.inputs.candidate,
        sourceManifestSha256: d.inputs.sourceManifestSha256, argv: [d.inputs.runtime.executable_path, config.preflight.runnerPath, ...config.preflight.runnerArgs],
        cwd: config.preflight.cwd, environmentSha256: h1.sha256(h1.canonicalJson(normalizedEnvironment(config.preflight.environment)))}, requirePass: true};
    d.receipts = h1.validateReceiptSet(receiptPaths, d.receiptOptions);
    ledger.revalidateJoinedLedger(d.originalLedger, ioDeps(d, io));
    const context = ledger.captureLedgerContext({configPath, receiptPaths}, ioDeps(d, io));
    d.h3 = data(descriptors.h3Result.value);
    d.joined = ledger.buildJoinedLedger(d.h3, context);
    ledger.readJoinedLedger(d.originalLedger.ledger_path, d.joined, ioDeps(d, io));
    ledger.revalidateJoinedLedger(d.originalLedger, ioDeps(d, io));
    d.ledgerFile = read(d.originalLedger.ledger_path, LEDGER_LIMITS.ledgerBytes, io, 0o400);
    d.transport = h2.freeze(checkTransport(descriptors.preflightCaptures.value, d));
    if (descriptors.preflightCaptures.value === null)
        d.limitations.push('Actual H2 preflight stdin snapshots were not supplied; expected bytes are derived, not observed transport.');
    const registryFile = read(config.registry_path, registry.LIMITS.registryBytes, io);
    const workers = registry.validateRegistry(h1.parseCanonicalJson(Buffer.from(registryFile.base64, 'base64'), {maxBytes: registry.LIMITS.registryBytes}), d.inputs).workers;
    d.outputs = workers.flatMap(w => w.outputs).sort().map(name => ({name,
        ...read(path.join(config.run_root, name), registry.LIMITS.outputBytes, io, 0o400)}));
    d.protected = [...d.inputs.manifest.files.map(row => row.path), config.run_root, path.dirname(d.ledgerFile.path),
        ...d.receipts.receipts.flatMap(row => [row.receiptDirectory, row.receipt.run_root.path])];
    const target = physical(directory, io);
    for (const p of d.protected) {
        const other = physical(p, io);
        ensure(!related(target, other) && !related(other, target), 'PATH', 'PATH_POLICY', 'evidence directory overlaps protected inputs/outputs/ledger/receipts');
    }
    verify(d, null, 'capture');
    const out = h2.freeze({schema_version: 1, purpose: config.purpose, manifest_path: manifestPath,
        ledger_path: d.ledgerFile.path, source_manifest_sha256: d.inputs.sourceManifestSha256});
    contexts.set(out, d); return out;
}

function directoryState(d, names, io) {
    h2.checkDirectoryGuards(d.parentGuards, io);
    h2.checkDirectoryGuards(d.rootGuards, io);
    const before = io.lstatSync(d.directory);
    ensure(Number(before.mode & 0o7777n) === 0o700, 'READBACK', 'DIRECTORY_CHANGED', 'evidence directory mode changed');
    const fd = io.openSync(d.directory, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
    try {
        ensure(h2.statIdentity(io.fstatSync(fd)) === h2.statIdentity(before), 'READBACK', 'DIRECTORY_CHANGED', 'directory descriptor differs');
        const dir = io.opendirSync(d.directory), found = [];
        try {
            for (;;) {
                const entry = dir.readSync(); if (entry === null) break;
                ensure(found.length < names.length, 'READBACK', 'MEMBERSHIP_MISMATCH', 'extra evidence entry');
                found.push(entry.name);
            }
        } finally { dir.closeSync(); }
        ensure(equal(found.sort(), [...names].sort()), 'READBACK', 'MEMBERSHIP_MISMATCH', 'missing/extra evidence entry');
        ensure(h2.statIdentity(io.fstatSync(fd)) === h2.statIdentity(before) && h2.statIdentity(io.lstatSync(d.directory)) === h2.statIdentity(before),
            'READBACK', 'DIRECTORY_CHANGED', 'directory changed while listing');
        h2.checkDirectoryGuards(d.rootGuards, io);
        return h2.statIdentity(before);
    } finally { io.closeSync(fd); }
}
function prefixCheck(d, saved, io) {
    const files = [...saved.files, ...(saved.review ? [saved.review] : [])];
    ensure(directoryState(d, files.map(row => path.basename(row.path)), io) === saved.rootIdentity,
        'REVALIDATION', 'MANIFEST_CHANGED', 'original evidence directory identity changed');
    for (const file of files) registry.recheckFile(file, io);
    ensure(directoryState(d, files.map(row => path.basename(row.path)), io) === saved.rootIdentity,
        'REVALIDATION', 'MANIFEST_CHANGED', 'original evidence directory identity changed after readback');
}
function verify(d, saved, phase, io = d.io) {
    const checks = [
        ['inputs', () => h2.revalidateInputs(d.inputs, {io}), 'INPUT_CHANGED'],
        ['receipts', () => h1.revalidateReceiptSet(d.receipts, {...d.receiptOptions, io}), 'RECEIPT_CHANGED'],
        ['outputs', () => { for (const file of d.outputs) registry.recheckFile(file, io); }, 'OUTPUT_CHANGED'],
        ['ledger', () => ledger.revalidateJoinedLedger(d.originalLedger, ioDeps(d, io)), 'LEDGER_CHANGED'],
        ['parents', () => { h2.checkDirectoryGuards(d.parentGuards, io); h2.checkDirectoryGuards(d.cwdGuards, io); }, 'PARENT_CHANGED'],
        ['environment', () => ensure(envSnapshot() === d.environment, 'REVALIDATION', 'ENVIRONMENT_CHANGED', 'parent environment changed'), 'ENVIRONMENT_CHANGED']
    ];
    if (saved) checks.push(['manifest', () => prefixCheck(d, saved, io), 'MANIFEST_CHANGED']);
    const failures = [];
    for (const [check, fn, reason] of checks) {
        try { fn(); }
        catch (error) { failures.push({phase, check, stage: 'REVALIDATION', reason, message: error.message}); }
    }
    // Cross-input reads must end with original metadata, not freshly blessed baselines.
    const files = [...d.inputs.snapshots, d.ledgerFile, ...(saved ? saved.files : []), ...(saved && saved.review ? [saved.review] : []), ...d.outputs];
    for (const file of files) {
        try {
            h2.checkDirectoryGuards(file.guards, io);
            ensure(h2.statIdentity(io.lstatSync(file.path)) === file.identity, 'REVALIDATION', 'FILE_CHANGED', `final identity changed: ${file.path}`);
        } catch (error) { failures.push({phase, check: 'final_metadata', stage: 'REVALIDATION', reason: 'FILE_CHANGED', message: error.message}); }
    }
    for (const receipt of d.receipts.receipts) {
        try {
            ensure(h2.statIdentity(io.lstatSync(receipt.receiptDirectory)) === receipt.receiptDirectoryIdentity,
                'REVALIDATION', 'RECEIPT_CHANGED', 'final receipt directory changed');
            [receipt.receiptPath, receipt.receipt.stdout.path, receipt.receipt.stderr.path].forEach((p, i) =>
                ensure(h2.statIdentity(io.lstatSync(p)) === receipt.fileIdentities[i], 'REVALIDATION', 'RECEIPT_CHANGED', 'final receipt file changed'));
        } catch (error) { failures.push({phase, check: 'final_receipts', stage: 'REVALIDATION', reason: 'RECEIPT_CHANGED', message: error.message}); }
    }
    try {
        h2.checkDirectoryGuards(d.parentGuards, io); h2.checkDirectoryGuards(d.cwdGuards, io);
        if (saved) ensure(h2.statIdentity(io.lstatSync(d.directory)) === saved.rootIdentity,
            'REVALIDATION', 'MANIFEST_CHANGED', 'evidence directory changed at final boundary');
        for (const file of d.outputs) ensure(h2.statIdentity(io.lstatSync(file.path)) === file.identity,
            'REVALIDATION', 'OUTPUT_CHANGED', 'output changed at final boundary');
    } catch (error) { failures.push({phase, check: 'final_boundary', stage: 'REVALIDATION', reason: error.reason || 'PARENT_CHANGED', message: error.message}); }
    d.failures.push(...failures);
    if (failures.length) throw new EvidenceError('REVALIDATION', failures[0].reason, failures.map(row => row.message).join('; '));
}
function syncDirectory(p, io, d) {
    const guards = h2.directoryGuards(p, io);
    const fd = io.openSync(p, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
    try {
        ensure(h2.directoryIdentity(io.fstatSync(fd)) === guards[guards.length - 1].identity, 'PUBLISH', 'PARENT_CHANGED', 'directory descriptor changed');
        try { io.fsyncSync(fd); }
        catch (error) {
            if (!['EINVAL', 'ENOTSUP', 'EOPNOTSUPP', 'ENOSYS'].includes(error.code)) throw error;
            const text = `${p}: directory fsync unavailable (${error.code})`;
            if (!d.durability.includes(text)) d.durability.push(text);
        }
        h2.checkDirectoryGuards(guards, io);
    } finally { io.closeSync(fd); }
}
function writeExclusive(d, p, bytes, cap, io) {
    ensure(bytes.length <= cap, 'PUBLISH', 'LIMIT_EXCEEDED', 'evidence file exceeds cap');
    h2.checkDirectoryGuards(d.rootGuards, io);
    let fd;
    try { fd = io.openSync(p, io.constants.O_WRONLY | io.constants.O_CREAT | io.constants.O_EXCL | io.constants.O_NOFOLLOW, 0o600); }
    catch (error) { if (error.code === 'EEXIST') throw new EvidenceError('PUBLISH', 'PATH_EXISTS', 'external file won exclusive creation'); throw error; }
    d.publication.paths.push(p);
    let identity;
    try {
        const opened = io.fstatSync(fd);
        ensure(opened.isFile() && opened.nlink === 1n && opened.size === 0n && h2.statIdentity(io.lstatSync(p)) === h2.statIdentity(opened),
            'PUBLISH', 'FILE_CHANGED', 'new file descriptor/path differs');
        for (let offset = 0; offset < bytes.length;) {
            const count = io.writeSync(fd, bytes, offset, bytes.length - offset);
            ensure(Number.isSafeInteger(count) && count > 0 && count <= bytes.length - offset, 'PUBLISH', 'SHORT_WRITE', 'invalid/zero write');
            offset += count;
        }
        io.fchmodSync(fd, 0o400); io.fsyncSync(fd);
        const after = io.fstatSync(fd);
        ensure(after.isFile() && after.dev === opened.dev && after.ino === opened.ino && after.nlink === 1n &&
            Number(after.mode & 0o7777n) === 0o400 && after.size === BigInt(bytes.length) &&
            h2.statIdentity(io.lstatSync(p)) === h2.statIdentity(after), 'PUBLISH', 'FILE_CHANGED', 'file changed during write');
        identity = h2.statIdentity(after);
    } finally { io.closeSync(fd); }
    const observed = read(p, cap, io, 0o400);
    ensure(observed.identity === identity && observed.base64 === bytes.toString('base64'), 'READBACK', 'READBACK_MISMATCH', 'written bytes/identity differ on readback');
    h1.parseCanonicalJson(Buffer.from(observed.base64, 'base64'), {maxBytes: cap});
    h2.checkDirectoryGuards(d.rootGuards, io);
    return observed;
}
function materialize(d) {
    const diagnostics = h1.canonicalJson(d.h3), transport = h1.canonicalJson({schema_version: 1, purpose: d.config.purpose, captures: d.transport});
    const bound = (name, bytes) => ({path: path.join(d.directory, name), bytes: bytes.length, sha256: h1.sha256(bytes)});
    const checker = {config: binding(d.checkerFile.path, d.checkerFile), source: d.inputs.manifest.files.find(row => row.path === d.checker.module_path),
        runtime: {...d.inputs.runtime, executable: d.inputs.manifest.files.find(row => row.roles.includes('runtime-executable'))},
        argv: [d.inputs.runtime.executable_path, d.checker.module_path, ...d.checker.args], cwd: d.checker.cwd,
        environment: d.checkerEnvironment, environment_sha256: h1.sha256(h1.canonicalJson(d.checkerEnvironment)),
        timeout_ms: d.checker.timeout_ms, max_stream_bytes: d.checker.max_stream_bytes};
    const manifest = h2.freeze({schema_version: 1, revision: SCHEMA.revision, purpose: d.config.purpose, kind: 'p0f-evidence',
        inputs: d.inputs.manifest, input_identities: d.inputs.snapshots.map(row => ({path: row.path, identity: row.identity, mode: row.mode})),
        receipts: d.receipts.receipts.map((row, i) => ({ordinal: i + 1, receipt_id: row.receipt.receipt_id,
            report: d.joined.genesis.receipts[i].report, stdout: d.joined.genesis.receipts[i].stdout, stderr: d.joined.genesis.receipts[i].stderr})),
        ledger: binding(d.ledgerFile.path, d.ledgerFile), h3_diagnostics: bound(FILES.diagnostics, diagnostics),
        preflight_transport: bound(FILES.transport, transport), worker_outputs: d.outputs.map(row => ({name: row.name, ...binding(row.path, row)})),
        checker, limitations: d.limitations});
    const manifestBytes = h1.canonicalJson(manifest);
    ensure(manifestBytes.length <= LIMITS.manifestBytes && diagnostics.length <= LIMITS.diagnosticsBytes && transport.length <= LIMITS.transportBytes,
        'PUBLISH', 'LIMIT_EXCEEDED', 'evidence inventory exceeds caps');
    return {manifest, checker, artifacts: [[path.join(d.directory, FILES.diagnostics), diagnostics, LIMITS.diagnosticsBytes],
        [path.join(d.directory, FILES.transport), transport, LIMITS.transportBytes], [d.manifestPath, manifestBytes, LIMITS.manifestBytes]]};
}

function readEvidenceManifest(manifestPath, context, dependencies = {}) {
    const d = contextData(context), hooks = hooksFor(dependencies, d.config.purpose), io = hooks.fs || d.io;
    ensure(Object.keys(hooks).every(k => k === 'fs'), 'CONFIG', 'SCHEMA_INVALID', 'read accepts only fs injection');
    ensure(manifestPath === d.manifestPath, 'READBACK', 'MANIFEST_MISMATCH', 'manifest path differs from captured expectation');
    ensure(d.rootGuards && d.written, 'READBACK', 'PUBLICATION_REQUIRED', 'original exclusive publication is required');
    const expected = materialize(d), files = [];
    ensure(directoryState(d, expected.artifacts.map(([p]) => path.basename(p)), io) === d.publishedRootIdentity,
        'READBACK', 'MANIFEST_MISMATCH', 'original publication directory changed');
    for (const [p, bytes, cap] of expected.artifacts) {
        const file = read(p, cap, io, 0o400);
        h1.parseCanonicalJson(Buffer.from(file.base64, 'base64'), {maxBytes: cap});
        ensure(file.base64 === bytes.toString('base64'), 'READBACK', 'MANIFEST_MISMATCH', 'evidence bytes differ from actual expectations');
        const original = d.written.find(row => row.path === p);
        ensure(original && file.identity === original.identity && file.base64 === original.base64,
            'READBACK', 'MANIFEST_MISMATCH', 'original published file identity/bytes changed');
        files.push(file);
    }
    const saved = {d, files, manifest: expected.manifest, checker: expected.checker, review: null, dispatched: false,
        rootIdentity: d.publishedRootIdentity};
    verify(d, saved, 'manifest-readback', io);
    const out = h2.freeze({schema_version: 1, purpose: d.config.purpose, status: 'READY', manifest_path: manifestPath,
        manifest_sha256: files[2].sha256, ledger_sha256: d.ledgerFile.sha256, manifest: expected.manifest});
    readbacks.set(out, saved); return out;
}
function publishEvidenceManifest(context, dependencies = {}) {
    const d = contextData(context), hooks = hooksFor(dependencies, d.config.purpose), io = hooks.fs || d.io;
    ensure(Object.keys(hooks).every(k => k === 'fs'), 'CONFIG', 'SCHEMA_INVALID', 'publish accepts only fs injection');
    try {
        verify(d, null, 'pre-publish', io);
        absent(d.manifestPath, io); absent(d.directory, io);
        h2.checkDirectoryGuards(d.parentGuards, io);
        try { io.mkdirSync(d.directory, {mode: 0o700}); }
        catch (error) { if (error.code === 'EEXIST') throw new EvidenceError('PUBLISH', 'PATH_EXISTS', 'external directory won creation'); throw error; }
        d.publication.directory_created = true;
        h2.checkDirectoryGuards(d.parentGuards, io);
        const before = io.lstatSync(d.directory);
        ensure(before.isDirectory() && !before.isSymbolicLink(), 'PUBLISH', 'DIRECTORY_CHANGED', 'new evidence directory is not real');
        const fd = io.openSync(d.directory, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
        let identity;
        try {
            ensure(h2.directoryIdentity(io.fstatSync(fd)) === h2.directoryIdentity(before), 'PUBLISH', 'DIRECTORY_CHANGED', 'new directory descriptor differs');
            io.fchmodSync(fd, 0o700); identity = h2.directoryIdentity(io.fstatSync(fd));
        } finally { io.closeSync(fd); }
        d.rootGuards = h2.directoryGuards(d.directory, io);
        ensure(d.rootGuards[d.rootGuards.length - 1].identity === identity, 'PUBLISH', 'DIRECTORY_CHANGED', 'new directory replaced');
        directoryState(d, [], io);
        const expected = materialize(d), written = [];
        for (const [p, bytes, cap] of expected.artifacts) written.push(writeExclusive(d, p, bytes, cap, io));
        d.written = written;
        d.publishedRootIdentity = directoryState(d, written.map(file => path.basename(file.path)), io);
        syncDirectory(d.directory, io, d); syncDirectory(d.parent, io, d);
        const previous = readEvidenceManifest(d.manifestPath, context, d.config.purpose === 'synthetic' ? {fs: io} : {});
        for (const file of written) registry.recheckFile(file, io);
        verify(d, issued(previous), 'post-publish', io);
        return previous;
    } catch (error) {
        try { verify(d, null, 'publish-failure', io); } catch { /* Retain the original publication failure. */ }
        throw wrap(error, d);
    }
}
function revalidateEvidenceManifest(previous, dependencies = {}) {
    const saved = issued(previous), d = saved.d, hooks = hooksFor(dependencies, d.config.purpose), io = hooks.fs || d.io;
    ensure(Object.keys(hooks).every(k => k === 'fs'), 'CONFIG', 'SCHEMA_INVALID', 'revalidate accepts only fs injection');
    verify(d, saved, 'revalidate', io); return previous;
}

function safeError(error) {
    const clean = (v, fallback, n) => {
        try { return Buffer.from(String(v).slice(0, n)).toString('utf8').replace(/\0/g, '') || fallback; }
        catch { return fallback; }
    };
    let code, message;
    try { code = error && error.code; message = error && error.message !== undefined ? error.message : error; }
    catch { message = 'unreadable exception'; }
    return {code: clean(code || 'ERROR', 'ERROR', 128), message: clean(message, 'unknown error', 2048)};
}
function wrap(error, d) {
    return new EvidenceError(error.stage || 'EVIDENCE', error.reason || 'IO_ERROR', safeError(error).message,
        {cause_code: error instanceof EvidenceError && error.details.cause_code || safeError(error).code,
            directory_created: d.publication.directory_created, paths: [...d.publication.paths],
            verification_failures: [...d.failures], durability_limitations: [...d.durability]});
}
function collectChecker(d, stdin, spawnProcess) {
    return new Promise(resolve => {
        const chunks = {stdout: [], stderr: []}, retained = {stdout: 0, stderr: 0}, observed = {stdout: 0, stderr: 0};
        let child, failure = null, error = null, timer, grace, finished = false, written = false;
        const finish = (exitCode, signal, closed = true) => {
            if (finished) return;
            finished = true; clearTimeout(timer); clearTimeout(grace);
            resolve({mode: 'process', argv: [d.inputs.runtime.executable_path, d.checker.module_path, ...d.checker.args],
                cwd: d.checker.cwd, environment_sha256: h1.sha256(h1.canonicalJson(d.checkerEnvironment)),
                stdin: {...h2.bytesSnapshot(stdin), write_completed: written},
                stdout: h2.bytesSnapshot(Buffer.concat(chunks.stdout, retained.stdout)), stderr: h2.bytesSnapshot(Buffer.concat(chunks.stderr, retained.stderr)),
                observed, child: {exit_code: exitCode, signal: signal || null, error}, failure, capture_complete: closed});
        };
        const stop = why => {
            if (finished) return;
            failure = failure || why;
            try { child.kill('SIGKILL'); } catch (cause) { error = error || safeError(cause); }
            if (!grace && !finished) grace = setTimeout(() => finish(null, null, false), LIMITS.killGraceMs);
        };
        try { child = spawnProcess(d.inputs.runtime.executable_path, [d.checker.module_path, ...d.checker.args],
            {cwd: d.checker.cwd, env: d.checkerEnvironment, shell: false, windowsHide: true, stdio: ['pipe', 'pipe', 'pipe']}); }
        catch (cause) { error = safeError(cause); failure = 'spawn threw'; finish(null, null, false); return; }
        if (!child || !child.stdin || typeof child.stdin.on !== 'function' || typeof child.stdin.end !== 'function' ||
            !child.stdout || typeof child.stdout.on !== 'function' || !child.stderr || typeof child.stderr.on !== 'function' ||
            typeof child.on !== 'function' || typeof child.kill !== 'function') {
            error = {code: 'INVALID_CHILD', message: 'invalid checker child'}; failure = 'invalid child';
            if (child && typeof child.kill === 'function') { try { child.kill('SIGKILL'); } catch { /* Retain invalid-child reason. */ } }
            finish(null, null, false); return;
        }
        for (const name of ['stdout', 'stderr']) {
            child[name].on('data', chunk => {
                if (finished) return;
                const bytes = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
                observed[name] = Math.min(Number.MAX_SAFE_INTEGER, observed[name] + bytes.length);
                const count = Math.min(bytes.length, d.checker.max_stream_bytes - retained[name]);
                if (count > 0) { chunks[name].push(Buffer.from(bytes.subarray(0, count))); retained[name] += count; }
                if (observed[name] > d.checker.max_stream_bytes) stop(`${name} cap exceeded`);
            });
            child[name].on('error', cause => { error = error || safeError(cause); stop(`${name} error`); });
        }
        child.on('error', cause => { error = safeError(cause); stop('process error'); });
        child.on('close', (code, signal) => finish(code, signal));
        child.stdin.on('error', cause => { error = error || safeError(cause); stop('stdin error'); });
        timer = setTimeout(() => stop('checker timeout'), d.checker.timeout_ms);
        try { child.stdin.end(stdin, cause => {
            if (cause) { error = error || safeError(cause); stop('stdin error'); } else written = true;
        }); } catch (cause) { error = error || safeError(cause); stop('stdin threw'); }
    });
}
function validateResult(result, context) {
    ensure(result && typeof result === 'object' && !Array.isArray(result), 'CHECKER', 'RESULT_MISSING', 'explicit checker result required');
    ensure(Object.hasOwn(result, 'status'), 'CHECKER', 'STATUS_MISSING', 'checker status required');
    ensure(['PASS', 'FAIL'].includes(result.status), 'CHECKER', 'STATUS_UNKNOWN', 'unrecognized checker status');
    keys(result, SCHEMA.result_keys, 'checker result');
    ensure(result.schema_version === 1 && result.purpose === context.purpose && result.manifest_sha256 === context.manifest.sha256 &&
        result.ledger_sha256 === context.ledger.sha256 && result.source_manifest_sha256 === context.source_manifest_sha256 &&
        equal(result.case_ids, context.case_ids) && result.output_count === context.output_count,
    'CHECKER', 'RESULT_MISMATCH', 'checker result differs from pinned expectations');
    ensure(Array.isArray(result.checks) && result.checks.length === CHECKS.length, 'CHECKER', 'CHECK_INVENTORY', 'checker checks missing/extra');
    result.checks.forEach((row, i) => {
        keys(row, SCHEMA.check_keys, 'checker check');
        ensure(row.id === CHECKS[i] && ['PASS', 'FAIL'].includes(row.status), 'CHECKER', 'CHECK_INVENTORY', 'checker check order/status differs');
    });
    ensure(result.status === 'PASS' && result.checks.every(row => row.status === 'PASS'), 'CHECKER', 'CHECKER_FAILED', 'automated checker reported failure');
    return result;
}
async function runEvidenceChecker(previous, dependencies = {}) {
    const saved = issued(previous), d = saved.d, hooks = hooksFor(dependencies, d.config.purpose), io = hooks.fs || d.io;
    ensure(!saved.dispatched, 'CHECKER', 'ALREADY_DISPATCHED', 'evidence checker attempt cannot be reused');
    const context = h2.freeze({schema_version: 1, revision: SCHEMA.revision, purpose: d.config.purpose,
        manifest: {path: d.manifestPath, bytes: saved.files[2].byte_length, sha256: previous.manifest_sha256},
        ledger: {path: d.ledgerFile.path, bytes: d.ledgerFile.byte_length, sha256: d.ledgerFile.sha256},
        source_manifest_sha256: d.inputs.sourceManifestSha256, case_ids: d.inputs.caseIds,
        output_count: d.outputs.length, required_checks: CHECKS});
    const stdin = h1.canonicalJson(context);
    ensure(stdin.length <= LIMITS.contextBytes, 'CHECKER', 'LIMIT_EXCEEDED', 'checker stdin exceeds cap');
    let dispatch = null, checked = null, failure = null, launches = 0;
    try {
        verify(d, saved, 'pre-dispatch', io);
        absent(path.join(d.directory, FILES.review), io);
        saved.dispatched = true; launches++;
        let raw;
        if (hooks.dispatch) {
            dispatch = {mode: 'synthetic-callback', stdin: h2.bytesSnapshot(stdin), stdout: h2.bytesSnapshot(Buffer.alloc(0)),
                stderr: h2.bytesSnapshot(Buffer.alloc(0)), child: null};
            let timer;
            try {
                raw = await Promise.race([Promise.resolve().then(() => hooks.dispatch(context)), new Promise((resolve, reject) => {
                    timer = setTimeout(() => reject(new EvidenceError('CHECKER', 'CHECKER_TIMEOUT', 'synthetic checker callback timeout')), d.checker.timeout_ms);
                })]);
            }
            catch (error) {
                dispatch.exception = safeError(error);
                if (error instanceof EvidenceError && error.reason === 'CHECKER_TIMEOUT') throw error;
                throw new EvidenceError('CHECKER', 'CHECKER_THROWN', safeError(error).message);
            }
            finally { clearTimeout(timer); }
        } else {
            dispatch = await collectChecker(d, stdin, hooks.spawn || spawn);
            verify(d, saved, 'post-dispatch', io);
            ensure(dispatch.capture_complete && !dispatch.failure && dispatch.stdin.write_completed && dispatch.child.exit_code === 0 &&
                dispatch.child.signal === null && dispatch.child.error === null, 'CHECKER', 'CHILD_NOT_PASS', dispatch.failure || 'checker process failed');
            ensure(dispatch.stdout.bytes > 0, 'CHECKER', 'RESULT_MISSING', 'checker stdout is empty');
            try { raw = h1.parseCanonicalJson(Buffer.from(dispatch.stdout.base64, 'base64'), {maxBytes: LIMITS.resultBytes}); }
            catch (error) { throw new EvidenceError('CHECKER', 'RESULT_PARSE_FAILED', error.message); }
        }
        verify(d, saved, 'post-dispatch', io);
        ensure(raw !== null && raw !== undefined && typeof raw === 'object' && !Array.isArray(raw),
            'CHECKER', 'RESULT_MISSING', 'explicit checker result required');
        checked = data(raw);
        validateResult(checked, context);
    } catch (error) {
        failure = error;
        try { verify(d, saved, 'dispatch-failure', io); } catch { /* Keep the first error and every failed observation. */ }
    }
    const review = {schema_version: 1, revision: SCHEMA.revision, purpose: d.config.purpose, kind: 'automated-checker-review',
        status: 'RECORDED', manifest_sha256: previous.manifest_sha256, ledger_sha256: d.ledgerFile.sha256,
        checker: saved.checker, dispatch, result: checked, error: failure ? {stage: failure.stage || 'CHECKER', reason: failure.reason || 'CHECKER_ERROR', ...safeError(failure)} : null,
        verification_scope: 'through-checker-dispatch; final publication checks require the issued H5 result/readback',
        verification_failures: [...d.failures], limitations: d.limitations};
    try {
        // A failed checker can leave an explicit FAIL review, but never overwrite
        // or bless a mutated manifest/root. Success is rechecked after publication.
        prefixCheck(d, saved, io);
        if (launches) {
            saved.review = writeExclusive(d, path.join(d.directory, FILES.review), h1.canonicalJson(review), LIMITS.reviewBytes, io);
            saved.rootIdentity = directoryState(d, [...saved.files, saved.review].map(file => path.basename(file.path)), io);
            syncDirectory(d.directory, io, d); syncDirectory(d.parent, io, d);
        }
        verify(d, saved, 'post-review', io);
    } catch (error) { failure = failure || error; try { verify(d, saved, 'review-failure', io); } catch { /* Preserve partial artifacts. */ } }
    const out = h2.freeze({schema_version: 1, purpose: d.config.purpose, status: failure ? 'FAIL' : 'PASS',
        stage: failure ? failure.stage || 'EVIDENCE' : 'COMPLETE', reason: failure ? failure.reason || 'IO_ERROR' : 'H5_EVIDENCE_COMPLETE',
        exit_code: failure ? 1 : 0, message: failure ? safeError(failure).message : 'evidence and automated review bound; no H7 qualification claimed',
        checker_launches: launches, manifest_path: d.manifestPath, manifest_sha256: previous.manifest_sha256,
        review_path: saved.review ? saved.review.path : null,
        directory_created: d.publication.directory_created, partial_paths: [...d.publication.paths], dispatch,
        verification_failures: [...d.failures], limitations: [...d.limitations], durability_limitations: [...d.durability]});
    readbacks.set(out, saved); return out;
}
async function runP0fEvidence(request, dependencies = {}) {
    let context, previous;
    try {
        context = captureEvidenceContext(request, dependencies);
        const d = contextData(context), io = d.hooks.fs || d.io;
        previous = publishEvidenceManifest(context, ioDeps(d, io));
        return await runEvidenceChecker(previous, dependencies);
    } catch (error) {
        const d = context ? contextData(context) : null;
        if (d) { try { verify(d, previous ? issued(previous) : null, 'failure'); } catch { /* Preserve the primary error. */ } }
        const details = d ? d.publication : {};
        return h2.freeze({schema_version: 1, purpose: d ? d.config.purpose : null,
            status: details.directory_created ? 'FAIL' : 'REFUSED', stage: error.stage || 'EVIDENCE', reason: error.reason || 'IO_ERROR',
            message: safeError(error).message, exit_code: details.directory_created ? 1 : 2, checker_launches: 0,
            directory_created: Boolean(details.directory_created), manifest_path: d ? d.manifestPath : null,
            partial_paths: details.paths ? [...details.paths] : [],
            cause_code: error instanceof EvidenceError && error.details.cause_code || safeError(error).code,
            verification_failures: d ? [...d.failures] : [], limitations: d ? [...d.limitations] : [...BASE_LIMITATIONS]});
    }
}

module.exports = Object.freeze({EvidenceError, captureEvidenceContext, publishEvidenceManifest,
    readEvidenceManifest, revalidateEvidenceManifest, runEvidenceChecker, runP0fEvidence});
