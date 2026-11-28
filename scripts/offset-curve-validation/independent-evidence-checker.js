/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {spawnSync} = require('node:child_process');
const {SCHEMA: LS, LIMITS: LL} = require('./ledger-descriptors');
const {SCHEMA: ES, LIMITS: EL, FILES, CHECKS} = require('./evidence-descriptors');
const RS = require('./schema-descriptors');

// Independent implementation: only built-ins and inert format data are imported.
const CAPS = Object.freeze({files: 384, total: 536870912, inputFiles: 128, inputTotal: 268435456,
    file: 8388608, runtime: 134217728, inventory: 1048576, cases: 4096, events: 32768});
const LIMITATIONS = Object.freeze([
    'Disk bindings and process records do not attest historical process origin or preflight stdin delivery.',
    'Filesystem identity/readback checks are observation points, not containment or race elimination.',
    'Test-case outcomes are not a count of JavaScript assertion calls or product feature completion.'
]);
const H5_LIMITATIONS = Object.freeze([
    'Automated checker execution is not human review or independent H7 qualification.',
    'Preflight stdin origin/delivery cannot be independently proven from receipts or caller-supplied capture snapshots.',
    'Filesystem checks are bounded open/fstat/readback observations, not a filesystem sandbox.',
    'Checker deadlines terminate the direct child; descendant containment is not provided.',
    'Synthetic callback controls cannot be forcibly cancelled and are not process transport evidence.'
]);
const CONFIG_KEYS = ['schema_version', 'purpose', 'registry_path', 'run_root', 'receipt_ids', 'receipt_roots', 'preflight'];
const PREFLIGHT_KEYS = ['purpose', 'candidatePath', 'runnerPath', 'sourcePaths', 'inputPaths', 'fixturePaths',
    'configPaths', 'caseInventoryPath', 'runnerArgs', 'cwd', 'environment'];
const RESULT_KEYS = ['schema_version', 'purpose', 'worker_id', 'ordinal', 'status', 'case_ids', 'events', 'outputs'];
const H3_KEYS = ['schema_version', 'purpose', 'status', 'stage', 'reason', 'message', 'exit_code', 'run_root',
    'root_created', 'worker_launches', 'source_manifest_sha256', 'registry_sha256', 'workers', 'last_dispatch',
    'durability_limitations', 'verification_failures'];

class CheckError extends Error {
    constructor(code) { super(code); this.name = 'CheckError'; this.code = code; }
}
function demand(ok, code) { if (!ok) throw new CheckError(code); }
const canonical = value => Buffer.from(`${JSON.stringify(value)}\n`);
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const equal = (a, b) => canonical(a).equals(canonical(b));
const identity = s => [s.dev, s.ino, s.mode, s.nlink, s.size, s.mtimeNs, s.ctimeNs].join(':');
const dirIdentity = s => [s.dev, s.ino, s.mode].join(':');
const blob = bytes => crypto.createHash('sha1').update(`blob ${bytes.length}\0`).update(bytes).digest('hex');
function keys(value, names) {
    demand(value && typeof value === 'object' && !Array.isArray(value) && equal(Object.keys(value), names), 'EXACT_KEYS');
}
function text(value, max = 4096, empty = false) {
    demand(typeof value === 'string' && (empty || value.length > 0) && Buffer.byteLength(value) <= max &&
        !value.includes('\0') && Buffer.from(value).toString('utf8') === value, 'STRING');
    return value;
}
function absolute(p) {
    text(p); demand(path.isAbsolute(p) && path.resolve(p) === p && path.normalize(p) === p, 'ABSOLUTE_PATH'); return p;
}
function array(value, max, nonempty = false) {
    demand(Array.isArray(value) && value.length <= max && (!nonempty || value.length > 0), 'ARRAY_LIMIT'); return value;
}
function uint(n, max = Number.MAX_SAFE_INTEGER, positive = false) {
    demand(Number.isSafeInteger(n) && n >= (positive ? 1 : 0) && n <= max, 'INTEGER_LIMIT'); return n;
}
function sha(s) { demand(typeof s === 'string' && /^[0-9a-f]{64}$/.test(s), 'SHA256'); return s; }
function sorted(values, max, identifiers = false) {
    array(values, max, true).forEach((s, i) => {
        text(s, 128); demand(!identifiers || /^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(s), 'IDENTIFIER');
        demand(i === 0 || values[i - 1] < s, 'SORTED_UNIQUE');
    });
}
function boundedTree(value) {
    let nodes = 0, bytes = 0;
    function visit(v, depth) {
        demand(++nodes <= EL.dataNodes && depth <= 64, 'JSON_TREE_LIMIT');
        if (typeof v === 'string') { text(v, EL.stringBytes, true); bytes += Buffer.byteLength(v); }
        else if (typeof v === 'number') demand(Number.isFinite(v), 'JSON_NUMBER');
        else if (v && typeof v === 'object') {
            if (Array.isArray(v)) array(v, EL.arrayItems);
            else {
                demand(Object.keys(v).length <= 128, 'JSON_KEYS_LIMIT');
                for (const k of Object.keys(v)) { text(k); bytes += Buffer.byteLength(k); }
            }
            for (const item of Object.values(v)) visit(item, depth + 1);
        } else demand(v === null || typeof v === 'boolean', 'JSON_TYPE');
        demand(bytes <= EL.diagnosticsBytes, 'JSON_BYTES_LIMIT');
    }
    visit(value, 0);
}
function parse(bytes, cap) {
    demand(Buffer.isBuffer(bytes) && bytes.length > 0 && bytes.length <= cap, 'JSON_BYTES_LIMIT');
    const source = bytes.toString('utf8');
    demand(Buffer.from(source).equals(bytes), 'JSON_UTF8');
    let depth = 0, quoted = false, escaped = false;
    for (const c of source) {
        if (quoted) { if (escaped) escaped = false; else if (c === '\\') escaped = true; else if (c === '"') quoted = false; }
        else if (c === '"') quoted = true;
        else if (c === '{' || c === '[') demand(++depth <= 64, 'JSON_DEPTH');
        else if (c === '}' || c === ']') depth--;
    }
    let value;
    try { value = JSON.parse(source); } catch { throw new CheckError('JSON_PARSE'); }
    boundedTree(value);
    // Duplicate keys, BOM, whitespace, number spellings and noncanonical escapes
    // cannot survive this exact byte comparison with the independently encoded tree.
    demand(canonical(value).equals(bytes), 'JSON_CANONICAL');
    return value;
}
function copyData(value) {
    const active = new Set();
    let nodes = 0;
    function copy(v, depth) {
        demand(++nodes <= EL.dataNodes && depth <= 64, 'JSON_TREE_LIMIT');
        if (v === null || ['string', 'number', 'boolean'].includes(typeof v)) return v;
        demand(v && typeof v === 'object' && !active.has(v) &&
            (Array.isArray(v) || [null, Object.prototype].includes(Object.getPrototypeOf(v))), 'PLAIN_DATA');
        const ds = Object.getOwnPropertyDescriptors(v), names = Reflect.ownKeys(ds);
        demand(names.every(k => typeof k === 'string' && Object.hasOwn(ds[k], 'value')), 'DATA_ACCESSOR');
        active.add(v);
        const out = Array.isArray(v) ? [] : Object.create(null);
        if (Array.isArray(v)) {
            uint(ds.length.value, EL.arrayItems);
            demand(names.length === ds.length.value + 1, 'DENSE_ARRAY');
            for (let i = 0; i < ds.length.value; i++) { demand(Object.hasOwn(ds, i), 'DENSE_ARRAY'); out.push(copy(ds[i].value, depth + 1)); }
        } else {
            demand(names.length <= 128, 'JSON_KEYS_LIMIT');
            for (const k of names) out[k] = copy(ds[k].value, depth + 1);
        }
        active.delete(v); return out;
    }
    const out = copy(value, 0); boundedTree(out); return out;
}

class Disk {
    constructor() { this.files = new Map(); this.guards = new Map(); this.directories = new Map(); this.absentPaths = new Set(); this.total = 0; }
    guard(p) {
        absolute(p);
        demand(fs.constants.O_NOFOLLOW && fs.constants.O_DIRECTORY, 'NOFOLLOW_UNAVAILABLE');
        const paths = [path.parse(p).root];
        for (const part of p.slice(paths[0].length).split(path.sep).filter(Boolean)) paths.push(path.join(paths[paths.length - 1], part));
        demand(paths.length <= 512, 'PATH_DEPTH');
        return paths.map(directory => {
            const s = fs.lstatSync(directory, {bigint: true});
            demand(s.isDirectory() && !s.isSymbolicLink(), 'DIRECTORY_SYMLINK');
            const id = dirIdentity(s), old = this.guards.get(directory);
            demand(old === undefined || old === id, 'DIRECTORY_CHANGED');
            const fd = fs.openSync(directory, fs.constants.O_RDONLY | fs.constants.O_DIRECTORY | fs.constants.O_NOFOLLOW);
            try { demand(dirIdentity(fs.fstatSync(fd, {bigint: true})) === id, 'DIRECTORY_CHANGED'); }
            finally { fs.closeSync(fd); }
            this.guards.set(directory, id); return {path: directory, identity: id};
        });
    }
    observe(p, cap, keep = true) {
        absolute(p); const guards = this.guard(path.dirname(p));
        const before = fs.lstatSync(p, {bigint: true});
        demand(before.isFile() && !before.isSymbolicLink() && before.nlink === 1n, 'FILE_SINGLE_LINK');
        demand(before.size <= BigInt(cap), 'FILE_BYTES_LIMIT');
        const fd = fs.openSync(p, fs.constants.O_RDONLY | fs.constants.O_NOFOLLOW), chunks = [], digest = crypto.createHash('sha256');
        let count = 0;
        try {
            demand(identity(fs.fstatSync(fd, {bigint: true})) === identity(before), 'FILE_CHANGED');
            const buffer = Buffer.alloc(65536);
            for (;;) {
                const n = fs.readSync(fd, buffer, 0, buffer.length, null); if (!n) break;
                count += n; demand(count <= cap, 'FILE_BYTES_LIMIT'); digest.update(buffer.subarray(0, n));
                if (keep) chunks.push(Buffer.from(buffer.subarray(0, n)));
            }
            demand(count === Number(before.size) && identity(fs.fstatSync(fd, {bigint: true})) === identity(before), 'FILE_CHANGED');
        } finally { fs.closeSync(fd); }
        demand(identity(fs.lstatSync(p, {bigint: true})) === identity(before), 'FILE_CHANGED');
        for (const g of guards) demand(dirIdentity(fs.lstatSync(g.path, {bigint: true})) === g.identity, 'DIRECTORY_CHANGED');
        return {path: p, cap, guards, stat: before, identity: identity(before), mode: Number(before.mode & 0o7777n),
            bytes: count, sha256: digest.digest('hex'), data: keep ? Buffer.concat(chunks, count) : null};
    }
    read(p, cap = CAPS.file, mode) {
        const old = this.files.get(p);
        const file = old || this.observe(p, cap);
        demand(file.bytes <= cap && (mode === undefined || file.mode === mode), 'FILE_METADATA');
        if (!old) {
            this.total += file.bytes; demand(this.total <= CAPS.total && this.files.size < CAPS.files, 'READ_BUDGET');
            this.files.set(p, file);
        }
        return file;
    }
    json(p, cap, mode) { return parse(this.read(p, cap, mode).data, cap); }
    absent(p) {
        absolute(p); this.guard(path.dirname(p));
        try { fs.lstatSync(p); } catch (error) { if (error.code === 'ENOENT') { this.absentPaths.add(p); return; } throw error; }
        throw new CheckError('PATH_NOT_ABSENT');
    }
    directory(p, names) {
        this.guard(p); const before = fs.lstatSync(p, {bigint: true});
        demand(Number(before.mode & 0o7777n) === 0o700, 'DIRECTORY_MODE');
        const fd = fs.openSync(p, fs.constants.O_RDONLY | fs.constants.O_NOFOLLOW | fs.constants.O_DIRECTORY);
        let dir; const found = [];
        try {
            dir = fs.opendirSync(p, {bufferSize: 1});
            demand(identity(fs.fstatSync(fd, {bigint: true})) === identity(before), 'DIRECTORY_CHANGED');
            for (;;) { const e = dir.readSync(); if (!e) break; demand(found.length < names.length, 'DIRECTORY_MEMBERSHIP'); found.push(e.name); }
            demand(equal(found.sort(), [...names].sort()), 'DIRECTORY_MEMBERSHIP');
            demand(identity(fs.fstatSync(fd, {bigint: true})) === identity(before) &&
                identity(fs.lstatSync(p, {bigint: true})) === identity(before), 'DIRECTORY_CHANGED');
        } finally { try { if (dir) dir.closeSync(); } finally { fs.closeSync(fd); } }
        const prior = this.directories.get(p);
        demand(!prior || prior.identity === identity(before), 'DIRECTORY_CHANGED');
        this.directories.set(p, {identity: identity(before), names: [...names]});
    }
    finish() {
        for (const saved of this.files.values()) {
            const now = this.observe(saved.path, saved.cap, false);
            demand(now.identity === saved.identity && now.sha256 === saved.sha256, 'READBACK_CHANGED');
        }
        for (const [p, state] of this.directories) this.directory(p, state.names);
        for (const p of this.absentPaths) this.absent(p);
        for (const [p, id] of this.guards) demand(dirIdentity(fs.lstatSync(p, {bigint: true})) === id, 'DIRECTORY_CHANGED');
        for (const f of this.files.values()) demand(identity(fs.lstatSync(f.path, {bigint: true})) === f.identity, 'READBACK_CHANGED');
        for (const [p, d] of this.directories) demand(identity(fs.lstatSync(p, {bigint: true})) === d.identity, 'DIRECTORY_CHANGED');
    }
}
function binding(file) { return {path: file.path, bytes: file.bytes, sha256: file.sha256}; }
function statBinding(file) { return {...binding(file), identity: file.identity, mode: file.mode}; }
function pinned(file) { return {...binding(file), git_blob: blob(file.data)}; }
function bound(disk, row, cap, mode, metadata = false) {
    keys(row, metadata ? ['path', 'bytes', 'sha256', 'identity', 'mode'] : ['path', 'bytes', 'sha256']);
    const file = disk.read(row.path, cap, mode);
    demand(equal(row, metadata ? statBinding(file) : binding(file)), 'DISK_BINDING'); return file;
}
function environment(value) {
    demand(value && typeof value === 'object' && !Array.isArray(value), 'ENVIRONMENT');
    const out = Object.create(null), names = Object.keys(value).sort(); demand(names.length <= 64, 'ENVIRONMENT');
    for (const name of names) {
        demand(/^[A-Za-z_][A-Za-z0-9_]*$/.test(name) && !/^(NODE_|LD_|DYLD_)/.test(name) &&
            !['ENV', 'BASH_ENV', 'OPENSSL_CONF', 'OPENSSL_MODULES', 'LC_OFFSET_CAPTURE_CONTEXT'].includes(name), 'ENVIRONMENT');
        out[name] = text(value[name], 8192, true);
    }
    demand(canonical(out).length <= 65536, 'ENVIRONMENT'); return out;
}
function argumentsList(args, max) {
    array(args, max).forEach(s => text(s, 8192, true)); demand(canonical(args).length <= 32768, 'ARGUMENTS');
}
function reconstructInputs(config, disk) {
    keys(config, PREFLIGHT_KEYS); demand(RS.PURPOSES.includes(config.purpose), 'PURPOSE');
    absolute(config.cwd); disk.guard(config.cwd); argumentsList(config.runnerArgs, 128); environment(config.environment);
    const runtime = {exec_path: process.execPath, version: process.version, executable_path: absolute(fs.realpathSync.native(process.execPath))};
    const declarations = new Map();
    const add = (p, role) => { absolute(p); if (!declarations.has(p)) declarations.set(p, new Set()); declarations.get(p).add(role); };
    for (const [name, role] of [['sourcePaths', 'source'], ['inputPaths', 'input'], ['fixturePaths', 'fixture'], ['configPaths', 'config']]) {
        array(config[name], CAPS.inputFiles); demand(new Set(config[name]).size === config[name].length, 'INPUT_DUPLICATE');
        config[name].forEach(p => add(p, role));
    }
    demand(config.sourcePaths.length > 0, 'SOURCE_REQUIRED');
    for (const name of ['input-capture.js', 'preflight-capture.js', 'preflight-receipt.js', 'schema-descriptors.js']) add(path.join(__dirname, name), 'capture-source');
    add(config.candidatePath, 'candidate'); add(config.runnerPath, 'runner'); add(config.caseInventoryPath, 'case-inventory'); add(runtime.executable_path, 'runtime-executable');
    demand(declarations.size <= CAPS.inputFiles, 'INPUT_COUNT');
    const files = [], identities = new Set(); let total = 0;
    for (const p of [...declarations.keys()].sort()) {
        const roles = [...declarations.get(p)].sort();
        const f = disk.read(p, roles.includes('runtime-executable') ? CAPS.runtime : roles.includes('case-inventory') ? CAPS.inventory : CAPS.file);
        demand(f.bytes > 0 || roles.every(r => r === 'input'), 'EMPTY_INPUT');
        demand(!roles.includes('runtime-executable') || (f.mode & 0o111) !== 0, 'RUNTIME_MODE');
        const id = `${f.stat.dev}:${f.stat.ino}`; demand(!identities.has(id), 'INPUT_ALIAS'); identities.add(id);
        total += f.bytes; demand(total <= CAPS.inputTotal, 'INPUT_TOTAL');
        files.push({path: p, roles, bytes: f.bytes, sha256: f.sha256, git_blob: blob(f.data)});
    }
    const inventory = disk.json(config.caseInventoryPath, CAPS.inventory);
    keys(inventory, ['schema_version', 'purpose', 'cases']); demand(inventory.schema_version === 1 && inventory.purpose === config.purpose, 'INVENTORY_PURPOSE');
    array(inventory.cases, CAPS.cases, true).forEach(row => {
        keys(row, ['id', 'source_path']); text(row.id, 128); absolute(row.source_path);
        demand(files.some(f => f.path === row.source_path && f.roles.some(r => ['source', 'fixture'].includes(r))), 'CASE_SOURCE');
    });
    const ids = inventory.cases.map(row => row.id); sorted(ids, CAPS.cases);
    const candidate = pinned(disk.read(config.candidatePath));
    return {schema_version: 1, revision: RS.REVISION, purpose: config.purpose, candidate, runtime,
        case_inventory_path: config.caseInventoryPath, cases: inventory.cases, case_ids_sha256: hash(canonical(ids)), files};
}
function deriveInputManifest(preflightConfig) {
    const disk = new Disk();
    const manifest = reconstructInputs(copyData(preflightConfig), disk); disk.finish(); return manifest;
}
function successChild(child) { keys(child, RS.CHILD_KEYS); demand(equal(child, {exit_code: 0, signal: null, error: null}), 'CHILD_NOT_PASS'); }
function stream(row, max, extra = []) {
    keys(row, ['base64', 'bytes', 'sha256', ...extra]); uint(row.bytes, max); sha(row.sha256); text(row.base64, Math.ceil(max / 3) * 4, true);
    const bytes = Buffer.from(row.base64, 'base64');
    demand(bytes.length === row.bytes && bytes.toString('base64') === row.base64 && hash(bytes) === row.sha256, 'STREAM_BYTES'); return bytes;
}
function sample(bytes) { return {base64: bytes.toString('base64'), bytes: bytes.length, sha256: hash(bytes)}; }
function rootProof(root, disk) {
    keys(root, RS.ROOT_PROOF_KEYS); keys(root.parent, RS.PARENT_KEYS); absolute(root.path);
    demand(root.parent.path === path.dirname(root.path), 'ROOT_PARENT'); disk.guard(root.parent.path);
    const stat = fs.lstatSync(root.parent.path, {bigint: true});
    demand(root.parent.device === String(stat.dev) && root.parent.inode === String(stat.ino), 'ROOT_PARENT');
    demand(root.absent_before === true && root.absent_after === true && root.parent_stable === true && root.write_attempts === 0 && root.error === null, 'ROOT_PROOF');
    disk.absent(root.path);
}
function preflightResult(value, inputs, root) {
    keys(value, RS.PREFLIGHT_RESULT_KEYS);
    demand(value.revision === RS.REVISION && value.purpose === inputs.purpose && value.status === 'PASS' &&
        equal(value.candidate, inputs.candidate) && value.source_manifest_sha256 === hash(canonical(inputs)) && equal(value.run_root, root), 'PREFLIGHT_IDENTITY');
    keys(value.inventory_gate, RS.INVENTORY_KEYS);
    demand(equal(value.inventory_gate, {status: 'PASS', expected_count: inputs.cases.length, actual_count: inputs.cases.length,
        expected_ids_sha256: inputs.case_ids_sha256, actual_ids_sha256: inputs.case_ids_sha256, missing_ids: [], extra_ids: [], duplicate_ids: []}), 'PREFLIGHT_INVENTORY');
    keys(value.patch_replays, RS.PATCH_REPLAYS_KEYS); demand(value.patch_replays.status === 'PASS', 'PATCH_STATUS');
    demand(array(value.patch_replays.directions, 4).length === 4, 'PATCH_COUNT');
    value.patch_replays.directions.forEach((row, i) => { keys(row, RS.PATCH_DIRECTION_KEYS); demand(row.name === RS.PATCH_DIRECTIONS[i] && row.status === 'PASS', 'PATCH_STATUS'); sha(row.patch_sha256); sha(row.result_sha256); });
    keys(value.audit_parity, RS.AUDIT_PARITY_KEYS); demand(value.audit_parity.status === 'PASS' && value.audit_parity.equal === true &&
        sha(value.audit_parity.primary_sha256) === sha(value.audit_parity.independent_sha256), 'AUDIT_PARITY');
    keys(value.suite_parity, RS.SUITE_PARITY_KEYS);
    demand(equal(value.suite_parity, {status: 'PASS', suite_count: 4, identical_repeated_outputs: true}), 'SUITE_PARITY');
    demand(array(value.suites, 4).length === 4, 'SUITE_COUNT');
    value.suites.forEach((suite, i) => {
        keys(suite, RS.SUITE_KEYS); demand(suite.name === RS.PREFLIGHT_SUITES[i] && suite.status === 'PASS' && array(suite.attempts, 2).length === 2, 'SUITE_STATUS');
        suite.attempts.forEach((a, j) => { keys(a, RS.SUITE_ATTEMPT_KEYS); demand(a.ordinal === j + 1 && a.status === 'PASS', 'ATTEMPT_STATUS'); uint(a.report_bytes, CAPS.file, true); sha(a.report_sha256); uint(a.assertions, CAPS.cases, true); });
        demand(equal({...suite.attempts[0], ordinal: 2}, suite.attempts[1]), 'SUITE_PARITY');
    });
    keys(value.syntax, RS.SYNTAX_KEYS); demand(value.syntax.status === 'PASS', 'SYNTAX_STATUS'); uint(value.syntax.checked_file_count, CAPS.inputFiles, true); sha(value.syntax.checked_paths_sha256);
    keys(value.caps, RS.CAPS_KEYS); demand(value.caps.status === 'PASS', 'CAP_STATUS');
    array(value.caps.checks, 128, true).forEach(c => { keys(c, RS.CAP_CHECK_KEYS); text(c.name, 256); uint(c.limit); uint(c.measured); demand(c.passed === true && c.measured <= c.limit, 'CAP_STATUS'); });
    keys(value.trace_probe, RS.TRACE_KEYS); demand(value.trace_probe.status === 'PASS' && value.trace_probe.exit_code === 0, 'TRACE_STATUS'); sha(value.trace_probe.stdout_sha256); sha(value.trace_probe.stderr_sha256);
}
function related(a, b) { const r = path.relative(a, b); return r === '' || (r !== '..' && !r.startsWith(`..${path.sep}`) && !path.isAbsolute(r)); }
function physical(p) {
    absolute(p); const tail = [];
    for (;;) {
        try { return path.join(fs.realpathSync.native(p), ...tail); }
        catch (error) {
            if (error.code !== 'ENOENT') throw error;
            try { fs.lstatSync(p); throw new CheckError('DANGLING_PATH'); }
            catch (cause) { if (cause.code !== 'ENOENT') throw cause; }
            const parent = path.dirname(p); demand(parent !== p, 'MISSING_ROOT'); tail.unshift(path.basename(p)); p = parent;
        }
    }
}
function disjoint(a, b) { a = physical(a); b = physical(b); demand(!related(a, b) && !related(b, a), 'PATH_OVERLAP'); }
function fullOutput(file) {
    return {name: path.basename(file.path), path: file.path, maxBytes: CAPS.file, guards: file.guards, identity: file.identity,
        base64: file.data.toString('base64'), byte_length: file.bytes, sha256: file.sha256, mode: file.mode,
        device: String(file.stat.dev), inode: String(file.stat.ino)};
}
function checkRegistry(value, inputs) {
    keys(value, ['schema_version', 'purpose', 'workers']); demand(value.schema_version === 1 && value.purpose === inputs.purpose, 'REGISTRY_IDENTITY');
    const cases = [], names = [], ids = [];
    array(value.workers, 32, true).forEach(w => {
        keys(w, ['id', 'module_path', 'args', 'case_ids', 'outputs', 'timeout_ms', 'max_stream_bytes']); sorted([w.id], 1, true);
        demand(!ids.includes(w.id), 'WORKER_DUPLICATE'); ids.push(w.id); absolute(w.module_path);
        demand(inputs.files.some(f => f.path === w.module_path && f.roles.some(r => ['source', 'fixture'].includes(r))), 'WORKER_SOURCE');
        argumentsList(w.args, 32); sorted(w.case_ids, 128, true); sorted(w.outputs, 128, true); uint(w.timeout_ms, 300000, true); uint(w.max_stream_bytes, 1048576, true);
        for (const id of w.case_ids) { demand(!cases.includes(id) && inputs.cases.some(c => c.id === id && c.source_path === w.module_path), 'REGISTRY_CASE'); cases.push(id); }
        for (const name of w.outputs) { demand(!names.includes(name.toLowerCase()), 'OUTPUT_DUPLICATE'); names.push(name.toLowerCase()); }
    });
    demand(names.length <= 128 && equal(cases.sort(), inputs.cases.map(c => c.id)), 'REGISTRY_INVENTORY'); return value.workers;
}

function verifyEvidence(context, disk, state) {
    const mf = bound(disk, context.manifest, EL.manifestBytes, 0o400), lf = bound(disk, context.ledger, LL.ledgerBytes, 0o400);
    const manifest = parse(mf.data, EL.manifestBytes), claimedLedger = parse(lf.data, LL.ledgerBytes);
    keys(manifest, ES.manifest_keys); keys(claimedLedger, LS.ledger_keys); keys(claimedLedger.genesis, LS.genesis_keys);
    demand(manifest.schema_version === 1 && manifest.revision === RS.REVISION && manifest.purpose === context.purpose && manifest.kind === 'p0f-evidence', 'MANIFEST_IDENTITY');
    const cf = bound(disk, claimedLedger.genesis.config, 65536), config = parse(cf.data, 65536);
    keys(config, CONFIG_KEYS); demand(config.schema_version === 1 && config.purpose === context.purpose && config.preflight.purpose === context.purpose, 'CONFIG_IDENTITY');
    absolute(config.registry_path); absolute(config.run_root);
    demand(array(config.receipt_ids, 2).length === 2 && new Set(config.receipt_ids).size === 2 && config.receipt_ids.every(id => typeof id === 'string' && /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(id)), 'RECEIPT_IDS');
    demand(array(config.receipt_roots, 2).length === 2 && new Set(config.receipt_roots).size === 2, 'RECEIPT_ROOTS'); config.receipt_roots.forEach(absolute);
    const inputs = reconstructInputs(config.preflight, disk), sourceHash = hash(canonical(inputs));
    demand(equal(manifest.inputs, inputs) && sourceHash === context.source_manifest_sha256, 'ACTUAL_INPUT_MANIFEST');
    const expectedIdentities = inputs.files.map(row => { const f = disk.files.get(row.path); return {path: row.path, identity: f.identity, mode: f.mode}; });
    demand(equal(manifest.input_identities, expectedIdentities), 'INPUT_IDENTITIES');
    const role = (p, r) => inputs.files.some(f => f.path === p && f.roles.includes(r) && f.bytes > 0);
    for (const name of ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js', 'formal-ledger.js', 'ledger-descriptors.js',
        'p0f-evidence.js', 'evidence-descriptors.js', 'independent-evidence-checker.js']) demand(role(path.join(__dirname, name), 'source'), 'SOURCE_CLOSURE');
    for (const p of [cf.path, config.registry_path]) demand(role(p, 'config'), 'CONFIG_CLOSURE');
    const rf = disk.read(config.registry_path, 65536), workers = checkRegistry(parse(rf.data, 65536), inputs);
    const caseIds = inputs.cases.map(c => c.id), outputNames = workers.flatMap(w => w.outputs).sort();
    demand(equal(caseIds, context.case_ids) && outputNames.length === context.output_count, 'CONTEXT_INVENTORY');
    keys(manifest.checker, ['config', 'source', 'runtime', 'argv', 'cwd', 'environment', 'environment_sha256', 'timeout_ms', 'max_stream_bytes']);
    const checkerFile = bound(disk, manifest.checker.config, EL.checkerConfigBytes, undefined, true);
    demand(role(checkerFile.path, 'config'), 'CHECKER_CONFIG_CLOSURE');
    const checker = parse(checkerFile.data, EL.checkerConfigBytes); keys(checker, ES.checker_keys);
    demand(checker.schema_version === 1 && checker.purpose === context.purpose && checker.module_path === __filename && equal(checker.args, []), 'CHECKER_IDENTITY');
    absolute(checker.cwd); uint(checker.timeout_ms, EL.timeoutMs, true); uint(checker.max_stream_bytes, EL.streamBytes, true);
    const checkerEnv = environment(checker.environment), runtimeFile = inputs.files.find(f => f.roles.includes('runtime-executable'));
    const runtime = {...inputs.runtime, executable: runtimeFile};
    const expectedChecker = {config: statBinding(checkerFile), source: inputs.files.find(f => f.path === __filename), runtime,
        argv: [inputs.runtime.executable_path, __filename], cwd: checker.cwd, environment: checkerEnv,
        environment_sha256: hash(canonical(checkerEnv)), timeout_ms: checker.timeout_ms, max_stream_bytes: checker.max_stream_bytes};
    demand(equal(manifest.checker, expectedChecker), 'CHECKER_BINDING');
    demand(process.cwd() === checker.cwd && equal(environment(process.env), checkerEnv), 'CHECKER_PROCESS');
    demand(equal(manifest.ledger, statBinding(lf)), 'LEDGER_BINDING');
    state.inputs = inputs; state.workers = workers.length; state.outputs = outputNames.length; state.checks[0] = 'PASS';
    demand(array(manifest.receipts, 2).length === 2, 'RECEIPT_COUNT');
    const receiptRows = [], receipts = [], parents = new Set();
    for (let i = 0; i < 2; i++) {
        const row = manifest.receipts[i]; keys(row, LS.receipt_keys);
        demand(row.ordinal === i + 1 && row.receipt_id === config.receipt_ids[i], 'RECEIPT_ORDER');
        const report = bound(disk, row.report, RS.MAX_REPORT_BYTES, 0o400), receipt = parse(report.data, RS.MAX_REPORT_BYTES);
        keys(receipt, RS.RECEIPT_KEYS); demand(receipt.schema_version === 2 && receipt.revision === RS.REVISION && receipt.purpose === inputs.purpose &&
            receipt.receipt_id === row.receipt_id && receipt.status === 'PASS' && equal(receipt.candidate, inputs.candidate) && receipt.source_manifest_sha256 === sourceHash, 'RECEIPT_IDENTITY');
        demand(receipt.run_root.path === config.receipt_roots[i], 'RECEIPT_ROOT'); rootProof(receipt.run_root, disk);
        const parentId = `${receipt.run_root.parent.device}:${receipt.run_root.parent.inode}`; demand(!parents.has(parentId), 'RECEIPT_PARENT_ALIAS'); parents.add(parentId);
        demand(equal(receipt.argv, [inputs.runtime.executable_path, config.preflight.runnerPath, ...config.preflight.runnerArgs]) &&
            receipt.cwd === config.preflight.cwd && receipt.environment_sha256 === hash(canonical(environment(config.preflight.environment))), 'RECEIPT_DISPATCH');
        successChild(receipt.child);
        const directory = path.dirname(report.path), prefix = row.receipt_id;
        demand(path.basename(report.path) === `${prefix}.json`, 'RECEIPT_BASENAME');
        disk.directory(directory, [`${prefix}.json`, `${prefix}.stdout`, `${prefix}.stderr`]);
        for (const name of ['stdout', 'stderr']) {
            keys(receipt[name], RS.STREAM_KEYS); const f = bound(disk, row[name], RS.MAX_STREAM_BYTES, 0o400);
            demand(f.path === path.join(directory, `${prefix}.${name}`) && equal(receipt[name], {...binding(f), file_sha256: f.sha256}), 'RECEIPT_STREAM');
        }
        demand(equal(parse(disk.files.get(row.stdout.path).data, RS.MAX_STREAM_BYTES), receipt.preflight_result), 'RECEIPT_STDOUT');
        preflightResult(receipt.preflight_result, inputs, receipt.run_root);
        receipts.push(receipt); receiptRows.push({ordinal: i + 1, receipt_id: row.receipt_id, report: binding(report), stdout: row.stdout, stderr: row.stderr});
    }
    for (let i = 0; i < 2; i++) {
        disjoint(path.dirname(receiptRows[i].report.path), path.dirname(receiptRows[1 - i].report.path));
        for (const root of config.receipt_roots) disjoint(path.dirname(receiptRows[i].report.path), root);
        for (const p of inputs.files.map(f => f.path)) { disjoint(config.receipt_roots[i], p); disjoint(path.dirname(receiptRows[i].report.path), p); }
    }
    checkPreflightArtifacts(config, inputs, receipts, receiptRows, disk, [path.dirname(mf.path), path.dirname(lf.path)]);
    state.checks[1] = 'PASS';
    const evidenceDir = path.dirname(mf.path), ledgerDir = path.dirname(lf.path);
    const protectedPaths = [...inputs.files.map(f => f.path), config.run_root, ledgerDir,
        ...receiptRows.map(r => path.dirname(r.report.path)), ...config.receipt_roots];
    for (const p of protectedPaths) disjoint(evidenceDir, p);
    for (const p of [...inputs.files.map(f => f.path), ...receiptRows.map(r => path.dirname(r.report.path)), ...config.receipt_roots]) { disjoint(config.run_root, p); disjoint(ledgerDir, p); }
    disjoint(config.run_root, ledgerDir); disjoint(config.receipt_roots[0], config.receipt_roots[1]);
    disk.directory(ledgerDir, [path.basename(lf.path)]); disk.directory(config.run_root, outputNames);
    const outputs = outputNames.map(name => disk.read(path.join(config.run_root, name), CAPS.file, 0o400));
    demand(outputs.reduce((n, f) => n + f.bytes, 0) <= 67108864, 'OUTPUT_TOTAL');
    demand(equal(manifest.worker_outputs, outputs.map(f => ({name: path.basename(f.path), ...statBinding(f)}))), 'ACTUAL_OUTPUTS');
    demand(manifest.h3_diagnostics.path === path.join(evidenceDir, FILES.diagnostics) && manifest.preflight_transport.path === path.join(evidenceDir, FILES.transport), 'EVIDENCE_PATH');
    const h3 = parse(bound(disk, manifest.h3_diagnostics, EL.diagnosticsBytes, 0o400).data, EL.diagnosticsBytes);
    keys(h3, H3_KEYS);
    demand(h3.schema_version === 1 && h3.purpose === inputs.purpose && h3.status === 'PASS' && h3.stage === 'COMPLETE' &&
        h3.reason === 'H3_DISPATCH_COMPLETE' && h3.exit_code === 0 && h3.root_created === true && h3.run_root === config.run_root &&
        h3.worker_launches === workers.length && h3.source_manifest_sha256 === sourceHash && h3.registry_sha256 === rf.sha256 &&
        equal(h3.verification_failures, []) && array(h3.workers, 32).length === workers.length, 'H3_STATUS');
    text(h3.message, 4096, true); array(h3.durability_limitations, 64).forEach(s => text(s, 8192, true));
    const genesis = {schema_version: 1, revision: RS.REVISION, purpose: inputs.purpose, candidate: inputs.candidate, runtime,
        source_manifest_sha256: sourceHash, config: binding(cf), registry: binding(rf), run_root: config.run_root,
        receipts: receiptRows, case_inventory_sha256: inputs.case_ids_sha256, worker_count: workers.length, case_count: caseIds.length, output_count: outputs.length};
    const genesisHash = hash(canonical(genesis)), rows = []; let previous = genesisHash;
    workers.forEach((w, k) => {
        const actual = h3.workers[k]; keys(actual, ['ordinal', 'worker_id', 'result', 'dispatch', 'outputs']);
        demand(actual.ordinal === k + 1 && actual.worker_id === w.id, 'H3_WORKER_ORDER');
        const result = {schema_version: 1, purpose: inputs.purpose, worker_id: w.id, ordinal: k + 1, status: 'PASS', case_ids: w.case_ids,
            events: w.case_ids.map((id, j) => ({ordinal: j + 1, case_id: id, status: 'PASS'})), outputs: w.outputs};
        keys(actual.result, RESULT_KEYS); demand(equal(actual.result, result), 'H3_EVENTS');
        const d = actual.dispatch, processMode = d.mode === 'process';
        keys(d, processMode ? ['worker_id', 'ordinal', 'worker_source', 'runtime', 'argv', 'environment_sha256', 'mode', 'child', 'failure', 'stdin', 'observed', 'stdout', 'stderr'] :
            ['mode', 'worker_id', 'ordinal', 'worker_source', 'runtime', 'stdin', 'child', 'stdout', 'stderr']);
        demand(processMode || inputs.purpose === 'synthetic' && d.mode === 'synthetic-callback', 'SYNTHETIC_PROMOTION');
        const source = inputs.files.find(f => f.path === w.module_path);
        demand(d.worker_id === w.id && d.ordinal === k + 1 && equal(d.worker_source, source) && equal(d.runtime, runtime), 'H3_DISPATCH_IDENTITY');
        const stdin = stream(d.stdin, 32768, processMode ? ['write_completed'] : []);
        demand(stdin.equals(canonical({schema_version: 1, purpose: inputs.purpose, worker_id: w.id, ordinal: k + 1, case_ids: w.case_ids,
            outputs: w.outputs, run_root: config.run_root, candidate: inputs.candidate, source_manifest_sha256: sourceHash, registry_sha256: rf.sha256})), 'H3_STDIN');
        const stdout = stream(d.stdout, w.max_stream_bytes), stderr = stream(d.stderr, w.max_stream_bytes);
        if (processMode) {
            successChild(d.child); keys(d.observed, ['stdout', 'stderr']);
            demand(equal(d.argv, [inputs.runtime.executable_path, w.module_path, ...w.args]) && d.environment_sha256 === hash(canonical(environment(config.preflight.environment))) &&
                d.failure === null && d.stdin.write_completed === true && d.observed.stdout === stdout.length && d.observed.stderr === stderr.length, 'H3_PROCESS');
            demand(equal(parse(stdout, 65536), result), 'H3_STDOUT');
        } else demand(d.child === null && stdout.length === 0 && stderr.length === 0, 'SYNTHETIC_DIAGNOSTICS');
        const files = outputs.filter(f => w.outputs.includes(path.basename(f.path)));
        demand(equal(actual.outputs, files.map(fullOutput)), 'H3_OUTPUT_SNAPSHOTS');
        w.case_ids.forEach((id, j) => {
            const event = {ordinal: rows.length + 1, previous_sha256: previous, genesis_sha256: genesisHash, worker_ordinal: k + 1,
                worker_id: w.id, event_ordinal: j + 1, case_id: id, status: 'PASS', source: {path: source.path, bytes: source.bytes, sha256: source.sha256},
                result_sha256: hash(canonical(result)), dispatch_mode: d.mode, stdin_sha256: hash(stdin), stdout_sha256: hash(stdout), stderr_sha256: hash(stderr),
                outputs: files.map(f => ({name: path.basename(f.path), bytes: f.bytes, sha256: f.sha256}))};
            previous = hash(canonical(event)); rows.push({event, sha256: previous});
        });
    });
    demand(equal(h3.last_dispatch, h3.workers[h3.workers.length - 1].dispatch), 'H3_LAST_DISPATCH'); state.checks[2] = 'PASS';
    const expectedLedger = {schema_version: 1, revision: RS.REVISION, purpose: inputs.purpose, status: 'PASS', genesis,
        genesis_sha256: genesisHash, rows, row_count: rows.length, tail_sha256: previous};
    demand(equal(claimedLedger, expectedLedger), 'DERIVED_LEDGER'); state.checks[3] = 'PASS';
    const transport = parse(bound(disk, manifest.preflight_transport, EL.transportBytes, 0o400).data, EL.transportBytes);
    keys(transport, ['schema_version', 'purpose', 'captures']); demand(transport.schema_version === 1 && transport.purpose === inputs.purpose && array(transport.captures, 2).length === 2, 'TRANSPORT');
    let missing = false;
    transport.captures.forEach((row, i) => {
        keys(row, ['ordinal', 'receipt_id', 'expected', 'observed', 'origin']);
        const expected = sample(canonical(preflightContext(inputs, receipts[i].run_root)));
        demand(row.ordinal === i + 1 && row.receipt_id === receipts[i].receipt_id && equal(row.expected, expected), 'TRANSPORT');
        if (row.observed === null) { missing = true; demand(row.origin === 'unavailable', 'TRANSPORT_ORIGIN'); }
        else demand(row.origin === 'caller-supplied-capture-snapshot' && equal(row.observed, {...expected, write_completed: true}), 'TRANSPORT_ORIGIN');
    });
    const limitations = [...H5_LIMITATIONS];
    if (missing) limitations.push('Actual H2 preflight stdin snapshots were not supplied; expected bytes are derived, not observed transport.');
    demand(equal(manifest.limitations, limitations), 'LIMITATIONS');
    const reviewPath = path.join(evidenceDir, FILES.review); let reviewExists = false;
    try { fs.lstatSync(reviewPath); reviewExists = true; } catch (error) { if (error.code !== 'ENOENT') throw error; }
    disk.directory(evidenceDir, [path.basename(mf.path), FILES.diagnostics, FILES.transport, ...(reviewExists ? [FILES.review] : [])]);
    if (reviewExists) checkRecordedReview(disk.json(reviewPath, EL.reviewBytes, 0o400), context, expectedChecker, limitations);
    checkWorkerArtifacts(workers, inputs, outputs, disk, config, rf.sha256);
    state.checks[4] = 'PASS';
}
function preflightContext(inputs, root) {
    return {purpose: inputs.purpose, candidate: inputs.candidate, source_manifest_sha256: hash(canonical(inputs)), case_inventory_path: inputs.case_inventory_path,
        case_count: inputs.cases.length, case_ids_sha256: inputs.case_ids_sha256, runtime: inputs.runtime, run_root: root};
}
function checkerResult(context, checks) {
    return {schema_version: 1, purpose: context.purpose, status: checks.every(s => s === 'PASS') ? 'PASS' : 'FAIL',
        manifest_sha256: context.manifest.sha256, ledger_sha256: context.ledger.sha256, source_manifest_sha256: context.source_manifest_sha256,
        case_ids: context.case_ids, output_count: context.output_count, checks: CHECKS.map((id, i) => ({id, status: checks[i]}))};
}
function checkRecordedReview(review, context, checker, limitations) {
    keys(review, ES.review_keys);
    demand(review.schema_version === 1 && review.revision === RS.REVISION && review.purpose === context.purpose && review.kind === 'automated-checker-review' &&
        review.status === 'RECORDED' && review.manifest_sha256 === context.manifest.sha256 && review.ledger_sha256 === context.ledger.sha256 &&
        equal(review.checker, checker) && review.error === null && equal(review.verification_failures, []) && equal(review.limitations, limitations) &&
        review.verification_scope === 'through-checker-dispatch; final publication checks require the issued H5 result/readback', 'RECORDED_REVIEW');
    const d = review.dispatch; keys(d, ['mode', 'argv', 'cwd', 'environment_sha256', 'stdin', 'stdout', 'stderr', 'observed', 'child', 'failure', 'capture_complete']);
    successChild(d.child); keys(d.observed, ['stdout', 'stderr']);
    const stdin = stream(d.stdin, EL.contextBytes, ['write_completed']), stdout = stream(d.stdout, checker.max_stream_bytes), stderr = stream(d.stderr, checker.max_stream_bytes);
    const result = checkerResult(context, CHECKS.map(() => 'PASS'));
    demand(d.mode === 'process' && d.failure === null && d.capture_complete === true && d.stdin.write_completed === true &&
        equal(d.argv, checker.argv) && d.cwd === checker.cwd && d.environment_sha256 === checker.environment_sha256 &&
        stdin.equals(canonical(context)) && equal(d.observed, {stdout: stdout.length, stderr: stderr.length}) &&
        stdout.equals(canonical(result)) && equal(review.result, result), 'RECORDED_REVIEW');
}

const ASSERTION_UNIT = 'TAP top-level test cases, not JavaScript assertion calls';
const NORMALIZATION = Object.freeze({assertion_unit: ASSERTION_UNIT,
    included: ['suite', 'ordered case IDs', 'exact test names', 'terminal outcomes', 'test-case counts'],
    excluded: ['duration', 'timestamps', 'PID', 'run namespace', 'diagnostic text'],
    raw_evidence: 'Actual Node events and process/runtime/source/context bindings are retained separately.'});
const SUITE_FILES = [ ['test-preflight-receipt.js'], ['test-input-capture.js', 'test-preflight-capture.js'],
    ['test-formal-runner.js', 'test-refusal-inventory.js'], ['test-formal-ledger.js', 'test-p0f-evidence.js'] ];
const SUITE_PREFIXES = [['H1'], ['H2I', 'H2P'], ['H3', 'H6'], ['H4', 'H5']];
function originalRefusals() {
    const ids = [], add = (group, n, suffix = '') => ids.push(`H6-${group}-${String(n).padStart(2, '0')}${suffix}`);
    for (let n = 1; n <= 11; n++) add('CLI', n);
    add('ID', 1);
    for (const s of 'abcdef') add('ID', 2, s);
    for (const s of 'abc') add('ID', 3, s);
    for (let n = 4; n <= 11; n++) add('ID', n);
    ['ab', 'abcdefgh', 'abcd', 'abc', 'abcdef', 'ab', 'ab', 'abc', 'abc'].forEach((ss, i) => { for (const s of ss) add('TREE', i + 1, s); });
    add('RACE', 1); add('RACE', 2, 'a'); add('RACE', 2, 'b'); add('RACE', 3); add('RACE', 4);
    for (let n = 1; n <= 4; n++) add('JOIN', n);
    return ids.sort();
}
function actualPin(disk, row, inputs, allowedRoles) {
    keys(row, ['path', 'bytes', 'sha256', 'git_blob']);
    const declared = inputs.files.find(f => f.path === row.path);
    demand(declared && (!allowedRoles || declared.roles.some(r => allowedRoles.includes(r))), 'DEFINITION_CLOSURE');
    const file = disk.read(row.path, declared.roles.includes('runtime-executable') ? CAPS.runtime : CAPS.file);
    demand(equal(row, pinned(file)), 'DEFINITION_BYTES'); return file;
}
function suiteDefinitions(p, inputs, disk) {
    const declared = inputs.files.find(f => f.path === p);
    demand(declared && declared.roles.includes('config'), 'DEFINITION_CLOSURE');
    const defs = disk.json(p, CAPS.inventory);
    keys(defs, ['schema_version', 'purpose', 'kind', 'worker', 'runtime', 'cwd', 'environment', 'sources', 'inputs', 'h6', 'normalization', 'suites']);
    demand(defs.schema_version === 1 && defs.purpose === inputs.purpose && defs.kind === 'canonical-node-suites', 'DEFINITION_IDENTITY');
    demand(defs.worker.path === path.join(__dirname, 'suite-worker.js'), 'DEFINITION_WORKER'); actualPin(disk, defs.worker, inputs, ['source']);
    keys(defs.runtime, ['exec_path', 'version', 'executable']); actualPin(disk, defs.runtime.executable, inputs, ['runtime-executable']);
    demand(defs.runtime.exec_path === inputs.runtime.exec_path && defs.runtime.version === inputs.runtime.version && defs.runtime.executable.path === inputs.runtime.executable_path, 'DEFINITION_RUNTIME');
    demand(absolute(defs.cwd) === __dirname, 'DEFINITION_CWD'); disk.guard(defs.cwd);
    demand(equal(defs.environment, environment(defs.environment)) && equal(defs.normalization, NORMALIZATION), 'DEFINITION_NORMALIZATION');
    for (const [rows, roles] of [[defs.sources, ['source', 'fixture']], [defs.inputs, ['input', 'fixture']]]) {
        array(rows, 128).forEach((row, i) => { demand(!i || rows[i - 1].path < row.path, 'DEFINITION_ORDER'); actualPin(disk, row, inputs, roles); });
    }
    demand(defs.sources.some(f => equal(f, defs.worker)), 'DEFINITION_CLOSURE');
    keys(defs.h6, ['required_ids', 'original_ids', 'pending_ids']);
    for (const list of Object.values(defs.h6)) { array(list, 4096); if (list.length) sorted(list, 4096, true); }
    demand(defs.h6.pending_ids.length === 0 && defs.h6.original_ids.every(id => defs.h6.required_ids.includes(id)), 'H6_PENDING');
    if (inputs.purpose === 'validation') {
        demand(equal(defs.h6.original_ids, originalRefusals()), 'H6_ORIGINAL_IDENTITIES');
        const extra = ['H6-ID-02g', 'H6-ID-02h', 'H6-ID-03d', 'H6-ID-03e',
            ...Array.from({length: 8}, (_, i) => `H6-WORKER-${String(i + 1).padStart(2, '0')}`),
            ...Array.from({length: 4}, (_, i) => `H6-EVIDENCE-${String(i + 1).padStart(2, '0')}`),
            ...Array.from({length: 3}, (_, i) => `H6-CONTROL-${String(i + 1).padStart(2, '0')}`),
            ...Array.from({length: 3}, (_, i) => `H6-POST-${String(i + 1).padStart(2, '0')}`)];
        demand(extra.every(id => defs.h6.required_ids.includes(id)), 'H6_REQUIRED_IDENTITIES');
        absolute(defs.environment.LC_OFFSET_ROADMAP_SNAPSHOT);
        demand(defs.inputs.some(f => f.path === defs.environment.LC_OFFSET_ROADMAP_SNAPSHOT), 'ROADMAP_INPUT');
    }
    demand(array(defs.suites, 4).length === 4, 'DEFINITION_SUITES'); const allIds = [];
    defs.suites.forEach((suite, index) => {
        keys(suite, ['name', 'files', 'cases', 'outputs', 'timeout_ms', 'max_event_bytes', 'max_stderr_bytes']);
        demand(suite.name === RS.PREFLIGHT_SUITES[index], 'DEFINITION_SUITE_ORDER');
        array(suite.files, 2, true).forEach(f => { actualPin(disk, f, inputs, ['source', 'fixture']); demand(defs.sources.some(s => equal(s, f)), 'DEFINITION_CLOSURE'); });
        demand(new Set(suite.files.map(f => f.path)).size === suite.files.length, 'DEFINITION_DUPLICATE');
        if (inputs.purpose === 'validation') demand(equal(suite.files.map(f => f.path), SUITE_FILES[index].map(n => path.join(__dirname, n))), 'VALIDATION_TEST_SUBSTITUTION');
        const ordinals = new Map(); let previousFile = -1;
        array(suite.cases, 128, true).forEach(c => {
            keys(c, ['id', 'file', 'name', 'declaration_ordinal']); text(c.name, 8192); sorted([c.id], 1, true);
            const fileIndex = suite.files.findIndex(f => f.path === c.file), ordinal = (ordinals.get(c.file) || 0) + 1;
            demand(fileIndex >= 0 && fileIndex >= previousFile && c.declaration_ordinal === ordinal, 'DEFINITION_CASE_ORDER');
            const prefix = SUITE_PREFIXES[index][fileIndex], h6Name = c.name.match(/^(H6-[A-Za-z0-9.-]+): /);
            const id = prefix === 'H6' ? h6Name ? h6Name[1] : 'H6-000-inventory' : `${prefix}-${String(ordinal).padStart(3, '0')}`;
            demand(prefix && c.id === id, 'DERIVED_CASE_ID');
            previousFile = fileIndex; ordinals.set(c.file, ordinal); allIds.push(c.id);
        });
        sorted(suite.cases.map(c => c.id), 128, true);
        demand(equal(suite.outputs, [`${suite.name}.diagnostics.json`, `${suite.name}.events.jsonl`, `${suite.name}.report.json`]), 'DEFINITION_OUTPUTS');
        uint(suite.timeout_ms, 300000, true); uint(suite.max_event_bytes, 4194304, true); uint(suite.max_stderr_bytes, 1048576, true);
    });
    demand(new Set(allIds).size === allIds.length && equal([...allIds].sort(), inputs.cases.map(c => c.id)), 'DEFINITION_CASE_INVENTORY');
    const h6 = allIds.filter(id => id.startsWith('H6-') && id !== 'H6-000-inventory').sort();
    demand(equal(h6, defs.h6.required_ids), 'H6_CASE_INVENTORY'); return defs;
}
function bytesSnapshot(bytes) { return {bytes: bytes.length, sha256: hash(bytes), base64: bytes.toString('base64')}; }
function byteSnapshot(row, max) {
    keys(row, ['bytes', 'sha256', 'base64']);
    return stream({base64: row.base64, bytes: row.bytes, sha256: row.sha256}, max);
}
function suiteSemantics(defs, suite, diagnostic, reportFile, eventsFile, inputs, worker = null) {
    const baseKeys = ['schema_version', 'kind', 'purpose', 'suite', 'runtime', 'worker', 'sources', 'inputs', 'normalization', 'semantic', 'verification_error', 'capture'];
    keys(diagnostic, [...baseKeys, ...(worker ? ['definitions', 'context', 'stdin'] : [])]);
    demand(diagnostic.schema_version === 1 && diagnostic.kind === 'node-suite-diagnostics' && diagnostic.purpose === inputs.purpose && diagnostic.suite === suite.name &&
        equal(diagnostic.runtime, defs.runtime) && equal(diagnostic.worker, defs.worker) && equal(diagnostic.sources, defs.sources) &&
        equal(diagnostic.inputs, defs.inputs) && equal(diagnostic.normalization, NORMALIZATION) && diagnostic.verification_error === null, 'SUITE_DIAGNOSTICS');
    const capture = diagnostic.capture;
    keys(capture, ['mode', 'argv', 'cwd', 'environment', 'environment_sha256', 'child', 'failure', 'stdout', 'stderr']);
    successChild(capture.child);
    demand(capture.mode === 'execution' && capture.failure === null && capture.cwd === defs.cwd && equal(capture.environment, defs.environment) &&
        capture.environment_sha256 === hash(canonical(defs.environment)) && equal(capture.argv,
        [inputs.runtime.executable_path, '--test', '--test-concurrency=1', `--test-reporter=${defs.worker.path}`, ...suite.files.map(f => f.path)]), 'SUITE_PROCESS');
    const decode = (row, cap) => {
        const bytes = stream(row, cap, ['observed_bytes', 'truncated']);
        demand(row.truncated === false && row.observed_bytes === bytes.length, 'SUITE_TRUNCATED'); return bytes;
    };
    const raw = decode(capture.stdout, suite.max_event_bytes); decode(capture.stderr, suite.max_stderr_bytes);
    demand(raw.equals(eventsFile.data) && raw.length > 0 && raw[raw.length - 1] === 10, 'SUITE_RAW_EVENTS');
    const lines = raw.toString('utf8').split('\n'); lines.pop(); demand(lines.length <= CAPS.events, 'EVENT_COUNT');
    const terminals = [], ordinals = new Map(), summaries = [];
    for (const [index, line] of lines.entries()) {
        const event = parse(Buffer.from(`${line}\n`), 4194304); keys(event, ['type', 'data']); text(event.type, 128);
        demand(event.data && typeof event.data === 'object' && !Array.isArray(event.data), 'NODE_EVENT');
        demand(event.type !== 'test:fail', 'NODE_TEST_FAILED');
        if (event.type === 'test:summary' && event.data.file === null) summaries.push({index, data: event.data});
        if (event.type !== 'test:pass') continue;
        const d = event.data;
        const inactive = key => Object.hasOwn(d, key) && d[key] !== false && d[key] !== null;
        demand(!inactive('skip') && !inactive('todo'), 'NODE_TEST_SKIPPED');
        if (d.details && d.details.type === 'test' && d.nesting === 0) {
            demand(suite.files.some(f => f.path === d.entryFile), 'NODE_UNEXPECTED_TOP_LEVEL');
            const n = (ordinals.get(d.entryFile) || 0) + 1; ordinals.set(d.entryFile, n);
            terminals.push({file: d.entryFile, name: d.name, declaration_ordinal: n});
        }
    }
    // A completed testcase prefix is not a completed Node execution. File-local
    // summaries do not replace the single final process-wide completion record.
    demand(summaries.length === 1, 'NODE_SUMMARY_COUNT');
    const summary = summaries[0]; demand(summary.index === lines.length - 1, 'NODE_SUMMARY_FINAL');
    keys(summary.data, ['success', 'counts', 'duration_ms', 'file']);
    keys(summary.data.counts, ['tests', 'failed', 'passed', 'cancelled', 'skipped', 'todo', 'topLevel', 'suites']);
    for (const n of Object.values(summary.data.counts)) uint(n, CAPS.events);
    demand(summary.data.success === true && ['failed', 'cancelled', 'skipped', 'todo'].every(k => summary.data.counts[k] === 0) &&
        summary.data.counts.tests > 0 && summary.data.counts.passed === summary.data.counts.tests &&
        summary.data.counts.tests >= terminals.length, 'NODE_SUMMARY_STATUS');
    demand(terminals.length === suite.cases.length, 'NODE_TEST_COUNT');
    suite.cases.forEach((c, i) => demand(equal(terminals[i], {file: c.file, name: c.name, declaration_ordinal: c.declaration_ordinal}), 'NODE_TEST_IDENTITY'));
    const expected = {schema_version: 1, kind: 'node-suite-semantics', purpose: inputs.purpose, suite: suite.name, status: 'PASS',
        assertion_unit: ASSERTION_UNIT, assertions: terminals.length, case_ids: suite.cases.map(c => c.id),
        cases: suite.cases.map(c => ({id: c.id, name: c.name, outcome: 'PASS'})), reason: null};
    demand(reportFile.data.equals(canonical(expected)), 'DERIVED_SUITE_REPORT');
    keys(diagnostic.semantic, ['bytes', 'sha256']); demand(equal(diagnostic.semantic, {bytes: reportFile.bytes, sha256: reportFile.sha256}), 'SUITE_REPORT_BINDING');
    if (worker) {
        demand(equal(diagnostic.definitions, worker.definitions) && equal(diagnostic.context, worker.context) &&
            byteSnapshot(diagnostic.stdin, 32768).equals(canonical(worker.context)), 'SUITE_WORKER_CONTEXT');
    }
    return expected;
}
function checkPreflightArtifacts(config, inputs, receipts, receiptRows, disk, protectedDirectories) {
    const knownDirectories = [], reserved = [...inputs.files.map(f => f.path), config.run_root,
        ...config.receipt_roots, ...receiptRows.map(r => path.dirname(r.report.path)), ...protectedDirectories];
    for (const [i, receipt] of receipts.entries()) {
        const stderr = disk.files.get(receiptRows[i].stderr.path).data;
        const legacySynthetic = inputs.purpose === 'synthetic' && config.preflight.runnerPath !== path.join(__dirname, 'genuine-preflight.js');
        let envelope;
        try { envelope = parse(stderr, RS.MAX_STREAM_BYTES); }
        catch (error) { if (legacySynthetic) continue; throw error; }
        if (!envelope || envelope.kind !== 'genuine-preflight-evidence') { demand(legacySynthetic, 'GENUINE_PREFLIGHT_REQUIRED'); continue; }
        keys(envelope, ['schema_version', 'purpose', 'kind', 'context_sha256', 'artifacts', 'limitations']);
        const context = preflightContext(inputs, receipt.run_root);
        demand(envelope.schema_version === 1 && envelope.purpose === inputs.purpose && envelope.context_sha256 === hash(canonical(context)), 'GENUINE_CONTEXT');
        demand(config.preflight.runnerPath === path.join(__dirname, 'genuine-preflight.js') && config.preflight.runnerArgs.length === 1, 'GENUINE_RUNNER');
        const qp = config.preflight.runnerArgs[0];
        demand(inputs.files.some(f => f.path === qp && f.roles.includes('config')), 'QUALIFICATION_CONFIG_CLOSURE');
        const q = disk.json(qp, 65536);
        keys(q, ['schema_version', 'purpose', 'formal_config_path', 'suite_definitions_path', 'base_path', 'product_path', 'candidate_path', 'diagnostic_parent']);
        demand(q.schema_version === 1 && q.purpose === inputs.purpose && q.candidate_path === inputs.candidate.path, 'QUALIFICATION_CONFIG');
        for (const p of [q.formal_config_path, q.suite_definitions_path, q.base_path, q.product_path, q.candidate_path]) demand(inputs.files.some(f => f.path === p && f.bytes > 0), 'GENUINE_SOURCE_CLOSURE');
        demand(equal(disk.json(q.formal_config_path, 65536), config), 'QUALIFICATION_FORMAL_CONFIG');
        for (const name of ['genuine-preflight.js', 'trace-probe.js', 'suite-inventory.js', 'suite-worker.js', 'independent-evidence-checker.js'])
            demand(inputs.files.some(f => f.path === path.join(__dirname, name) && f.roles.includes('source')), 'GENUINE_SOURCE_CLOSURE');
        const directory = path.join(absolute(q.diagnostic_parent), hash(canonical(receipt.run_root)).slice(0, 32));
        for (const p of [...reserved, ...knownDirectories]) disjoint(directory, p); knownDirectories.push(directory);
        const roles = ['input-audit', 'patch-replays', 'syntax'];
        for (const suite of RS.PREFLIGHT_SUITES) for (const n of [1, 2]) for (const type of ['report', 'diagnostics', 'events']) roles.push(`suite:${suite}:attempt:${n}:${type}`);
        roles.push('trace-probe');
        demand(array(envelope.artifacts, 64).length === roles.length, 'GENUINE_ARTIFACT_COUNT');
        const artifacts = new Map(), names = [], ids = new Set();
        envelope.artifacts.forEach((a, n) => {
            keys(a, ['role', 'path', 'bytes', 'sha256']); demand(a.role === roles[n] && path.dirname(a.path) === directory, 'GENUINE_ARTIFACT_ROLE');
            const f = bound(disk, {path: a.path, bytes: a.bytes, sha256: a.sha256}, CAPS.file, 0o400);
            const id = `${f.stat.dev}:${f.stat.ino}`; demand(!ids.has(id), 'GENUINE_ARTIFACT_ALIAS'); ids.add(id);
            artifacts.set(a.role, f); names.push(path.basename(a.path));
        });
        demand(new Set(names).size === names.length, 'GENUINE_ARTIFACT_DUPLICATE'); disk.directory(directory, names);
        demand(equal(envelope.limitations, ['Byte replacement replay is not a Git textual patch or proof of source semantics.',
            ASSERTION_UNIT, 'Filesystem checks observe boundaries; they do not provide containment.']), 'GENUINE_LIMITATIONS');
        const artifact = role => parse(artifacts.get(role).data, CAPS.file), result = receipt.preflight_result;
        const audit = artifact('input-audit'); keys(audit, ['schema_version', 'primary', 'independent', 'primary_sha256', 'independent_sha256']);
        const independentlyRead = reconstructInputs(config.preflight, disk);
        demand(audit.schema_version === 1 && equal(audit.primary, inputs) && equal(audit.independent, independentlyRead) &&
            audit.primary_sha256 === hash(canonical(audit.primary)) && audit.independent_sha256 === hash(canonical(independentlyRead)) &&
            equal(result.audit_parity, {status: 'PASS', primary_sha256: audit.primary_sha256, independent_sha256: audit.independent_sha256, equal: true}), 'GENUINE_AUDIT');
        const files = [q.base_path, q.product_path, q.candidate_path].map(p => disk.read(p));
        const pairs = [[0, 1], [1, 2], [2, 1], [0, 2]];
        const directions = pairs.map(([a, b], n) => ({name: RS.PATCH_DIRECTIONS[n],
            patch: {format: 'bounded-whole-file-replacement-v1', from: {bytes: files[a].bytes, sha256: files[a].sha256}, target: bytesSnapshot(files[b].data)},
            result: bytesSnapshot(files[b].data)}));
        const patches = artifact('patch-replays');
        demand(equal(patches, {schema_version: 1, format: 'bounded-whole-file-replacement-v1', base: {path: files[0].path, ...bytesSnapshot(files[0].data)},
            product: {path: files[1].path, ...bytesSnapshot(files[1].data)}, candidate: {path: files[2].path, ...bytesSnapshot(files[2].data)}, directions}), 'GENUINE_PATCH_REPLAY');
        demand(equal(result.patch_replays, {status: 'PASS', directions: directions.map(d => ({name: d.name, status: 'PASS', patch_sha256: hash(canonical(d.patch)), result_sha256: d.result.sha256}))}), 'GENUINE_PATCH_BINDING');
        const syntax = artifact('syntax'); keys(syntax, ['schema_version', 'checks']);
        const js = inputs.files.filter(f => f.path.endsWith('.js'));
        demand(syntax.schema_version === 1 && array(syntax.checks, 128).length === js.length && js.length > 0, 'GENUINE_SYNTAX_INVENTORY');
        syntax.checks.forEach((s, n) => {
            keys(s, ['path', 'exit_code', 'signal', 'error', 'stdout', 'stderr']);
            demand(s.path === js[n].path && s.exit_code === 0 && s.signal === null && s.error === null, 'GENUINE_SYNTAX_STATUS');
            byteSnapshot(s.stdout, 65536); byteSnapshot(s.stderr, 65536);
            const own = spawnSync(inputs.runtime.executable_path, ['--check', s.path], {cwd: config.preflight.cwd,
                env: environment(config.preflight.environment), timeout: 10000, maxBuffer: 65536});
            demand(own.status === 0 && own.signal === null && !own.error, 'INDEPENDENT_SYNTAX');
        });
        demand(equal(result.syntax, {status: 'PASS', checked_file_count: js.length, checked_paths_sha256: hash(canonical(js.map(f => f.path)))}), 'GENUINE_SYNTAX_BINDING');
        const defs = suiteDefinitions(q.suite_definitions_path, inputs, disk);
        demand(equal(defs.environment, environment(config.preflight.environment)), 'GENUINE_SUITE_ENVIRONMENT');
        defs.suites.forEach((suite, index) => {
            for (const ordinal of [1, 2]) {
                const prefix = `suite:${suite.name}:attempt:${ordinal}:`, reportFile = artifacts.get(`${prefix}report`);
                const derived = suiteSemantics(defs, suite, artifact(`${prefix}diagnostics`), reportFile, artifacts.get(`${prefix}events`), inputs);
                demand(equal(result.suites[index].attempts[ordinal - 1], {ordinal, status: 'PASS', report_bytes: reportFile.bytes,
                    report_sha256: reportFile.sha256, assertions: derived.assertions}), 'GENUINE_SUITE_BINDING');
            }
        });
        const trace = artifact('trace-probe'); keys(trace, ['schema_version', 'argv', 'stdin', 'stdout', 'stderr', 'exit_code', 'signal', 'error']);
        const traceInput = canonical({purpose: inputs.purpose, candidate: inputs.candidate, source_manifest_sha256: hash(canonical(inputs)), case_ids_sha256: inputs.case_ids_sha256});
        const traceOut = canonical({schema_version: 1, kind: 'stdin-trace', bytes: traceInput.length, sha256: hash(traceInput), runtime: inputs.runtime.version});
        const traceErr = Buffer.from('offset stdin trace completed\n');
        demand(trace.schema_version === 1 && trace.exit_code === 0 && trace.signal === null && trace.error === null &&
            equal(trace.argv, [inputs.runtime.executable_path, path.join(__dirname, 'trace-probe.js')]) && byteSnapshot(trace.stdin, 32768).equals(traceInput) &&
            byteSnapshot(trace.stdout, 65536).equals(traceOut) && byteSnapshot(trace.stderr, 65536).equals(traceErr), 'GENUINE_TRACE');
        demand(equal(result.trace_probe, {status: 'PASS', stdout_sha256: hash(traceOut), stderr_sha256: hash(traceErr), exit_code: 0}), 'GENUINE_TRACE_BINDING');
        const caps = [{name: 'actual-input-file-count', limit: 128, measured: inputs.files.length},
            {name: 'actual-input-total-bytes', limit: CAPS.inputTotal, measured: inputs.files.reduce((n, f) => n + f.bytes, 0)},
            {name: 'actual-case-count', limit: CAPS.cases, measured: inputs.cases.length},
            {name: 'actual-suite-execution-count', limit: 8, measured: 8},
            {name: 'largest-persisted-artifact', limit: CAPS.file, measured: Math.max(...envelope.artifacts.map(a => a.bytes))}].map(c => ({...c, passed: c.measured <= c.limit}));
        demand(equal(result.caps, {status: 'PASS', checks: caps}), 'GENUINE_CAPS');
    }
}
function checkWorkerArtifacts(workers, inputs, outputs, disk, config, registryHash) {
    for (const [index, worker] of workers.entries()) {
        if (worker.module_path !== path.join(__dirname, 'suite-worker.js')) { demand(inputs.purpose === 'synthetic', 'VALIDATION_SUITE_WORKER'); continue; }
        demand(worker.args.length === 4 && worker.args[0] === '--definitions' && worker.args[2] === '--suite', 'SUITE_WORKER_ARGUMENTS');
        const defs = suiteDefinitions(worker.args[1], inputs, disk), suite = defs.suites.find(s => s.name === worker.args[3]);
        demand(suite && worker.id === suite.name && equal(worker.case_ids, suite.cases.map(c => c.id)) && equal(worker.outputs, suite.outputs), 'SUITE_WORKER_INVENTORY');
        const files = worker.outputs.map(name => outputs.find(f => path.basename(f.path) === name));
        const context = {schema_version: 1, purpose: inputs.purpose, worker_id: worker.id, ordinal: index + 1,
            case_ids: worker.case_ids, outputs: worker.outputs, run_root: config.run_root, candidate: inputs.candidate,
            source_manifest_sha256: hash(canonical(inputs)), registry_sha256: registryHash};
        suiteSemantics(defs, suite, parse(files[0].data, CAPS.file), files[2], files[1], inputs,
            {definitions: pinned(disk.read(worker.args[1], CAPS.inventory)), context});
    }
}

function checkEvidence(contextBytes) {
    const context = parse(contextBytes, EL.contextBytes); keys(context, ES.context_keys);
    demand(context.schema_version === 1 && context.revision === RS.REVISION && RS.PURPOSES.includes(context.purpose) &&
        equal(context.required_checks, CHECKS), 'CONTEXT_IDENTITY');
    sha(context.source_manifest_sha256); sorted(context.case_ids, CAPS.cases, true); uint(context.output_count, 128, true);
    for (const b of [context.manifest, context.ledger]) { keys(b, LS.binding_keys); absolute(b.path); uint(b.bytes, EL.diagnosticsBytes, true); sha(b.sha256); }
    demand(canonical(checkerResult(context, CHECKS.map(() => 'PASS'))).length <= EL.resultBytes, 'RESULT_LIMIT');
    const disk = new Disk(), state = {checks: CHECKS.map(() => 'FAIL'), inputs: null, workers: 0, outputs: 0};
    let reason = null, readback = null;
    try { verifyEvidence(context, disk, state); }
    catch (error) { reason = error instanceof CheckError ? error.code : `IO_${error.code || 'ERROR'}`; }
    try { disk.finish(); } catch (error) { readback = error instanceof CheckError ? error.code : `IO_${error.code || 'ERROR'}`; }
    if (reason || readback) state.checks[state.checks.findIndex(s => s === 'FAIL') < 0 ? 4 : state.checks.findIndex(s => s === 'FAIL')] = 'FAIL';
    const result = checkerResult(context, state.checks);
    const report = {schema_version: 1, kind: 'independent-evidence-semantics', purpose: context.purpose, status: result.status,
        reason, readback_failure: readback, case_ids: context.case_ids, worker_count: state.workers, output_count: state.outputs,
        checks: result.checks, limitations: [...LIMITATIONS]};
    return {result, report};
}
function writeAll(fd, bytes) {
    for (let offset = 0; offset < bytes.length;) {
        const n = fs.writeSync(fd, bytes, offset, bytes.length - offset);
        demand(Number.isSafeInteger(n) && n > 0 && n <= bytes.length - offset, 'OUTPUT_WRITE'); offset += n;
    }
}
function main() {
    demand(process.argv.length === 2, 'ARGUMENTS');
    const chunks = [], buffer = Buffer.alloc(4096); let total = 0;
    for (;;) { const n = fs.readSync(0, buffer, 0, buffer.length, null); if (!n) break; total += n; demand(total <= EL.contextBytes, 'STDIN_LIMIT'); chunks.push(Buffer.from(buffer.subarray(0, n))); }
    const {result, report} = checkEvidence(Buffer.concat(chunks, total));
    writeAll(1, canonical(result)); writeAll(2, canonical(report)); process.exitCode = result.status === 'PASS' ? 0 : 1;
}
if (require.main === module) {
    try { main(); }
    catch (error) {
        writeAll(2, canonical({schema_version: 1, kind: 'independent-evidence-semantics', status: 'FAIL',
            reason: error instanceof CheckError ? error.code : `IO_${error.code || 'ERROR'}`, limitations: [...LIMITATIONS]})); process.exitCode = 1;
    }
}
module.exports = Object.freeze({deriveInputManifest, checkEvidence, LIMITATIONS});
