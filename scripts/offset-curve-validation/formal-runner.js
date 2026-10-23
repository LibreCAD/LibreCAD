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
const registryApi = require('./formal-registry');
const {LIMITS, RunnerError, ensure, readStableFile, recheckFile} = registryApi;

// This is an H3 execution boundary only. Outputs and dispatch diagnostics are
// returned in memory; no genesis, ledger, evidence manifest, or H4/H5 join is
// published. Workers use the explicit H2 dependency closure, not import discovery.
// Deadlines kill the direct child; descendant/process-tree containment is not
// provided. A worker must close inherited pipes and leave no background work.
// worker_launches counts dispatch attempts, including spawn errors, not PIDs.
// Checks are observational open/fstat/readback guards, not a filesystem sandbox.

function relation(a, b) {
    const relative = path.relative(a, b);
    return relative === '' || (relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
}

function absent(target, io, stage = 'ROOT_PREFLIGHT') {
    try { io.lstatSync(target); }
    catch (error) { if (error.code === 'ENOENT') return; throw error; }
    throw new RunnerError(stage, 'ROOT_ALREADY_EXISTS', `path already exists: ${target}`);
}

function environmentSnapshot() {
    return JSON.stringify(Object.keys(process.env).sort().map(key => [key, process.env[key]]));
}

function validateRequest(request) {
    ensure(request && typeof request === 'object', 'CLI', 'ARG_INVALID', 'run request is required');
    ensure(Array.isArray(request.receiptPaths) && request.receiptPaths.length === 2,
        'CLI', 'RECEIPT_COUNT', 'exactly two preflight receipt paths are required');
    ensure(new Set(request.receiptPaths).size === 2, 'CLI', 'RECEIPT_PATH_DUPLICATE', 'receipt paths must be distinct');
    for (const p of [request.configPath, request.runRoot, ...request.receiptPaths]) {
        ensure(typeof p === 'string' && path.isAbsolute(p), 'CLI', 'ARG_RELATIVE_PATH', 'all paths must be absolute');
        try { h2.canonicalPath(p); }
        catch (error) { throw new RunnerError('CLI', 'ARG_PATH_INVALID', error.message); }
    }
    return h2.freeze({configPath: request.configPath, runRoot: request.runRoot, receiptPaths: [...request.receiptPaths]});
}

function receiptError(error) {
    const message = error.message || String(error);
    if (error.code === 'ENOENT') return new RunnerError('RECEIPT_IO', 'RECEIPT_NOT_FOUND', message);
    const mappings = [
        [/receipt IDs must be distinct/, 'RECEIPT_JOIN', 'RECEIPT_ID_DUPLICATE'],
        [/receipt directories must be distinct|receipt directories must not alias/, 'RECEIPT_JOIN', 'RECEIPT_TREE_DUPLICATE'],
        [/schema\/revision|revision mismatch/, 'IDENTITY', 'REVISION_MISMATCH'],
        [/purpose/, 'IDENTITY', 'PURPOSE_MISMATCH'],
        [/candidate/, 'IDENTITY', 'CANDIDATE_MISMATCH'],
        [/manifest/, 'IDENTITY', 'MANIFEST_MISMATCH'],
        [/environment/, 'IDENTITY', 'ENVIRONMENT_MISMATCH'],
        [/argv/, 'IDENTITY', 'RUNNER_MISMATCH'],
        [/cwd/, 'IDENTITY', 'CWD_MISMATCH'],
        [/symlink|single-link|alias/, 'FILESYSTEM', 'PATH_NOT_REGULAR_SINGLE_LINK'],
        [/membership/, 'FILESYSTEM', 'RECEIPT_TREE_MEMBERSHIP'],
        [/mode|regular file|descriptor\/path/, 'FILESYSTEM', 'FILE_METADATA_MISMATCH'],
        [/forbidden|overlap|contain one another/, 'PATH_POLICY', 'PATH_POLICY_VIOLATION'],
        [/duplicate object key|canonical|keys\/order|BOM|CR|LF/, 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'],
        [/UTF-8|JSON|parse/, 'RECEIPT_PARSE', 'REPORT_PARSE_FAILED'],
        [/stdout/, 'RECEIPT_STREAM', 'STDOUT_SHAPE'],
        [/non-PASS|unsuccessful child|failed preflight|PASS preflight/, 'RECEIPT_RESULT', 'CHILD_NOT_PASS'],
        [/byte|digest|filename|basename/, 'RECEIPT_BINDING', 'FILE_BINDING_MISMATCH']
    ];
    const found = mappings.find(([pattern]) => pattern.test(message));
    return new RunnerError(found ? found[1] : 'RECEIPT_IO', found ? found[2] : 'RECEIPT_INVALID', message);
}

function checkReceiptIdentities(receipts, config, inputs, environmentSha, io) {
    const actualIds = receipts.receipts.map(row => row.receipt.receipt_id);
    ensure(JSON.stringify(actualIds) === JSON.stringify(config.receipt_ids), 'RECEIPT_JOIN', 'RECEIPT_ORDER_MISMATCH', 'receipt IDs/order differ from the pinned config');
    receipts.receipts.forEach((row, i) => {
        const receipt = row.receipt;
        ensure(JSON.stringify(receipt.candidate) === JSON.stringify(inputs.candidate), 'IDENTITY', 'CANDIDATE_MISMATCH', 'receipt candidate differs from actual bytes');
        ensure(receipt.source_manifest_sha256 === inputs.sourceManifestSha256, 'IDENTITY', 'MANIFEST_MISMATCH', 'receipt does not bind the actual H2 manifest');
        const inventory = receipt.preflight_result.inventory_gate;
        ensure(inventory.expected_count === inputs.caseIds.length && inventory.actual_count === inputs.caseIds.length &&
            inventory.expected_ids_sha256 === inputs.manifest.case_ids_sha256 &&
            inventory.actual_ids_sha256 === inputs.manifest.case_ids_sha256,
        'IDENTITY', 'CASE_INVENTORY_MISMATCH', 'receipt cases differ from the actual H2 inventory');
        ensure(receipt.argv[0] === inputs.runtime.executable_path, 'IDENTITY', 'NODE_MISMATCH', 'receipt runtime executable differs');
        ensure(JSON.stringify(receipt.argv) === JSON.stringify([inputs.runtime.executable_path, config.preflight.runnerPath, ...config.preflight.runnerArgs]),
            'IDENTITY', 'RUNNER_MISMATCH', 'receipt runner/arguments differ');
        ensure(receipt.cwd === config.preflight.cwd, 'IDENTITY', 'CWD_MISMATCH', 'receipt cwd differs');
        ensure(receipt.environment_sha256 === environmentSha, 'IDENTITY', 'ENVIRONMENT_MISMATCH', 'receipt environment differs');
        ensure(receipt.run_root.path === config.receipt_roots[i], 'IDENTITY', 'RUN_ROOT_MISMATCH', 'receipt names an unexpected reserved root');
        h2.directoryGuards(receipt.run_root.parent.path, io);
        const parent = io.lstatSync(receipt.run_root.parent.path);
        ensure(String(parent.dev) === receipt.run_root.parent.device && String(parent.ino) === receipt.run_root.parent.inode,
            'ROOT_PREFLIGHT', 'PARENT_IDENTITY_CHANGED', 'original receipt run-root parent changed');
        absent(receipt.run_root.path, io);
    });
}

function readOutputs(runRoot, names, io) {
    const before = io.lstatSync(runRoot);
    const directory = io.opendirSync(runRoot);
    const actual = [];
    try {
        for (;;) {
            const entry = directory.readSync();
            if (entry === null) break;
            ensure(actual.length < LIMITS.outputs, 'OUTPUT', 'OUTPUT_INVENTORY_MISMATCH', 'too many output entries');
            actual.push(entry.name);
        }
    } finally { directory.closeSync(); }
    ensure(actual.every(name => names.includes(name)), 'OUTPUT', 'OUTPUT_UNEXPECTED', 'actual output inventory has undeclared entries');
    ensure(names.every(name => actual.includes(name)), 'OUTPUT', 'OUTPUT_MISSING', 'actual output inventory is incomplete');
    let total = 0;
    const files = [...names].sort().map(name => {
        const p = path.join(runRoot, name);
        let read;
        try { read = readStableFile(p, LIMITS.outputBytes, io); }
        catch (error) { throw new RunnerError('OUTPUT', 'OUTPUT_METADATA_MISMATCH', error.message); }
        ensure(read.mode === 0o400 && Number(io.lstatSync(p).mode & 0o7777n) === 0o400,
            'OUTPUT', 'OUTPUT_METADATA_MISMATCH', 'output mode must be 0400');
        total += read.byte_length;
        ensure(total <= LIMITS.totalOutputBytes, 'OUTPUT', 'OUTPUT_CAP_EXCEEDED', 'output inventory exceeds total byte cap');
        return h2.freeze({name, ...read});
    });
    ensure(h2.statIdentity(io.lstatSync(runRoot)) === h2.statIdentity(before), 'REVALIDATION', 'OUTPUT_CHANGED', 'output directory changed during readback');
    for (const file of files) recheckFile(file, io);
    ensure(h2.statIdentity(io.lstatSync(runRoot)) === h2.statIdentity(before), 'REVALIDATION', 'OUTPUT_CHANGED', 'output directory changed after readback');
    return Object.freeze(files);
}

function safeError(error) {
    const text = (value, fallback, limit) => {
        try {
            // Normalize after truncation so a cut surrogate cannot break CLI JSON.
            return Buffer.from(String(value).slice(0, limit)).toString('utf8').replace(/\0/g, '') || fallback;
        } catch { return fallback; }
    };
    let code;
    let message = error;
    try {
        code = error && error.code;
        if (error && error.message !== undefined) message = error.message;
    } catch { message = 'unreadable thrown value'; }
    return {code: text(code || 'PROCESS_ERROR', 'PROCESS_ERROR', 128), message: text(message, 'unknown error', 2048)};
}

function dispatchChild(worker, contextBytes, executable, environment, cwd, spawnProcess) {
    return new Promise(resolve => {
        const chunks = {stdout: [], stderr: []};
        const retained = {stdout: 0, stderr: 0};
        const observed = {stdout: 0, stderr: 0};
        let child;
        let error = null;
        let failure = null;
        let timer;
        let finished = false;
        let stdinComplete = false;
        const finish = (exitCode, signal) => {
            if (finished) return;
            finished = true;
            clearTimeout(timer);
            resolve(h2.freeze({mode: 'process', child: {exit_code: exitCode, signal: signal || null, error}, failure,
                stdin: {...h2.bytesSnapshot(contextBytes), write_completed: stdinComplete}, observed,
                stdout: h2.bytesSnapshot(Buffer.concat(chunks.stdout, retained.stdout)),
                stderr: h2.bytesSnapshot(Buffer.concat(chunks.stderr, retained.stderr))}));
        };
        const stop = reason => {
            failure = failure || reason;
            try { child.kill('SIGKILL'); } catch (cause) { error = error || safeError(cause); }
        };
        try {
            child = spawnProcess(executable, [worker.module_path, ...worker.args], {cwd, env: environment,
                shell: false, windowsHide: true, stdio: ['pipe', 'pipe', 'pipe']});
        } catch (cause) { error = safeError(cause); failure = 'spawn threw'; finish(null, null); return; }
        if (!child || !child.stdin || typeof child.stdin.on !== 'function' || typeof child.stdin.end !== 'function' ||
            !child.stdout || typeof child.stdout.on !== 'function' || !child.stderr || typeof child.stderr.on !== 'function' ||
            typeof child.on !== 'function' || typeof child.kill !== 'function') {
            error = {code: 'INVALID_CHILD', message: 'invalid child process'};
            failure = 'invalid child process';
            if (child && typeof child.kill === 'function') {
                try { child.kill('SIGKILL'); } catch { /* Keep the invalid-child diagnosis. */ }
            }
            finish(null, null);
            return;
        }
        for (const name of ['stdout', 'stderr']) {
            child[name].on('data', chunk => {
                const bytes = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
                observed[name] = Math.min(Number.MAX_SAFE_INTEGER, observed[name] + bytes.length);
                const count = Math.min(bytes.length, worker.max_stream_bytes - retained[name]);
                if (count > 0) { chunks[name].push(Buffer.from(bytes.subarray(0, count))); retained[name] += count; }
                if (observed[name] > worker.max_stream_bytes) stop(`${name} cap exceeded`);
            });
            child[name].on('error', cause => { error = error || safeError(cause); stop(`${name} stream error`); });
        }
        child.on('error', cause => { error = safeError(cause); });
        child.on('close', finish);
        child.stdin.on('error', cause => { error = error || safeError(cause); stop('stdin error'); });
        timer = setTimeout(() => stop('worker timeout'), worker.timeout_ms);
        try {
            child.stdin.end(contextBytes, cause => {
                if (cause) { error = error || safeError(cause); stop('stdin error'); }
                else stdinComplete = true;
            });
        } catch (cause) { error = error || safeError(cause); stop('stdin threw'); }
    });
}

function syncDirectory(p, io, limitations) {
    const guards = h2.directoryGuards(p, io);
    const fd = io.openSync(p, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
    try {
        ensure(h2.directoryIdentity(io.fstatSync(fd)) === guards[guards.length - 1].identity,
            'POST_ROOT', 'PARENT_IDENTITY_CHANGED', 'directory descriptor changed');
        try { io.fsyncSync(fd); }
        catch (error) {
            if (!['EINVAL', 'ENOTSUP', 'EOPNOTSUPP', 'ENOSYS'].includes(error.code)) throw error;
            const row = `${p}: directory fsync unavailable (${error.code})`;
            if (!limitations.includes(row)) limitations.push(row);
        }
        h2.checkDirectoryGuards(guards, io);
    } finally { io.closeSync(fd); }
}

// Boundary order: request, raw config/H2 inputs/registry, H1 tree/pair, pinned
// IDs and tuple, pre-root revalidation, exclusive CAS, then per worker process
// completion and input rechecks, strict result/events, and actual output readback.
async function runFormalTransaction(requestInput, dependencies = {}) {
    let io = h2.nodeFsAdapter;
    const beforeEnvironment = environmentSnapshot();
    let request;
    let config;
    let inputs;
    let registryFile;
    let hooks;
    let rootCreated = false;
    let workerLaunches = 0;
    let verify;
    let lastDispatch = null;
    let outputFiles = [];
    const workers = [];
    const verificationFailures = [];
    const limitations = [];
    const result = (status, stage, reason, message) => h2.freeze({schema_version: 1, purpose: config ? config.purpose : null,
        status, stage, reason, message, exit_code: status === 'PASS' ? 0 : status === 'REFUSED' ? 2 : 1,
        run_root: request ? request.runRoot : null, root_created: rootCreated, worker_launches: workerLaunches,
        source_manifest_sha256: inputs ? inputs.sourceManifestSha256 : null,
        registry_sha256: registryFile ? registryFile.sha256 : null, workers, last_dispatch: lastDispatch,
        durability_limitations: limitations, verification_failures: verificationFailures});
    try {
        ensure(dependencies && typeof dependencies === 'object' && !Array.isArray(dependencies),
            'REGISTRY', 'SCHEMA_INVALID', 'dependencies must be an explicit object');
        request = validateRequest(requestInput);
        // Only trusted I/O may establish whether synthetic dependency hooks are permitted.
        const configFile = readStableFile(request.configPath, LIMITS.configBytes, h2.nodeFsAdapter);
        config = registryApi.validateConfiguration(h1.parseCanonicalJson(Buffer.from(configFile.base64, 'base64'), {maxBytes: LIMITS.configBytes}));
        const descriptors = Object.getOwnPropertyDescriptors(dependencies);
        const keys = Reflect.ownKeys(descriptors);
        const allowed = ['fs', 'spawn', 'dispatch', 'beforeCreate'];
        const prototype = Object.getPrototypeOf(dependencies);
        const plain = prototype === Object.prototype || prototype === null;
        const inherited = allowed.some(name => !Object.hasOwn(descriptors, name) && name in dependencies);
        ensure(config.purpose === 'synthetic' || (keys.length === 0 && plain && !inherited),
            'REGISTRY', 'SYNTHETIC_INJECTION_FORBIDDEN', 'dependency injection is restricted to synthetic controls');
        ensure(plain && !inherited && keys.every(key => allowed.includes(key)) &&
            keys.every(key => Object.hasOwn(descriptors[key], 'value')),
        'REGISTRY', 'SCHEMA_INVALID', 'synthetic hooks must be own data properties on a plain object');
        for (const key of keys) ensure(key === 'fs' ? descriptors[key].value &&
            typeof descriptors[key].value === 'object' && !Array.isArray(descriptors[key].value) :
            typeof descriptors[key].value === 'function', 'REGISTRY', 'SCHEMA_INVALID', `invalid synthetic hook: ${key}`);
        const selected = Object.create(null);
        for (const key of keys) selected[key] = descriptors[key].value;
        hooks = Object.freeze(selected);
        io = Object.hasOwn(hooks, 'fs') ? hooks.fs : h2.nodeFsAdapter;
        recheckFile(configFile, io);
        ensure(request.runRoot === config.run_root, 'IDENTITY', 'RUN_ROOT_MISMATCH', 'formal root differs from pinned configuration');
        const rootParentGuards = h2.directoryGuards(path.dirname(request.runRoot), io);
        const cwdGuards = h2.directoryGuards(config.preflight.cwd, io);
        absent(request.runRoot, io);
        inputs = h2.captureInputs(config.preflight, {io});
        const pathsWithRole = role => inputs.manifest.files.filter(row => row.roles.includes(role)).map(row => row.path);
        ensure([request.configPath, config.registry_path].every(p => pathsWithRole('config').includes(p)),
            'REGISTRY', 'CONFIG_UNPINNED', 'formal config and registry must be actual H2 configPaths');
        for (const name of ['formal-registry.js', 'formal-runner.js', 'run-formal-gates.js'])
            ensure(pathsWithRole('source').includes(path.join(__dirname, name)), 'REGISTRY', 'RUNNER_SOURCE_UNPINNED', 'H3 source modules must be declared in H2 sourcePaths');
        registryFile = readStableFile(config.registry_path, LIMITS.registryBytes, io);
        const registry = registryApi.validateRegistry(h1.parseCanonicalJson(Buffer.from(registryFile.base64, 'base64'), {maxBytes: LIMITS.registryBytes}), inputs);
        recheckFile(configFile, io);
        const environment = normalizedEnvironment(config.preflight.environment);
        if (process.platform === 'darwin') ensure(environment.__CF_USER_TEXT_ENCODING === process.env.__CF_USER_TEXT_ENCODING &&
            typeof environment.__CF_USER_TEXT_ENCODING === 'string', 'IDENTITY', 'ENVIRONMENT_MISMATCH', 'macOS encoding environment must be explicitly pinned');
        const environmentSha = h1.sha256(h1.canonicalJson(environment));
        const runtimeSource = inputs.manifest.files.find(row => row.roles.includes('runtime-executable'));
        const dispatchPlans = registry.workers.map((worker, index) => {
            const context = h2.freeze({schema_version: 1, purpose: config.purpose, worker_id: worker.id, ordinal: index + 1,
                case_ids: worker.case_ids, outputs: worker.outputs, run_root: request.runRoot,
                candidate: inputs.candidate, source_manifest_sha256: inputs.sourceManifestSha256, registry_sha256: registryFile.sha256});
            const contextBytes = h1.canonicalJson(context);
            ensure(contextBytes.length <= LIMITS.contextBytes, 'REGISTRY', 'LIMIT_INVALID', 'worker stdin context exceeds byte cap');
            return {context, contextBytes, workerSource: inputs.manifest.files.find(row => row.path === worker.module_path)};
        });
        for (const p of inputs.manifest.files.map(row => row.path))
            ensure(!relation(request.runRoot, p) && !relation(p, request.runRoot), 'PATH_POLICY', 'PATH_POLICY_VIOLATION', 'formal root overlaps an actual input');
        const receiptOptions = {io, expected: {purpose: config.purpose}, requirePass: true,
            forbiddenRoots: [request.runRoot, __dirname, ...inputs.manifest.files.map(row => row.path)]};
        let receipts;
        try { receipts = h1.validateReceiptSet(request.receiptPaths, receiptOptions); }
        catch (error) { throw receiptError(error); }
        const receiptParentGuards = receipts.receipts.flatMap(row => [
            h2.directoryGuards(row.receipt.run_root.parent.path, io),
            h2.directoryGuards(path.dirname(row.receiptDirectory), io)
        ]);
        checkReceiptIdentities(receipts, config, inputs, environmentSha, io);
        let rootGuards;
        let ownedRootIdentity;
        verify = stage => {
            const checks = [
                ['inputs', () => { h2.revalidateInputs(inputs, {io}); recheckFile(configFile, io); recheckFile(registryFile, io); }, 'REVALIDATION', 'INPUT_CHANGED'],
                ['receipts', () => {
                    for (const guards of receiptParentGuards) h2.checkDirectoryGuards(guards, io);
                    h1.revalidateReceiptSet(receipts, receiptOptions);
                    checkReceiptIdentities(receipts, config, inputs, environmentSha, io);
                    for (const guards of receiptParentGuards) h2.checkDirectoryGuards(guards, io);
                }, 'REVALIDATION', 'RECEIPT_CHANGED'],
                ['root_parent', () => h2.checkDirectoryGuards(rootParentGuards, io), rootCreated ? 'POST_ROOT' : 'ROOT_PREFLIGHT', 'PARENT_IDENTITY_CHANGED'],
                ['root', () => {
                    if (!rootCreated) absent(request.runRoot, io);
                    else {
                        h2.checkDirectoryGuards(rootGuards, io);
                        ensure(h2.directoryIdentity(io.lstatSync(request.runRoot)) === ownedRootIdentity,
                            'POST_ROOT', 'ROOT_CHANGED', 'owned root identity changed');
                    }
                }, rootCreated ? 'POST_ROOT' : 'ROOT_PREFLIGHT', rootCreated ? 'ROOT_CHANGED' : 'ROOT_ALREADY_EXISTS'],
                ['cwd', () => h2.checkDirectoryGuards(cwdGuards, io), 'REVALIDATION', 'CWD_CHANGED'],
                ['environment', () => ensure(environmentSnapshot() === beforeEnvironment, 'REVALIDATION', 'ENVIRONMENT_CHANGED', 'parent environment changed'), 'REVALIDATION', 'ENVIRONMENT_CHANGED'],
                ['output_prefix', () => { for (const file of outputFiles) recheckFile(file, io); }, 'REVALIDATION', 'OUTPUT_CHANGED']
            ];
            const failures = [];
            for (const [name, check, boundary, reason] of checks) {
                try { check(); }
                catch (error) { failures.push({observation: stage, check: name, stage: boundary, reason, message: error.message}); }
            }
            verificationFailures.push(...failures);
            if (failures.length) throw new RunnerError(failures[0].stage, failures[0].reason, failures.map(row => row.message).join('; '));
        };
        if (hooks.beforeCreate) await hooks.beforeCreate();
        verify('pre-root');
        try { io.mkdirSync(request.runRoot, {mode: 0o700}); }
        catch (error) { if (error.code === 'EEXIST') throw new RunnerError('ROOT_CAS', 'ROOT_ALREADY_EXISTS', 'external root appeared at exclusive creation'); throw error; }
        rootCreated = true;
        h2.checkDirectoryGuards(rootParentGuards, io);
        const rootBefore = io.lstatSync(request.runRoot);
        ensure(rootBefore.isDirectory() && !rootBefore.isSymbolicLink(), 'POST_ROOT', 'ROOT_CHANGED', 'new root is not a real directory');
        const fd = io.openSync(request.runRoot, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
        try {
            ensure(h2.directoryIdentity(io.fstatSync(fd)) === h2.directoryIdentity(rootBefore), 'POST_ROOT', 'ROOT_CHANGED', 'new root changed while opening');
            io.fchmodSync(fd, 0o700);
            ownedRootIdentity = h2.directoryIdentity(io.fstatSync(fd));
        } finally { io.closeSync(fd); }
        rootGuards = h2.directoryGuards(request.runRoot, io);
        ensure(rootGuards[rootGuards.length - 1].identity === ownedRootIdentity,
            'POST_ROOT', 'ROOT_CHANGED', 'new root was replaced after exclusive creation/open');
        ensure(Number(io.lstatSync(request.runRoot).mode & 0o7777n) === 0o700, 'POST_ROOT', 'ROOT_CHANGED', 'formal root mode differs');
        syncDirectory(path.dirname(request.runRoot), io, limitations);
        outputFiles = readOutputs(request.runRoot, [], io);
        for (let index = 0; index < registry.workers.length; index++) {
            const worker = registry.workers[index];
            verify('pre-dispatch');
            readOutputs(request.runRoot, outputFiles.map(row => row.name), io);
            for (const saved of outputFiles) recheckFile(saved, io);
            const {context, contextBytes, workerSource} = dispatchPlans[index];
            workerLaunches++;
            let workerResult;
            if (hooks.dispatch) {
                lastDispatch = {mode: 'synthetic-callback', worker_id: worker.id, ordinal: index + 1,
                    worker_source: workerSource, runtime: {...inputs.runtime, executable: runtimeSource},
                    stdin: h2.bytesSnapshot(contextBytes), child: null, stdout: h2.bytesSnapshot(Buffer.alloc(0)), stderr: h2.bytesSnapshot(Buffer.alloc(0))};
                try { workerResult = await hooks.dispatch(context, worker); }
                catch (error) {
                    lastDispatch.exception = safeError(error);
                    verify('post-dispatch-exception');
                    throw new RunnerError('WORKER', 'WORKER_THROWN', lastDispatch.exception.message);
                }
            } else {
                const diagnostics = await dispatchChild(worker, contextBytes, inputs.runtime.executable_path,
                    environment, config.preflight.cwd, hooks.spawn || spawn);
                lastDispatch = {worker_id: worker.id, ordinal: index + 1,
                    worker_source: workerSource, runtime: {...inputs.runtime, executable: runtimeSource},
                    argv: [inputs.runtime.executable_path, worker.module_path, ...worker.args], environment_sha256: environmentSha, ...diagnostics};
                verify('post-dispatch');
                ensure(!diagnostics.failure && diagnostics.stdin.write_completed && diagnostics.child.exit_code === 0 &&
                    diagnostics.child.signal === null && diagnostics.child.error === null,
                'WORKER', diagnostics.child.error ? 'WORKER_PROCESS_ERROR' : 'CHILD_NOT_PASS', diagnostics.failure || 'worker process did not succeed');
                const stdout = Buffer.from(diagnostics.stdout.base64, 'base64');
                ensure(stdout.length > 0, 'WORKER', 'WORKER_RESULT_MISSING', 'worker stdout result is missing');
                try { workerResult = h1.parseCanonicalJson(stdout, {maxBytes: LIMITS.resultBytes, label: 'worker result'}); }
                catch (error) { throw new RunnerError('WORKER', 'WORKER_RESULT_PARSE_FAILED', error.message); }
            }
            verify('post-dispatch');
            const checked = registryApi.validateWorkerResult(workerResult, worker, index + 1, config.purpose);
            const names = [...outputFiles.map(row => row.name), ...worker.outputs];
            const current = readOutputs(request.runRoot, names, io);
            for (const saved of outputFiles) recheckFile(saved, io);
            outputFiles = Object.freeze([...outputFiles, ...current.filter(row => worker.outputs.includes(row.name))]);
            syncDirectory(request.runRoot, io, limitations);
            verify('post-output-readback');
            workers.push(h2.freeze({ordinal: index + 1, worker_id: worker.id, result: checked,
                dispatch: lastDispatch, outputs: current.filter(row => worker.outputs.includes(row.name))}));
        }
        verify('complete');
        readOutputs(request.runRoot, outputFiles.map(row => row.name), io);
        verify('final-readback');
        return result('PASS', 'COMPLETE', 'H3_DISPATCH_COMPLETE', 'strict H3 dispatch completed; no H4/H5 persistence is claimed');
    } catch (error) {
        if (verify) { try { verify('failure'); } catch { /* Retain all failed boundary checks and the original refusal. */ } }
        return result(rootCreated ? 'FAIL' : 'REFUSED', error instanceof RunnerError ? error.stage : rootCreated ? 'POST_ROOT' : 'PREFLIGHT',
            error instanceof RunnerError ? error.reason : 'INPUT_INVALID', safeError(error).message);
    }
}

module.exports = Object.freeze({RunnerError, validateRequest, runFormalTransaction});
