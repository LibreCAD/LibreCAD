/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const path = require('node:path');
const h1 = require('./preflight-receipt');
const input = require('./input-capture');

// H3 consumes H2's actual input manifest, not the historical scratch manifest.
// Declare these three H3 modules and worker modules in H2 sourcePaths/fixturePaths;
// declare the canonical formal config and registry files in H2 configPaths.
// Registry order defines dispatch order. Each worker prints one canonical result
// and creates only its declared flat 0400 outputs under the exclusively owned root.
// No callback, registry row, result, or output inventory defaults to PASS.
// The formal config's preflight object uses exactly PREFLIGHT_KEYS below. Include
// config/registry paths before H2 capturePreflight, then add that capture's separate
// receiptId/runRoot/receiptDirectory arguments. H3 recaptures the identical H2
// manifest, including the real executable and original preflight runner; workers'
// module_path must equal source_path for their cases in the H2 case inventory.
const LIMITS = Object.freeze({configBytes: 65536, registryBytes: 65536, workers: 32,
    casesPerWorker: 128, outputs: 128, outputBytes: 8388608, totalOutputBytes: 67108864,
    contextBytes: 32768, resultBytes: 65536, streamBytes: 1048576, timeoutMs: 300000});
const REGISTRY_KEYS = Object.freeze(['schema_version', 'purpose', 'workers']);
const WORKER_KEYS = Object.freeze(['id', 'module_path', 'args', 'case_ids', 'outputs', 'timeout_ms', 'max_stream_bytes']);
const RESULT_KEYS = Object.freeze(['schema_version', 'purpose', 'worker_id', 'ordinal', 'status', 'case_ids', 'events', 'outputs']);
const EVENT_KEYS = Object.freeze(['ordinal', 'case_id', 'status']);
const CONFIG_KEYS = Object.freeze(['schema_version', 'purpose', 'registry_path', 'run_root', 'receipt_ids', 'receipt_roots', 'preflight']);
const PREFLIGHT_KEYS = Object.freeze(['purpose', 'candidatePath', 'runnerPath', 'sourcePaths', 'inputPaths',
    'fixturePaths', 'configPaths', 'caseInventoryPath', 'runnerArgs', 'cwd', 'environment']);

class RunnerError extends Error {
    constructor(stage, reason, message) {
        super(message);
        this.name = 'RunnerError';
        this.stage = stage;
        this.reason = reason;
    }
}

function ensure(condition, stage, reason, message) {
    if (!condition) throw new RunnerError(stage, reason, message);
}

function exactKeys(value, keys, label) {
    ensure(value && typeof value === 'object' && !Array.isArray(value) &&
        JSON.stringify(Object.keys(value)) === JSON.stringify(keys), 'REGISTRY', 'SCHEMA_INVALID', `${label} keys/order mismatch`);
}

function identifier(value, label) {
    ensure(typeof value === 'string' && /^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(value),
        'REGISTRY', 'IDENTIFIER_INVALID', `${label} is invalid`);
}

function uuid(value) {
    return typeof value === 'string' && /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(value);
}

function sortedUniqueStrings(values, label, max) {
    ensure(Array.isArray(values) && values.length > 0 && values.length <= max,
        'REGISTRY', 'INVENTORY_INVALID', `${label} must be nonempty and bounded`);
    for (let i = 0; i < values.length; i++) {
        identifier(values[i], label);
        ensure(i === 0 || values[i - 1] < values[i], 'REGISTRY', 'INVENTORY_INVALID', `${label} must be sorted and unique`);
    }
}

function readStableFile(filePath, maxBytes, io = input.nodeFsAdapter) {
    input.canonicalPath(filePath);
    const guards = input.directoryGuards(path.dirname(filePath), io);
    const before = io.lstatSync(filePath);
    const read = h1.readReceiptFile(filePath, {io, maxBytes, mode: Number(before.mode & 0o7777n)});
    ensure(input.statIdentity(io.lstatSync(filePath)) === input.statIdentity(before), 'REVALIDATION', 'INPUT_CHANGED', `input changed: ${filePath}`);
    input.checkDirectoryGuards(guards, io);
    return input.freeze({path: filePath, maxBytes, guards, identity: input.statIdentity(before), ...read});
}

function recheckFile(saved, io = input.nodeFsAdapter) {
    input.checkDirectoryGuards(saved.guards, io);
    const current = readStableFile(saved.path, saved.maxBytes, io);
    ensure(current.identity === saved.identity && current.base64 === saved.base64, 'REVALIDATION', 'INPUT_CHANGED', `input changed: ${saved.path}`);
}

function validateConfiguration(config) {
    exactKeys(config, CONFIG_KEYS, 'formal config');
    ensure(config.schema_version === 1 && ['validation', 'synthetic'].includes(config.purpose), 'REGISTRY', 'SCHEMA_INVALID', 'formal config version/purpose invalid');
    input.canonicalPath(config.registry_path);
    input.canonicalPath(config.run_root);
    ensure(Array.isArray(config.receipt_ids) && config.receipt_ids.length === 2 && config.receipt_ids.every(uuid) &&
        new Set(config.receipt_ids).size === 2, 'REGISTRY', 'SCHEMA_INVALID', 'exactly two distinct expected receipt UUIDs are required');
    ensure(Array.isArray(config.receipt_roots) && config.receipt_roots.length === 2 &&
        new Set(config.receipt_roots).size === 2, 'REGISTRY', 'SCHEMA_INVALID', 'exactly two distinct receipt roots are required');
    config.receipt_roots.forEach(p => input.canonicalPath(p));
    exactKeys(config.preflight, PREFLIGHT_KEYS, 'H2 preflight configuration');
    ensure(config.preflight.purpose === config.purpose, 'REGISTRY', 'PURPOSE_MISMATCH', 'formal/preflight purpose differs');
    input.canonicalPath(config.preflight.cwd);
    ensure(Array.isArray(config.preflight.runnerArgs) && config.preflight.runnerArgs.length <= 128,
        'REGISTRY', 'SCHEMA_INVALID', 'preflight arguments must be explicit and bounded');
    for (const arg of config.preflight.runnerArgs) if (arg !== '') input.string(arg, 'preflight argument', 8192);
    ensure(h1.canonicalJson(config.preflight.runnerArgs).length <= 32768, 'REGISTRY', 'SCHEMA_INVALID', 'preflight arguments exceed byte cap');
    return input.freeze(config);
}

function validateRegistry(registry, capturedInputs) {
    exactKeys(registry, REGISTRY_KEYS, 'registry');
    ensure(registry.schema_version === 1 && registry.purpose === capturedInputs.manifest.purpose,
        'REGISTRY', 'PURPOSE_MISMATCH', 'registry version/purpose mismatch');
    ensure(Array.isArray(registry.workers) && registry.workers.length > 0 && registry.workers.length <= LIMITS.workers,
        'REGISTRY', 'WORKERS_NOT_CONFIGURED', 'explicit nonempty bounded worker registry is required');
    const ids = new Set();
    const cases = new Set();
    const outputs = new Set();
    for (const worker of registry.workers) {
        exactKeys(worker, WORKER_KEYS, 'worker');
        identifier(worker.id, 'worker ID');
        ensure(!ids.has(worker.id), 'REGISTRY', 'WORKER_DUPLICATE', 'duplicate worker ID');
        ids.add(worker.id);
        input.canonicalPath(worker.module_path);
        ensure(capturedInputs.manifest.files.some(row => row.path === worker.module_path &&
            row.bytes > 0 && row.roles.some(role => role === 'source' || role === 'fixture')),
        'REGISTRY', 'WORKER_SOURCE_UNPINNED', 'worker module is not a real declared H2 source/fixture');
        ensure(Array.isArray(worker.args) && worker.args.length <= 32, 'REGISTRY', 'SCHEMA_INVALID', 'worker args must be explicit and bounded');
        for (const arg of worker.args) if (arg !== '') input.string(arg, 'worker argument', 8192);
        ensure(h1.canonicalJson(worker.args).length <= 32768, 'REGISTRY', 'SCHEMA_INVALID', 'worker argv exceeds byte cap');
        sortedUniqueStrings(worker.case_ids, 'worker case IDs', LIMITS.casesPerWorker);
        sortedUniqueStrings(worker.outputs, 'worker outputs', LIMITS.outputs);
        for (const id of worker.case_ids) {
            ensure(!cases.has(id), 'REGISTRY', 'CASE_DUPLICATE', 'case is assigned more than once');
            cases.add(id);
            ensure(capturedInputs.manifest.cases.some(row => row.id === id && row.source_path === worker.module_path),
                'REGISTRY', 'CASE_SOURCE_MISMATCH', 'case definition does not name its declared worker module');
        }
        for (const name of worker.outputs) {
            const folded = name.toLowerCase();
            ensure(!outputs.has(folded), 'REGISTRY', 'OUTPUT_DUPLICATE', 'output names collide, including case aliases');
            outputs.add(folded);
        }
        ensure(Number.isSafeInteger(worker.timeout_ms) && worker.timeout_ms > 0 && worker.timeout_ms <= LIMITS.timeoutMs,
            'REGISTRY', 'LIMIT_INVALID', 'worker timeout is invalid');
        ensure(Number.isSafeInteger(worker.max_stream_bytes) && worker.max_stream_bytes > 0 && worker.max_stream_bytes <= LIMITS.streamBytes,
            'REGISTRY', 'LIMIT_INVALID', 'worker stream cap is invalid');
    }
    ensure(outputs.size <= LIMITS.outputs, 'REGISTRY', 'LIMIT_INVALID', 'too many output files');
    ensure(JSON.stringify([...cases].sort()) === JSON.stringify(capturedInputs.caseIds),
        'REGISTRY', 'CASE_INVENTORY_MISMATCH', 'registry must cover the exact H2 case inventory once');
    return input.freeze(registry);
}

function validateWorkerResult(result, worker, ordinal, purpose) {
    ensure(result !== null && result !== undefined && typeof result === 'object' && !Array.isArray(result),
        'WORKER', 'WORKER_RESULT_MISSING', 'worker must return an explicit result object');
    ensure(Object.hasOwn(result, 'status') && result.status !== undefined, 'WORKER', 'WORKER_STATUS_MISSING', 'worker status is required');
    ensure(result.status === 'PASS' || result.status === 'FAIL', 'WORKER', 'WORKER_STATUS_UNKNOWN', 'worker terminal status is unrecognized');
    ensure(result.status === 'PASS', 'WORKER', 'WORKER_FAILED', 'worker explicitly reported FAIL');
    ensure(JSON.stringify(Object.keys(result)) === JSON.stringify(RESULT_KEYS) && result.schema_version === 1 &&
        result.purpose === purpose && result.worker_id === worker.id && result.ordinal === ordinal,
    'WORKER', 'WORKER_RESULT_INVALID', 'worker result schema/purpose/identity mismatch');
    ensure(JSON.stringify(result.case_ids) === JSON.stringify(worker.case_ids), 'WORKER', 'WORKER_CASE_MISMATCH', 'worker case inventory differs');
    ensure(Array.isArray(result.events) && result.events.length === worker.case_ids.length,
        'WORKER', 'WORKER_EVENT_MISMATCH', 'worker event count differs');
    for (const [i, event] of result.events.entries()) ensure(event && JSON.stringify(Object.keys(event)) === JSON.stringify(EVENT_KEYS) &&
        event.ordinal === i + 1 && event.case_id === worker.case_ids[i] && event.status === 'PASS',
    'WORKER', 'WORKER_EVENT_MISMATCH', 'worker events must be explicit PASS in registry case order');
    ensure(Array.isArray(result.outputs), 'WORKER', 'WORKER_OUTPUT_MISSING', 'worker output inventory is required');
    ensure(new Set(result.outputs).size === result.outputs.length, 'WORKER', 'WORKER_OUTPUT_DUPLICATE', 'worker reported duplicate outputs');
    ensure(result.outputs.every(name => worker.outputs.includes(name)), 'WORKER', 'WORKER_OUTPUT_UNEXPECTED', 'worker reported undeclared outputs');
    ensure(worker.outputs.every(name => result.outputs.includes(name)), 'WORKER', 'WORKER_OUTPUT_MISSING', 'worker did not report every declared output');
    ensure(JSON.stringify(result.outputs) === JSON.stringify(worker.outputs), 'WORKER', 'WORKER_OUTPUT_MISMATCH', 'worker reported missing/extra/reordered outputs');
    return input.freeze(result);
}

module.exports = Object.freeze({LIMITS, REGISTRY_KEYS, WORKER_KEYS, RESULT_KEYS, EVENT_KEYS, CONFIG_KEYS, PREFLIGHT_KEYS,
    RunnerError, ensure, uuid, readStableFile, recheckFile, validateConfiguration, validateRegistry, validateWorkerResult});
