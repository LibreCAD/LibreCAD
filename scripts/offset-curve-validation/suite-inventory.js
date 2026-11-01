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
const input = require('./input-capture');

const SUITES = Object.freeze([
    Object.freeze({name: 'receipt-schema', files: Object.freeze(['test-preflight-receipt.js']), prefixes: Object.freeze(['H1'])}),
    Object.freeze({name: 'capture-readback', files: Object.freeze(['test-input-capture.js', 'test-preflight-capture.js']), prefixes: Object.freeze(['H2I', 'H2P'])}),
    Object.freeze({name: 'formal-runner-boundary', files: Object.freeze(['test-formal-runner.js', 'test-refusal-inventory.js']), prefixes: Object.freeze(['H3', 'H6'])}),
    Object.freeze({name: 'genesis-ledger-p0f-joins', files: Object.freeze(['test-formal-ledger.js', 'test-p0f-evidence.js']), prefixes: Object.freeze(['H4', 'H5'])})
]);
const LIMITS = Object.freeze({fileBytes: 8388608, runtimeBytes: 134217728, definitionBytes: 1048576,
    contextBytes: 32768, eventBytes: 4194304, stderrBytes: 1048576, timeoutMs: 300000,
    casesPerWorker: 128, files: 128, events: 32768, eventDepth: 32, eventNodes: 262144,
    generatorBytes: 65536, generatorDepth: 8, generatorNodes: 1024, generatorEntries: 256});
const ASSERTION_UNIT = 'TAP top-level test cases, not JavaScript assertion calls';
const NORMALIZATION = Object.freeze({assertion_unit: ASSERTION_UNIT,
    included: Object.freeze(['suite', 'ordered case IDs', 'exact test names', 'terminal outcomes', 'test-case counts']),
    excluded: Object.freeze(['duration', 'timestamps', 'PID', 'run namespace', 'diagnostic text']),
    raw_evidence: 'Actual Node events and process/runtime/source/context bindings are retained separately.'});
const WORKER_PATH = path.join(__dirname, 'suite-worker.js');
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const canonical = value => Buffer.from(`${JSON.stringify(value)}\n`);

function ensure(ok, message) { if (!ok) throw new TypeError(message); }
function equal(a, b) { return canonical(a).equals(canonical(b)); }
function exactKeys(value, keys, label) {
    ensure(value && typeof value === 'object' && !Array.isArray(value) && equal(Object.keys(value), keys), `${label} keys/order differ`);
}

function snapshotGeneratorOptions(options) {
    let nodes = 0, bytes = 0;
    const active = new WeakSet();
    const account = count => { bytes += count; ensure(bytes <= LIMITS.generatorBytes, 'generator options exceed byte cap'); };
    const textBytes = value => {
        ensure(Buffer.byteLength(value) <= LIMITS.generatorBytes, 'generator option string exceeds byte cap');
        return Buffer.byteLength(JSON.stringify(value)); // Only a primitive string, never caller objects/getters.
    };
    const copy = (value, depth) => {
        ensure(++nodes <= LIMITS.generatorNodes && depth <= LIMITS.generatorDepth, 'generator options exceed structural cap');
        if (value === null) { account(4); return null; }
        if (typeof value === 'string') { account(textBytes(value)); return value; }
        ensure(typeof value === 'object', 'generator options must contain only inert own-data strings, arrays, objects and null');
        const array = Array.isArray(value), prototype = Object.getPrototypeOf(value);
        ensure(array ? prototype === Array.prototype : prototype === Object.prototype || prototype === null,
            'generator options cannot contain custom prototypes or inherited fields');
        ensure(!active.has(value), 'generator options cannot contain cycles');
        const descriptors = Object.getOwnPropertyDescriptors(value), keys = Reflect.ownKeys(descriptors);
        ensure(keys.length <= LIMITS.generatorEntries && keys.every(key => typeof key === 'string' &&
            Object.hasOwn(descriptors[key], 'value')), 'generator options cannot contain symbols or accessors');
        active.add(value);
        account(2);
        let out;
        if (array) {
            const length = descriptors.length && descriptors.length.value;
            ensure(Number.isSafeInteger(length) && length >= 0 && length < LIMITS.generatorEntries &&
                keys.length === length + 1 && keys.every(key => key === 'length' || /^(0|[1-9][0-9]*)$/.test(key) && Number(key) < length),
            'generator option arrays must be bounded, dense and free of extra fields');
            out = [];
            for (let i = 0; i < length; i++) {
                ensure(Object.hasOwn(descriptors, String(i)), 'generator option arrays cannot contain holes');
                if (i) account(1);
                out.push(copy(descriptors[i].value, depth + 1));
            }
        } else {
            out = Object.create(null);
            for (const [i, key] of keys.entries()) {
                account(textBytes(key) + 1 + (i ? 1 : 0));
                out[key] = copy(descriptors[key].value, depth + 1);
            }
        }
        active.delete(value);
        return Object.freeze(out);
    };
    const saved = copy(options, 0);
    ensure(saved && !Array.isArray(saved) && equal(Object.keys(saved).sort(),
        ['purpose', 'definitionsPath', 'registryPath', 'inventoryPath', 'groups', 'environment'].sort()),
    'generator options must contain the six explicit keys');
    ensure(['validation', 'synthetic'].includes(saved.purpose), 'explicit generator purpose required');
    for (const key of ['definitionsPath', 'registryPath', 'inventoryPath']) input.canonicalPath(saved[key]);
    ensure(saved.groups === null || saved.purpose === 'synthetic', 'validation cannot replace canonical groups');
    if (saved.groups !== null) {
        ensure(Array.isArray(saved.groups) && saved.groups.length === SUITES.length, 'four canonical groups required');
        for (const [i, group] of saved.groups.entries()) {
            ensure(group && !Array.isArray(group) && equal(Object.keys(group).sort(), ['files', 'name']) &&
                group.name === SUITES[i].name && Array.isArray(group.files) && group.files.length > 0 && group.files.length <= SUITES[i].files.length,
            'generator groups require exact names and bounded file arrays');
            group.files.forEach(file => input.canonicalPath(file));
        }
    }
    ensure(saved.environment && !Array.isArray(saved.environment) && typeof saved.environment === 'object' &&
        Object.values(saved.environment).every(value => typeof value === 'string'), 'generator environment must be an own-data string map');
    return saved;
}

function pinFile(filePath, maxBytes = LIMITS.fileBytes) {
    input.canonicalPath(filePath);
    const guards = input.directoryGuards(path.dirname(filePath));
    const before = fs.lstatSync(filePath, {bigint: true});
    ensure(before.isFile() && before.nlink === 1n && before.size <= BigInt(maxBytes), `invalid/oversized single-link input: ${filePath}`);
    const fd = fs.openSync(filePath, fs.constants.O_RDONLY | fs.constants.O_NOFOLLOW);
    const sha = crypto.createHash('sha256'), blob = crypto.createHash('sha1');
    blob.update(`blob ${before.size}\0`);
    let bytes = 0;
    try {
        ensure(input.statIdentity(fs.fstatSync(fd, {bigint: true})) === input.statIdentity(before), `input changed while opening: ${filePath}`);
        const buffer = Buffer.alloc(65536);
        for (;;) {
            const count = fs.readSync(fd, buffer, 0, Math.min(buffer.length, maxBytes - bytes + 1), null);
            if (!count) break;
            bytes += count;
            ensure(bytes <= maxBytes, `input exceeded byte cap: ${filePath}`);
            sha.update(buffer.subarray(0, count)); blob.update(buffer.subarray(0, count));
        }
        ensure(BigInt(bytes) === before.size && input.statIdentity(fs.fstatSync(fd, {bigint: true})) === input.statIdentity(before),
            `input changed while reading: ${filePath}`);
    } finally { fs.closeSync(fd); }
    input.checkDirectoryGuards(guards);
    ensure(input.statIdentity(fs.lstatSync(filePath, {bigint: true})) === input.statIdentity(before), `input replaced: ${filePath}`);
    return {path: filePath, bytes, sha256: sha.digest('hex'), git_blob: blob.digest('hex')};
}

function runtimeIdentity() {
    return {exec_path: process.execPath, version: process.version,
        executable: pinFile(fs.realpathSync.native(process.execPath), LIMITS.runtimeBytes)};
}

function readDefinitions(filePath) {
    const saved = pinFile(filePath, LIMITS.definitionBytes);
    const read = h1.readReceiptFile(filePath, {maxBytes: LIMITS.definitionBytes,
        mode: fs.statSync(filePath).mode & 0o7777});
    const value = h1.parseCanonicalJson(Buffer.from(read.base64, 'base64'), {maxBytes: LIMITS.definitionBytes});
    ensure(read.sha256 === saved.sha256, 'suite definitions read differs from pinned bytes');
    ensure(equal(saved, pinFile(filePath, LIMITS.definitionBytes)), 'suite definitions changed during read');
    validateDefinitions(value);
    return {value, binding: saved};
}

function validateDefinitions(value) {
    exactKeys(value, ['schema_version', 'purpose', 'kind', 'worker', 'runtime', 'cwd', 'environment', 'sources', 'inputs', 'h6', 'normalization', 'suites'], 'definitions');
    ensure(value.schema_version === 1 && ['validation', 'synthetic'].includes(value.purpose) && value.kind === 'canonical-node-suites', 'invalid definitions identity');
    const binding = row => {
        exactKeys(row, ['path', 'bytes', 'sha256', 'git_blob'], 'file binding'); input.canonicalPath(row.path);
        ensure(Number.isSafeInteger(row.bytes) && row.bytes > 0 && /^[a-f0-9]{64}$/.test(row.sha256) && /^[a-f0-9]{40}$/.test(row.git_blob), 'invalid file binding');
    };
    binding(value.worker);
    ensure(value.worker.path === WORKER_PATH, 'definitions must name the actual suite worker');
    exactKeys(value.runtime, ['exec_path', 'version', 'executable'], 'runtime');
    input.canonicalPath(value.runtime.exec_path); binding(value.runtime.executable);
    input.canonicalPath(value.cwd);
    ensure(value.cwd === __dirname, 'nested Node cwd must be the declared tools directory');
    ensure(typeof value.runtime.version === 'string' && value.runtime.version.length <= 128, 'invalid runtime version');
    ensure(value.environment && typeof value.environment === 'object' && !Array.isArray(value.environment) &&
        equal(Object.keys(value.environment), Object.keys(value.environment).sort()) &&
        Object.entries(value.environment).every(([key, item]) => typeof item === 'string' && !key.includes('\0') && !item.includes('\0')) &&
        canonical(value.environment).length <= LIMITS.contextBytes, 'invalid pinned environment');
    ensure(equal(value.normalization, NORMALIZATION), 'unsupported semantic normalization');
    for (const rows of [value.sources, value.inputs]) {
        ensure(Array.isArray(rows) && rows.length <= LIMITS.files, 'invalid source/input list');
        rows.forEach(binding);
        ensure(rows.every((row, i) => !i || rows[i - 1].path < row.path), 'source/input paths must be sorted and unique');
    }
    ensure(value.sources.some(row => equal(row, value.worker)), 'worker source is not pinned');
    if (value.purpose === 'validation') {
        ensure(typeof value.environment.LC_OFFSET_ROADMAP_SNAPSHOT === 'string', 'production roadmap snapshot must be explicitly declared');
        input.canonicalPath(value.environment.LC_OFFSET_ROADMAP_SNAPSHOT);
        ensure(value.inputs.some(row => row.path === value.environment.LC_OFFSET_ROADMAP_SNAPSHOT), 'production roadmap snapshot must be a pinned input');
    }
    exactKeys(value.h6, ['required_ids', 'original_ids', 'pending_ids'], 'H6 inventory');
    for (const ids of Object.values(value.h6)) ensure(Array.isArray(ids) && ids.length <= 4096 && ids.every((id, i) =>
        typeof id === 'string' && /^H6-[A-Za-z0-9.-]+$/.test(id) && (!i || ids[i - 1] < id)), 'H6 IDs must be sorted and unique');
    ensure(value.h6.pending_ids.length === 0, 'H6 still has pending declarations; discovery is not completion');
    ensure(value.h6.original_ids.every(id => value.h6.required_ids.includes(id)), 'original H6 IDs were lost');
    ensure(Array.isArray(value.suites) && value.suites.length === SUITES.length, 'exactly four suites required');
    const allIds = new Set(), names = new Set();
    for (const [i, suite] of value.suites.entries()) {
        exactKeys(suite, ['name', 'files', 'cases', 'outputs', 'timeout_ms', 'max_event_bytes', 'max_stderr_bytes'], 'suite');
        ensure(suite.name === SUITES[i].name, 'suite names/order differ');
        ensure(Array.isArray(suite.files) && suite.files.length > 0 && suite.files.length <= 2, 'invalid suite files');
        suite.files.forEach(binding);
        ensure(new Set(suite.files.map(row => row.path)).size === suite.files.length, 'duplicate suite file');
        if (value.purpose === 'validation') ensure(equal(suite.files.map(row => row.path), SUITES[i].files.map(name => path.join(__dirname, name))), 'validation suite cannot substitute test files');
        ensure(suite.files.every(row => value.sources.some(source => equal(source, row))), 'test source is not pinned');
        ensure(Array.isArray(suite.cases) && suite.cases.length > 0 && suite.cases.length <= LIMITS.casesPerWorker, 'suite case count exceeds H3 limit');
        const ordinals = new Map();
        let previousFile = -1;
        for (const entry of suite.cases) {
            exactKeys(entry, ['id', 'file', 'name', 'declaration_ordinal'], 'case mapping');
            ensure(typeof entry.id === 'string' && /^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(entry.id) && !allIds.has(entry.id), 'invalid/duplicate case ID');
            ensure(typeof entry.name === 'string' && entry.name.length > 0 && Buffer.byteLength(entry.name) <= 8192, 'invalid test name');
            const fileIndex = suite.files.findIndex(row => row.path === entry.file);
            ensure(fileIndex >= previousFile && fileIndex >= 0, 'case module order differs'); previousFile = fileIndex;
            const next = (ordinals.get(entry.file) || 0) + 1;
            ensure(entry.declaration_ordinal === next, 'case declaration ordinal differs'); ordinals.set(entry.file, next);
            allIds.add(entry.id);
        }
        ensure(suite.cases.every((entry, j) => !j || suite.cases[j - 1].id < entry.id), 'actual test registration order must match sorted H3 case IDs');
        ensure(equal(suite.outputs, outputNames(suite.name)), 'suite output names differ');
        for (const name of suite.outputs) { ensure(!names.has(name), 'duplicate output name'); names.add(name); }
        for (const [key, max] of [['timeout_ms', LIMITS.timeoutMs], ['max_event_bytes', LIMITS.eventBytes], ['max_stderr_bytes', LIMITS.stderrBytes]])
            ensure(Number.isSafeInteger(suite[key]) && suite[key] > 0 && suite[key] <= max, `invalid ${key}`);
    }
    const h6 = value.suites.flatMap(suite => suite.cases.map(entry => entry.id)).filter(id => id.startsWith('H6-') && id !== 'H6-000-inventory').sort();
    ensure(equal(h6, value.h6.required_ids), 'discovered H6 cases differ from required inventory');
    if (value.purpose === 'validation') ensure(value.h6.original_ids.length === 71, 'all 71 original H6 IDs are required');
    return value;
}

function outputNames(name) { return [`${name}.diagnostics.json`, `${name}.events.jsonl`, `${name}.report.json`]; }

function sourceClosure(canonicalGroups) {
    if (!canonicalGroups) return ['suite-worker.js', 'suite-inventory.js', 'generate-suite-inventory.js',
        'input-capture.js', 'preflight-receipt.js', 'schema-descriptors.js'].map(name => pinFile(path.join(__dirname, name)))
        .sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
    const found = [];
    for (const directory of [__dirname, path.join(__dirname, 'fixtures')]) {
        for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
            if (entry.name.endsWith('.js')) {
                ensure(entry.isFile(), 'source closure contains a nonregular JavaScript entry'); found.push(path.join(directory, entry.name));
            }
        }
    }
    ensure(found.length <= LIMITS.files, 'source closure exceeds H2 file limit');
    return found.sort().map(file => pinFile(file));
}

async function generateSuiteInventory(options) {
    options = snapshotGeneratorOptions(options);
    ensure(['validation', 'synthetic'].includes(options.purpose), 'explicit generator purpose required');
    const paths = ['definitionsPath', 'registryPath', 'inventoryPath'].map(key => input.canonicalPath(options[key]));
    ensure(new Set(paths).size === paths.length, 'generated configuration paths must differ');
    ensure(options.groups === null || options.purpose === 'synthetic', 'validation cannot replace canonical groups');
    const groups = options.groups || SUITES.map(suite => ({name: suite.name, files: suite.files.map(file => path.join(__dirname, file))}));
    ensure(Array.isArray(groups) && groups.length === 4 && groups.every((group, i) => group.name === SUITES[i].name), 'four canonical groups required');
    const sources = sourceClosure(options.groups === null);
    for (const file of groups.flatMap(group => group.files)) if (!sources.some(row => row.path === file)) sources.push(pinFile(file));
    sources.sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
    const runtime = runtimeIdentity(), discovery = [], suites = [];
    const {discoverTests, environmentSnapshot} = require('./suite-worker');
    const environment = environmentSnapshot(options.environment);
    if (options.purpose === 'validation') input.canonicalPath(environment.LC_OFFSET_ROADMAP_SNAPSHOT);
    for (const [i, group] of groups.entries()) {
        const files = group.files.map(file => pinFile(file));
        const observed = await discoverTests(files.map(row => row.path), {environment});
        if (observed.status !== 'DISCOVERED') {
            const error = new TypeError(`discovery refused ${group.name}: ${observed.reason}`);
            error.discovery = observed;
            throw error;
        }
        discovery.push(observed);
        const cases = observed.tests.map(entry => {
            const fileIndex = files.findIndex(row => row.path === entry.file);
            const prefix = SUITES[i].prefixes[fileIndex];
            const h6 = prefix === 'H6' && entry.name.match(/^(H6-[A-Za-z0-9.-]+): /);
            ensure(prefix !== 'H6' || h6 || entry.declaration_ordinal === 1 || options.purpose === 'synthetic', 'unmapped production H6 declaration');
            const id = h6 ? h6[1] : prefix === 'H6' ? entry.declaration_ordinal === 1 ? 'H6-000-inventory' :
                `H6-SYN-${String(entry.declaration_ordinal).padStart(3, '0')}` : `${prefix}-${String(entry.declaration_ordinal).padStart(3, '0')}`;
            return {id, file: entry.file, name: entry.name, declaration_ordinal: entry.declaration_ordinal};
        });
        suites.push({name: group.name, files, cases, outputs: outputNames(group.name), timeout_ms: LIMITS.timeoutMs,
            max_event_bytes: LIMITS.eventBytes, max_stderr_bytes: LIMITS.stderrBytes});
    }
    const actualGroups = options.groups === null;
    const refusal = actualGroups ? require('./refusal-inventory') : null;
    const required = refusal ? [...refusal.IDS] : suites.flatMap(suite => suite.cases.map(row => row.id)).filter(id => id.startsWith('H6-') && id !== 'H6-000-inventory').sort();
    const inputs = actualGroups ? [pinFile(environment.LC_OFFSET_ROADMAP_SNAPSHOT || path.join(__dirname, '..', '..', 'docs', 'plan_offset_curve_2961.md'))] : [];
    const definitions = {schema_version: 1, purpose: options.purpose, kind: 'canonical-node-suites', worker: pinFile(WORKER_PATH), runtime, cwd: __dirname, environment,
        sources, inputs, h6: {required_ids: required, original_ids: refusal ? [...refusal.ORIGINAL_IDS] : [],
            pending_ids: refusal ? refusal.PENDING.map(row => row.id).sort() : []}, normalization: NORMALIZATION, suites};
    validateDefinitions(definitions);
    const registry = {schema_version: 1, purpose: options.purpose, workers: suites.map(suite => ({id: suite.name, module_path: WORKER_PATH,
        args: ['--definitions', options.definitionsPath, '--suite', suite.name], case_ids: suite.cases.map(entry => entry.id), outputs: suite.outputs,
        timeout_ms: LIMITS.timeoutMs, max_stream_bytes: LIMITS.stderrBytes}))};
    const inventory = {schema_version: 1, purpose: options.purpose,
        cases: suites.flatMap(suite => suite.cases.map(entry => ({id: entry.id, source_path: WORKER_PATH}))).sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0)};
    for (const row of [...sources, ...inputs]) ensure(equal(row, pinFile(row.path)), `source/input changed during discovery: ${row.path}`);
    ensure(equal(runtime, runtimeIdentity()), 'runtime changed during discovery');
    const capture = {sourcePaths: sources.map(row => row.path), fixturePaths: [], inputPaths: inputs.map(row => row.path), configPaths: paths};
    ensure(capture.sourcePaths.length + capture.inputPaths.length + paths.length + 2 <= LIMITS.files, 'generated closure exceeds H2 file limit');
    return {definitions, registry, inventory, capture, discovery};
}

module.exports = Object.freeze({SUITES, LIMITS, NORMALIZATION, ASSERTION_UNIT, WORKER_PATH, hash, canonical, ensure,
    equal, exactKeys, snapshotGeneratorOptions, pinFile, runtimeIdentity, readDefinitions, validateDefinitions, outputNames, generateSuiteInventory});
