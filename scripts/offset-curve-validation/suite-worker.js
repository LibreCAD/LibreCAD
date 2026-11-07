/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const {spawn} = require('node:child_process');
const api = require('./suite-inventory');
const {LIMITS, ensure, equal, canonical, hash} = api;
const h1 = require('./preflight-receipt');
const input = require('./input-capture');

// Recent Node versions suppress pattern-filtered definitions altogether. This
// explicitly pinned DISCOVERY-ONLY preload registers real tests as skipped;
// actual suite execution never supplies --require and runs unmodified node:test.
if (process.execArgv.includes(`--require=${__filename}`)) {
    const Module = require('node:module');
    const load = Module._load;
    const original = load.call(Module, 'node:test', module, false);
    const wrappers = new WeakMap();
    const wrap = fn => {
        if (wrappers.has(fn)) return wrappers.get(fn);
        const result = new Proxy(fn, {
            apply(target, self, args) {
                const values = [...args];
                if (typeof values[0] === 'string') {
                    if (values[1] && typeof values[1] === 'object') values[1] = {...values[1], skip: true};
                    else values.splice(1, 0, {skip: true});
                } else throw new TypeError('discovery requires explicitly named tests');
                return Reflect.apply(target, self, values);
            },
            get(target, key) {
                const value = Reflect.get(target, key);
                return ['test', 'it', 'describe', 'suite', 'skip', 'todo', 'only'].includes(key) && typeof value === 'function' ? wrap(value) : value;
            }
        });
        wrappers.set(fn, result); return result;
    };
    const replacement = wrap(original);
    Module._load = function(request, parent, main) {
        return request === 'node:test' || request === 'test' ? replacement : Reflect.apply(load, this, [request, parent, main]);
    };
}

// Used by Node's real --test reporter loader; never synthesize test outcomes.
async function* nodeEventReporter(events) {
    let count = 0;
    for await (const event of events) {
        ensure(++count <= LIMITS.events, 'Node event count exceeded cap');
        let nodes = 0;
        const copy = (value, depth = 0) => {
            ensure(depth <= LIMITS.eventDepth && ++nodes <= LIMITS.eventNodes, 'Node event shape exceeded cap');
            if (value === null || ['string', 'boolean'].includes(typeof value)) return value;
            if (typeof value === 'number') return Number.isFinite(value) ? value : String(value);
            if (value === undefined) return null;
            if (typeof value === 'bigint') return String(value);
            if (value instanceof Error) return {name: value.name, message: value.message, stack: value.stack || null,
                code: value.code || null, cause: value.cause === undefined ? null : copy(value.cause, depth + 1)};
            if (Array.isArray(value)) return value.map(item => copy(item, depth + 1));
            ensure(typeof value === 'object', 'unsupported Node event value');
            return Object.fromEntries(Object.entries(value).map(([key, item]) => [key, copy(item, depth + 1)]));
        };
        yield canonical(copy(event));
    }
}

function environmentSnapshot(environment) {
    ensure(environment && typeof environment === 'object' && !Array.isArray(environment), 'explicit child environment required');
    const out = {};
    for (const key of Object.keys(environment).sort()) {
        ensure(typeof environment[key] === 'string' && !key.includes('\0') && !environment[key].includes('\0'), 'invalid child environment');
        ensure(!/^(NODE_OPTIONS|NODE_PATH|LD_PRELOAD|DYLD_.*|OPENSSL_CONF|OPENSSL_MODULES)$/.test(key), 'loader environment is forbidden');
        out[key] = environment[key];
    }
    ensure(canonical(out).length <= LIMITS.contextBytes, 'child environment exceeds byte cap');
    return out;
}

function captureNode(files, {discovery = false, cwd = __dirname, environment = process.env, timeoutMs = LIMITS.timeoutMs,
    maxEventBytes = LIMITS.eventBytes, maxStderrBytes = LIMITS.stderrBytes} = {}) {
    ensure(Array.isArray(files) && files.length > 0 && files.length <= 2 && new Set(files).size === files.length, 'explicit unique test modules required');
    files.forEach(file => { ensure(path.isAbsolute(file) && path.normalize(file) === file, 'absolute test module required'); });
    for (const [value, max] of [[timeoutMs, LIMITS.timeoutMs], [maxEventBytes, LIMITS.eventBytes], [maxStderrBytes, LIMITS.stderrBytes]])
        ensure(Number.isSafeInteger(value) && value > 0 && value <= max, 'invalid Node capture cap');
    const env = environmentSnapshot(environment);
    const argv = [fs.realpathSync.native(process.execPath), '--test', '--test-concurrency=1', `--test-reporter=${__filename}`,
        ...(discovery ? [`--require=${__filename}`] : []), ...files];
    return new Promise(resolve => {
        const chunks = {stdout: [], stderr: []}, observed = {stdout: 0, stderr: 0}, kept = {stdout: 0, stderr: 0};
        let failure = null, error = null, closed = false, killTimer;
        const child = spawn(argv[0], argv.slice(1), {cwd, env, stdio: ['ignore', 'pipe', 'pipe'], shell: false});
        const stop = reason => {
            failure = failure || reason;
            child.kill('SIGTERM');
            if (!killTimer) { killTimer = setTimeout(() => child.kill('SIGKILL'), 1000); killTimer.unref(); }
        };
        const timeout = setTimeout(() => stop('timeout'), timeoutMs);
        for (const name of ['stdout', 'stderr']) child[name].on('data', chunk => {
            observed[name] += chunk.length;
            const limit = name === 'stdout' ? maxEventBytes : maxStderrBytes;
            const take = Math.min(chunk.length, limit - kept[name]);
            if (take) { chunks[name].push(Buffer.from(chunk.subarray(0, take))); kept[name] += take; }
            if (observed[name] > limit) stop(`${name}-overflow`);
        });
        child.on('error', cause => { error = {code: cause.code || null, message: String(cause.message).slice(0, 4096)}; failure = failure || 'spawn-error'; });
        child.on('close', (exitCode, signal) => {
            if (closed) return; closed = true; clearTimeout(timeout); clearTimeout(killTimer);
            const snapshot = name => {
                const bytes = Buffer.concat(chunks[name], kept[name]);
                return {base64: bytes.toString('base64'), bytes: bytes.length, sha256: hash(bytes), observed_bytes: observed[name], truncated: observed[name] !== bytes.length};
            };
            resolve({mode: discovery ? 'discovery' : 'execution', argv, cwd, environment: env, environment_sha256: hash(canonical(env)),
                child: {exit_code: exitCode, signal, error}, failure, stdout: snapshot('stdout'), stderr: snapshot('stderr')});
        });
    });
}

function decodeEvents(capture) {
    ensure(capture && ['execution', 'discovery'].includes(capture.mode) && capture.stdout &&
        typeof capture.stdout.base64 === 'string' && capture.stdout.base64.length <= Math.ceil(LIMITS.eventBytes / 3) * 4, 'invalid Node capture');
    const bytes = Buffer.from(capture.stdout.base64, 'base64');
    ensure(bytes.length === capture.stdout.bytes && bytes.length === capture.stdout.observed_bytes &&
        bytes.toString('base64') === capture.stdout.base64 && hash(bytes) === capture.stdout.sha256, 'Node event stream binding differs');
    ensure(!capture.stdout.truncated && bytes.length > 0 && bytes[bytes.length - 1] === 10, 'missing/truncated Node events');
    const lines = new TextDecoder('utf-8', {fatal: true, ignoreBOM: true}).decode(bytes).split('\n'); lines.pop();
    ensure(lines.length <= LIMITS.events, 'too many Node events');
    return lines.map(line => {
        const value = h1.parseCanonicalJson(Buffer.from(`${line}\n`), {maxBytes: LIMITS.eventBytes});
        ensure(value && typeof value.type === 'string' && value.data && typeof value.data === 'object', 'invalid Node event envelope');
        return value;
    });
}

function terminalTests(events, files) {
    const tests = [], ordinals = new Map();
    for (const event of events) {
        if (!['test:pass', 'test:fail'].includes(event.type)) continue;
        const data = event.data;
        // File-wrapper outcomes are not testcase assertions; failed wrappers still refuse the run below.
        if (data.details && data.details.type === 'test' && data.nesting === 0 && files.includes(data.entryFile)) {
            const ordinal = (ordinals.get(data.entryFile) || 0) + 1; ordinals.set(data.entryFile, ordinal);
            tests.push({file: data.entryFile, name: data.name, declaration_ordinal: ordinal,
                outcome: data.skip ? 'SKIP' : data.todo ? 'TODO' : event.type === 'test:pass' ? 'PASS' : 'FAIL'});
        }
    }
    return tests;
}

async function discoverTests(files, options = {}) {
    const capture = await captureNode(files, {...options, discovery: true});
    let events = [], tests = [], reason = null;
    try {
        events = decodeEvents(capture); tests = terminalTests(events, files);
        ensure(!capture.failure && capture.child.exit_code === 0 && capture.child.signal === null && capture.child.error === null, 'discovery process did not complete');
        ensure(tests.length > 0 && tests.every(test => test.outcome === 'SKIP'), 'discovery must list definitions as skipped, never executed PASS');
        ensure(!events.some(event => event.type === 'test:fail'), 'discovery contains a failed wrapper');
        ensure(files.every(file => tests.some(test => test.file === file)), 'discovery omitted a module');
    } catch (error) { reason = error.message; }
    return {status: reason ? 'REFUSED' : 'DISCOVERED', reason, tests, capture};
}

function semanticReport(suite, purpose, capture) {
    let tests = [], reason = null, events = [];
    try {
        events = decodeEvents(capture); tests = terminalTests(events, suite.files.map(row => row.path));
        ensure(!capture.failure && capture.child.exit_code === 0 && capture.child.signal === null && capture.child.error === null, 'Node execution process did not complete successfully');
        ensure(!events.some(event => event.type === 'test:fail'), 'Node execution includes a failed test or wrapper');
        ensure(capture.mode === 'execution' && !capture.argv.some(arg => arg.startsWith('--require=') || arg.startsWith('--test-name-pattern=')), 'discovery is not execution qualification');
        const summaries = events.filter(event => event.type === 'test:summary' && event.data.file === null);
        ensure(summaries.length === 1 && summaries[0].data.success === true && summaries[0].data.counts.failed === 0 &&
            summaries[0].data.counts.cancelled === 0 && summaries[0].data.counts.skipped === 0 && summaries[0].data.counts.todo === 0,
        'missing or unsuccessful final Node summary');
        ensure(events.filter(event => ['test:pass', 'test:fail'].includes(event.type) && event.data.nesting === 0 && event.data.entryFile).length === tests.length,
            'unexpected top-level testcase/module');
        ensure(tests.length === suite.cases.length, 'executed testcase count differs');
        for (const [i, test] of tests.entries()) {
            const expected = suite.cases[i];
            ensure(test.file === expected.file && test.name === expected.name && test.declaration_ordinal === expected.declaration_ordinal,
                'executed testcase identity/order differs');
            ensure(test.outcome === 'PASS', 'skipped, TODO or failed testcase is not a qualification PASS');
        }
    } catch (error) { reason = error.message; }
    const cases = suite.cases.map((entry, i) => ({id: entry.id, name: entry.name,
        outcome: tests[i] && tests[i].file === entry.file && tests[i].name === entry.name ? tests[i].outcome : 'MISSING'}));
    return {schema_version: 1, kind: 'node-suite-semantics', purpose, suite: suite.name, status: reason ? 'FAIL' : 'PASS',
        assertion_unit: api.ASSERTION_UNIT, assertions: tests.filter(test => test.outcome === 'PASS' || test.outcome === 'FAIL').length,
        case_ids: suite.cases.map(entry => entry.id), cases, reason};
}

async function executeSuite(definitions, suiteName, options = {}) {
    api.validateDefinitions(definitions);
    const suite = definitions.suites.find(row => row.name === suiteName); ensure(suite, 'unknown suite');
    const verify = () => {
        ensure(equal(definitions.runtime, api.runtimeIdentity()), 'actual runtime differs from pinned definitions');
        for (const saved of [...definitions.sources, ...definitions.inputs]) ensure(equal(saved, api.pinFile(saved.path)), `pinned source/input changed: ${saved.path}`);
    };
    verify();
    const environment = environmentSnapshot(options.environment || definitions.environment);
    ensure(equal(environment, definitions.environment), 'suite environment differs from pinned definitions');
    ensure(options.cwd === undefined || options.cwd === definitions.cwd, 'nested Node cwd differs from pinned definitions');
    for (const [key, bound] of [['timeoutMs', suite.timeout_ms], ['maxEventBytes', suite.max_event_bytes], ['maxStderrBytes', suite.max_stderr_bytes]])
        ensure(options[key] === undefined || options[key] <= bound, 'execution cannot raise pinned caps');
    const capture = await captureNode(suite.files.map(row => row.path), {timeoutMs: suite.timeout_ms,
        maxEventBytes: suite.max_event_bytes, maxStderrBytes: suite.max_stderr_bytes, ...options, cwd: definitions.cwd, environment, discovery: false});
    const report = semanticReport(suite, definitions.purpose, capture);
    let verificationError = null;
    try { verify(); } catch (error) { report.status = 'FAIL'; report.reason = 'SOURCE_REVALIDATION_FAILED'; verificationError = error.message; }
    const semantic = canonical(report);
    return {report, semantic, assertions: report.assertions, assertion_unit: api.ASSERTION_UNIT,
        rawEvents: Buffer.from(capture.stdout.base64, 'base64'), diagnostics: {schema_version: 1, kind: 'node-suite-diagnostics',
            purpose: definitions.purpose, suite: suite.name, runtime: definitions.runtime, worker: definitions.worker,
            sources: definitions.sources, inputs: definitions.inputs, normalization: definitions.normalization,
            semantic: {bytes: semantic.length, sha256: hash(semantic)}, verification_error: verificationError, capture}};
}

function publish(filePath, bytes) {
    ensure(bytes.length <= LIMITS.fileBytes, 'worker output exceeds H3 byte cap');
    const fd = fs.openSync(filePath, fs.constants.O_CREAT | fs.constants.O_EXCL | fs.constants.O_NOFOLLOW | fs.constants.O_WRONLY, 0o600);
    try {
        let offset = 0;
        while (offset < bytes.length) { const count = fs.writeSync(fd, bytes, offset, bytes.length - offset); ensure(count > 0, 'zero output write'); offset += count; }
        fs.fchmodSync(fd, 0o400); fs.fsyncSync(fd);
    } finally { fs.closeSync(fd); }
    ensure(api.pinFile(filePath).sha256 === hash(bytes), 'worker output readback differs');
}

async function workerMain(argv) {
    ensure(argv.length === 4 && argv[0] === '--definitions' && argv[2] === '--suite', 'expected --definitions ABS --suite NAME');
    const {value: definitions, binding} = api.readDefinitions(argv[1]);
    const chunks = []; let length = 0;
    for await (const chunk of process.stdin) { length += chunk.length; ensure(length <= LIMITS.contextBytes, 'worker stdin exceeds cap'); chunks.push(chunk); }
    const stdin = Buffer.concat(chunks, length), context = h1.parseCanonicalJson(stdin, {maxBytes: LIMITS.contextBytes});
    api.exactKeys(context, ['schema_version', 'purpose', 'worker_id', 'ordinal', 'case_ids', 'outputs', 'run_root', 'candidate', 'source_manifest_sha256', 'registry_sha256'], 'worker context');
    const suite = definitions.suites.find(row => row.name === argv[3]); ensure(suite, 'unknown suite');
    ensure(context.schema_version === 1 && context.purpose === definitions.purpose && context.worker_id === suite.name &&
        context.ordinal === definitions.suites.indexOf(suite) + 1 && equal(context.case_ids, suite.cases.map(entry => entry.id)) && equal(context.outputs, suite.outputs), 'worker context differs from suite definitions');
    ensure(context.candidate && equal(context.candidate, api.pinFile(context.candidate.path)) &&
        /^[a-f0-9]{64}$/.test(context.source_manifest_sha256) && /^[a-f0-9]{64}$/.test(context.registry_sha256), 'invalid context candidate/manifest/registry binding');
    ensure(path.isAbsolute(context.run_root) && path.normalize(context.run_root) === context.run_root, 'invalid worker output root');
    const root = fs.lstatSync(context.run_root, {bigint: true}); ensure(root.isDirectory() && !root.isSymbolicLink(), 'worker root is not a real directory');
    const rootGuards = input.directoryGuards(context.run_root);
    const result = await executeSuite(definitions, suite.name, {environment: process.env});
    result.diagnostics.definitions = binding; result.diagnostics.context = context;
    result.diagnostics.stdin = {bytes: stdin.length, sha256: hash(stdin), base64: stdin.toString('base64')};
    ensure(equal(binding, api.pinFile(argv[1], LIMITS.definitionBytes)), 'definitions changed during execution');
    const artifacts = [canonical(result.diagnostics), result.rawEvents, result.semantic];
    suite.outputs.forEach((name, i) => {
        input.checkDirectoryGuards(rootGuards);
        publish(path.join(context.run_root, name), artifacts[i]);
    });
    input.checkDirectoryGuards(rootGuards);
    const response = {schema_version: 1, purpose: context.purpose, worker_id: context.worker_id, ordinal: context.ordinal,
        status: result.report.status, case_ids: context.case_ids,
        events: result.report.cases.map((entry, i) => ({ordinal: i + 1, case_id: entry.id, status: entry.outcome === 'PASS' && result.report.status === 'PASS' ? 'PASS' : 'FAIL'})), outputs: suite.outputs};
    process.stdout.write(canonical(response));
    process.exitCode = result.report.status === 'PASS' ? 0 : 1;
    return response;
}

if (require.main === module) workerMain(process.argv.slice(2)).catch(error => {
    process.stderr.write(`${error.stack || error}\n`); process.exitCode = 1;
});

module.exports = Object.freeze(Object.assign(nodeEventReporter, {environmentSnapshot, captureNode, decodeEvents, terminalTests,
    discoverTests, semanticReport, executeSuite, workerMain}));
