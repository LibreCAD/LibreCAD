/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const registry = require('./formal-registry');
const suites = require('./suite-inventory');
const {executeSuite} = require('./suite-worker');

const MAX_FILE = 8388608;
const check = (ok, message) => { if (!ok) throw new TypeError(message); };
const equal = (a, b) => h1.canonicalJson(a).equals(h1.canonicalJson(b));
const snapshot = bytes => ({bytes: bytes.length, sha256: h1.sha256(bytes), base64: bytes.toString('base64')});
const keys = (object, expected) => check(object && equal(Object.keys(object), expected), 'preflight config keys/order differ');

// Deliberately simple, explicit byte-patch format. This is not represented as a
// Git textual patch or a semantic C++ diff. Replay checks actual source bytes.
function replacementPatch(from, to) {
    check(Buffer.isBuffer(from) && Buffer.isBuffer(to) && from.length <= MAX_FILE && to.length <= MAX_FILE, 'patch input exceeds cap');
    return {format: 'bounded-whole-file-replacement-v1', from: {bytes: from.length, sha256: h1.sha256(from)}, target: snapshot(to)};
}
function replayPatch(from, patch) {
    keys(patch, ['format', 'from', 'target']);
    keys(patch.from, ['bytes', 'sha256']); keys(patch.target, ['bytes', 'sha256', 'base64']);
    check(patch.format === 'bounded-whole-file-replacement-v1' && Buffer.isBuffer(from) && from.length <= MAX_FILE &&
        patch.from.bytes === from.length && patch.from.sha256 === h1.sha256(from), 'patch source differs');
    check(typeof patch.target.base64 === 'string' && patch.target.base64.length <= Math.ceil(MAX_FILE / 3) * 4, 'patch target exceeds cap');
    const result = Buffer.from(patch.target.base64, 'base64');
    check(result.toString('base64') === patch.target.base64 && result.length <= MAX_FILE &&
        equal(snapshot(result), patch.target), 'patch target binding differs');
    return result;
}

function read(p, limit = MAX_FILE) {
    return Buffer.from(registry.readStableFile(p, limit).base64, 'base64');
}

function persist(directory, name, role, bytes, artifacts, guards) {
    check(bytes.length <= MAX_FILE && /^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$/.test(name), 'invalid preflight artifact');
    h2.checkDirectoryGuards(guards);
    const p = path.join(directory, name);
    const fd = fs.openSync(p, fs.constants.O_CREAT | fs.constants.O_EXCL | fs.constants.O_WRONLY | fs.constants.O_NOFOLLOW, 0o600);
    let identity;
    try {
        const before = fs.fstatSync(fd, {bigint: true});
        check(before.isFile() && before.nlink === 1n && before.size === 0n &&
            h2.statIdentity(fs.lstatSync(p, {bigint: true})) === h2.statIdentity(before), 'preflight artifact descriptor differs');
        for (let offset = 0; offset < bytes.length;) {
            const n = fs.writeSync(fd, bytes, offset, bytes.length - offset);
            check(n > 0, 'short preflight artifact write'); offset += n;
        }
        fs.fchmodSync(fd, 0o400); fs.fsyncSync(fd);
        const after = fs.fstatSync(fd, {bigint: true});
        check(after.ino === before.ino && after.dev === before.dev && after.nlink === 1n &&
            Number(after.mode & 0o7777n) === 0o400 && after.size === BigInt(bytes.length), 'preflight artifact changed during write');
        identity = h2.statIdentity(after);
    } finally { fs.closeSync(fd); }
    check(read(p).equals(bytes) && h2.statIdentity(fs.lstatSync(p, {bigint: true})) === identity, 'preflight artifact readback differs');
    h2.checkDirectoryGuards(guards);
    artifacts.push({role, path: p, bytes: bytes.length, sha256: h1.sha256(bytes)});
}

function rootGuard(context) {
    const root = context.run_root;
    h2.canonicalPath(root.path); h2.directoryGuards(root.parent.path);
    const parent = fs.lstatSync(root.parent.path, {bigint: true});
    check(String(parent.dev) === root.parent.device && String(parent.ino) === root.parent.inode,
        'original reserved-root parent changed');
    try { fs.lstatSync(root.path); }
    catch (error) { if (error.code === 'ENOENT') return; throw error; }
    throw new TypeError('reserved root is present');
}

async function runPreflight(context, configPath) {
    const config = h1.parseCanonicalJson(read(configPath, 65536), {maxBytes: 65536});
    keys(config, ['schema_version', 'purpose', 'formal_config_path', 'suite_definitions_path',
        'base_path', 'product_path', 'candidate_path', 'diagnostic_parent']);
    check(config.schema_version === 1 && config.purpose === context.purpose && ['validation', 'synthetic'].includes(config.purpose), 'preflight purpose differs');
    for (const key of Object.keys(config).slice(2)) h2.canonicalPath(config[key]);
    const formal = h1.parseCanonicalJson(read(config.formal_config_path, 65536), {maxBytes: 65536});
    const captured = h2.captureInputs(formal.preflight);
    check(equal(captured.candidate, context.candidate) && captured.sourceManifestSha256 === context.source_manifest_sha256 &&
        captured.caseIds.length === context.case_count && captured.manifest.case_ids_sha256 === context.case_ids_sha256 &&
        equal(captured.runtime, context.runtime) && context.case_inventory_path === captured.manifest.case_inventory_path,
    'preflight stdin differs from actual input/runtime/inventory');
    const declared = p => captured.manifest.files.some(row => row.path === p && row.bytes > 0);
    for (const p of [__filename, path.join(__dirname, 'formal-registry.js'), path.join(__dirname, 'trace-probe.js'), path.join(__dirname, 'independent-evidence-checker.js'),
        configPath, config.formal_config_path, config.suite_definitions_path, config.base_path, config.product_path, config.candidate_path])
        check(declared(p), `preflight dependency is not pinned: ${p}`);
    check(config.candidate_path === context.candidate.path, 'patch candidate differs from actual candidate');
    rootGuard(context);
    h2.directoryGuards(config.diagnostic_parent);
    const directory = path.join(config.diagnostic_parent, h1.sha256(h1.canonicalJson(context.run_root)).slice(0, 32));
    const related = (a, b) => a === b || b.startsWith(`${a}${path.sep}`);
    const protectedPaths = [...captured.manifest.files.map(row => row.path), context.run_root.path, formal.run_root];
    check(!protectedPaths.some(p => related(directory, p) || related(p, directory)), 'preflight artifacts overlap protected paths');
    fs.mkdirSync(directory, {mode: 0o700});
    const guards = h2.directoryGuards(directory);
    const artifacts = [];
    const save = (name, role, value) => persist(directory, name, role, h1.canonicalJson(value), artifacts, guards);
    const definitions = suites.readDefinitions(config.suite_definitions_path).value;
    check(definitions.purpose === config.purpose && equal(definitions.suites.map(row => row.name), h1.PREFLIGHT_SUITES), 'preflight suite identity differs');
    check(equal(definitions.suites.flatMap(row => row.cases.map(entry => entry.id)).sort(), captured.caseIds), 'preflight discovered inventory differs');
    const independent = require('./independent-evidence-checker').deriveInputManifest(formal.preflight);
    const primaryHash = h1.sha256(h1.canonicalJson(captured.manifest)), independentHash = h1.sha256(h1.canonicalJson(independent));
    save('input-audit.json', 'input-audit', {schema_version: 1, primary: captured.manifest, independent,
        primary_sha256: primaryHash, independent_sha256: independentHash});
    check(primaryHash === independentHash && equal(captured.manifest, independent), 'independent input audit disagrees');
    const base = read(config.base_path), product = read(config.product_path), candidate = read(config.candidate_path);
    const directions = [[base, product], [product, candidate], [candidate, product], [base, candidate]].map(([from, to], i) => {
        const patch = replacementPatch(from, to), replayed = replayPatch(from, patch);
        check(replayed.equals(to), 'actual byte patch replay disagrees');
        return {name: h1.PATCH_DIRECTIONS[i], patch, result: snapshot(replayed)};
    });
    save('patch-replays.json', 'patch-replays', {schema_version: 1, format: 'bounded-whole-file-replacement-v1',
        base: {path: config.base_path, ...snapshot(base)}, product: {path: config.product_path, ...snapshot(product)},
        candidate: {path: config.candidate_path, ...snapshot(candidate)}, directions});
    const syntax = [];
    for (const row of captured.manifest.files.filter(row => row.path.endsWith('.js'))) {
        const child = spawnSync(captured.runtime.executable_path, ['--check', row.path],
            {cwd: formal.preflight.cwd, env: formal.preflight.environment, timeout: 10000, maxBuffer: 65536});
        const observed = {path: row.path, exit_code: child.status, signal: child.signal,
            error: child.error ? String(child.error.message) : null, stdout: snapshot(child.stdout || Buffer.alloc(0)),
            stderr: snapshot(child.stderr || Buffer.alloc(0))};
        syntax.push(observed);
        check(child.status === 0 && !child.signal && !child.error, `actual syntax check failed: ${row.path}`);
    }
    check(syntax.length > 0, 'syntax inventory is empty');
    save('syntax.json', 'syntax', {schema_version: 1, checks: syntax});
    const attempts = [];
    for (let offset = 0; offset < definitions.suites.length; offset += 2) {
        // At most two suite pairs (four independent real executions) overlap.
        // Retain per-pair guards and settle every launched sibling on refusal.
        const pending = [];
        let boundaryError = null;
        for (const suite of definitions.suites.slice(offset, offset + 2)) {
            try {
                rootGuard(context); h2.revalidateInputs(captured);
                pending.push({suite, executions: Promise.allSettled([1, 2].map(async () =>
                    executeSuite(definitions, suite.name, {environment: formal.preflight.environment, cwd: definitions.cwd}))).then(executions => {
                    // Observe this pair's completion boundary immediately, but
                    // defer refusal until every sibling and diagnostic is retained.
                    try { rootGuard(context); h2.revalidateInputs(captured); }
                    catch (error) { boundaryError = boundaryError || error; }
                    return executions;
                })});
            } catch (error) { boundaryError = error; break; }
        }
        const completion = Promise.allSettled(pending.map(row => row.executions));
        const pairs = [];
        // Publication order is canonical, not completion order. Preserve every
        // available actual sibling result before reporting a boundary/status refusal.
        try {
            for (const {suite, executions: task} of pending) {
                const [outcome] = await Promise.allSettled([task]), pair = [];
                const executions = outcome.status === 'fulfilled' ? outcome.value : [];
                for (let ordinal = 1; ordinal <= 2; ordinal++) {
                    const observed = executions[ordinal - 1];
                    if (!observed || observed.status !== 'fulfilled') continue;
                    const execution = observed.value;
                    const prefix = `${suite.name}-${ordinal}`;
                    persist(directory, `${prefix}.report.json`, `suite:${suite.name}:attempt:${ordinal}:report`, execution.semantic, artifacts, guards);
                    save(`${prefix}.diagnostics.json`, `suite:${suite.name}:attempt:${ordinal}:diagnostics`, execution.diagnostics);
                    persist(directory, `${prefix}.events.jsonl`, `suite:${suite.name}:attempt:${ordinal}:events`, execution.rawEvents, artifacts, guards);
                    pair.push({ordinal, status: execution.report.status, report_bytes: execution.semantic.length,
                        report_sha256: h1.sha256(execution.semantic), assertions: execution.assertions});
                }
                pairs.push(pair);
            }
        } finally { await completion; }
        const completed = await completion;
        if (boundaryError) throw boundaryError;
        for (const [index, {suite}] of pending.entries()) {
            const outcome = completed[index];
            if (outcome.status === 'rejected') throw outcome.reason;
            const executions = outcome.value, pair = pairs[index];
            const rejected = executions.find(row => row.status === 'rejected');
            if (rejected) throw rejected.reason;
            for (const row of executions) check(row.value.report.status === 'PASS' && row.value.assertions > 0,
                `actual suite failed: ${suite.name}: ${row.value.report.reason}`);
            check(pair[0].report_bytes === pair[1].report_bytes && pair[0].report_sha256 === pair[1].report_sha256 &&
                pair[0].assertions === pair[1].assertions, `repeated semantic reports differ: ${suite.name}`);
            attempts.push({name: suite.name, status: 'PASS', attempts: pair});
        }
    }
    const traceInput = h1.canonicalJson({purpose: context.purpose, candidate: context.candidate,
        source_manifest_sha256: context.source_manifest_sha256, case_ids_sha256: context.case_ids_sha256});
    const trace = spawnSync(captured.runtime.executable_path, [path.join(__dirname, 'trace-probe.js')],
        {input: traceInput, cwd: formal.preflight.cwd, env: formal.preflight.environment, timeout: 5000, maxBuffer: 65536});
    check(trace.status === 0 && !trace.signal && !trace.error, 'actual trace child failed');
    const traced = h1.parseCanonicalJson(trace.stdout);
    check(traced.kind === 'stdin-trace' && traced.bytes === traceInput.length && traced.sha256 === h1.sha256(traceInput) &&
        traced.runtime === process.version, 'trace child did not report actual stdin consumption');
    save('trace-probe.json', 'trace-probe', {schema_version: 1, argv: [captured.runtime.executable_path, path.join(__dirname, 'trace-probe.js')],
        stdin: snapshot(traceInput), stdout: snapshot(trace.stdout), stderr: snapshot(trace.stderr), exit_code: trace.status,
        signal: trace.signal, error: null});
    rootGuard(context); h2.revalidateInputs(captured); h2.checkDirectoryGuards(guards);
    check(equal(fs.readdirSync(directory).sort(), artifacts.map(row => path.basename(row.path)).sort()), 'preflight artifact membership differs');
    for (const row of artifacts) {
        const observed = suites.pinFile(row.path);
        check(observed.sha256 === row.sha256 && observed.bytes === row.bytes && (fs.lstatSync(row.path).mode & 0o7777) === 0o400,
            'preflight artifact changed before result');
    }
    const caps = [{name: 'actual-input-file-count', limit: h2.LIMITS.files, measured: captured.manifest.files.length},
        {name: 'actual-input-total-bytes', limit: h2.LIMITS.totalBytes, measured: captured.manifest.files.reduce((n, row) => n + row.bytes, 0)},
        {name: 'actual-case-count', limit: h2.LIMITS.cases, measured: captured.caseIds.length},
        {name: 'actual-suite-execution-count', limit: 8, measured: attempts.reduce((n, row) => n + row.attempts.length, 0)},
        {name: 'largest-persisted-artifact', limit: MAX_FILE, measured: Math.max(...artifacts.map(row => row.bytes))}]
        .map(row => ({...row, passed: row.measured <= row.limit}));
    check(caps.every(row => row.passed), 'measured cap failed');
    const result = {revision: h1.REVISION, purpose: context.purpose, status: 'PASS', candidate: captured.candidate,
        source_manifest_sha256: captured.sourceManifestSha256,
        inventory_gate: {status: 'PASS', expected_count: captured.caseIds.length, actual_count: captured.caseIds.length,
            expected_ids_sha256: captured.manifest.case_ids_sha256, actual_ids_sha256: captured.manifest.case_ids_sha256,
            missing_ids: [], extra_ids: [], duplicate_ids: []},
        patch_replays: {status: 'PASS', directions: directions.map(row => ({name: row.name, status: 'PASS',
            patch_sha256: h1.sha256(h1.canonicalJson(row.patch)), result_sha256: row.result.sha256}))},
        audit_parity: {status: 'PASS', primary_sha256: primaryHash, independent_sha256: independentHash, equal: true},
        suite_parity: {status: 'PASS', suite_count: attempts.length, identical_repeated_outputs: true}, suites: attempts,
        syntax: {status: 'PASS', checked_file_count: syntax.length, checked_paths_sha256: h1.sha256(h1.canonicalJson(syntax.map(row => row.path)))},
        caps: {status: 'PASS', checks: caps}, trace_probe: {status: 'PASS', stdout_sha256: h1.sha256(trace.stdout),
            stderr_sha256: h1.sha256(trace.stderr), exit_code: trace.status},
        run_root: {...context.run_root, absent_before: true, absent_after: true, parent_stable: true, write_attempts: 0, error: null}};
    // Match H1's ordered root-proof schema rather than the ingress context order.
    result.run_root = {path: context.run_root.path, absent_before: true, absent_after: true, parent_stable: true,
        parent: context.run_root.parent, write_attempts: 0, error: null};
    check(h1.canonicalJson(result).length <= h1.MAX_REPORT_BYTES, 'actual preflight result exceeds report cap');
    return {result, evidence: {schema_version: 1, purpose: context.purpose, kind: 'genuine-preflight-evidence',
        context_sha256: h1.sha256(h1.canonicalJson(context)), artifacts,
        limitations: ['Byte replacement replay is not a Git textual patch or proof of source semantics.',
            suites.ASSERTION_UNIT, 'Filesystem checks observe boundaries; they do not provide containment.']}};
}

async function cli(argv) {
    check(argv.length === 1, 'one explicit qualification config path is required');
    const chunks = [], buffer = Buffer.alloc(4096); let count = 0;
    for (;;) {
        const n = fs.readSync(0, buffer, 0, buffer.length, null); if (!n) break;
        count += n; check(count <= 32768, 'preflight stdin exceeds cap'); chunks.push(Buffer.from(buffer.subarray(0, n)));
    }
    const context = h1.parseCanonicalJson(Buffer.concat(chunks, count), {maxBytes: 32768});
    const out = await runPreflight(context, argv[0]);
    process.stderr.write(h1.canonicalJson(out.evidence)); process.stdout.write(h1.canonicalJson(out.result));
}
if (require.main === module) cli(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.stack || error}\n`); process.exitCode = 1; });

module.exports = Object.freeze({replacementPatch, replayPatch, runPreflight, cli});
