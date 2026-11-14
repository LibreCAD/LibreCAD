/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const {TextDecoder} = require('node:util');

const {
    REVISION,
    SCHEMA_VERSION,
    PURPOSES,
    MAX_JSON_DEPTH,
    MAX_REPORT_BYTES,
    MAX_STREAM_BYTES,
    RECEIPT_DIRECTORY_MODE,
    RECEIPT_FILE_MODE,
    PREFLIGHT_SUITES,
    PATCH_DIRECTIONS,
    RECEIPT_KEYS,
    CANDIDATE_KEYS,
    CHILD_KEYS,
    ERROR_KEYS,
    STREAM_KEYS,
    ROOT_PROOF_KEYS,
    PARENT_KEYS,
    RECEIPT_SUMMARY_KEYS,
    PREFLIGHT_RESULT_KEYS,
    INVENTORY_KEYS,
    PATCH_REPLAYS_KEYS,
    PATCH_DIRECTION_KEYS,
    AUDIT_PARITY_KEYS,
    SUITE_PARITY_KEYS,
    SUITE_KEYS,
    SUITE_ATTEMPT_KEYS,
    SYNTAX_KEYS,
    CAPS_KEYS,
    CAP_CHECK_KEYS,
    TRACE_KEYS,
    RECEIPT_SCHEMA,
    PREFLIGHT_RESULT_SCHEMA,
} = require('./schema-descriptors');

const nodeFs = Object.freeze({
    constants: fs.constants,
    lstatSync: (p) => fs.lstatSync(p, {bigint: true}),
    openSync: (...args) => fs.openSync(...args),
    fstatSync: (fd) => fs.fstatSync(fd, {bigint: true}),
    readSync: (...args) => fs.readSync(...args),
    closeSync: (...args) => fs.closeSync(...args),
    realpathSync: (p) => fs.realpathSync.native(p),
    opendirSync: (p) => fs.opendirSync(p, {bufferSize: 1})
});

function deepFreeze(value) {
    if (value && typeof value === 'object' && !Object.isFrozen(value)) {
        for (const key of Object.keys(value)) deepFreeze(value[key]);
        Object.freeze(value);
    }
    return value;
}

function assert(condition, message) {
    if (!condition) throw new TypeError(message);
}

function isRecord(value) {
    return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function exactKeys(value, expected, label) {
    assert(isRecord(value), `${label} must be an object`);
    const actual = Object.keys(value);
    assert(actual.length === expected.length && actual.every((key, i) => key === expected[i]),
        `${label} keys/order mismatch`);
}

function canonicalJson(value) {
    let serialized;
    try {
        serialized = JSON.stringify(value);
    } catch (error) {
        throw new TypeError(`cannot serialize canonical JSON: ${error.message}`);
    }
    assert(typeof serialized === 'string', 'canonical JSON value is not serializable');
    return Buffer.from(`${serialized}\n`, 'utf8');
}

function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

function validSha(value, label) {
    assert(typeof value === 'string' && /^[0-9a-f]{64}$/.test(value), `${label} must be lowercase SHA-256`);
}

function nonnegativeInteger(value, label) {
    assert(Number.isSafeInteger(value) && value >= 0, `${label} must be a nonnegative safe integer`);
}

function stringValue(value, label, {nonempty = true, maxBytes = 4096} = {}) {
    assert(typeof value === 'string', `${label} must be a string`);
    assert(!nonempty || value.length > 0, `${label} must be nonempty`);
    assert(!value.includes('\0'), `${label} contains NUL`);
    assert(Buffer.byteLength(value, 'utf8') <= maxBytes, `${label} is too long`);
    assert(Buffer.from(value, 'utf8').toString('utf8') === value, `${label} must contain well-formed Unicode`);
}

function parseJsonRejectDuplicates(source) {
    let i = 0;
    const failAt = message => { throw new TypeError(`${message} at JSON byte ${i}`); };
    const whitespace = () => { while (source[i] === ' ' || source[i] === '\t' || source[i] === '\n') i++; };

    function parseString() {
        const start = i;
        if (source[i++] !== '"') failAt('expected JSON string');
        while (i < source.length) {
            const code = source.charCodeAt(i);
            if (code === 0x22) {
                i++;
                try { return JSON.parse(source.slice(start, i)); }
                catch { failAt('invalid JSON string'); }
            }
            if (code < 0x20) failAt('unescaped control in JSON string');
            if (code === 0x5c) {
                i++;
                const escape = source[i++];
                if (escape === 'u') {
                    if (!/^[0-9a-fA-F]{4}$/.test(source.slice(i, i + 4))) failAt('invalid unicode escape');
                    i += 4;
                } else if (!'"\\/bfnrt'.includes(escape)) failAt('invalid JSON escape');
            } else i++;
        }
        failAt('unterminated JSON string');
    }

    function parseValue(depth = 0) {
        whitespace();
        const char = source[i];
        if ((char === '{' || char === '[') && depth >= MAX_JSON_DEPTH)
            failAt('JSON nesting limit exceeded');
        if (char === '"') return parseString();
        if (char === '{') {
            i++;
            whitespace();
            const object = {};
            const keys = new Set();
            if (source[i] === '}') { i++; return object; }
            for (;;) {
                whitespace();
                if (source[i] !== '"') failAt('object key must be a string');
                const key = parseString();
                if (keys.has(key)) failAt(`duplicate object key ${JSON.stringify(key)}`);
                keys.add(key);
                whitespace();
                if (source[i++] !== ':') failAt('expected colon');
                const value = parseValue(depth + 1);
                Object.defineProperty(object, key, {value, enumerable: true, configurable: true, writable: true});
                whitespace();
                if (source[i] === '}') { i++; return object; }
                if (source[i++] !== ',') failAt('expected comma');
            }
        }
        if (char === '[') {
            i++;
            whitespace();
            const array = [];
            if (source[i] === ']') { i++; return array; }
            for (;;) {
                array.push(parseValue(depth + 1));
                whitespace();
                if (source[i] === ']') { i++; return array; }
                if (source[i++] !== ',') failAt('expected comma');
            }
        }
        for (const [literal, value] of [['true', true], ['false', false], ['null', null]]) {
            if (source.startsWith(literal, i)) { i += literal.length; return value; }
        }
        const number = source.slice(i).match(/^-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?/);
        if (number) {
            i += number[0].length;
            const value = Number(number[0]);
            if (!Number.isFinite(value)) failAt('non-finite JSON number');
            return value;
        }
        failAt('invalid JSON value');
    }

    const result = parseValue();
    whitespace();
    if (i !== source.length) failAt('trailing JSON data');
    return result;
}

function parseCanonicalJson(bytes, {maxBytes = MAX_REPORT_BYTES, label = 'JSON'} = {}) {
    assert(Buffer.isBuffer(bytes), `${label} must be a Buffer`);
    assert(bytes.length <= maxBytes, `${label} exceeds ${maxBytes} bytes`);
    assert(bytes.length > 0 && bytes[bytes.length - 1] === 0x0a, `${label} must end in one LF`);
    assert(bytes.length < 2 || bytes[bytes.length - 2] !== 0x0a, `${label} has an extra LF`);
    assert(!bytes.includes(0x0d), `${label} contains CR`);
    assert(!(bytes.length >= 3 && bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf), `${label} contains BOM`);
    let decoded;
    try { decoded = new TextDecoder('utf-8', {fatal: true}).decode(bytes.subarray(0, -1)); }
    catch { throw new TypeError(`${label} is not strict UTF-8`); }
    const value = parseJsonRejectDuplicates(decoded);
    assert(canonicalJson(value).equals(bytes), `${label} is not canonical JSON+LF`);
    return value;
}

function canonicalAbsolute(value, label) {
    stringValue(value, label);
    assert(path.isAbsolute(value), `${label} must be absolute`);
    assert(path.normalize(value) === value && path.resolve(value) === value, `${label} is not canonical`);
    return value;
}

function safeError(value, label) {
    if (value === null) return;
    exactKeys(value, ERROR_KEYS, label);
    stringValue(value.code, `${label}.code`, {maxBytes: 256});
    stringValue(value.message, `${label}.message`, {nonempty: false, maxBytes: 2048});
}

function validateCandidate(value, label = 'candidate') {
    exactKeys(value, CANDIDATE_KEYS, label);
    canonicalAbsolute(value.path, `${label}.path`);
    nonnegativeInteger(value.bytes, `${label}.bytes`);
    validSha(value.sha256, `${label}.sha256`);
    assert(typeof value.git_blob === 'string' && /^[0-9a-f]{40}$/.test(value.git_blob), `${label}.git_blob must be a Git SHA-1 blob id`);
}

function validateRootProof(value, label) {
    exactKeys(value, ROOT_PROOF_KEYS, label);
    canonicalAbsolute(value.path, `${label}.path`);
    for (const key of ['absent_before', 'absent_after', 'parent_stable'])
        assert(typeof value[key] === 'boolean', `${label}.${key} must be boolean`);
    exactKeys(value.parent, PARENT_KEYS, `${label}.parent`);
    canonicalAbsolute(value.parent.path, `${label}.parent.path`);
    assert(path.dirname(value.path) === value.parent.path, `${label}.parent.path is not the run-root parent`);
    for (const key of ['device', 'inode'])
        assert(typeof value.parent[key] === 'string' && /^(0|[1-9][0-9]*)$/.test(value.parent[key]), `${label}.parent.${key} must be an unsigned decimal string`);
    nonnegativeInteger(value.write_attempts, `${label}.write_attempts`);
    safeError(value.error, `${label}.error`);
    if (value.status !== undefined) assert(value.status === 'PASS' || value.status === 'FAIL', `${label}.status is invalid`);
}

function statusValue(value, label) {
    assert(value === 'PASS' || value === 'FAIL', `${label} must be PASS or FAIL`);
}

function validateReceiptSummary(summary, expectedOrdinal) {
    exactKeys(summary, RECEIPT_SUMMARY_KEYS, 'receipt summary');
    assert(summary.ordinal === expectedOrdinal, 'receipt summary ordinal mismatch');
    assert(typeof summary.receipt_id === 'string' && /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(summary.receipt_id), 'receipt summary ID invalid');
    for (const key of ['receipt_directory', 'report_path', 'stdout_path', 'stderr_path', 'run_root_path', 'run_root_parent_path'])
        canonicalAbsolute(summary[key], `receipt summary.${key}`);
    for (const key of ['receipt_directory_mode', 'report_mode', 'stdout_mode', 'stderr_mode']) nonnegativeInteger(summary[key], `receipt summary.${key}`);
    assert(summary.receipt_directory_mode === RECEIPT_DIRECTORY_MODE, 'receipt summary directory mode mismatch');
    for (const key of ['report_mode', 'stdout_mode', 'stderr_mode'])
        assert(summary[key] === RECEIPT_FILE_MODE, `receipt summary ${key} mismatch`);
    for (const key of ['report_bytes', 'stdout_bytes', 'stderr_bytes', 'root_write_attempts']) nonnegativeInteger(summary[key], `receipt summary.${key}`);
    assert(summary.report_bytes <= MAX_REPORT_BYTES && summary.stdout_bytes <= MAX_STREAM_BYTES && summary.stderr_bytes <= MAX_STREAM_BYTES,
        'receipt summary byte cap exceeded');
    for (const key of ['report_sha256', 'stdout_sha256', 'stderr_sha256']) validSha(summary[key], `receipt summary.${key}`);
    for (const key of ['root_absent_before', 'root_absent_after']) assert(typeof summary[key] === 'boolean', `receipt summary.${key} must be boolean`);
    for (const key of ['run_root_parent_device', 'run_root_parent_inode'])
        assert(typeof summary[key] === 'string' && /^(0|[1-9][0-9]*)$/.test(summary[key]), `receipt summary.${key} invalid`);
}

function stringArray(value, label) {
    assert(Array.isArray(value), `${label} must be an array`);
    value.forEach((item, i) => stringValue(item, `${label}[${i}]`, {maxBytes: 1024}));
    assert(new Set(value).size === value.length, `${label} contains duplicates`);
}

function validatePreflightResult(value) {
    exactKeys(value, PREFLIGHT_RESULT_KEYS, 'preflight_result');
    assert(value.revision === REVISION, 'preflight_result.revision mismatch');
    assert(PURPOSES.includes(value.purpose), 'preflight_result.purpose is invalid');
    statusValue(value.status, 'preflight_result.status');
    validateCandidate(value.candidate, 'preflight_result.candidate');
    validSha(value.source_manifest_sha256, 'preflight_result.source_manifest_sha256');

    exactKeys(value.inventory_gate, INVENTORY_KEYS, 'inventory_gate');
    statusValue(value.inventory_gate.status, 'inventory_gate.status');
    for (const key of ['expected_count', 'actual_count']) nonnegativeInteger(value.inventory_gate[key], `inventory_gate.${key}`);
    for (const key of ['expected_ids_sha256', 'actual_ids_sha256']) validSha(value.inventory_gate[key], `inventory_gate.${key}`);
    for (const key of ['missing_ids', 'extra_ids', 'duplicate_ids']) stringArray(value.inventory_gate[key], `inventory_gate.${key}`);

    exactKeys(value.patch_replays, PATCH_REPLAYS_KEYS, 'patch_replays');
    statusValue(value.patch_replays.status, 'patch_replays.status');
    assert(Array.isArray(value.patch_replays.directions) && value.patch_replays.directions.length === PATCH_DIRECTIONS.length, 'patch_replays.directions count mismatch');
    value.patch_replays.directions.forEach((row, i) => {
        exactKeys(row, PATCH_DIRECTION_KEYS, `patch_replays.directions[${i}]`);
        assert(row.name === PATCH_DIRECTIONS[i], `patch_replays.directions[${i}].name/order mismatch`);
        statusValue(row.status, `patch_replays.directions[${i}].status`);
        validSha(row.patch_sha256, `patch_replays.directions[${i}].patch_sha256`);
        validSha(row.result_sha256, `patch_replays.directions[${i}].result_sha256`);
    });

    exactKeys(value.audit_parity, AUDIT_PARITY_KEYS, 'audit_parity');
    statusValue(value.audit_parity.status, 'audit_parity.status');
    validSha(value.audit_parity.primary_sha256, 'audit_parity.primary_sha256');
    validSha(value.audit_parity.independent_sha256, 'audit_parity.independent_sha256');
    assert(typeof value.audit_parity.equal === 'boolean', 'audit_parity.equal must be boolean');

    exactKeys(value.suite_parity, SUITE_PARITY_KEYS, 'suite_parity');
    statusValue(value.suite_parity.status, 'suite_parity.status');
    assert(value.suite_parity.suite_count === PREFLIGHT_SUITES.length, 'suite_parity.suite_count mismatch');
    assert(typeof value.suite_parity.identical_repeated_outputs === 'boolean', 'suite_parity.identical_repeated_outputs must be boolean');
    assert(Array.isArray(value.suites) && value.suites.length === PREFLIGHT_SUITES.length, 'suites count mismatch');
    value.suites.forEach((suite, i) => {
        exactKeys(suite, SUITE_KEYS, `suites[${i}]`);
        assert(suite.name === PREFLIGHT_SUITES[i], `suites[${i}].name/order mismatch`);
        statusValue(suite.status, `suites[${i}].status`);
        assert(Array.isArray(suite.attempts) && suite.attempts.length === 2, `suites[${i}].attempts count mismatch`);
        suite.attempts.forEach((attempt, j) => {
            exactKeys(attempt, SUITE_ATTEMPT_KEYS, `suites[${i}].attempts[${j}]`);
            assert(attempt.ordinal === j + 1, `suites[${i}].attempts[${j}].ordinal mismatch`);
            statusValue(attempt.status, `suites[${i}].attempts[${j}].status`);
            nonnegativeInteger(attempt.report_bytes, `suites[${i}].attempts[${j}].report_bytes`);
            validSha(attempt.report_sha256, `suites[${i}].attempts[${j}].report_sha256`);
            nonnegativeInteger(attempt.assertions, `suites[${i}].attempts[${j}].assertions`);
        });
        assert(suite.attempts[0].report_bytes === suite.attempts[1].report_bytes &&
            suite.attempts[0].report_sha256 === suite.attempts[1].report_sha256 &&
            suite.attempts[0].assertions === suite.attempts[1].assertions,
        `suites[${i}] repeated outputs differ`);
    });

    exactKeys(value.syntax, SYNTAX_KEYS, 'syntax');
    statusValue(value.syntax.status, 'syntax.status');
    nonnegativeInteger(value.syntax.checked_file_count, 'syntax.checked_file_count');
    validSha(value.syntax.checked_paths_sha256, 'syntax.checked_paths_sha256');

    exactKeys(value.caps, CAPS_KEYS, 'caps');
    statusValue(value.caps.status, 'caps.status');
    assert(Array.isArray(value.caps.checks) && value.caps.checks.length > 0, 'caps.checks must be nonempty');
    value.caps.checks.forEach((check, i) => {
        exactKeys(check, CAP_CHECK_KEYS, `caps.checks[${i}]`);
        stringValue(check.name, `caps.checks[${i}].name`, {maxBytes: 256});
        nonnegativeInteger(check.limit, `caps.checks[${i}].limit`);
        nonnegativeInteger(check.measured, `caps.checks[${i}].measured`);
        assert(typeof check.passed === 'boolean', `caps.checks[${i}].passed must be boolean`);
        assert(check.passed === (check.measured <= check.limit), `caps.checks[${i}] pass flag contradicts its measurement`);
    });

    exactKeys(value.trace_probe, TRACE_KEYS, 'trace_probe');
    statusValue(value.trace_probe.status, 'trace_probe.status');
    validSha(value.trace_probe.stdout_sha256, 'trace_probe.stdout_sha256');
    validSha(value.trace_probe.stderr_sha256, 'trace_probe.stderr_sha256');
    assert(Number.isSafeInteger(value.trace_probe.exit_code), 'trace_probe.exit_code must be a safe integer');
    validateRootProof(value.run_root, 'preflight_result.run_root');

    if (value.status === 'PASS') {
        assert(value.inventory_gate.status === 'PASS' && value.inventory_gate.expected_count === value.inventory_gate.actual_count, 'PASS preflight inventory mismatch');
        assert(value.inventory_gate.expected_ids_sha256 === value.inventory_gate.actual_ids_sha256, 'PASS preflight inventory hashes differ');
        assert(value.inventory_gate.missing_ids.length === 0 && value.inventory_gate.extra_ids.length === 0 && value.inventory_gate.duplicate_ids.length === 0, 'PASS preflight inventory has gaps');
        assert(value.patch_replays.status === 'PASS' && value.patch_replays.directions.every(x => x.status === 'PASS'), 'PASS preflight replay failure');
        assert(value.audit_parity.status === 'PASS' && value.audit_parity.equal && value.audit_parity.primary_sha256 === value.audit_parity.independent_sha256, 'PASS preflight checker disagreement');
        assert(value.suite_parity.status === 'PASS' && value.suite_parity.identical_repeated_outputs, 'PASS preflight suite parity failure');
        assert(value.suites.every(s => s.status === 'PASS' && s.attempts.every(a => a.status === 'PASS' && a.assertions > 0 && a.report_bytes > 0)), 'PASS preflight suite failed or had no assertions');
        assert(value.syntax.status === 'PASS' && value.syntax.checked_file_count > 0, 'PASS preflight syntax inventory empty');
        assert(value.caps.status === 'PASS' && value.caps.checks.every(x => x.passed && x.measured <= x.limit), 'PASS preflight cap failure');
        assert(value.trace_probe.status === 'PASS' && value.trace_probe.exit_code === 0, 'PASS preflight trace probe failure');
        assert(value.run_root.absent_before && value.run_root.absent_after && value.run_root.parent_stable && value.run_root.write_attempts === 0 && value.run_root.error === null, 'PASS preflight root proof failed');
    }
}

function validateReceiptShape(receipt, expected = {}) {
    exactKeys(receipt, RECEIPT_KEYS, 'receipt');
    assert(receipt.schema_version === SCHEMA_VERSION && receipt.revision === REVISION, 'receipt schema/revision mismatch');
    assert(typeof receipt.receipt_id === 'string' && /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(receipt.receipt_id), 'receipt_id must be a canonical UUIDv4');
    assert(PURPOSES.includes(receipt.purpose), 'receipt.purpose is invalid');
    validateCandidate(receipt.candidate);
    validSha(receipt.source_manifest_sha256, 'source_manifest_sha256');
    assert(Array.isArray(receipt.argv) && receipt.argv.length >= 1, 'argv must be a nonempty array');
    receipt.argv.forEach((arg, i) => stringValue(arg, `argv[${i}]`, {maxBytes: 8192}));
    canonicalAbsolute(receipt.cwd, 'cwd');
    validSha(receipt.environment_sha256, 'environment_sha256');

    exactKeys(receipt.child, CHILD_KEYS, 'child');
    assert(receipt.child.exit_code === null || Number.isSafeInteger(receipt.child.exit_code), 'child.exit_code must be a safe integer or null');
    assert(receipt.child.signal === null || typeof receipt.child.signal === 'string', 'child.signal must be string or null');
    if (receipt.child.signal !== null) stringValue(receipt.child.signal, 'child.signal', {maxBytes: 64});
    safeError(receipt.child.error, 'child.error');

    for (const name of ['stdout', 'stderr']) {
        const stream = receipt[name];
        exactKeys(stream, STREAM_KEYS, name);
        canonicalAbsolute(stream.path, `${name}.path`);
        nonnegativeInteger(stream.bytes, `${name}.bytes`);
        assert(stream.bytes <= MAX_STREAM_BYTES, `${name} exceeds stream limit`);
        validSha(stream.sha256, `${name}.sha256`);
        validSha(stream.file_sha256, `${name}.file_sha256`);
    }
    validatePreflightResult(receipt.preflight_result);
    validateRootProof(receipt.run_root, 'run_root');
    statusValue(receipt.status, 'receipt.status');

    if (receipt.status === 'PASS') assert(receipt.preflight_result.status === 'PASS', 'PASS receipt contains a failed preflight result');
    assert(receipt.preflight_result.purpose === receipt.purpose, 'receipt/preflight purpose mismatch');
    assert(JSON.stringify(receipt.preflight_result.candidate) === JSON.stringify(receipt.candidate), 'receipt/preflight candidate mismatch');
    assert(receipt.preflight_result.source_manifest_sha256 === receipt.source_manifest_sha256, 'receipt/preflight manifest mismatch');
    assert(JSON.stringify(receipt.preflight_result.run_root) === JSON.stringify(receipt.run_root), 'receipt/preflight run-root mismatch');
    if (receipt.status === 'PASS') {
        assert(receipt.child.exit_code === 0 && receipt.child.signal === null && receipt.child.error === null, 'PASS receipt has unsuccessful child');
        assert(receipt.run_root.absent_before && receipt.run_root.absent_after && receipt.run_root.parent_stable && receipt.run_root.write_attempts === 0 && receipt.run_root.error === null, 'PASS receipt root proof failed');
    }

    if (expected.purpose !== undefined) assert(receipt.purpose === expected.purpose, 'receipt purpose differs from expected tuple');
    if (expected.revision !== undefined) assert(receipt.revision === expected.revision, 'receipt revision differs from expected tuple');
    if (expected.candidate !== undefined) assert(JSON.stringify(receipt.candidate) === JSON.stringify(expected.candidate), 'receipt candidate differs from expected tuple');
    if (expected.sourceManifestSha256 !== undefined) assert(receipt.source_manifest_sha256 === expected.sourceManifestSha256, 'receipt manifest differs from expected tuple');
    if (expected.argv !== undefined) assert(JSON.stringify(receipt.argv) === JSON.stringify(expected.argv), 'receipt argv differs from expected tuple');
    if (expected.cwd !== undefined) assert(receipt.cwd === expected.cwd, 'receipt cwd differs from expected tuple');
    if (expected.environmentSha256 !== undefined) assert(receipt.environment_sha256 === expected.environmentSha256, 'receipt environment differs from expected tuple');
}

function freezeBytes(bytes) {
    return Object.freeze({base64: bytes.toString('base64'), byte_length: bytes.length, sha256: sha256(bytes)});
}

function statIdentity(stat) {
    return [stat.dev, stat.ino, stat.mode, stat.nlink, stat.size, stat.mtimeNs, stat.ctimeNs].map(String).join(':');
}

function directoryIdentity(stat) {
    return [stat.dev, stat.ino, stat.mode].map(String).join(':');
}

function componentIdentity(stat) {
    return stat.isDirectory() ? directoryIdentity(stat) : statIdentity(stat);
}

function assertCanonicalPathComponents(filePath, io) {
    canonicalAbsolute(filePath, 'filesystem path');
    const rootPath = path.parse(filePath).root;
    const noFollow = io.constants.O_NOFOLLOW;
    const directoryFlag = io.constants.O_DIRECTORY;
    assert(Number.isInteger(noFollow) && Number.isInteger(directoryFlag), 'platform lacks no-follow directory open flags');
    const ancestors = [];
    const rootStat = io.lstatSync(rootPath);
    assert(rootStat.isDirectory() && !rootStat.isSymbolicLink(), `invalid filesystem root: ${rootPath}`);
    const rootFd = io.openSync(rootPath, io.constants.O_RDONLY | noFollow | directoryFlag);
    try {
        const openedRoot = io.fstatSync(rootFd);
        assert(openedRoot.isDirectory() && directoryIdentity(openedRoot) === directoryIdentity(rootStat), `root descriptor/path mismatch: ${rootPath}`);
    } finally {
        io.closeSync(rootFd);
    }
    ancestors.push([rootPath, directoryIdentity(rootStat)]);
    let current = rootPath;
    const pieces = filePath.slice(rootPath.length).split(path.sep).filter(Boolean);
    for (let i = 0; i < pieces.length; i++) {
        current = path.join(current, pieces[i]);
        const stat = io.lstatSync(current);
        assert(!stat.isSymbolicLink(), `symlink path component: ${current}`);
        if (i < pieces.length - 1) {
            assert(stat.isDirectory(), `non-directory path component: ${current}`);
            const fd = io.openSync(current, io.constants.O_RDONLY | noFollow | directoryFlag);
            try {
                const opened = io.fstatSync(fd);
                assert(opened.isDirectory() && directoryIdentity(opened) === directoryIdentity(stat), `directory descriptor/path mismatch: ${current}`);
            } finally {
                io.closeSync(fd);
            }
        }
        ancestors.push([current, componentIdentity(stat)]);
    }
    return ancestors;
}

function assertPathComponentsStable(ancestors, io) {
    for (const [component, identity] of ancestors)
        assert(componentIdentity(io.lstatSync(component)) === identity, `path component changed: ${component}`);
}

function readNoFollow(filePath, {io = nodeFs, maxBytes = MAX_REPORT_BYTES, mode = RECEIPT_FILE_MODE} = {}) {
    const ancestors = assertCanonicalPathComponents(filePath, io);
    const before = io.lstatSync(filePath);
    assert(before.isFile() && !before.isSymbolicLink(), 'receipt component is not a regular file');
    assert(before.nlink === 1n, 'receipt component is not single-link');
    assert(Number(before.mode & 0o7777n) === mode, 'receipt component mode mismatch');
    assert(before.size <= BigInt(maxBytes), 'receipt component exceeds byte limit');
    const noFollow = io.constants.O_NOFOLLOW;
    assert(Number.isInteger(noFollow), 'platform does not expose O_NOFOLLOW');
    const fd = io.openSync(filePath, io.constants.O_RDONLY | noFollow);
    try {
        const opened = io.fstatSync(fd);
        assert(opened.isFile() && statIdentity(opened) === statIdentity(before), 'opened descriptor/path identity mismatch');
        const chunks = [];
        let total = 0;
        const chunk = Buffer.allocUnsafe(65536);
        for (;;) {
            const count = io.readSync(fd, chunk, 0, chunk.length, null);
            if (count === 0) break;
            total += count;
            assert(total <= maxBytes, 'receipt component grew beyond byte limit');
            chunks.push(Buffer.from(chunk.subarray(0, count)));
        }
        const bytes = Buffer.concat(chunks, total);
        const openedAfter = io.fstatSync(fd);
        const pathAfter = io.lstatSync(filePath);
        assert(statIdentity(openedAfter) === statIdentity(opened), 'descriptor changed while reading');
        assert(statIdentity(pathAfter) === statIdentity(before), 'path changed while reading');
        assertPathComponentsStable(ancestors, io);
        assert(bytes.length === Number(before.size), 'read size differs from descriptor size');
        return {bytes, stat: before, path: filePath};
    } finally {
        io.closeSync(fd);
    }
}

function readDirectory(directoryPath, {io = nodeFs, mode = RECEIPT_DIRECTORY_MODE} = {}) {
    const ancestors = assertCanonicalPathComponents(directoryPath, io);
    const before = io.lstatSync(directoryPath);
    assert(before.isDirectory() && !before.isSymbolicLink(), 'receipt directory is not a real directory');
    assert(Number(before.mode & 0o7777n) === mode, 'receipt directory mode mismatch');
    const noFollow = io.constants.O_NOFOLLOW;
    const directoryFlag = io.constants.O_DIRECTORY;
    assert(Number.isInteger(noFollow) && Number.isInteger(directoryFlag), 'platform lacks no-follow directory open flags');
    const fd = io.openSync(directoryPath, io.constants.O_RDONLY | noFollow | directoryFlag);
    try {
        const opened = io.fstatSync(fd);
        assert(opened.isDirectory() && statIdentity(opened) === statIdentity(before), 'opened directory/path identity mismatch');
        const entries = [];
        const directory = io.opendirSync(directoryPath);
        try {
            for (;;) {
                const entry = directory.readSync();
                if (entry === null) break;
                assert(entries.length < 3, 'receipt directory membership exceeds three entries');
                entries.push(entry.name);
            }
        } finally {
            directory.closeSync();
        }
        entries.sort();
        const openedAfter = io.fstatSync(fd);
        const pathAfter = io.lstatSync(directoryPath);
        assert(statIdentity(openedAfter) === statIdentity(opened), 'directory descriptor changed while listing');
        assert(statIdentity(pathAfter) === statIdentity(before), 'directory path changed while listing');
        assertPathComponentsStable(ancestors, io);
        return {entries, stat: before};
    } finally {
        io.closeSync(fd);
    }
}

function readReceiptFile(filePath, options = {}) {
    const result = readNoFollow(filePath, options);
    return Object.freeze({
        path: result.path,
        ...freezeBytes(result.bytes),
        mode: Number(result.stat.mode & 0o777n),
        device: String(result.stat.dev),
        inode: String(result.stat.ino)
    });
}

function isEqualOrAncestor(parent, child) {
    const relative = path.relative(parent, child);
    return relative === '' || (!relative.startsWith(`..${path.sep}`) && relative !== '..' && !path.isAbsolute(relative));
}

function fileIdentity(result) {
    return statIdentity(result.stat);
}

function physicalPathIncludingMissingTail(filePath, io) {
    canonicalAbsolute(filePath, 'protected filesystem path');
    const missing = [];
    let existing = filePath;
    for (;;) {
        try {
            // realpath also handles case aliases on case-insensitive volumes.
            const physical = io.realpathSync(existing);
            return path.join(physical, ...missing);
        } catch (error) {
            if (error.code !== 'ENOENT') throw error;
            // ENOENT can also mean an existing dangling symlink, not a missing tail.
            try {
                io.lstatSync(existing);
                throw new TypeError(`unresolvable protected path component: ${existing}`);
            } catch (entryError) {
                if (entryError.code !== 'ENOENT') throw entryError;
            }
            const parent = path.dirname(existing);
            assert(parent !== existing, 'filesystem root does not exist');
            missing.unshift(path.basename(existing));
            existing = parent;
        }
    }
}

function assertReceiptIdentityStable(directoryPath, directoryIdentity, filePaths, fileIdentities, io) {
    for (let i = 0; i < filePaths.length; i++) {
        const ancestors = assertCanonicalPathComponents(filePaths[i], io);
        assert(statIdentity(io.lstatSync(filePaths[i])) === fileIdentities[i], 'receipt file changed during validation');
        assertPathComponentsStable(ancestors, io);
    }
    assertCanonicalPathComponents(directoryPath, io);
    assert(statIdentity(io.lstatSync(directoryPath)) === directoryIdentity, 'receipt directory changed during validation');
}

function validateReceipt(reportPath, {io = nodeFs, expected = {}, ordinal = 0} = {}) {
    canonicalAbsolute(reportPath, 'report path');
    const reportFile = readNoFollow(reportPath, {io, maxBytes: MAX_REPORT_BYTES, mode: RECEIPT_FILE_MODE});
    const receipt = parseCanonicalJson(reportFile.bytes, {maxBytes: MAX_REPORT_BYTES, label: 'receipt report'});
    validateReceiptShape(receipt, expected);
    const receiptDirectory = path.dirname(reportPath);
    const expectedReportName = `${receipt.receipt_id}.json`;
    assert(path.basename(reportPath) === expectedReportName, 'report filename is not bound to receipt_id');
    const directory = readDirectory(receiptDirectory, {io});
    const expectedEntries = [`${receipt.receipt_id}.json`, `${receipt.receipt_id}.stderr`, `${receipt.receipt_id}.stdout`].sort();
    assert(JSON.stringify(directory.entries) === JSON.stringify(expectedEntries), 'receipt directory membership mismatch');

    const streams = {};
    const identities = new Set([fileIdentity(reportFile)]);
    for (const name of ['stdout', 'stderr']) {
        const streamPath = receipt[name].path;
        canonicalAbsolute(streamPath, `${name}.path`);
        assert(path.dirname(streamPath) === receiptDirectory, `${name} path is outside receipt directory`);
        assert(path.basename(streamPath) === `${receipt.receipt_id}.${name}`, `${name} basename is not bound to receipt_id`);
        const result = readNoFollow(streamPath, {io, maxBytes: MAX_STREAM_BYTES, mode: RECEIPT_FILE_MODE});
        assert(!identities.has(fileIdentity(result)), 'receipt files alias the same device/inode');
        identities.add(fileIdentity(result));
        const digest = sha256(result.bytes);
        assert(result.bytes.length === receipt[name].bytes, `${name} byte count mismatch`);
        assert(digest === receipt[name].sha256 && digest === receipt[name].file_sha256, `${name} digest mismatch`);
        streams[name] = result;
    }

    const stdout = streams.stdout.bytes;
    try {
        assert(stdout.length > 0 && stdout[stdout.length - 1] === 0x0a && (stdout.length < 2 || stdout[stdout.length - 2] !== 0x0a), 'stdout must contain exactly one canonical result line');
        const stdoutResult = parseCanonicalJson(stdout, {maxBytes: MAX_STREAM_BYTES, label: 'preflight stdout result'});
        validatePreflightResult(stdoutResult);
        assert(canonicalJson(stdoutResult).equals(stdout), 'stdout result line is not canonical');
        assert(canonicalJson(receipt.preflight_result).equals(stdout), 'stdout result differs from receipt.preflight_result');
    } catch (cause) {
        const error = new TypeError(cause.message, {cause});
        error.receiptBoundary = 'STDOUT_SHAPE';
        throw error;
    }

    const files = [reportFile, streams.stdout, streams.stderr];
    const snapshot = Object.freeze({
        receipt: freezeBytes(reportFile.bytes),
        stdout: freezeBytes(streams.stdout.bytes),
        stderr: freezeBytes(streams.stderr.bytes)
    });
    const parent = receipt.run_root.parent;
    const summary = Object.freeze({
        ordinal,
        receipt_id: receipt.receipt_id,
        receipt_directory: receiptDirectory,
        receipt_directory_mode: RECEIPT_DIRECTORY_MODE,
        report_path: reportPath,
        report_bytes: reportFile.bytes.length,
        report_sha256: sha256(reportFile.bytes),
        report_mode: RECEIPT_FILE_MODE,
        stdout_path: receipt.stdout.path,
        stdout_bytes: streams.stdout.bytes.length,
        stdout_sha256: sha256(streams.stdout.bytes),
        stdout_mode: RECEIPT_FILE_MODE,
        stderr_path: receipt.stderr.path,
        stderr_bytes: streams.stderr.bytes.length,
        stderr_sha256: sha256(streams.stderr.bytes),
        stderr_mode: RECEIPT_FILE_MODE,
        run_root_path: receipt.run_root.path,
        root_absent_before: receipt.run_root.absent_before,
        root_absent_after: receipt.run_root.absent_after,
        root_write_attempts: receipt.run_root.write_attempts,
        run_root_parent_path: parent.path,
        run_root_parent_device: parent.device,
        run_root_parent_inode: parent.inode
    });
    validateReceiptSummary(summary, ordinal);
    assertReceiptIdentityStable(receiptDirectory, statIdentity(directory.stat),
        files.map(file => file.path), files.map(fileIdentity), io);

    return Object.freeze({
        receipt: deepFreeze(receipt),
        summary,
        snapshot,
        receiptPath: reportPath,
        receiptDirectory,
        receiptDirectoryIdentity: statIdentity(directory.stat),
        fileIdentities: Object.freeze(files.map(fileIdentity)),
        usable: receipt.status === 'PASS' && receipt.purpose ===
            (expected.purpose === undefined ? 'validation' : expected.purpose)
    });
}

function validateReceiptSet(reportPaths, {io = nodeFs, expected = {}, requirePass = true, forbiddenRoots = []} = {}) {
    assert(Array.isArray(reportPaths) && reportPaths.length === 2, 'exactly two receipt report paths are required');
    assert(Array.isArray(forbiddenRoots), 'forbiddenRoots must be an array');
    forbiddenRoots.forEach((p, i) => canonicalAbsolute(p, `forbiddenRoots[${i}]`));
    const physicalForbiddenRoots = forbiddenRoots.map(p => physicalPathIncludingMissingTail(p, io));
    const binding = {...expected, purpose: expected.purpose === undefined ? 'validation' : expected.purpose};
    const receipts = reportPaths.map((p, i) => validateReceipt(p, {io, expected: binding, ordinal: i + 1}));
    if (requirePass) assert(receipts.every(r => r.usable), 'non-PASS receipt cannot enter formal run');

    const receiptIds = receipts.map(r => r.receipt.receipt_id);
    const receiptDirs = receipts.map(r => r.receiptDirectory);
    const receiptDirectoryIdentities = receipts.map(r => r.receiptDirectoryIdentity);
    const reportFiles = receipts.map(r => r.receiptPath);
    const sidecars = receipts.flatMap(r => [r.receipt.stdout.path, r.receipt.stderr.path]);
    const runRoots = receipts.map(r => r.receipt.run_root.path);
    const runParents = receipts.map(r => r.receipt.run_root.parent);
    assert(new Set(receiptIds).size === 2, 'receipt IDs must be distinct');
    assert(new Set(receiptDirs).size === 2, 'receipt directories must be distinct');
    assert(new Set(receiptDirectoryIdentities).size === 2, 'receipt directories must not alias the same filesystem object');
    assert(new Set(reportFiles).size === 2 && new Set(sidecars).size === 4, 'receipt report/sidecar paths must be distinct');
    assert(new Set(runRoots).size === 2, 'receipt run roots must be distinct');
    assert(new Set(runParents.map(p => `${p.device}:${p.inode}`)).size === 2, 'receipt run-root parents must be physically distinct');

    const receiptFiles = receipts.map(r => [r.receiptPath, r.receipt.stdout.path, r.receipt.stderr.path]);
    const allPaths = [...receiptDirs, ...receiptFiles.flat(), ...runRoots];
    for (let i = 0; i < allPaths.length; i++) canonicalAbsolute(allPaths[i], `receipt path ${i}`);

    // A receipt directory must contain its own three files; only cross-receipt
    // containment is forbidden. Comparing every directory with its own files
    // would reject every valid receipt set.
    assert(!isEqualOrAncestor(receiptDirs[0], receiptDirs[1]) && !isEqualOrAncestor(receiptDirs[1], receiptDirs[0]),
        'receipt directories overlap or contain one another');
    for (let i = 0; i < receiptDirs.length; i++) {
        for (let j = 0; j < receiptDirs.length; j++) {
            if (i === j) continue;
            for (const file of receiptFiles[i])
                assert(!isEqualOrAncestor(receiptDirs[j], file), 'receipt file is contained by the other receipt directory');
        }
    }
    const independentPaths = [...receiptFiles.flat(), ...runRoots];
    for (let i = 0; i < independentPaths.length; i++) {
        for (let j = i + 1; j < independentPaths.length; j++)
            assert(!isEqualOrAncestor(independentPaths[i], independentPaths[j]) && !isEqualOrAncestor(independentPaths[j], independentPaths[i]),
                'receipt files or run roots overlap or contain one another');
    }
    for (let i = 0; i < receiptDirs.length; i++) {
        for (const root of runRoots)
            assert(!isEqualOrAncestor(root, receiptDirs[i]) && !isEqualOrAncestor(receiptDirs[i], root),
                'run root overlaps a receipt directory');
        for (let j = 0; j < receiptDirs.length; j++) {
            if (i === j) continue;
            for (const file of receiptFiles[i])
                assert(!isEqualOrAncestor(file, receiptDirs[j]) && !isEqualOrAncestor(receiptDirs[j], file),
                    'receipt file overlaps the other receipt directory');
        }
    }
    const physicalPaths = physicalForbiddenRoots.length ? allPaths.map(p => physicalPathIncludingMissingTail(p, io)) : [];
    for (const forbidden of physicalForbiddenRoots) {
        for (const candidate of physicalPaths)
            assert(!isEqualOrAncestor(forbidden, candidate) && !isEqualOrAncestor(candidate, forbidden),
                `receipt path overlaps forbidden root ${forbidden}`);
    }

    const first = receipts[0].receipt;
    const second = receipts[1].receipt;
    assert(JSON.stringify(first.candidate) === JSON.stringify(second.candidate), 'receipt candidates differ');
    assert(first.source_manifest_sha256 === second.source_manifest_sha256, 'receipt source manifests differ');
    assert(JSON.stringify(first.argv) === JSON.stringify(second.argv), 'receipt argv differs');
    assert(first.cwd === second.cwd, 'receipt cwd differs');
    assert(first.environment_sha256 === second.environment_sha256, 'receipt environment differs');
    for (const receipt of receipts)
        assertReceiptIdentityStable(receipt.receiptDirectory, receipt.receiptDirectoryIdentity,
            [receipt.receiptPath, receipt.receipt.stdout.path, receipt.receipt.stderr.path], receipt.fileIdentities, io);
    forbiddenRoots.forEach((p, i) => assert(physicalPathIncludingMissingTail(p, io) === physicalForbiddenRoots[i],
        'forbidden root changed during validation'));

    return Object.freeze({
        receipts: Object.freeze(receipts),
        summaries: Object.freeze(receipts.map(r => r.summary)),
        receiptPaths: Object.freeze([...reportPaths]),
        snapshot: Object.freeze(receipts.map(r => r.snapshot))
    });
}

function revalidateReceiptSet(previous, options = {}) {
    assert(previous && Array.isArray(previous.receiptPaths) && Array.isArray(previous.snapshot), 'prior receipt-set snapshot is required');
    const io = options.io || nodeFs;
    // Compare the original identities and bytes before reparsing. Stable metadata
    // with substituted readback is distinct from an on-disk receipt mutation.
    for (let i = 0; i < previous.receipts.length; ++i) {
        const saved = previous.receipts[i];
        const paths = [saved.receiptPath, saved.receipt.stdout.path, saved.receipt.stderr.path];
        assert(statIdentity(io.lstatSync(saved.receiptDirectory)) === saved.receiptDirectoryIdentity,
            `receipt ${i + 1} directory identity changed after initial validation`);
        for (let j = 0; j < paths.length; ++j)
            assert(statIdentity(io.lstatSync(paths[j])) === saved.fileIdentities[j],
                `receipt ${i + 1} file identity changed after initial validation`);
        assertReceiptIdentityStable(saved.receiptDirectory, saved.receiptDirectoryIdentity, paths, saved.fileIdentities, io);
        for (let j = 0; j < paths.length; ++j) {
            const file = readNoFollow(paths[j], {io, maxBytes: j === 0 ? MAX_REPORT_BYTES : MAX_STREAM_BYTES});
            assert(fileIdentity(file) === saved.fileIdentities[j], 'receipt file changed during readback');
            if (file.bytes.toString('base64') !== previous.snapshot[i][['receipt', 'stdout', 'stderr'][j]].base64) {
                const error = new TypeError(`receipt ${i + 1} readback bytes differ from the original snapshot`);
                error.receiptBoundary = 'READBACK_MISMATCH';
                throw error;
            }
        }
        assertReceiptIdentityStable(saved.receiptDirectory, saved.receiptDirectoryIdentity, paths, saved.fileIdentities, io);
    }
    const current = validateReceiptSet(previous.receiptPaths, options);
    assert(current.snapshot.length === previous.snapshot.length, 'receipt-set cardinality changed');
    for (let i = 0; i < current.snapshot.length; i++) {
        for (const key of ['receipt', 'stdout', 'stderr'])
            assert(current.snapshot[i][key].base64 === previous.snapshot[i][key].base64 && current.snapshot[i][key].sha256 === previous.snapshot[i][key].sha256,
                `receipt ${i + 1} ${key} bytes changed after initial validation`);
        assert(current.receipts[i].receiptDirectoryIdentity === previous.receipts[i].receiptDirectoryIdentity,
            `receipt ${i + 1} directory identity changed after initial validation`);
        assert(JSON.stringify(current.receipts[i].fileIdentities) === JSON.stringify(previous.receipts[i].fileIdentities),
            `receipt ${i + 1} file identity changed after initial validation`);
    }
    assert(JSON.stringify(current.summaries) === JSON.stringify(previous.summaries), 'normalized receipt summaries changed');
    return current;
}

module.exports = Object.freeze({
    REVISION,
    SCHEMA_VERSION,
    PURPOSES,
    MAX_JSON_DEPTH,
    MAX_REPORT_BYTES,
    MAX_STREAM_BYTES,
    RECEIPT_DIRECTORY_MODE,
    RECEIPT_FILE_MODE,
    RECEIPT_SUMMARY_KEYS,
    PREFLIGHT_SUITES,
    PATCH_DIRECTIONS,
    RECEIPT_SCHEMA,
    PREFLIGHT_RESULT_SCHEMA,
    canonicalJson,
    parseCanonicalJson,
    sha256,
    readReceiptFile,
    validateReceipt,
    validateReceiptSet,
    revalidateReceiptSet,
    validatePreflightResult,
    validateReceiptSummary
});
