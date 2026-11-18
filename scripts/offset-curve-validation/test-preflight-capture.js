/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const {EventEmitter} = require('node:events');
const {PassThrough, Writable} = require('node:stream');
const test = require('node:test');
const h1 = require('./preflight-receipt');
const inputs = require('./input-capture');
const {makeResult} = require('./fixtures/capture-child');
const {capturePreflight, normalizedEnvironment, nodeFsAdapter, MAX_CONTEXT_BYTES, CaptureError} = require('./preflight-capture');

function fixture(t, mode = 'pass') {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-capture-test-'));
    t.after(() => fs.rmSync(base, {recursive: true, force: true}));
    const source = path.join(base, 'source.js');
    const raw = path.join(base, 'raw.txt');
    const candidatePath = path.join(base, 'candidate.md');
    const caseInventoryPath = path.join(base, 'cases.json');
    const rootParent = path.join(base, 'root-parent');
    const receiptParent = path.join(base, 'receipt-parent');
    fs.mkdirSync(rootParent, {mode: 0o700});
    fs.mkdirSync(receiptParent, {mode: 0o700});
    fs.writeFileSync(source, 'module.exports = "actual source";\n');
    fs.writeFileSync(raw, 'actual input bytes\n');
    fs.writeFileSync(candidatePath, 'actual candidate bytes\n');
    fs.writeFileSync(caseInventoryPath, h1.canonicalJson({schema_version: 1, purpose: 'synthetic',
        cases: [{id: 'capture-control-01', source_path: source}]}));
    const config = {purpose: 'synthetic', candidatePath, runnerPath: path.join(__dirname, 'fixtures', 'capture-child.js'),
        runnerArgs: [mode], sourcePaths: [source], inputPaths: [raw], fixturePaths: [], configPaths: [], caseInventoryPath,
        cwd: base, environment: {LC_ALL: 'C', ...(process.platform === 'darwin' ?
            {__CF_USER_TEXT_ENCODING: process.env.__CF_USER_TEXT_ENCODING} : {})},
        receiptId: '10000000-0000-4000-8000-000000000001',
        runRoot: path.join(rootParent, 'formal-run'), receiptDirectory: path.join(receiptParent, 'receipt'), timeoutMs: 5000};
    return {base, source, raw, rootParent, receiptParent, config};
}

function fakeSpawn({beforeClose = () => {}, mutateResult = () => {}, stdout, stderr = Buffer.from('diagnostic\n'),
    exitCode = 0, signal = null, error = null} = {}) {
    return (executable, args, options) => {
        assert.equal(executable, fs.realpathSync.native(process.execPath));
        assert.equal(options.shell, false);
        assert.deepEqual(options.stdio, ['pipe', 'pipe', 'pipe']);
        assert.equal(Object.hasOwn(options.env, 'LC_OFFSET_CAPTURE_CONTEXT'), false);
        const child = new EventEmitter();
        child.stdin = new PassThrough();
        child.stdout = new PassThrough();
        child.stderr = new PassThrough();
        const stdin = [];
        child.stdin.on('data', chunk => stdin.push(Buffer.from(chunk)));
        let killed = false;
        child.kill = () => { if (!killed) { killed = true; setImmediate(() => child.emit('close', null, 'SIGKILL')); } return true; };
        child.stdin.on('finish', () => {
            const context = h1.parseCanonicalJson(Buffer.concat(stdin), {maxBytes: MAX_CONTEXT_BYTES});
            const result = makeResult(context);
            mutateResult(result);
            setImmediate(() => {
                child.stdout.end(stdout === undefined ? h1.canonicalJson(result) : stdout);
                child.stderr.end(stderr);
                beforeClose(context, options);
                if (error) child.emit('error', Object.assign(new Error(error.message), {code: error.code}));
                if (!killed) child.emit('close', exitCode, signal);
            });
        });
        return child;
    };
}

function diagnostic(error, stdout, stderr) {
    assert.ok(error instanceof CaptureError);
    if (stdout) assert.deepEqual(Buffer.from(error.details.stdout.base64, 'base64'), stdout);
    if (stderr) assert.deepEqual(Buffer.from(error.details.stderr.base64, 'base64'), stderr);
    assert.equal(Object.isFrozen(error.details.stdout), true);
    return true;
}

test('real child capture binds actual inputs/runtime and sanitized environment with exact H1 readback', async t => {
    const {config} = fixture(t);
    const env = JSON.stringify(Object.entries(process.env).sort());
    const pinned = inputs.captureInputs(config);
    const captured = await capturePreflight(config);
    assert.equal(captured.receipt.status, 'PASS');
    assert.equal(captured.receipt.purpose, 'synthetic');
    assert.equal(captured.validated.usable, true);
    assert.deepEqual(captured.receipt.candidate, pinned.candidate);
    assert.equal(captured.inputs.sourceManifestSha256, pinned.sourceManifestSha256);
    assert.equal(captured.receipt.source_manifest_sha256, pinned.sourceManifestSha256);
    assert.deepEqual(captured.rootParent, {device: fs.statSync(path.dirname(config.runRoot), {bigint: true}).dev,
        inode: fs.statSync(path.dirname(config.runRoot), {bigint: true}).ino});
    assert.deepEqual(fs.readdirSync(config.receiptDirectory).sort(), [config.receiptId + '.json', config.receiptId + '.stderr', config.receiptId + '.stdout']);
    assert.equal(fs.statSync(config.receiptDirectory).mode & 0o7777, 0o700);
    for (const suffix of ['json', 'stdout', 'stderr']) {
        const stat = fs.statSync(path.join(config.receiptDirectory, config.receiptId + '.' + suffix));
        assert.equal(stat.mode & 0o7777, 0o400);
        assert.equal(stat.nlink, 1);
    }
    const stderr = JSON.parse(Buffer.from(captured.validated.snapshot.stderr.base64, 'base64'));
    assert.deepEqual(stderr.keys, Object.keys(captured.environment).sort());
    assert.deepEqual(stderr.keys, ['LC_ALL', ...(process.platform === 'darwin' ? ['__CF_USER_TEXT_ENCODING'] : [])].sort());
    assert.deepEqual(stderr.environment, captured.environment);
    assert.equal(captured.stdin.write_completed, true);
    assert.equal(stderr.stdin_bytes, captured.stdin.bytes);
    assert.equal(stderr.stdin_sha256, captured.stdin.sha256);
    assert.ok(captured.stdin.bytes <= MAX_CONTEXT_BYTES);
    const context = h1.parseCanonicalJson(Buffer.from(captured.stdin.base64, 'base64'), {maxBytes: MAX_CONTEXT_BYTES});
    assert.deepEqual(context.candidate, captured.inputs.candidate);
    assert.equal(context.source_manifest_sha256, captured.inputs.sourceManifestSha256);
    assert.deepEqual(context.run_root, captured.receipt.run_root);
    assert.equal(stderr.exec_path, process.execPath);
    assert.equal(stderr.version, process.version);
    assert.equal(JSON.stringify(Object.entries(process.env).sort()), env);
    assert.doesNotThrow(() => inputs.revalidateInputs(pinned));
    assert.equal(fs.existsSync(config.runRoot), false);
    assert.ok(captured.observations.includes('pre-spawn') && captured.observations.includes('final-readback'));
});

test('the real fixture refuses validation purpose and injected validation execution is forbidden', async t => {
    const {config} = fixture(t);
    await assert.rejects(capturePreflight({...config, purpose: 'validation'}, {spawn: fakeSpawn()}), /restricted to synthetic/);
    const inventory = JSON.parse(fs.readFileSync(config.caseInventoryPath));
    inventory.purpose = 'validation';
    fs.writeFileSync(config.caseInventoryPath, h1.canonicalJson(inventory));
    await assert.rejects(capturePreflight({...config, purpose: 'validation'}), /synthetic fixture/);
    assert.equal(fs.existsSync(config.receiptDirectory), false);
});

test('purpose is pinned before hooks and cannot transition from synthetic to validation', async t => {
    const {config} = fixture(t);
    let reads = 0;
    Object.defineProperty(config, 'purpose', {get() { return ++reads < 3 ? 'synthetic' : 'validation'; },
        enumerable: true, configurable: true});
    let launches = 0;
    await assert.rejects(capturePreflight(config, {spawn() { ++launches; }}), /own data properties/);
    assert.equal(reads, 0);
    assert.equal(launches, 0);
    assert.equal(fs.existsSync(config.receiptDirectory), false);
    assert.equal(fs.existsSync(config.runRoot), false);

    Object.defineProperty(config, 'purpose', {value: 'synthetic', writable: true, enumerable: true});
    const spawn = fakeSpawn();
    const result = await capturePreflight(config, {spawn(...args) {
        config.purpose = 'validation';
        return spawn(...args);
    }});
    assert.equal(config.purpose, 'validation');
    assert.equal(result.receipt.purpose, 'synthetic');
    assert.equal(result.receipt.preflight_result.purpose, 'synthetic');
    assert.equal(result.inputs.manifest.purpose, 'synthetic');
});

test('validation rejects inherited and non-enumerable spawn/fs before input capture or publication', async t => {
    for (const shape of ['inherited', 'non-enumerable']) for (const key of ['spawn', 'fs']) {
        const {base, config, source} = fixture(t);
        config.purpose = 'validation';
        config.runnerPath = source;
        const inventory = JSON.parse(fs.readFileSync(config.caseInventoryPath));
        inventory.purpose = 'validation';
        fs.writeFileSync(config.caseInventoryPath, h1.canonicalJson(inventory));
        let spawns = 0;
        let filesystemCalls = 0;
        const value = key === 'spawn' ? () => { spawns++; throw new Error('must not spawn'); } :
            {...nodeFsAdapter, lstatSync: p => { filesystemCalls++; return nodeFsAdapter.lstatSync(p); }};
        const dependencies = shape === 'inherited' ? Object.create({[key]: value}) :
            Object.defineProperty({}, key, {value});
        const snapshot = () => fs.readdirSync(base).sort().map(name => {
            const p = path.join(base, name);
            const stat = fs.lstatSync(p, {bigint: true});
            return [name, inputs.statIdentity(stat), stat.isFile() ? h1.sha256(fs.readFileSync(p)) : null];
        });
        const before = snapshot();
        await assert.rejects(capturePreflight(config, dependencies), error => {
            assert.ok(error instanceof CaptureError);
            assert.equal(error.stage, 'configuration');
            assert.match(error.message, /restricted to synthetic/);
            assert.equal(error.details.inputs, null);
            assert.equal(error.details.child, null);
            assert.equal(error.details.receiptDirectory, null);
            return true;
        });
        assert.equal(spawns, 0);
        assert.equal(filesystemCalls, 0);
        assert.equal(fs.existsSync(config.runRoot), false);
        assert.equal(fs.existsSync(config.receiptDirectory), false);
        assert.deepEqual(fs.readdirSync(path.dirname(config.runRoot)), []);
        assert.deepEqual(fs.readdirSync(path.dirname(config.receiptDirectory)), []);
        assert.deepEqual(snapshot(), before);
    }
});

test('validation refuses dependency accessors without evaluating inherited or own getters', async t => {
    for (const shape of ['inherited', 'enumerable', 'non-enumerable']) for (const key of ['spawn', 'fs']) {
        const {config} = fixture(t);
        let reads = 0;
        const holder = Object.defineProperty({}, key, {enumerable: shape === 'enumerable', get() {
            reads++;
            throw new Error('dependency getter must not execute');
        }});
        const dependencies = shape === 'inherited' ? Object.create(holder) : holder;
        await assert.rejects(capturePreflight({...config, purpose: 'validation'}, dependencies), error => {
            assert.ok(error instanceof CaptureError);
            assert.equal(error.stage, 'configuration');
            assert.match(error.message, /restricted to synthetic/);
            assert.equal(error.details.inputs, null);
            assert.equal(error.details.receiptDirectory, null);
            return true;
        });
        assert.equal(reads, 0);
        assert.equal(fs.existsSync(config.receiptDirectory), false);
        assert.equal(fs.existsSync(config.runRoot), false);
    }
});

test('synthetic own data hooks work on ordinary and null-prototype objects regardless of enumerability', async t => {
    for (const nullPrototype of [false, true]) for (const enumerable of [false, true]) {
        const {config} = fixture(t);
        const dependencies = Object.create(nullPrototype ? null : Object.prototype, {
            fs: {value: nodeFsAdapter, enumerable}, spawn: {value: fakeSpawn(), enumerable}
        });
        const result = await capturePreflight(config, dependencies);
        assert.equal(result.receipt.purpose, 'synthetic');
        assert.equal(result.receipt.status, 'PASS');
        assert.equal(result.validated.usable, true);
        assert.equal(fs.existsSync(config.runRoot), false);
        assert.equal(fs.existsSync(config.receiptDirectory), true);
    }
});

test('synthetic dependency shape rejects accessors, inherited hooks, unknown keys and invalid values before I/O', async t => {
    const {config} = fixture(t);
    let reads = 0;
    for (const dependencies of [null, [], () => {}, 'not an object',
        {unknown: true}, {[Symbol('spawn')]: () => {}}, {spawn: undefined}, {fs: null},
        Object.create({spawn: fakeSpawn()}),
        Object.defineProperty({}, 'fs', {get() { reads++; throw new Error('must not evaluate getter'); }})]) {
        await assert.rejects(capturePreflight(config, dependencies), error => {
            assert.ok(error instanceof CaptureError);
            assert.equal(error.stage, 'configuration');
            assert.equal(error.details.inputs, null);
            assert.equal(error.details.child, null);
            assert.equal(error.details.receiptDirectory, null);
            return true;
        });
        assert.equal(fs.existsSync(config.runRoot), false);
        assert.equal(fs.existsSync(config.receiptDirectory), false);
    }
    assert.equal(reads, 0);
});

test('two real-child receipts join H1 with identical actual argv/environment and distinct stdin root contexts', async t => {
    const {base, config} = fixture(t);
    const secondRootParent = path.join(base, 'root-parent-second');
    const secondReceiptParent = path.join(base, 'receipt-parent-second');
    fs.mkdirSync(secondRootParent, {mode: 0o700});
    fs.mkdirSync(secondReceiptParent, {mode: 0o700});
    const secondConfig = {...config, receiptId: '20000000-0000-4000-8000-000000000002',
        runRoot: path.join(secondRootParent, 'formal-run'), receiptDirectory: path.join(secondReceiptParent, 'receipt')};
    const pinned = inputs.captureInputs(config);
    const environmentBefore = JSON.stringify(Object.entries(process.env).sort());
    const first = await capturePreflight(config);
    const second = await capturePreflight(secondConfig);
    assert.deepEqual(first.inputs.manifest, second.inputs.manifest);
    assert.deepEqual(first.inputs.runtime, second.inputs.runtime);
    assert.deepEqual(first.receipt.argv, second.receipt.argv);
    assert.equal(first.receipt.cwd, second.receipt.cwd);
    assert.deepEqual(first.environment, second.environment);
    const actualEnvironmentSha = h1.sha256(h1.canonicalJson(first.environment));
    assert.equal(first.receipt.environment_sha256, actualEnvironmentSha);
    assert.equal(second.receipt.environment_sha256, actualEnvironmentSha);
    assert.equal(Object.hasOwn(first.environment, 'LC_OFFSET_CAPTURE_CONTEXT'), false);
    assert.notEqual(first.stdin.sha256, second.stdin.sha256);
    const contexts = [first, second].map(capture => h1.parseCanonicalJson(Buffer.from(capture.stdin.base64, 'base64'),
        {maxBytes: MAX_CONTEXT_BYTES}));
    assert.notEqual(contexts[0].run_root.path, contexts[1].run_root.path);
    assert.notDeepEqual(contexts[0].run_root.parent, contexts[1].run_root.parent);
    for (const capture of [first, second]) {
        assert.equal(capture.stdin.write_completed, true);
        const reported = JSON.parse(Buffer.from(capture.validated.snapshot.stderr.base64, 'base64'));
        assert.deepEqual(reported.environment, capture.environment);
        assert.equal(reported.stdin_sha256, capture.stdin.sha256);
    }
    const expected = {purpose: 'synthetic', candidate: pinned.candidate,
        sourceManifestSha256: pinned.sourceManifestSha256, argv: first.receipt.argv,
        cwd: config.cwd, environmentSha256: actualEnvironmentSha};
    const joined = h1.validateReceiptSet([first.receiptPath, second.receiptPath], {expected});
    assert.equal(joined.receipts.length, 2);
    assert.ok(joined.receipts.every(capture => capture.usable));
    assert.doesNotThrow(() => h1.revalidateReceiptSet(joined, {expected}));
    assert.doesNotThrow(() => inputs.revalidateInputs(pinned));
    assert.equal(fs.existsSync(config.runRoot), false);
    assert.equal(fs.existsSync(secondConfig.runRoot), false);
    assert.equal(JSON.stringify(Object.entries(process.env).sort()), environmentBefore);
});

test('stdin write failure preserves child diagnostics and checks original roots/inputs before refusal', async t => {
    const {config} = fixture(t);
    const stdout = Buffer.from('stdout after stdin failure\n');
    const stderr = Buffer.from('stderr after stdin failure\n');
    const spawn = () => {
        const child = new EventEmitter();
        child.stdin = new Writable({write: (chunk, encoding, callback) => {
            callback(Object.assign(new Error('injected stdin EPIPE'), {code: 'EPIPE'}));
        }});
        child.stdout = new PassThrough();
        child.stderr = new PassThrough();
        child.kill = () => true;
        setImmediate(() => {
            child.stdout.end(stdout);
            child.stderr.end(stderr);
            child.emit('close', 7, null);
        });
        return child;
    };
    await assert.rejects(capturePreflight(config, {spawn}), error => {
        diagnostic(error, stdout, stderr);
        assert.equal(error.details.child.error.code, 'EPIPE');
        assert.equal(error.details.captureFailure, 'stdin write error');
        assert.equal(error.details.stdin.write_completed, false);
        assert.ok(error.details.stdin.bytes > 0 && error.details.stdin.bytes <= MAX_CONTEXT_BYTES);
        assert.equal(error.details.stdin.sha256, h1.sha256(Buffer.from(error.details.stdin.base64, 'base64')));
        assert.deepEqual(error.details.verificationFailures, []);
        return true;
    });
    assert.equal(fs.existsSync(config.receiptDirectory), false);
    assert.equal(fs.existsSync(config.runRoot), false);
});

test('environment is explicit, bounded, Unicode-safe and excludes loader injection', () => {
    assert.deepEqual(normalizedEnvironment({Z: '', A: 'yes'}), {A: 'yes', Z: ''});
    const prototypeName = normalizedEnvironment({['__proto__']: 'literal environment value'});
    assert.equal(Object.hasOwn(prototypeName, '__proto__'), true);
    assert.equal(prototypeName.__proto__, 'literal environment value');
    assert.equal(Object.getPrototypeOf(prototypeName), Object.prototype);
    for (const key of ['NODE_OPTIONS', 'NODE_PATH', 'LD_PRELOAD', 'DYLD_LIBRARY_PATH', 'OPENSSL_CONF', 'LC_OFFSET_CAPTURE_CONTEXT'])
        assert.throws(() => normalizedEnvironment({[key]: 'bad'}), /unsafe environment/);
    assert.throws(() => normalizedEnvironment({A: '\0'}), /NUL-free/);
    assert.throws(() => normalizedEnvironment({A: '\ud800'}), /Unicode/);
    assert.throws(() => normalizedEnvironment({A: 'x'.repeat(8193)}), /bounded/);
    assert.throws(() => normalizedEnvironment(undefined), /explicit environment/);
});

test('missing raw input and existing receipt path refuse before child launch without mutation', async t => {
    const {config, raw} = fixture(t);
    let launched = false;
    fs.unlinkSync(raw);
    await assert.rejects(capturePreflight(config, {spawn: () => { launched = true; }}), /ENOENT/);
    assert.equal(launched, false);
    fs.writeFileSync(raw, 'restored raw\n');
    fs.mkdirSync(config.receiptDirectory);
    fs.writeFileSync(path.join(config.receiptDirectory, 'sentinel'), 'do not overwrite');
    await assert.rejects(capturePreflight(config, {spawn: () => { launched = true; }}), /already exists/);
    assert.equal(launched, false);
    assert.equal(fs.readFileSync(path.join(config.receiptDirectory, 'sentinel'), 'utf8'), 'do not overwrite');
});

test('nonzero real child preserves binary stdout/stderr and exit diagnostics without fabricating a receipt', async t => {
    const {config} = fixture(t, 'fail');
    await assert.rejects(capturePreflight(config), error => {
        diagnostic(error, Buffer.from([0, 255, 10]), Buffer.from('real child failed\n'));
        assert.equal(error.details.child.exit_code, 7);
        assert.deepEqual(error.details.verificationFailures, []);
        return true;
    });
    assert.equal(fs.existsSync(config.receiptDirectory), false);
    assert.equal(fs.existsSync(config.runRoot), false);
});

test('malformed successful-child output is preserved, not repaired', async t => {
    const {config} = fixture(t, 'malformed');
    await assert.rejects(capturePreflight(config), error => diagnostic(error, Buffer.from('not JSON\n')));
    assert.equal(fs.existsSync(config.receiptDirectory), false);
});

test('real stream overflow retains the bounded prefix and waits for killed-child completion', async t => {
    const {config} = fixture(t, 'overflow');
    await assert.rejects(capturePreflight({...config, maxStreamBytes: 128}), error => {
        diagnostic(error, Buffer.alloc(128, 0x78));
        assert.match(error.details.captureFailure, /stdout exceeded/);
        assert.equal(error.details.child.signal, 'SIGKILL');
        assert.ok(error.details.observed.stdout > 128);
        return true;
    });
    assert.equal(fs.existsSync(config.receiptDirectory), false);
});

test('real child timeout and signal completion retain actual diagnostic bytes', async t => {
    const {config} = fixture(t, 'timeout');
    await assert.rejects(capturePreflight({...config, timeoutMs: 1000}), error => {
        diagnostic(error, Buffer.from('before timeout\n'));
        assert.equal(error.details.captureFailure, 'child timeout');
        assert.equal(error.details.child.signal, 'SIGKILL');
        return true;
    });
    await assert.rejects(capturePreflight({...config, runnerArgs: ['signal']}), error => {
        diagnostic(error, null, Buffer.from('before signal\n'));
        assert.equal(error.details.child.signal, 'SIGTERM');
        return true;
    });
});

test('same original bigint root-parent identity is required on success and child failure paths', async t => {
    for (const exitCode of [0, 7]) {
        const {config, rootParent} = fixture(t);
        const original = fs.lstatSync(rootParent, {bigint: true});
        const stdout = Buffer.from('retained stdout\n');
        await assert.rejects(capturePreflight(config, {spawn: fakeSpawn({stdout, exitCode, beforeClose: () => {
            fs.renameSync(rootParent, rootParent + '.original');
            fs.mkdirSync(rootParent, {mode: 0o700});
        }})}), error => {
            diagnostic(error, stdout, Buffer.from('diagnostic\n'));
            assert.ok(error.details.verificationFailures.some(row => row.check === 'root' && /identity changed/.test(row.message)));
            return true;
        });
        assert.notEqual(fs.lstatSync(rootParent, {bigint: true}).ino, original.ino);
        assert.equal(fs.existsSync(config.receiptDirectory), false);
    }
});

test('root creation by a real child is refused and the external tree is preserved', async t => {
    const {config} = fixture(t, 'root-write');
    await assert.rejects(capturePreflight(config), error => {
        assert.ok(error.details.stdout.bytes > 0);
        assert.ok(error.details.verificationFailures.some(row => row.check === 'root' && /already exists/.test(row.message)));
        return true;
    });
    assert.equal(fs.readFileSync(path.join(config.runRoot, 'sentinel'), 'utf8'), 'external root\n');
    assert.equal(fs.existsSync(config.receiptDirectory), false);
});

test('input mutation on a failed-child path is checked without losing child diagnostics', async t => {
    const {config, raw} = fixture(t);
    const stdout = Buffer.from('failed output\n');
    await assert.rejects(capturePreflight(config, {spawn: fakeSpawn({stdout, exitCode: 9,
        beforeClose: () => fs.appendFileSync(raw, 'mutation')})}), error => {
        diagnostic(error, stdout, Buffer.from('diagnostic\n'));
        assert.equal(error.details.child.exit_code, 9);
        assert.ok(error.details.verificationFailures.some(row => row.check === 'inputs' && /changed/.test(row.message)));
        return true;
    });
});

test('synchronous spawn errors still check original roots and preserve the spawn cause', async t => {
    const {config} = fixture(t);
    await assert.rejects(capturePreflight(config, {spawn: () => {
        fs.mkdirSync(config.runRoot);
        throw Object.assign(new Error('spawn failed'), {code: 'EACCES'});
    }}), error => {
        assert.equal(error.details.child.error.code, 'EACCES');
        assert.equal(error.details.captureFailure, 'spawn threw');
        assert.ok(error.details.verificationFailures.some(row => row.check === 'root'));
        return true;
    });
    assert.equal(fs.existsSync(config.runRoot), true);
});

test('parent environment mutation is detected and never silently restored by capture', async t => {
    const {config} = fixture(t);
    const key = 'LC_CAPTURE_TEST_PARENT_MUTATION';
    const saved = process.env[key];
    try {
        await assert.rejects(capturePreflight(config, {spawn: fakeSpawn({beforeClose: () => { process.env[key] = 'changed'; }})}), error => {
            assert.ok(error.details.verificationFailures.some(row => row.check === 'parent_environment'));
            return true;
        });
        assert.equal(process.env[key], 'changed');
    } finally { if (saved === undefined) delete process.env[key]; else process.env[key] = saved; }
});

test('child cannot replace the captured tuple, purpose or exact case inventory', async t => {
    for (const mutateResult of [
        result => { result.candidate.sha256 = 'a'.repeat(64); },
        result => { result.purpose = 'validation'; },
        result => { result.inventory_gate.actual_count = result.inventory_gate.expected_count = 2; },
        result => { result.inventory_gate.actual_ids_sha256 = result.inventory_gate.expected_ids_sha256 = 'b'.repeat(64); }
    ]) {
        const {config} = fixture(t);
        await assert.rejects(capturePreflight(config, {spawn: fakeSpawn({mutateResult})}), /purpose|tuple|candidate|inventory/);
        assert.equal(fs.existsSync(config.receiptDirectory), false);
    }
});

test('file fsync failure preserves the partial attempt and original child bytes', async t => {
    const {config} = fixture(t);
    const io = {...nodeFsAdapter, fsyncSync: fd => {
        if (fs.fstatSync(fd).isFile()) throw Object.assign(new Error('injected fsync EIO'), {code: 'EIO'});
        fs.fsyncSync(fd);
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), error => {
        assert.equal(error.details.causeCode, 'EIO');
        assert.ok(error.details.stdout.bytes > 0);
        return true;
    });
    assert.deepEqual(fs.readdirSync(config.receiptDirectory), [config.receiptId + '.stdout']);
});

test('unsupported directory fsync is explicitly recorded, not confused with file fsync success', async t => {
    const {config} = fixture(t);
    const io = {...nodeFsAdapter, fsyncSync: fd => {
        if (fs.fstatSync(fd).isDirectory()) throw Object.assign(new Error('unsupported directory flush'), {code: 'EINVAL'});
        fs.fsyncSync(fd);
    }};
    const result = await capturePreflight(config, {fs: io, spawn: fakeSpawn()});
    assert.equal(result.durabilityLimitations.length, 2);
    assert.ok(result.durabilityLimitations.every(message => /directory fsync unavailable/.test(message)));
});

test('input mutation during publication is refused and preserves published diagnostic artifacts', async t => {
    const {config, source} = fixture(t);
    let changed = false;
    const io = {...nodeFsAdapter, fsyncSync: fd => {
        fs.fsyncSync(fd);
        if (fs.fstatSync(fd).isFile() && !changed) { changed = true; fs.appendFileSync(source, '// changed\n'); }
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), /captured input changed/);
    assert.equal(fs.readdirSync(config.receiptDirectory).length, 3);
});

test('same-byte publication substitution is refused by write-time inode identity', async t => {
    const {config} = fixture(t);
    let changed = false;
    const stdoutPath = path.join(config.receiptDirectory, config.receiptId + '.stdout');
    const io = {...nodeFsAdapter, fsyncSync: fd => {
        fs.fsyncSync(fd);
        if (fs.fstatSync(fd).isFile() && !changed) {
            changed = true;
            const bytes = fs.readFileSync(stdoutPath);
            fs.renameSync(stdoutPath, stdoutPath + '.original');
            fs.writeFileSync(stdoutPath, bytes, {flag: 'wx', mode: 0o400});
        }
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), /published file identity/);
    assert.equal(changed, true);
    assert.equal(fs.existsSync(stdoutPath + '.original'), true);
});

test('exclusive publication refuses an injected existing sidecar without overwriting it', async t => {
    const {config} = fixture(t);
    const io = {...nodeFsAdapter, mkdirSync: (p, options) => {
        fs.mkdirSync(p, options);
        if (p === config.receiptDirectory) fs.writeFileSync(path.join(p, config.receiptId + '.stdout'), 'external sentinel');
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), /EEXIST/);
    assert.equal(fs.readFileSync(path.join(config.receiptDirectory, config.receiptId + '.stdout'), 'utf8'), 'external sentinel');
});

test('H1 opendir readback refuses extra members and keeps the failed tree', async t => {
    const {config} = fixture(t);
    const io = {...nodeFsAdapter, opendirSync: p => {
        const dir = fs.opendirSync(p, {bufferSize: 1});
        let done = false;
        return {readSync: () => { const entry = dir.readSync(); if (entry) return entry;
            if (!done) { done = true; return {name: 'injected-extra'}; } return null; }, closeSync: () => dir.closeSync()};
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), /membership/);
    assert.equal(fs.readdirSync(config.receiptDirectory).length, 3);
});

test('zero writes and invalid stream/timeout caps fail explicitly', async t => {
    const {config} = fixture(t);
    await assert.rejects(capturePreflight({...config, maxStreamBytes: h1.MAX_STREAM_BYTES + 1}), /stream byte cap/);
    await assert.rejects(capturePreflight({...config, timeoutMs: 0}), /timeout/);
    await assert.rejects(capturePreflight(config, {spawn: fakeSpawn(), fs: {...nodeFsAdapter, writeSync: () => 0}}), /zero write/);
});

test('original run-root parent remains pinned through publication and failure propagation', async t => {
    const {config, rootParent} = fixture(t);
    let changed = false;
    const original = fs.lstatSync(rootParent, {bigint: true});
    const io = {...nodeFsAdapter, fsyncSync: fd => {
        fs.fsyncSync(fd);
        if (fs.fstatSync(fd).isFile() && !changed) {
            changed = true;
            fs.renameSync(rootParent, rootParent + '.original');
            fs.mkdirSync(rootParent, {mode: 0o700});
        }
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), error => {
        assert.ok(error.details.verificationFailures.some(row => row.stage === 'post-publish' && row.check === 'root'));
        assert.ok(error.details.verificationFailures.some(row => row.stage === 'failure' && row.check === 'root'));
        assert.ok(error.details.stdout.bytes > 0);
        return true;
    });
    assert.notEqual(fs.lstatSync(rootParent, {bigint: true}).ino, original.ino);
    assert.equal(fs.readdirSync(config.receiptDirectory).length, 3);
});

test('receipt-parent replacement at exclusive directory creation is refused without following the replacement', async t => {
    const {config, receiptParent} = fixture(t);
    let changed = false;
    const io = {...nodeFsAdapter, mkdirSync: (p, options) => {
        fs.mkdirSync(p, options);
        if (p === config.receiptDirectory) {
            changed = true;
            fs.renameSync(receiptParent, receiptParent + '.original');
            fs.mkdirSync(receiptParent, {mode: 0o700});
        }
    }};
    await assert.rejects(capturePreflight(config, {fs: io, spawn: fakeSpawn()}), /directory identity changed/);
    assert.equal(changed, true);
    assert.deepEqual(fs.readdirSync(receiptParent), []);
    assert.deepEqual(fs.readdirSync(path.join(receiptParent + '.original', 'receipt')), []);
});

test('macOS environment injection must be explicitly pinned rather than omitted from the digest', async t => {
    if (process.platform !== 'darwin') return;
    const {config} = fixture(t);
    delete config.environment.__CF_USER_TEXT_ENCODING;
    let launched = false;
    await assert.rejects(capturePreflight(config, {spawn: () => { launched = true; }}), /explicitly pinned/);
    assert.equal(launched, false);
});

test('asynchronous spawn errors retain streams and completion metadata after boundary checks', async t => {
    const {config} = fixture(t);
    const stdout = Buffer.from('partial spawn stdout\n');
    const stderr = Buffer.from('partial spawn stderr\n');
    await assert.rejects(capturePreflight(config, {spawn: fakeSpawn({stdout, stderr, exitCode: null,
        error: {code: 'EIO', message: 'injected asynchronous spawn error'}})}), error => {
        diagnostic(error, stdout, stderr);
        assert.deepEqual(error.details.child, {exit_code: null, signal: null,
            error: {code: 'EIO', message: 'injected asynchronous spawn error'}});
        assert.deepEqual(error.details.verificationFailures, []);
        return true;
    });
    assert.equal(fs.existsSync(config.receiptDirectory), false);
});
