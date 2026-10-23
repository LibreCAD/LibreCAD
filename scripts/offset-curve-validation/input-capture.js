/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const h1 = require('./preflight-receipt');

const LIMITS = Object.freeze({files: 128, fileBytes: 8388608, runtimeBytes: 134217728,
    totalBytes: 268435456, inventoryBytes: 1048576, cases: 4096});
const INPUT_SCHEMA = Object.freeze({schema_version: 1, revision: h1.REVISION,
    purposes: Object.freeze(['validation', 'synthetic']),
    candidate_keys: Object.freeze(['path', 'bytes', 'sha256', 'git_blob']),
    manifest_keys: Object.freeze(['schema_version', 'revision', 'purpose', 'candidate', 'runtime',
        'case_inventory_path', 'cases', 'case_ids_sha256', 'files']),
    file_keys: Object.freeze(['path', 'roles', 'bytes', 'sha256', 'git_blob']),
    runtime_keys: Object.freeze(['exec_path', 'version', 'executable_path']),
    inventory_keys: Object.freeze(['schema_version', 'purpose', 'cases']),
    case_keys: Object.freeze(['id', 'source_path'])});
const nodeFsAdapter = Object.freeze({
    constants: fs.constants,
    lstatSync: p => fs.lstatSync(p, {bigint: true}),
    openSync: (...args) => fs.openSync(...args),
    fstatSync: fd => fs.fstatSync(fd, {bigint: true}),
    readSync: (...args) => fs.readSync(...args),
    closeSync: fd => fs.closeSync(fd),
    realpathSync: p => fs.realpathSync.native(p),
    opendirSync: p => fs.opendirSync(p, {bufferSize: 1}),
    mkdirSync: (...args) => fs.mkdirSync(...args),
    writeSync: (...args) => fs.writeSync(...args),
    fchmodSync: (...args) => fs.fchmodSync(...args),
    fsyncSync: fd => fs.fsyncSync(fd)
});
const captured = new WeakSet();

function check(condition, message) {
    if (!condition) throw new TypeError(message);
}

function freeze(value) {
    if (value && typeof value === 'object' && !Object.isFrozen(value)) {
        Object.values(value).forEach(freeze);
        Object.freeze(value);
    }
    return value;
}

function string(value, label, maxBytes = 4096) {
    check(typeof value === 'string' && value.length > 0 && !value.includes('\0') &&
        Buffer.byteLength(value) <= maxBytes && Buffer.from(value).toString('utf8') === value,
    `${label} must be a bounded, nonempty, NUL-free Unicode string`);
    return value;
}

function canonicalPath(value, label = 'path') {
    string(value, label);
    check(path.isAbsolute(value) && path.normalize(value) === value && path.resolve(value) === value,
        `${label} must be canonical and absolute`);
    return value;
}

function statIdentity(stat) {
    return [stat.dev, stat.ino, stat.mode, stat.nlink, stat.size, stat.mtimeNs, stat.ctimeNs].map(String).join(':');
}

function directoryIdentity(stat) {
    return [stat.dev, stat.ino, stat.mode].map(String).join(':');
}

function directoryGuards(directory, io = nodeFsAdapter) {
    canonicalPath(directory);
    const {O_NOFOLLOW, O_DIRECTORY, O_RDONLY} = io.constants;
    check(Number.isInteger(O_NOFOLLOW) && O_NOFOLLOW !== 0 && Number.isInteger(O_DIRECTORY) && O_DIRECTORY !== 0,
        'platform lacks no-follow directory flags');
    const root = path.parse(directory).root;
    const paths = [root];
    let current = root;
    for (const part of directory.slice(root.length).split(path.sep).filter(Boolean)) {
        current = path.join(current, part);
        paths.push(current);
    }
    const guards = paths.map(p => {
        const before = io.lstatSync(p);
        check(typeof before.dev === 'bigint' && typeof before.ino === 'bigint', 'bigint stat metadata is required');
        check(before.isDirectory() && !before.isSymbolicLink(), `not a real directory: ${p}`);
        const fd = io.openSync(p, O_RDONLY | O_NOFOLLOW | O_DIRECTORY);
        try {
            const opened = io.fstatSync(fd);
            check(opened.isDirectory() && directoryIdentity(opened) === directoryIdentity(before), `directory changed while opening: ${p}`);
        } finally { io.closeSync(fd); }
        return Object.freeze({path: p, identity: directoryIdentity(before)});
    });
    checkDirectoryGuards(guards, io);
    return Object.freeze(guards);
}

function checkDirectoryGuards(guards, io = nodeFsAdapter) {
    for (const guard of guards) {
        const stat = io.lstatSync(guard.path);
        check(stat.isDirectory() && !stat.isSymbolicLink() && directoryIdentity(stat) === guard.identity,
            `directory identity changed: ${guard.path}`);
    }
}

function bytesSnapshot(bytes) {
    return Object.freeze({base64: bytes.toString('base64'), bytes: bytes.length, sha256: h1.sha256(bytes)});
}

function gitBlob(bytes) {
    return crypto.createHash('sha1').update(Buffer.from(`blob ${bytes.length}\0`)).update(bytes).digest('hex');
}

function readPinnedFile(filePath, maxBytes, io) {
    const guards = directoryGuards(path.dirname(canonicalPath(filePath)), io);
    const before = io.lstatSync(filePath);
    check(before.isFile() && !before.isSymbolicLink() && before.nlink === 1n, `input is not a regular single-link file: ${filePath}`);
    check(before.size <= BigInt(maxBytes), `input exceeds byte cap: ${filePath}`);
    const read = h1.readReceiptFile(filePath, {io, maxBytes, mode: Number(before.mode & 0o7777n)});
    check(statIdentity(io.lstatSync(filePath)) === statIdentity(before), `input changed during read: ${filePath}`);
    checkDirectoryGuards(guards, io);
    const bytes = Buffer.from(read.base64, 'base64');
    return freeze({path: filePath, maxBytes, identity: statIdentity(before), device: String(before.dev),
        inode: String(before.ino), mode: Number(before.mode & 0o7777n), guards,
        ...bytesSnapshot(bytes), git_blob: gitBlob(bytes)});
}

function exactKeys(value, keys, label) {
    check(value && typeof value === 'object' && !Array.isArray(value) &&
        JSON.stringify(Object.keys(value)) === JSON.stringify(keys), `${label} keys/order mismatch`);
}

function captureInputs(config, {io = nodeFsAdapter} = {}) {
    check(config && typeof config === 'object', 'input configuration is required');
    check(config.candidate === undefined && config.sourceManifestSha256 === undefined,
        'supplied candidate/digest tuples are not capture inputs');
    check(INPUT_SCHEMA.purposes.includes(config.purpose), 'explicit input purpose is required');
    const purpose = config.purpose;
    check(purpose === 'synthetic' || io === nodeFsAdapter, 'input I/O injection is restricted to synthetic controls');
    const candidatePath = canonicalPath(config.candidatePath, 'candidatePath');
    const runnerPath = canonicalPath(config.runnerPath, 'runnerPath');
    const inventoryPath = canonicalPath(config.caseInventoryPath, 'caseInventoryPath');
    const runtime = {exec_path: process.execPath, version: process.version,
        executable_path: canonicalPath(io.realpathSync(process.execPath), 'runtime executable')};
    const rows = new Map();
    function include(p, role) {
        canonicalPath(p, role);
        if (!rows.has(p)) rows.set(p, new Set());
        rows.get(p).add(role);
    }
    const declarations = [['sourcePaths', 'source'], ['inputPaths', 'input'], ['fixturePaths', 'fixture'], ['configPaths', 'config']];
    for (const [key, role] of declarations) {
        check(Array.isArray(config[key]) && config[key].length <= LIMITS.files, `${key} must be an explicit bounded array`);
        check(new Set(config[key]).size === config[key].length, `${key} contains duplicate paths`);
        for (const p of config[key]) include(p, role);
    }
    check(config.sourcePaths.length > 0, 'sourcePaths must contain actual source files');
    for (const name of ['input-capture.js', 'preflight-capture.js', 'preflight-receipt.js', 'schema-descriptors.js'])
        include(path.join(__dirname, name), 'capture-source');
    include(candidatePath, 'candidate');
    include(runnerPath, 'runner');
    include(inventoryPath, 'case-inventory');
    include(runtime.executable_path, 'runtime-executable');
    check(rows.size <= LIMITS.files, 'too many captured files');
    const snapshots = [];
    const files = [];
    let totalBytes = 0;
    const identities = new Set();
    for (const p of [...rows.keys()].sort()) {
        const roles = [...rows.get(p)].sort();
        const maxBytes = roles.includes('runtime-executable') ? LIMITS.runtimeBytes :
            roles.includes('case-inventory') ? LIMITS.inventoryBytes : LIMITS.fileBytes;
        const snapshot = readPinnedFile(p, maxBytes, io);
        check(snapshot.bytes > 0 || roles.every(role => role === 'input'), `empty source/configuration file: ${p}`);
        if (roles.includes('runtime-executable')) check((snapshot.mode & 0o111) !== 0, 'runtime executable is not executable');
        const identity = `${snapshot.device}:${snapshot.inode}`;
        check(!identities.has(identity), 'captured input paths alias the same file');
        identities.add(identity);
        totalBytes += snapshot.bytes;
        check(totalBytes <= LIMITS.totalBytes, 'captured inputs exceed total byte cap');
        snapshots.push(snapshot);
        files.push({path: p, roles, bytes: snapshot.bytes, sha256: snapshot.sha256, git_blob: snapshot.git_blob});
    }
    const inventorySnapshot = snapshots.find(row => row.path === inventoryPath);
    const inventory = h1.parseCanonicalJson(Buffer.from(inventorySnapshot.base64, 'base64'),
        {maxBytes: LIMITS.inventoryBytes, label: 'case inventory'});
    exactKeys(inventory, INPUT_SCHEMA.inventory_keys, 'case inventory');
    check(inventory.schema_version === 1 && inventory.purpose === purpose, 'case inventory schema/purpose mismatch');
    check(Array.isArray(inventory.cases) && inventory.cases.length > 0 && inventory.cases.length <= LIMITS.cases,
        'case inventory must be nonempty and bounded');
    const caseIds = [];
    for (const row of inventory.cases) {
        exactKeys(row, INPUT_SCHEMA.case_keys, 'case');
        string(row.id, 'case ID', 128);
        canonicalPath(row.source_path, 'case source_path');
        check(rows.has(row.source_path) && [...rows.get(row.source_path)].some(role => role === 'source' || role === 'fixture'),
            'case definition is not a declared source/fixture input');
        check(caseIds.length === 0 || caseIds[caseIds.length - 1] < row.id, 'case IDs must be sorted and unique');
        caseIds.push(row.id);
    }
    const source = snapshots.find(row => row.path === candidatePath);
    const candidate = {path: source.path, bytes: source.bytes, sha256: source.sha256, git_blob: source.git_blob};
    const manifest = {schema_version: 1, revision: h1.REVISION, purpose, candidate, runtime,
        case_inventory_path: inventoryPath, cases: inventory.cases,
        case_ids_sha256: h1.sha256(h1.canonicalJson(caseIds)), files};
    const manifestSnapshot = bytesSnapshot(h1.canonicalJson(manifest));
    const result = freeze({candidate, runtime, manifest, manifestSnapshot,
        sourceManifestSha256: manifestSnapshot.sha256, caseIds, snapshots});
    captured.add(result);
    revalidateInputs(result, {io});
    return result;
}

function revalidateInputs(previous, {io = nodeFsAdapter} = {}) {
    check(captured.has(previous), 'original in-process input capture is required');
    check(previous.manifest.purpose === 'synthetic' || io === nodeFsAdapter, 'input I/O injection is restricted to synthetic controls');
    check(process.execPath === previous.runtime.exec_path && process.version === previous.runtime.version &&
        io.realpathSync(process.execPath) === previous.runtime.executable_path, 'runtime identity changed');
    for (const saved of previous.snapshots) {
        checkDirectoryGuards(saved.guards, io);
        const current = readPinnedFile(saved.path, saved.maxBytes, io);
        check(current.identity === saved.identity && current.base64 === saved.base64 && current.sha256 === saved.sha256,
            `captured input changed: ${saved.path}`);
    }
    for (const saved of previous.snapshots) {
        checkDirectoryGuards(saved.guards, io);
        check(statIdentity(io.lstatSync(saved.path)) === saved.identity, `captured input changed: ${saved.path}`);
    }
    return previous;
}

module.exports = Object.freeze({INPUT_SCHEMA, LIMITS, nodeFsAdapter, canonicalPath, string, freeze,
    statIdentity, directoryIdentity, directoryGuards, checkDirectoryGuards, bytesSnapshot,
    captureInputs, revalidateInputs});
