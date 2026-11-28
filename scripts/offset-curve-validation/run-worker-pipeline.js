/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const path = require('node:path');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const registry = require('./formal-registry');
const h3 = require('./formal-runner');
const h4 = require('./formal-ledger');
const h5 = require('./p0f-evidence');
const {parseRunArguments} = require('./run-formal-gates');

const {ensure, RunnerError} = registry;
const paths = ['ledgerPath', 'checkerConfigPath', 'manifestPath'];
const fields = ['configPath', 'runRoot', 'receiptPaths', ...paths, 'preflightCaptures'];

function ownData(value) {
    ensure(value && typeof value === 'object' && [Object.prototype, null].includes(Object.getPrototypeOf(value)),
        'PIPELINE', 'ARG_INVALID', 'plain request data required');
    const descriptors = Object.getOwnPropertyDescriptors(value);
    ensure(Reflect.ownKeys(descriptors).every(key => typeof key === 'string' && Object.hasOwn(descriptors[key], 'value')),
        'PIPELINE', 'ARG_INVALID', 'accessors and symbols are forbidden');
    return descriptors;
}

function pair(value) {
    ensure(Array.isArray(value) && Object.getPrototypeOf(value) === Array.prototype,
        'PIPELINE', 'ARG_INVALID', 'an explicit pair is required');
    const descriptors = Object.getOwnPropertyDescriptors(value);
    ensure(descriptors.length.value === 2 && Reflect.ownKeys(descriptors).length === 3 &&
        ['0', '1'].every(key => descriptors[key] && Object.hasOwn(descriptors[key], 'value')),
    'PIPELINE', 'ARG_INVALID', 'an accessor-free pair is required');
    return [descriptors[0].value, descriptors[1].value];
}

function transportSnapshot(value) {
    if (value === null || value === undefined) return null;
    return pair(value).map(capture => {
        const row = ownData(capture);
        ensure(row.receiptPath && row.stdin, 'PIPELINE', 'ARG_INVALID', 'capture path and stdin required');
        const stdin = ownData(row.stdin.value);
        const keys = ['base64', 'bytes', 'sha256', 'write_completed'];
        ensure(JSON.stringify(Object.keys(stdin)) === JSON.stringify(keys), 'PIPELINE', 'ARG_INVALID', 'capture stdin keys differ');
        ensure(typeof row.receiptPath.value === 'string' && typeof stdin.base64.value === 'string' &&
            stdin.base64.value.length <= 65536 && Number.isSafeInteger(stdin.bytes.value) && stdin.bytes.value >= 0 &&
            typeof stdin.sha256.value === 'string' && /^[a-f0-9]{64}$/.test(stdin.sha256.value) &&
            typeof stdin.write_completed.value === 'boolean', 'PIPELINE', 'ARG_INVALID', 'invalid capture snapshot');
        return {receiptPath: row.receiptPath.value,
            stdin: Object.fromEntries(keys.map(key => [key, stdin[key].value]))};
    });
}

function validatePipelineRequest(input) {
    const descriptors = ownData(input);
    ensure(Object.keys(descriptors).every(key => fields.includes(key)) &&
        fields.filter(key => key !== 'preflightCaptures').every(key => Object.hasOwn(descriptors, key)),
    'PIPELINE', 'ARG_INVALID', 'missing or unknown pipeline request field');
    const formal = h3.validateRequest({configPath: descriptors.configPath.value, runRoot: descriptors.runRoot.value,
        receiptPaths: pair(descriptors.receiptPaths.value)});
    const request = {...formal};
    for (const key of paths) {
        try { request[key] = h2.canonicalPath(descriptors[key].value); }
        catch (error) { throw new RunnerError('PIPELINE', 'ARG_PATH_INVALID', error.message); }
    }
    request.preflightCaptures = transportSnapshot(descriptors.preflightCaptures && descriptors.preflightCaptures.value);
    return h2.freeze(request);
}

function related(a, b) {
    const r = path.relative(a, b);
    return r === '' || r !== '..' && !r.startsWith(`..${path.sep}`) && !path.isAbsolute(r);
}

function safeMessage(error) {
    return Buffer.from(String(error && error.message || error).slice(0, 2048)).toString('utf8').replace(/\0/g, '');
}

// No dependency injection at this orchestration boundary. Synthetic refusal
// hooks belong to the focused H3/H4/H5 tests, never a successful pipeline run.
async function runWorkerPipeline(input, dependencies = {}) {
    let request, config, captured, formal = null, ledger = null, evidence = null, phase = 'INGRESS';
    let failureDetails = null;
    const result = (status, stage, reason, message) => h2.freeze({schema_version: 1,
        purpose: config ? config.purpose : null, status: status === 'REFUSED' && formal && formal.root_created ? 'FAIL' : status,
        stage, reason, message,
        exit_code: status === 'PASS' ? 0 : formal && formal.root_created ? 1 : 2,
        phase, h3: formal, ledger, evidence, failure_details: failureDetails,
        completion_scope: 'automated-worker-evidence-only',
        limitations: ['No human approval or native feature completion is asserted.',
            'Original failure artifacts and independently created paths are never removed.',
            'Filesystem observations and direct-child deadlines are not filesystem or process-tree containment.']});
    try {
        const hooks = ownData(dependencies);
        ensure(Reflect.ownKeys(hooks).length === 0, 'PIPELINE', 'INJECTION_FORBIDDEN', 'pipeline dependency injection is forbidden');
        request = validatePipelineRequest(input);
        const file = registry.readStableFile(request.configPath, registry.LIMITS.configBytes, h2.nodeFsAdapter);
        config = registry.validateConfiguration(h1.parseCanonicalJson(Buffer.from(file.base64, 'base64'), {maxBytes: registry.LIMITS.configBytes}));
        captured = h2.captureInputs(config.preflight);
        ensure(captured.manifest.files.some(row => row.path === __filename && row.roles.includes('source') && row.bytes > 0),
            'PIPELINE', 'SOURCE_UNPINNED', 'pipeline implementation must be an actual H2 source');
        ensure(config.run_root === request.runRoot, 'IDENTITY', 'RUN_ROOT_MISMATCH', 'pipeline root differs from pinned configuration');
        const outputDirectories = [request.runRoot, path.dirname(request.ledgerPath), path.dirname(request.manifestPath)];
        const protectedPaths = [...captured.manifest.files.map(row => row.path),
            ...request.receiptPaths.map(p => path.dirname(p)), ...config.receipt_roots];
        const physical = p => {
            const parent = path.dirname(p);
            h2.directoryGuards(parent);
            return path.join(h2.nodeFsAdapter.realpathSync(parent), path.basename(p));
        };
        const actualOutputs = outputDirectories.map(physical);
        const actualProtected = protectedPaths.map(physical);
        for (let i = 0; i < actualOutputs.length; i++) {
            ensure(!actualOutputs.slice(i + 1).some(p => related(p, actualOutputs[i]) || related(actualOutputs[i], p)) &&
                !actualProtected.some(p => related(p, actualOutputs[i]) || related(actualOutputs[i], p)),
            'PIPELINE', 'PATH_POLICY', 'pipeline outputs must be disjoint from each other and protected inputs');
        }
        registry.recheckFile(file, h2.nodeFsAdapter);
        h2.revalidateInputs(captured);
        phase = 'H3';
        formal = await h3.runFormalTransaction({configPath: request.configPath, runRoot: request.runRoot, receiptPaths: request.receiptPaths});
        if (formal.status !== 'PASS') return result(formal.status, formal.stage, formal.reason, formal.message);
        h2.revalidateInputs(captured);
        phase = 'H4';
        const context = h4.captureLedgerContext({configPath: request.configPath, receiptPaths: request.receiptPaths});
        ledger = h4.publishJoinedLedger(h4.buildJoinedLedger(formal, context), request.ledgerPath);
        phase = 'H5';
        evidence = await h5.runP0fEvidence({configPath: request.configPath, receiptPaths: request.receiptPaths,
            h3Result: formal, ledgerReadback: ledger, preflightCaptures: request.preflightCaptures,
            checkerConfigPath: request.checkerConfigPath, manifestPath: request.manifestPath});
        if (evidence.status !== 'PASS') return result(evidence.status, evidence.stage, evidence.reason, evidence.message);
        h4.revalidateJoinedLedger(ledger);
        h2.revalidateInputs(captured);
        h5.revalidateEvidenceManifest(evidence);
        phase = 'COMPLETE';
        return result('PASS', 'COMPLETE', 'AUTOMATED_EVIDENCE_PASS', 'Workers, joined ledger and automated evidence checker passed');
    } catch (error) {
        if (error && error.details) failureDetails = error.details;
        return result(formal && formal.root_created ? 'FAIL' : 'REFUSED', error.stage || 'PIPELINE',
            error.reason || 'IO_ERROR', safeMessage(error));
    }
}

function parsePipelineArguments(argv) {
    ensure(Array.isArray(argv) && argv.length <= 64, 'CLI', 'ARG_INVALID', 'bounded arguments required');
    const extra = Object.create(null), forwarded = [];
    const flags = {'--ledger': 'ledgerPath', '--checker-config': 'checkerConfigPath', '--evidence-manifest': 'manifestPath'};
    for (let i = 0; i < argv.length; i++) {
        const key = Object.hasOwn(flags, argv[i]) && flags[argv[i]];
        if (!key) { forwarded.push(argv[i]); continue; }
        ensure(!Object.hasOwn(extra, key), 'CLI', 'ARG_DUPLICATE_OPTION', `duplicate ${argv[i]}`);
        const value = argv[++i];
        ensure(typeof value === 'string' && !value.startsWith('--'), 'CLI', 'ARG_MISSING_VALUE', 'pipeline option requires a value');
        extra[key] = value;
    }
    ensure(paths.every(key => Object.hasOwn(extra, key)), 'CLI', 'ARG_MISSING_VALUE', 'ledger, checker config and evidence manifest are required');
    return validatePipelineRequest({...parseRunArguments(forwarded), ...extra, preflightCaptures: null});
}

async function cli(argv) {
    let result;
    try { result = await runWorkerPipeline(parsePipelineArguments(argv)); }
    catch (error) {
        result = {schema_version: 1, status: 'REFUSED', stage: error.stage || 'CLI',
            reason: error.reason || 'ARG_INVALID', message: safeMessage(error), exit_code: 2};
    }
    process.stdout.write(h1.canonicalJson(result));
    process.exitCode = result.exit_code;
    return result;
}

if (require.main === module) cli(process.argv.slice(2));

module.exports = Object.freeze({validatePipelineRequest, parsePipelineArguments, runWorkerPipeline, cli});
