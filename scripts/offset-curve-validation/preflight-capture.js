/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const path = require('node:path');
const {spawn} = require('node:child_process');
const h1 = require('./preflight-receipt');
const input = require('./input-capture');
const {nodeFsAdapter, canonicalPath, string, freeze, directoryIdentity,
    directoryGuards, checkDirectoryGuards, bytesSnapshot} = input;

const MAX_CONTEXT_BYTES = 32768;
const MAX_ENV_BYTES = 65536;
const MAX_TIMEOUT_MS = 300000;
const BLOCKED_ENVIRONMENT_KEYS = Object.freeze(['ENV', 'BASH_ENV', 'OPENSSL_CONF', 'OPENSSL_MODULES']);

class CaptureError extends Error {
    constructor(stage, message, details = {}) {
        super(`${stage}: ${message}`);
        this.name = 'CaptureError';
        this.stage = stage;
        this.details = freeze(details);
    }
}

function check(condition, stage, message) {
    if (!condition) throw new CaptureError(stage, message);
}

function normalizedEnvironment(environment) {
    check(environment && typeof environment === 'object' && !Array.isArray(environment), 'configuration', 'explicit environment is required');
    const keys = Object.keys(environment).sort();
    check(keys.length <= 64, 'configuration', 'too many environment keys');
    const normalized = {};
    for (const key of keys) {
        check(/^[A-Za-z_][A-Za-z0-9_]*$/.test(key) && key !== 'LC_OFFSET_CAPTURE_CONTEXT' &&
            !/^(NODE_|LD_|DYLD_)/.test(key) && !BLOCKED_ENVIRONMENT_KEYS.includes(key),
        'configuration', `unsafe environment key: ${key}`);
        const value = environment[key];
        check(typeof value === 'string', 'configuration', `environment ${key} must be a string`);
        if (value !== '') string(value, `environment ${key}`, 8192);
        Object.defineProperty(normalized, key, {value, enumerable: true});
    }
    check(h1.canonicalJson(normalized).length <= MAX_ENV_BYTES, 'configuration', 'environment exceeds byte cap');
    return Object.freeze(normalized);
}

function parentEnvironment() {
    return JSON.stringify(Object.keys(process.env).sort().map(key => [key, process.env[key]]));
}

function absent(target, io) {
    try { io.lstatSync(target); }
    catch (error) { if (error.code === 'ENOENT') return; throw error; }
    throw new TypeError(`path already exists: ${target}`);
}

function overlaps(a, b) {
    const relative = path.relative(a, b);
    return relative === '' || (relative !== '..' && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
}

function safeChildError(error) {
    const clean = (value, size) => String(value).replace(/\0/g, '').replace(/[\uD800-\uDFFF]/g, '?').slice(0, size);
    return {code: clean(error.code || 'SPAWN_ERROR', 256), message: clean(error.message || error, 1024)};
}

function collectChild(config, spawnProcess) {
    return new Promise(resolve => {
        const chunks = {stdout: [], stderr: []};
        const retained = {stdout: 0, stderr: 0};
        const observed = {stdout: 0, stderr: 0};
        let error = null;
        let failure = null;
        let child;
        let timer;
        let finished = false;
        let stdinWriteCompleted = false;
        function finish(exitCode, signal) {
            if (finished) return;
            finished = true;
            clearTimeout(timer);
            resolve({child: {exit_code: exitCode, signal: signal || null, error}, failure, observed, stdinWriteCompleted,
                stdout: Buffer.concat(chunks.stdout, retained.stdout), stderr: Buffer.concat(chunks.stderr, retained.stderr)});
        }
        function stop(reason) {
            failure = failure || reason;
            try { child.kill('SIGKILL'); } catch (killError) { error = error || safeChildError(killError); }
        }
        try {
            child = spawnProcess(config.executable, config.args, {cwd: config.cwd, env: config.environment,
                shell: false, windowsHide: true, stdio: ['pipe', 'pipe', 'pipe']});
        } catch (spawnError) {
            error = safeChildError(spawnError);
            failure = 'spawn threw';
            finish(null, null);
            return;
        }
        if (!child || !child.stdin || typeof child.stdin.end !== 'function' || typeof child.stdin.on !== 'function' || !child.stdout || !child.stderr ||
            typeof child.on !== 'function' || typeof child.kill !== 'function') {
            error = {code: 'INVALID_CHILD', message: 'spawn returned an invalid child process'};
            failure = 'invalid child process';
            if (child && typeof child.kill === 'function') child.kill('SIGKILL');
            finish(null, null);
            return;
        }
        for (const name of ['stdout', 'stderr']) {
            child[name].on('data', chunk => {
                const bytes = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
                observed[name] = Math.min(Number.MAX_SAFE_INTEGER, observed[name] + bytes.length);
                const count = Math.min(bytes.length, config.maxStreamBytes - retained[name]);
                if (count > 0) {
                    chunks[name].push(Buffer.from(bytes.subarray(0, count)));
                    retained[name] += count;
                }
                if (observed[name] > config.maxStreamBytes) stop(`${name} exceeded stream byte cap`);
            });
            child[name].on('error', streamError => { error = error || safeChildError(streamError); stop(`${name} stream error`); });
        }
        child.on('error', spawnError => { error = safeChildError(spawnError); });
        child.on('close', finish);
        timer = setTimeout(() => stop('child timeout'), config.timeoutMs);
        child.stdin.on('error', stdinError => { error = error || safeChildError(stdinError); stop('stdin write error'); });
        try {
            child.stdin.end(config.contextBytes, stdinError => {
                if (stdinError) { error = error || safeChildError(stdinError); stop('stdin write error'); }
                else stdinWriteCompleted = true;
            });
        } catch (stdinError) {
            error = error || safeChildError(stdinError);
            stop('stdin write threw');
        }
    });
}

function fsyncDirectory(directory, expected, io, limitations) {
    const guards = directoryGuards(directory, io);
    check(guards[guards.length - 1].identity === expected, 'publish', `directory identity changed: ${directory}`);
    const fd = io.openSync(directory, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
    try {
        check(directoryIdentity(io.fstatSync(fd)) === expected, 'publish', 'directory descriptor changed before fsync');
        try { io.fsyncSync(fd); }
        catch (error) {
            if (!['EINVAL', 'ENOTSUP', 'EOPNOTSUPP', 'ENOSYS'].includes(error.code)) throw error;
            const limitation = `${directory}: directory fsync unavailable (${error.code})`;
            if (!limitations.includes(limitation)) limitations.push(limitation);
        }
        check(directoryIdentity(io.fstatSync(fd)) === expected, 'publish', 'directory descriptor changed during fsync');
        checkDirectoryGuards(guards, io);
    } finally { io.closeSync(fd); }
}

function writeExclusive(filePath, bytes, io) {
    const fd = io.openSync(filePath, io.constants.O_WRONLY | io.constants.O_CREAT | io.constants.O_EXCL | io.constants.O_NOFOLLOW, 0o600);
    try {
        const opened = io.fstatSync(fd);
        check(opened.isFile() && opened.nlink === 1n, 'publish', 'new file is not regular/single-link');
        let offset = 0;
        while (offset < bytes.length) {
            const count = io.writeSync(fd, bytes, offset, bytes.length - offset, null);
            check(Number.isInteger(count) && count > 0 && count <= bytes.length - offset, 'publish', 'invalid/zero write count');
            offset += count;
        }
        io.fchmodSync(fd, h1.RECEIPT_FILE_MODE);
        io.fsyncSync(fd);
        const after = io.fstatSync(fd);
        check(after.dev === opened.dev && after.ino === opened.ino && after.nlink === 1n &&
            Number(after.mode & 0o7777n) === h1.RECEIPT_FILE_MODE && after.size === BigInt(bytes.length) &&
            input.statIdentity(io.lstatSync(filePath)) === input.statIdentity(after), 'publish', 'published file identity/metadata changed');
        return input.statIdentity(after);
    } finally { io.closeSync(fd); }
}

async function capturePreflight(config, dependencies = {}) {
    let inputs;
    let child;
    let verifyBoundary;
    let created = false;
    let receiptDirectory;
    let stdinSnapshot;
    const verificationFailures = [];
    const durabilityLimitations = [];
    const observations = [];
    try {
        check(config && typeof config === 'object' && !Array.isArray(config) &&
            [Object.prototype, null].includes(Object.getPrototypeOf(config)),
        'configuration', 'configuration must be a plain object');
        const configDescriptors = Object.getOwnPropertyDescriptors(config);
        const configKeys = Reflect.ownKeys(configDescriptors);
        check(configKeys.every(key => typeof key === 'string' && Object.hasOwn(configDescriptors[key], 'value')),
            'configuration', 'configuration must contain only own data properties');
        // Pin purpose once, before authorizing hooks; callbacks cannot relabel their synthetic run.
        config = Object.freeze(Object.assign(Object.create(null), Object.fromEntries(
            configKeys.map(key => [key, configDescriptors[key].value]))));
        check(config && ['validation', 'synthetic'].includes(config.purpose), 'configuration', 'explicit capture purpose is required');
        check(dependencies && typeof dependencies === 'object' && !Array.isArray(dependencies),
            'configuration', 'dependencies must be an explicit object');
        const prototype = Object.getPrototypeOf(dependencies);
        const plain = prototype === Object.prototype || prototype === null;
        // Snapshot descriptors without evaluating hooks, including non-enumerable ones.
        const descriptors = Object.getOwnPropertyDescriptors(dependencies);
        const keys = Reflect.ownKeys(descriptors);
        const inherited = ['fs', 'spawn'].some(key => !Object.hasOwn(descriptors, key) && key in dependencies);
        check(config.purpose === 'synthetic' || (keys.length === 0 && plain && !inherited),
            'configuration', 'dependency injection is restricted to synthetic controls');
        check(plain && !inherited && keys.every(key => key === 'fs' || key === 'spawn'),
            'configuration', 'synthetic dependencies must be own fs/spawn properties on a plain object');
        check(keys.every(key => Object.hasOwn(descriptors[key], 'value')),
            'configuration', 'synthetic dependencies must be data properties');
        check(!Object.hasOwn(descriptors, 'spawn') || typeof descriptors.spawn.value === 'function',
            'configuration', 'synthetic spawn dependency must be a function');
        check(!Object.hasOwn(descriptors, 'fs') || (descriptors.fs.value &&
            typeof descriptors.fs.value === 'object' && !Array.isArray(descriptors.fs.value)),
        'configuration', 'synthetic fs dependency must be an object');
        const io = Object.hasOwn(descriptors, 'fs') ? descriptors.fs.value : nodeFsAdapter;
        const spawnProcess = Object.hasOwn(descriptors, 'spawn') ? descriptors.spawn.value : spawn;
        const environmentBefore = parentEnvironment();
        const baseEnvironment = normalizedEnvironment(config.environment);
        if (process.platform === 'darwin')
            check(typeof process.env.__CF_USER_TEXT_ENCODING === 'string' &&
                baseEnvironment.__CF_USER_TEXT_ENCODING === process.env.__CF_USER_TEXT_ENCODING,
            'configuration', 'macOS requires explicitly pinned __CF_USER_TEXT_ENCODING from the current runtime');
        check(Array.isArray(config.runnerArgs) && config.runnerArgs.length <= 128, 'configuration', 'runnerArgs must be an explicit bounded array');
        const runnerArgs = [...config.runnerArgs];
        for (const arg of runnerArgs) if (arg !== '') string(arg, 'runner argument', 8192);
        check(runnerArgs.every(arg => typeof arg === 'string') && h1.canonicalJson(runnerArgs).length <= 32768,
            'configuration', 'invalid/oversized runner arguments');
        const cwd = canonicalPath(config.cwd, 'cwd');
        receiptDirectory = canonicalPath(config.receiptDirectory, 'receiptDirectory');
        const runRoot = canonicalPath(config.runRoot, 'runRoot');
        check(path.dirname(runRoot) !== runRoot && path.dirname(receiptDirectory) !== receiptDirectory,
            'configuration', 'output paths cannot be filesystem roots');
        check(!overlaps(runRoot, receiptDirectory) && !overlaps(receiptDirectory, runRoot), 'configuration', 'receipt directory and run root overlap');
        const receiptId = config.receiptId;
        check(typeof receiptId === 'string' && /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/.test(receiptId),
            'configuration', 'receiptId must be a canonical UUIDv4');
        const maxStreamBytes = config.maxStreamBytes === undefined ? h1.MAX_STREAM_BYTES : config.maxStreamBytes;
        const timeoutMs = config.timeoutMs === undefined ? MAX_TIMEOUT_MS : config.timeoutMs;
        check(Number.isSafeInteger(maxStreamBytes) && maxStreamBytes > 0 && maxStreamBytes <= h1.MAX_STREAM_BYTES,
            'configuration', 'stream byte cap is invalid');
        check(Number.isSafeInteger(timeoutMs) && timeoutMs > 0 && timeoutMs <= MAX_TIMEOUT_MS, 'configuration', 'timeout is invalid');
        inputs = input.captureInputs(config, {io});
        const purpose = inputs.manifest.purpose;
        const runnerPath = canonicalPath(config.runnerPath);
        check(purpose === 'synthetic' || runnerPath !== path.join(__dirname, 'fixtures', 'capture-child.js'),
            'configuration', 'capture-child is a synthetic fixture, not a validation runner');
        for (const row of inputs.snapshots)
            check(!overlaps(receiptDirectory, row.path) && !overlaps(runRoot, row.path) &&
                !overlaps(row.path, receiptDirectory) && !overlaps(row.path, runRoot), 'configuration', 'output path overlaps a captured input');
        const runParent = path.dirname(runRoot);
        const receiptParent = path.dirname(receiptDirectory);
        const runGuards = directoryGuards(runParent, io);
        const receiptGuards = directoryGuards(receiptParent, io);
        const cwdGuards = directoryGuards(cwd, io);
        // This original bigint sample is retained through every success/failure boundary.
        const originalParent = io.lstatSync(runParent);
        check(directoryIdentity(originalParent) === runGuards[runGuards.length - 1].identity,
            'pre-spawn', 'run-root parent changed during initial sample');
        const rootProof = {path: runRoot, absent_before: true, absent_after: true, parent_stable: true,
            parent: {path: runParent, device: String(originalParent.dev), inode: String(originalParent.ino)},
            write_attempts: 0, error: null};
        verifyBoundary = stage => {
            const failures = [];
            const checks = {
                root: () => { checkDirectoryGuards(runGuards, io); absent(runRoot, io); },
                receipt_parent: () => checkDirectoryGuards(receiptGuards, io),
                cwd: () => checkDirectoryGuards(cwdGuards, io),
                inputs: () => input.revalidateInputs(inputs, {io}),
                parent_environment: () => check(parentEnvironment() === environmentBefore, stage, 'parent environment changed')
            };
            for (const [name, verify] of Object.entries(checks)) {
                try { verify(); }
                catch (error) { failures.push({stage, check: name, message: error.message, code: error.code || null}); }
            }
            verificationFailures.push(...failures);
            observations.push(stage);
            if (failures.length) throw new CaptureError(stage, failures.map(row => `${row.check}: ${row.message}`).join('; '));
        };
        const context = {purpose, candidate: inputs.candidate, source_manifest_sha256: inputs.sourceManifestSha256,
            case_inventory_path: inputs.manifest.case_inventory_path, case_count: inputs.caseIds.length,
            case_ids_sha256: inputs.manifest.case_ids_sha256, runtime: inputs.runtime, run_root: rootProof};
        const contextBytes = h1.canonicalJson(context);
        check(contextBytes.length <= MAX_CONTEXT_BYTES, 'configuration', 'stdin context exceeds byte cap');
        stdinSnapshot = bytesSnapshot(contextBytes);
        const environment = baseEnvironment;
        const args = Object.freeze([runnerPath, ...runnerArgs]);
        const argv = [inputs.runtime.executable_path, ...args];
        const expected = {purpose, candidate: inputs.candidate, sourceManifestSha256: inputs.sourceManifestSha256,
            argv, cwd, environmentSha256: h1.sha256(h1.canonicalJson(environment))};
        absent(receiptDirectory, io);
        verifyBoundary('pre-spawn');
        child = await collectChild({executable: inputs.runtime.executable_path, args, cwd, environment,
            contextBytes, maxStreamBytes, timeoutMs}, spawnProcess);
        verifyBoundary('post-child');
        check(!child.failure && child.stdinWriteCompleted && child.child.exit_code === 0 && child.child.signal === null && child.child.error === null,
            'child', child.failure || 'child did not exit successfully');
        const result = h1.parseCanonicalJson(child.stdout, {maxBytes: h1.MAX_STREAM_BYTES, label: 'preflight stdout'});
        h1.validatePreflightResult(result);
        check(result.status === 'PASS' && result.purpose === purpose, 'child-result', 'child result status/purpose mismatch');
        check(JSON.stringify(result.candidate) === JSON.stringify(inputs.candidate) && result.source_manifest_sha256 === inputs.sourceManifestSha256,
            'child-result', 'child candidate/manifest differs from actual inputs');
        check(JSON.stringify(result.run_root) === JSON.stringify(rootProof), 'child-result', 'child root proof differs from original parent/absence observations');
        check(result.inventory_gate.expected_count === inputs.caseIds.length && result.inventory_gate.actual_count === inputs.caseIds.length &&
            result.inventory_gate.expected_ids_sha256 === inputs.manifest.case_ids_sha256 && result.inventory_gate.actual_ids_sha256 === inputs.manifest.case_ids_sha256,
            'child-result', 'child case inventory differs from explicit captured inventory');
        const reportPath = path.join(receiptDirectory, `${receiptId}.json`);
        const stdoutPath = path.join(receiptDirectory, `${receiptId}.stdout`);
        const stderrPath = path.join(receiptDirectory, `${receiptId}.stderr`);
        const stream = (p, bytes) => ({path: p, bytes: bytes.length, sha256: h1.sha256(bytes), file_sha256: h1.sha256(bytes)});
        const receipt = {schema_version: h1.SCHEMA_VERSION, revision: h1.REVISION, purpose, receipt_id: receiptId,
            candidate: inputs.candidate, source_manifest_sha256: inputs.sourceManifestSha256, argv, cwd,
            environment_sha256: expected.environmentSha256, child: child.child,
            stdout: stream(stdoutPath, child.stdout), stderr: stream(stderrPath, child.stderr),
            preflight_result: result, run_root: rootProof, status: 'PASS'};
        const reportBytes = h1.canonicalJson(receipt);
        check(reportBytes.length <= h1.MAX_REPORT_BYTES, 'publish', 'receipt report exceeds byte cap');
        verifyBoundary('pre-publish');
        absent(receiptDirectory, io);
        io.mkdirSync(receiptDirectory, {mode: h1.RECEIPT_DIRECTORY_MODE});
        created = true;
        checkDirectoryGuards(receiptGuards, io);
        const createdStat = io.lstatSync(receiptDirectory);
        check(createdStat.isDirectory() && !createdStat.isSymbolicLink(), 'publish', 'new receipt path is not a real directory');
        const dirFd = io.openSync(receiptDirectory, io.constants.O_RDONLY | io.constants.O_NOFOLLOW | io.constants.O_DIRECTORY);
        let ownedIdentity;
        try {
            check(directoryIdentity(io.fstatSync(dirFd)) === directoryIdentity(createdStat), 'publish', 'new receipt directory changed while opening');
            io.fchmodSync(dirFd, h1.RECEIPT_DIRECTORY_MODE);
            ownedIdentity = directoryIdentity(io.fstatSync(dirFd));
        } finally { io.closeSync(dirFd); }
        const ownedGuards = directoryGuards(receiptDirectory, io);
        check(ownedGuards[ownedGuards.length - 1].identity === ownedIdentity, 'publish', 'new receipt directory replaced');
        checkDirectoryGuards(receiptGuards, io);
        fsyncDirectory(receiptParent, receiptGuards[receiptGuards.length - 1].identity, io, durabilityLimitations);
        const written = [];
        for (const [p, bytes] of [[stdoutPath, child.stdout], [stderrPath, child.stderr], [reportPath, reportBytes]]) {
            checkDirectoryGuards(ownedGuards, io);
            checkDirectoryGuards(receiptGuards, io);
            written.push({path: p, identity: writeExclusive(p, bytes, io)});
        }
        fsyncDirectory(receiptDirectory, ownedIdentity, io, durabilityLimitations);
        fsyncDirectory(receiptParent, receiptGuards[receiptGuards.length - 1].identity, io, durabilityLimitations);
        verifyBoundary('post-publish');
        const validated = h1.validateReceipt(reportPath, {io, expected});
        for (const [key, bytes] of [['receipt', reportBytes], ['stdout', child.stdout], ['stderr', child.stderr]])
            check(validated.snapshot[key].base64 === bytes.toString('base64'), 'readback', `${key} readback differs from captured bytes`);
        verifyBoundary('final-readback');
        checkDirectoryGuards(ownedGuards, io);
        check(input.statIdentity(io.lstatSync(receiptDirectory)) === validated.receiptDirectoryIdentity, 'readback', 'receipt directory changed after readback');
        for (const saved of written)
            check(input.statIdentity(io.lstatSync(saved.path)) === saved.identity, 'readback', 'published file changed after write/readback');
        return freeze({receiptPath: reportPath, receiptDirectory, validated, receipt: validated.receipt, inputs,
            durabilityLimitations, observations, environment, stdin: {...stdinSnapshot, write_completed: child.stdinWriteCompleted},
            rootParent: {device: originalParent.dev, inode: originalParent.ino}});
    } catch (error) {
        if (verifyBoundary) {
            try { verifyBoundary('failure'); } catch { /* Preserve both the original error and all failed checks. */ }
        }
        throw new CaptureError(error.stage || 'capture', error.message, {
            causeCode: error.code || null, child: child ? child.child : null,
            stdout: bytesSnapshot(child ? child.stdout : Buffer.alloc(0)),
            stderr: bytesSnapshot(child ? child.stderr : Buffer.alloc(0)),
            stdin: stdinSnapshot ? {...stdinSnapshot, write_completed: child ? child.stdinWriteCompleted : false} : null,
            observed: child ? child.observed : {stdout: 0, stderr: 0},
            captureFailure: child ? child.failure : null, inputs: inputs || null,
            receiptDirectory: created ? receiptDirectory : null, verificationFailures, durabilityLimitations
        });
    }
}

module.exports = Object.freeze({CaptureError, MAX_CONTEXT_BYTES, BLOCKED_ENVIRONMENT_KEYS,
    nodeFsAdapter, normalizedEnvironment, capturePreflight});
