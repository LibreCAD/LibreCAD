/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const {runFormalTransaction} = require('./formal-runner');
const {nodeFsAdapter} = require('./input-capture');
const inventory = require('./refusal-inventory');
const fixture = require('./fixtures/transaction-fixture');
const {resultFor, writeOutputs} = require('./fixtures/formal-child');
const post = require('./post-refusal-definitions');

const cases = new Map();
const mismatches = [];
function register(id, definition) {
    assert.equal(cases.has(id), false, `duplicate execution registration: ${id}`);
    cases.set(id, definition);
}

function cliArguments(change) {
    return {capture: false, prepare: f => ({args: change(fixture.argumentsFor(f.request), f)})};
}

register('H6-CLI-01', cliArguments(args => args.slice(1)));
register('H6-CLI-02', cliArguments(args => [...args, '--unknown']));
register('H6-CLI-03', cliArguments(args => [...args, '--run']));
register('H6-CLI-04', cliArguments(args => args.filter((value, i) => i !== 2)));
register('H6-CLI-05', cliArguments(args => args.slice(0, 5)));
register('H6-CLI-06', cliArguments(args => args.slice(0, 7)));
register('H6-CLI-07', cliArguments((args, f) => [...args, '--preflight-receipt', path.join(f.base, 'third.json')]));
register('H6-CLI-08', cliArguments(args => [...args.slice(0, 8), args[6]]));
register('H6-CLI-09', {duplicateId: true});
register('H6-CLI-10', {prepare: f => {
    f.request.receiptPaths.reverse();
}});
register('H6-CLI-11', cliArguments(args => [...args.slice(0, 6), 'relative.json', ...args.slice(7)]));
register('H6-ID-01', {prepare: f => { fs.unlinkSync(f.request.receiptPaths[0]); }});

function revision(value) {
    return {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.revision = value; })};
}

register('H6-ID-02a', revision('V506'));
register('H6-ID-02b', revision('V507'));
register('H6-ID-02c', revision('V508'));
register('H6-ID-02d', revision('V509'));
register('H6-ID-02e', revision('V510'));
register('H6-ID-02f', revision('V511'));
register('H6-ID-02g', revision('V512'));
register('H6-ID-02h', revision('V513-pre-consolidation'));

function staleCandidate(version) {
    return {prepare: f => fixture.rewriteReceipt(f, 0, row => {
        // Synthetic negative metadata only; never a historical routing path.
        row.candidate = {path: path.join(f.inputDirectory, `synthetic-stale-${version}.md`), bytes: version.length,
            sha256: fixture.hash(Buffer.from(version)),
            git_blob: crypto.createHash('sha1').update(`synthetic-${version}`).digest('hex')};
        row.preflight_result.candidate = row.candidate;
    })};
}

register('H6-ID-03a', staleCandidate('V509'));
register('H6-ID-03b', staleCandidate('V510'));
register('H6-ID-03c', staleCandidate('V511'));
register('H6-ID-03d', staleCandidate('V512'));
register('H6-ID-03e', staleCandidate('V513-pre-consolidation'));
register('H6-ID-04', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.source_manifest_sha256 = fixture.hash(Buffer.from('synthetic-stale-manifest'));
    row.preflight_result.source_manifest_sha256 = row.source_manifest_sha256;
})});
register('H6-ID-05', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.candidate.bytes++;
    row.preflight_result.candidate = row.candidate;
})});
register('H6-ID-06', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.source_manifest_sha256 = fixture.hash(Buffer.from('other-manifest-field'));
    row.preflight_result.source_manifest_sha256 = row.source_manifest_sha256;
})});
register('H6-ID-07', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.environment_sha256 = fixture.hash(Buffer.from('other-environment'));
})});
register('H6-ID-08', {prepare: f => {
    // Change both receipts to reach the runtime comparison, not pair argv equality.
    for (let i = 0; i < 2; i++) fixture.rewriteReceipt(f, i, row => {
        row.argv[0] = path.join(f.inputDirectory, 'other-node');
    });
}});
register('H6-ID-09', {prepare: f => {
    for (let i = 0; i < 2; i++) fixture.rewriteReceipt(f, i, row => {
        row.argv[1] = path.join(f.inputDirectory, 'other-runner.js');
    });
}});
register('H6-ID-10', {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.cwd = f.base; })});
register('H6-ID-11', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.join(path.dirname(f.config.receipt_roots[0]), 'other-root'));
}});

function reportBytes(change) {
    return {prepare: f => {
        const p = f.request.receiptPaths[0];
        fixture.replace(p, change(fs.readFileSync(p)));
    }};
}

register('H6-TREE-01a', reportBytes(() => Buffer.from([0xc3, 0x28, 0x0a])));
register('H6-TREE-01b', reportBytes(() => Buffer.from('{"schema_version":\n')));
register('H6-TREE-02a', reportBytes(value => Buffer.from(value.toString().replace('{', '{"schema_version":2,'))));
register('H6-TREE-02b', reportBytes(value => {
    const row = JSON.parse(value);
    return fixture.bytes(Object.fromEntries(Object.entries(row).reverse()));
}));
register('H6-TREE-02c', reportBytes(value => {
    const row = JSON.parse(value);
    delete row.status;
    return fixture.bytes(row);
}));
register('H6-TREE-02d', reportBytes(value => fixture.bytes({...JSON.parse(value), unexpected: true})));
register('H6-TREE-02e', reportBytes(value => Buffer.concat([Buffer.from([0xef, 0xbb, 0xbf]), value])));
register('H6-TREE-02f', reportBytes(value => Buffer.concat([value.subarray(0, -1), Buffer.from('\r\n')])));
register('H6-TREE-02g', reportBytes(value => Buffer.concat([value, Buffer.from('\n')])));
register('H6-TREE-02h', reportBytes(value => Buffer.from(` ${value.toString()}`)));
register('H6-TREE-03a', {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.status = 'FAIL'; })});
register('H6-TREE-03b', {prepare: f => fixture.rewriteReceipt(f, 0, row => { row.child.exit_code = 7; })});
register('H6-TREE-03c', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.child.exit_code = null;
    row.child.signal = 'SIGTERM';
})});
register('H6-TREE-03d', {prepare: f => fixture.rewriteReceipt(f, 0, row => {
    row.child.exit_code = null;
    row.child.error = {code: 'ENOENT', message: 'synthetic preflight spawn failure'};
})});
register('H6-TREE-04a', {prepare: f => fixture.setStdout(f, Buffer.alloc(0))});
register('H6-TREE-04b', {prepare: f => {
    const value = fs.readFileSync(fixture.readReceipt(f).stdout.path);
    fixture.setStdout(f, Buffer.concat([value, value]));
}});
register('H6-TREE-04c', {prepare: f => {
    fixture.setStdout(f, Buffer.from(`${JSON.stringify(fixture.readReceipt(f).preflight_result, null, 2)}\n`));
}});
register('H6-TREE-05d', {prepare: f => {
    const p = fixture.readReceipt(f).stdout.path;
    fixture.replace(p, Buffer.concat([fs.readFileSync(p), Buffer.from('unbound stdout')]));
}});
register('H6-TREE-05e', {prepare: f => {
    fixture.replace(fixture.readReceipt(f).stderr.path, Buffer.from('unbound stderr'));
}});
register('H6-TREE-05f', {prepare: f => {
    const target = path.join(path.dirname(f.request.receiptPaths[0]), '30000000-0000-4000-8000-000000000003.json');
    fs.renameSync(f.request.receiptPaths[0], target);
    f.request.receiptPaths[0] = target;
}});
register('H6-TREE-06a', {prepare: f => { fs.chmodSync(f.request.receiptPaths[0], 0o600); }});
register('H6-TREE-06b', {prepare: f => {
    fs.unlinkSync(f.request.receiptPaths[0]);
    fs.mkdirSync(f.request.receiptPaths[0], {mode: 0o400});
}});
register('H6-TREE-07a', {prepare: f => { fs.unlinkSync(fixture.readReceipt(f).stderr.path); }});
register('H6-TREE-07b', {prepare: f => {
    fs.writeFileSync(path.join(path.dirname(f.request.receiptPaths[0]), 'extra'), 'extra', {mode: 0o400});
}});
register('H6-TREE-08a', {prepare: f => {
    const p = f.request.receiptPaths[0];
    fs.renameSync(p, path.join(f.derived, 'symlink-target'));
    fs.symlinkSync(path.join(f.derived, 'symlink-target'), p);
}});
register('H6-TREE-08b', {prepare: f => {
    fs.linkSync(f.request.receiptPaths[0], path.join(f.derived, 'hardlink-alias'));
}});
register('H6-TREE-08c', {prepare: f => {
    const alias = path.join(f.derived, 'storage-alias');
    fs.symlinkSync(path.dirname(f.request.receiptPaths[0]), alias);
    f.request.receiptPaths[0] = path.join(alias, path.basename(f.request.receiptPaths[0]));
}});
register('H6-TREE-09a', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.dirname(f.request.receiptPaths[1]));
}});
register('H6-TREE-09b', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.dirname(f.config.receipt_roots[1]));
}});
register('H6-TREE-09c', {prepare: f => {
    fixture.setReservedRoot(f, 0, path.dirname(fixture.workerPath));
}});

register('H6-RACE-01', {prepare: f => {
    let changed = false;
    let preserved;
    return {changesRunParent: true, dependencies: {beforeCreate() {
        fs.renameSync(f.runParent, `${f.runParent}.original`);
        fs.mkdirSync(f.runParent, {mode: 0o700});
        changed = true;
        preserved = [fixture.tree(f.runParent), fixture.tree(`${f.runParent}.original`)];
    }}, assertAfter() {
        assert.equal(changed, true, 'parent replacement hook must be reached');
        assert.deepEqual([fixture.tree(f.runParent), fixture.tree(`${f.runParent}.original`)], preserved);
    }};
}});

function receiptRace(change) {
    return {prepare: f => {
        let afterMutation;
        return {derivedAfter: () => afterMutation, dependencies: {beforeCreate() {
            change(f);
            afterMutation = fixture.tree(f.derived);
        }}, assertAfter() { assert.ok(afterMutation, 'receipt mutation hook must be reached'); }};
    }};
}

register('H6-RACE-02a', receiptRace(f => fixture.rewriteReceipt(f, 0, row => {
    row.environment_sha256 = fixture.hash(Buffer.from('changed-between-validations'));
}, {syncStdout: false})));
register('H6-RACE-02b', receiptRace(f => {
    const p = fixture.readReceipt(f).stderr.path;
    fixture.replace(p, Buffer.concat([fs.readFileSync(p), Buffer.from('changed-between-validations')]));
}));
register('H6-RACE-03', {prepare: f => {
    const target = fixture.readReceipt(f).stderr.path;
    const descriptors = new Map();
    let armed = false;
    let altered = 0;
    return {dependencies: {beforeCreate() { armed = true; }, fs: {...nodeFsAdapter,
        openSync(p, ...args) {
            const fd = fs.openSync(p, ...args);
            descriptors.set(fd, p);
            return fd;
        },
        closeSync(fd) { descriptors.delete(fd); fs.closeSync(fd); },
        readSync(fd, buffer, offset, length, position) {
            const count = fs.readSync(fd, buffer, offset, length, position);
            if (armed && descriptors.get(fd) === target && count > 0) {
                buffer[offset] ^= 1;
                altered++;
            }
            return count;
        }
    }}, assertAfter() { assert.ok(altered > 0, 'actual receipt readback must be altered'); }};
}});
register('H6-RACE-04', {prepare: f => {
    let external;
    return {externalRoot: true, changesRunParent: true, dependencies: {fs: {...nodeFsAdapter,
        mkdirSync(p, options) {
            if (p === f.request.runRoot) {
                fs.mkdirSync(p, options);
                fs.writeFileSync(path.join(p, 'external-sentinel'), 'external race winner\n', {mode: 0o400, flag: 'wx'});
                external = fixture.tree(p);
            }
            return fs.mkdirSync(p, options);
        }
    }}, assertAfter() {
        assert.ok(external, 'external root race must reach exclusive creation');
        assert.deepEqual(fixture.tree(f.request.runRoot), external);
    }};
}});
register('H6-JOIN-01', {prepare: f => { f.request.receiptPaths[1] = f.request.receiptPaths[0]; }});
register('H6-JOIN-02', {duplicateId: true});
register('H6-JOIN-03', {prepare: f => { f.request.receiptPaths.reverse(); }});

function workerCase(mutate) {
    return {prepare: f => {
        const launches = [];
        let prefix;
        return {dependencies: {dispatch(context) {
            launches.push(context.worker_id);
            if (context.ordinal === 2) prefix = fixture.statRecord(path.join(context.run_root, 'a.json'));
            writeOutputs(context);
            const result = resultFor(context);
            return context.ordinal === 2 ? mutate(result) : result;
        }}, assertAfter(result) {
            assert.deepEqual(launches, ['worker-a', 'worker-b'], 'no third worker may launch');
            assert.equal(result.worker_launches, 2);
            assert.equal(result.workers.length, 1, 'first verified prefix must remain');
            assert.equal(result.workers[0].worker_id, 'worker-a');
            assert.deepEqual(fixture.statRecord(path.join(f.request.runRoot, 'a.json')), prefix);
            assert.equal(result.workers[0].outputs[0].sha256, prefix.sha256);
            assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json']);
            assert.deepEqual(result.verification_failures, []);
        }};
    }};
}

register('H6-WORKER-01', workerCase(() => undefined));
register('H6-WORKER-02', workerCase(row => { delete row.status; return row; }));
register('H6-WORKER-03', workerCase(row => { row.status = 'UNKNOWN'; return row; }));
register('H6-WORKER-04', workerCase(row => { row.status = 'FAIL'; return row; }));
register('H6-WORKER-05', workerCase(row => { row.outputs = []; return row; }));
register('H6-WORKER-06', workerCase(row => { row.outputs.push('extra.json'); return row; }));
register('H6-WORKER-07', workerCase(row => { row.outputs.push(row.outputs[0]); return row; }));
register('H6-WORKER-08', workerCase(row => { row.status = undefined; return row; }));

function assertPositive(f, result) {
    assert.equal(result.worker_launches, 3);
    assert.equal(result.root_created, true);
    assert.deepEqual(result.workers.map(row => row.worker_id), ['worker-a', 'worker-b', 'worker-c']);
    assert.deepEqual(fs.readdirSync(f.request.runRoot).sort(), ['a.json', 'b.json', 'c.json']);
    for (const worker of result.workers) {
        assert.deepEqual(worker.dispatch.argv, [process.execPath, fixture.workerPath, 'pass']);
        assert.equal(worker.dispatch.mode, 'process');
        const stderr = JSON.parse(Buffer.from(worker.dispatch.stderr.base64, 'base64').toString());
        assert.deepEqual(stderr.environment, f.config.preflight.environment);
        const context = JSON.parse(Buffer.from(worker.dispatch.stdin.base64, 'base64').toString());
        assert.equal(context.run_root, f.request.runRoot);
        assert.equal(context.worker_id, worker.worker_id);
        for (const output of worker.outputs) {
            assert.equal(output.sha256, fixture.hash(fs.readFileSync(output.path)));
            assert.equal(output.byte_length, fs.statSync(output.path).size);
            assert.equal(fs.statSync(output.path).mode & 0o7777, 0o400);
        }
    }
    assert.deepEqual(result.verification_failures, []);
}

register('H6-CONTROL-01', {prepare: f => ({assertAfter: result => assertPositive(f, result)})});
register('H6-CONTROL-02', {prepare: f => ({assertAfter: result => assertPositive(f, result)})});
register('H6-CONTROL-03', {prepare: f => ({args: ['--preflight-receipt', f.request.receiptPaths[0],
    '--run-dir', f.request.runRoot, '--run', '--preflight-receipt', f.request.receiptPaths[1],
    '--config', f.request.configPath], assertAfter: result => assertPositive(f, result)})});

for (const [id, definition] of post.DEFINITIONS) register(id, definition);

function roadmapOriginalRows() {
    // Qualification pins an immutable copy as a real H2 input. Normal local
    // tests still check the live roadmap; later plan commits cannot rewrite an
    // already captured qualification's expectations.
    const roadmapPath = process.env.LC_OFFSET_ROADMAP_SNAPSHOT ||
        path.join(__dirname, '..', '..', 'docs', 'plan_offset_curve_2961.md');
    require('./input-capture').canonicalPath(roadmapPath);
    const plan = fs.readFileSync(roadmapPath, 'utf8');
    const table = plan.split('### 13.1 Preserved H6 Refusal Inventory\n')[1].split('**Correction to inherited expectations:**')[0];
    return table.split('\n').filter(line => line.startsWith('| H6-')).flatMap(line => {
        const cell = line.split('|')[1].trim();
        const [stage, reason] = line.split('|')[3].trim().split(' / ');
        const expected = id => ({id, stage, reason});
        const range = cell.match(/^(H6-[A-Z]+-)(\d+)([a-z])\.\.(\d+)([a-z])$/);
        if (range) {
            assert.equal(range[2], range[4]);
            return Array.from({length: range[5].charCodeAt(0) - range[3].charCodeAt(0) + 1}, (_, i) =>
                expected(`${range[1]}${range[2]}${String.fromCharCode(range[3].charCodeAt(0) + i)}`));
        }
        const parts = cell.split(',');
        return parts.map((value, i) => expected(i === 0 ? value : `${parts[0].slice(0, -2)}${value}`));
    }).sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0);
}

test('inventory preserves every expanded original identity and exactly accounts for executable and pending rows', t => {
    const all = inventory.INVENTORY.map(row => row.id);
    const sortedUnique = [...new Set(all)].sort();
    assert.equal(all.length, sortedUnique.length, 'inventory must not conceal duplicate identities');
    assert.deepEqual(inventory.IDS, sortedUnique);
    assert.equal(inventory.COUNT, sortedUnique.length);
    const roadmap = roadmapOriginalRows();
    assert.deepEqual(inventory.ORIGINAL_IDS, roadmap.map(row => row.id));
    assert.deepEqual(inventory.INVENTORY.filter(row => row.original).map(({id, stage, reason}) => ({id, stage, reason}))
        .sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0), roadmap,
    'original expected classifications must not broaden away from the roadmap');
    assert.deepEqual([...cases.keys()].sort(), inventory.EXECUTABLE.map(row => row.id).sort());
    assert.deepEqual([...inventory.EXECUTABLE, ...inventory.PENDING].map(row => row.id).sort(), sortedUnique);
    for (const row of inventory.PENDING) {
        assert.equal(cases.has(row.id), false, 'pending declaration cannot masquerade as an executed case');
        assert.ok(row.dependency && row.blockedBy);
        t.diagnostic(`PENDING ${row.id}: ${row.dependency}: ${row.blockedBy}`);
    }
    t.diagnostic(`Inventory: ${inventory.COUNT} unique obligations; ${inventory.ORIGINAL_IDS.length} original; ` +
        `${inventory.EXECUTABLE.length} executable; ${inventory.PENDING.length} pending declarations, not completed tests.`);
});

for (const entry of [...inventory.EXECUTABLE].sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0)) test(`${entry.id}: ${entry.condition}`, async t => {
    const definition = cases.get(entry.id);
    assert.ok(definition, `no implementation for ${entry.id}`);
    if (definition.execution === 'post') {
        const observed = await definition.execute(t);
        const error = observed.exception;
        const actual = {protocol: observed.protocol, status: observed.result ? observed.result.status : null,
            stage: error ? error.stage : observed.result.stage,
            reason: error ? error.reason : observed.result.reason,
            exceptionName: error ? error.name : null};
        const expected = {protocol: entry.protocol, status: entry.status, stage: entry.stage,
            reason: entry.reason, exceptionName: entry.exceptionName || null};
        assert.deepEqual(actual, expected, entry.id);
        assert.equal(observed.observations.scope, entry.scope);
        assert.equal(observed.observations.h3_worker_launches, 2);
        assert.equal(observed.observations.original_trees_unchanged, true);
        assert.equal(observed.observations.original_h3_unchanged, true);
        assert.equal(observed.observations.environment_unchanged, true);
        observed.cleanup();
        return;
    }
    const f = await fixture.setup(definition);
    t.diagnostic(`attempt artifacts: ${f.base}; retained on failure`);
    const scenario = definition.prepare ? (await definition.prepare(f) || {}) : {};
    const before = fixture.immutableSnapshot(f);
    const derivedBefore = fixture.tree(f.derived);
    const parentBefore = fixture.tree(f.runParent);
    let unexpectedLaunches = 0;
    const dependencies = {...scenario.dependencies};
    if (entry.phase === 'pre-root' && entry.ingress === 'transaction') dependencies.dispatch = () => {
        unexpectedLaunches++;
        throw new Error('pre-root refusal unexpectedly dispatched a worker');
    };
    let result;
    try {
        result = entry.ingress === 'cli' ? fixture.realCli(scenario.args || fixture.argumentsFor(f.request)) :
            await runFormalTransaction(f.request, dependencies);
    } finally {
        assert.deepEqual(fixture.immutableSnapshot(f), before, 'original captures, all pinned inputs, and full environment must remain immutable');
        assert.deepEqual(fixture.tree(f.derived), scenario.derivedAfter ? scenario.derivedAfter() : derivedBefore,
            'derived tree must retain exactly the deliberate mutation, with no runner writes');
    }
    const expected = {status: entry.status, stage: entry.stage, reason: entry.reason};
    const actual = {status: result.status, stage: result.stage, reason: result.reason};
    if (JSON.stringify(actual) !== JSON.stringify(expected)) {
        const mismatch = {id: entry.id, expected, actual, message: result.message, artifacts: f.base};
        mismatches.push(mismatch);
        t.diagnostic(`PRODUCTION_BOUNDARY_MISMATCH ${JSON.stringify(mismatch)}`);
    }
    if (entry.phase === 'pre-root') {
        assert.equal(unexpectedLaunches, 0);
        assert.equal(result.worker_launches, 0);
        assert.equal(result.root_created, false);
        assert.deepEqual(result.workers, []);
        assert.equal(fs.existsSync(f.request.runRoot), Boolean(scenario.externalRoot));
        if (!scenario.changesRunParent) assert.deepEqual(fixture.tree(f.runParent), parentBefore);
    } else {
        assert.equal(result.root_created, true);
        assert.equal(fs.existsSync(f.request.runRoot), true);
    }
    if (scenario.assertAfter) scenario.assertAfter(result);
    assert.deepEqual(actual, expected, `${entry.id}: ${result.message}`);
    fs.rmSync(f.base, {recursive: true, force: true});
});

test.after(() => {
    // These are failure observations for the parent, not a broadened oracle.
    process.stdout.write(`# H6_PRODUCTION_MISMATCHES ${JSON.stringify(mismatches)}\n`);
    process.stdout.write(`# H6_PENDING_DECLARATIONS ${JSON.stringify(inventory.PENDING)}\n`);
});
