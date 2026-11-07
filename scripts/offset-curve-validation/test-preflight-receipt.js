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
const descriptors = require('./schema-descriptors');
const test = require('node:test');
const schema = require('./preflight-receipt');

const HASH_A = 'a'.repeat(64);
const HASH_B = 'b'.repeat(64);
const HASH_C = 'c'.repeat(64);
const HASH_D = 'd'.repeat(64);

function candidate() {
    return {path: '/tmp/frozen-candidate.md', bytes: 17, sha256: HASH_A, git_blob: '1'.repeat(40)};
}

function rootProof(rootPath, parentPath, parentInode) {
    return {
        path: rootPath,
        absent_before: true,
        absent_after: true,
        parent_stable: true,
        parent: {path: parentPath, device: '1', inode: String(parentInode)},
        write_attempts: 0,
        error: null
    };
}

function preflightResult(root, parentPath, parentInode) {
    return {
        revision: schema.REVISION,
        purpose: 'synthetic',
        status: 'PASS',
        candidate: candidate(),
        source_manifest_sha256: HASH_B,
        inventory_gate: {
            status: 'PASS', expected_count: 1, actual_count: 1,
            expected_ids_sha256: HASH_C, actual_ids_sha256: HASH_C,
            missing_ids: [], extra_ids: [], duplicate_ids: []
        },
        patch_replays: {
            status: 'PASS',
            directions: schema.PATCH_DIRECTIONS.map(name => ({name, status: 'PASS', patch_sha256: HASH_A, result_sha256: HASH_B}))
        },
        audit_parity: {status: 'PASS', primary_sha256: HASH_C, independent_sha256: HASH_C, equal: true},
        suite_parity: {status: 'PASS', suite_count: 4, identical_repeated_outputs: true},
        suites: schema.PREFLIGHT_SUITES.map(name => ({
            name, status: 'PASS', attempts: [1, 2].map(ordinal => ({
                ordinal, status: 'PASS', report_bytes: 10, report_sha256: HASH_D, assertions: 1
            }))
        })),
        syntax: {status: 'PASS', checked_file_count: 1, checked_paths_sha256: HASH_A},
        caps: {status: 'PASS', checks: [{name: 'test-cap', limit: 10, measured: 1, passed: true}]},
        trace_probe: {status: 'PASS', stdout_sha256: HASH_B, stderr_sha256: HASH_C, exit_code: 0},
        run_root: rootProof(root, parentPath, parentInode)
    };
}

function receipt(id, root, parentPath, parentInode, directory) {
    const result = preflightResult(root, parentPath, parentInode);
    const stdout = schema.canonicalJson(result);
    const stderr = Buffer.alloc(0);
    const stream = (streamPath, bytes) => ({
        path: streamPath, bytes: bytes.length, sha256: schema.sha256(bytes), file_sha256: schema.sha256(bytes)
    });
    return {
        schema_version: schema.SCHEMA_VERSION,
        revision: schema.REVISION,
        purpose: 'synthetic',
        receipt_id: id,
        candidate: candidate(),
        source_manifest_sha256: HASH_B,
        argv: ['/usr/bin/node', '/tmp/frozen-runner.js'],
        cwd: '/tmp',
        environment_sha256: HASH_C,
        child: {exit_code: 0, signal: null, error: null},
        stdout: stream(path.join(directory, `${id}.stdout`), stdout),
        stderr: stream(path.join(directory, `${id}.stderr`), stderr),
        preflight_result: result,
        run_root: result.run_root,
        status: 'PASS'
    };
}

function writeReceipt(directory, value) {
    const id = value.receipt_id;
    fs.mkdirSync(directory, {recursive: true, mode: 0o700});
    fs.chmodSync(directory, 0o700);
    const stdout = schema.canonicalJson(value.preflight_result);
    const stderr = Buffer.alloc(0);
    fs.writeFileSync(value.stdout.path, stdout, {flag: 'wx', mode: 0o400});
    fs.writeFileSync(value.stderr.path, stderr, {flag: 'wx', mode: 0o400});
    const reportPath = path.join(directory, `${id}.json`);
    fs.writeFileSync(reportPath, schema.canonicalJson(value), {flag: 'wx', mode: 0o400});
    for (const file of [value.stdout.path, value.stderr.path, reportPath]) fs.chmodSync(file, 0o400);
    return reportPath;
}

function fixturePair(mutator = () => {}) {
    const base = fs.mkdtempSync(path.join(fs.realpathSync(os.tmpdir()), 'lc-offset-receipt-test-'));
    const reports = [];
    const tuples = [
        {id: '10000000-0000-4000-8000-000000000001', receiptDir: path.join(base, 'receipt-parent-a', 'receipt-a'), rootParent: path.join(base, 'root-parent-a'), root: path.join(base, 'root-parent-a', 'formal-run')},
        {id: '20000000-0000-4000-8000-000000000002', receiptDir: path.join(base, 'receipt-parent-b', 'receipt-b'), rootParent: path.join(base, 'root-parent-b'), root: path.join(base, 'root-parent-b', 'formal-run')}
    ];
    for (let i = 0; i < tuples.length; i++) {
        const t = tuples[i];
        fs.mkdirSync(path.dirname(t.receiptDir), {recursive: true, mode: 0o700});
        fs.mkdirSync(t.rootParent, {recursive: true, mode: 0o700});
        const parentStat = fs.statSync(t.rootParent, {bigint: true});
        const value = receipt(t.id, t.root, t.rootParent, parentStat.ino, t.receiptDir);
        mutator(value, i, t);
        reports.push(writeReceipt(t.receiptDir, value));
    }
    return {base, reports};
}

function withFixture(fn, mutator) {
    const fixture = fixturePair(mutator);
    try { return fn(fixture); }
    finally { fs.rmSync(fixture.base, {recursive: true, force: true}); }
}

function fileSystemAdapter(changes = {}) {
    return {
        constants: fs.constants,
        lstatSync: p => fs.lstatSync(p, {bigint: true}),
        openSync: (...args) => fs.openSync(...args),
        fstatSync: fd => fs.fstatSync(fd, {bigint: true}),
        readSync: (...args) => fs.readSync(...args),
        closeSync: fd => fs.closeSync(fd),
        realpathSync: p => fs.realpathSync.native(p),
        opendirSync: p => fs.opendirSync(p, {bufferSize: 1}),
        ...changes
    };
}

test('schemas are immutable and expose the one pinned result shape', () => {
    assert.equal(Object.isFrozen(schema.RECEIPT_SCHEMA), true);
    assert.equal(Object.isFrozen(schema.PREFLIGHT_RESULT_SCHEMA), true);
    assert.deepEqual(schema.PREFLIGHT_RESULT_SCHEMA.suite_names, [
        'receipt-schema', 'capture-readback', 'formal-runner-boundary', 'genesis-ledger-p0f-joins'
    ]);
    assert.throws(() => schema.RECEIPT_SCHEMA.revision = 'other', TypeError);
});

test('canonical parser rejects duplicates, noncanonical, and malformed encodings', () => {
    for (const text of [
        '{"a":1,"a":1}\n',
        '{ "a":1}\n',
        '{"a":1}\n\n',
        '\uFEFF{}\n',
        '{}\r\n'
    ]) assert.throws(() => schema.parseCanonicalJson(Buffer.from(text), {label: 'test'}));
    assert.throws(() => schema.parseCanonicalJson(Buffer.from([0xc3, 0x28, 0x0a]), {label: 'test'}));
    assert.deepEqual(schema.parseCanonicalJson(Buffer.from('{"a":1}\n')), {a: 1});
});

test('valid pair validates, snapshots, and revalidates byte-identically', () => {
    withFixture(({reports}) => {
        const first = schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}, requirePass: true});
        assert.equal(first.receipts.length, 2);
        assert.equal(Object.isFrozen(first.receipts[0].receipt), true);
        assert.equal(Object.isFrozen(first.receipts[0].receipt.preflight_result), true);
        assert.equal(first.summaries[0].ordinal, 1);
        assert.equal(first.summaries[1].ordinal, 2);
        assert.equal(first.snapshot[0].receipt.sha256.length, 64);
        const second = schema.revalidateReceiptSet(first, {expected: {purpose: 'synthetic'}, requirePass: true});
        assert.deepEqual(second.summaries, first.summaries);
    });
});

test('readReceiptFile returns an immutable, identity-bound byte summary', () => {
    withFixture(({reports}) => {
        const report = schema.readReceiptFile(reports[0]);
        assert.equal(report.path, reports[0]);
        assert.equal(report.mode, 0o400);
        assert.equal(report.sha256, schema.sha256(Buffer.from(report.base64, 'base64')));
        assert.equal(report.byte_length, Buffer.from(report.base64, 'base64').length);
        assert.equal(Object.isFrozen(report), true);
    });
});

test('different declared parent paths cannot hide a repeated physical parent', () => {
    let firstParent;
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}}),
            /run-root parents must be physically distinct/);
    }, (value, ordinal) => {
        if (ordinal === 0) firstParent = value.run_root.parent;
        else {
            value.run_root.parent.device = firstParent.device;
            value.run_root.parent.inode = firstParent.inode;
            assert.notEqual(value.run_root.parent.path, firstParent.path);
            const stdout = schema.canonicalJson(value.preflight_result);
            value.stdout.bytes = stdout.length;
            value.stdout.sha256 = value.stdout.file_sha256 = schema.sha256(stdout);
        }
    });
});

test('receipt file mutation after initial validation is detected', () => {
    withFixture(({reports}) => {
        const before = schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}});
        const report = reports[0];
        fs.chmodSync(report, 0o600);
        fs.appendFileSync(report, Buffer.from(' '));
        fs.chmodSync(report, 0o400);
        assert.throws(() => schema.revalidateReceiptSet(before, {expected: {purpose: 'synthetic'}}), /canonical|extra LF|byte|size|digest|changed|keys|LF/i);
    });
});

test('byte-identical path replacement is detected by device/inode snapshot', () => {
    withFixture(({base, reports}) => {
        const before = schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}});
        const pathToReplace = before.receipts[0].receipt.stderr.path;
        fs.unlinkSync(pathToReplace);
        fs.writeFileSync(pathToReplace, Buffer.alloc(0), {flag: 'wx', mode: 0o400});
        fs.chmodSync(pathToReplace, 0o400);
        assert.throws(() => schema.revalidateReceiptSet(before, {expected: {purpose: 'synthetic'}}), /identity changed/);
        assert.ok(fs.existsSync(base));
    });
});

test('symlink, hardlink, alias, extra entry, and mode violations are refused', () => {
    withFixture(({base, reports}) => {
        const sidecar = reports[0].replace(/\.json$/, '.stdout');
        const contents = fs.readFileSync(sidecar);
        const replacement = path.join(base, 'sidecar-replacement');
        fs.writeFileSync(replacement, contents, {mode: 0o400});
        fs.chmodSync(replacement, 0o400);
        fs.unlinkSync(sidecar);
        fs.symlinkSync(replacement, sidecar);
        assert.throws(() => schema.validateReceipt(reports[0]));
        fs.unlinkSync(sidecar);
        fs.linkSync(replacement, sidecar);
        assert.throws(() => schema.validateReceipt(reports[0]), /single-link/);
        fs.unlinkSync(sidecar);
        fs.copyFileSync(replacement, sidecar, fs.constants.COPYFILE_FICLONE);
        fs.chmodSync(sidecar, 0o400);
        const extra = path.join(path.dirname(reports[0]), 'extra');
        fs.writeFileSync(extra, 'x', {mode: 0o400});
        fs.chmodSync(extra, 0o400);
        assert.throws(() => schema.validateReceipt(reports[0]), /membership/);
        fs.unlinkSync(extra);
        fs.chmodSync(reports[0], 0o600);
        assert.throws(() => schema.validateReceipt(reports[0]), /mode/);
        assert.ok(fs.existsSync(base));
    });
});

test('tuple mismatch and forbidden or overlapping paths are rejected', () => {
    withFixture(({base, reports}) => {
        const fixture = fixturePair((value, i) => { if (i === 1) value.argv[1] = '/tmp/other-runner.js'; });
        try { assert.throws(() => schema.validateReceiptSet(fixture.reports, {expected: {purpose: 'synthetic'}}), /argv differs/); }
        finally { fs.rmSync(fixture.base, {recursive: true, force: true}); }
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic', revision: 'V511'}}), /expected/);
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}, forbiddenRoots: [base]}), /forbidden root/);
        assert.throws(() => schema.validateReceiptSet([reports[0], reports[0]], {expected: {purpose: 'synthetic'}}), /distinct|overlap|exactly/i);
        const first = JSON.parse(fs.readFileSync(reports[0], 'utf8'));
        const reordered = Object.fromEntries(Object.entries(first).reverse());
        fs.chmodSync(reports[0], 0o600);
        fs.writeFileSync(reports[0], schema.canonicalJson(reordered));
        fs.chmodSync(reports[0], 0o400);
        assert.throws(() => schema.validateReceipt(reports[0]), /keys\/order/);
    });
});

test('preflight PASS cannot hide failed invariant rows', () => {
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceipt(reports[0]), /repeated outputs differ|PASS preflight suite failed/);
    }, value => { value.preflight_result.suites[0].attempts[1].assertions = 0; });
});


test('inert descriptors identify the stable schema and explicit receipt purposes', () => {
    assert.equal(descriptors.REVISION, 'offset-2961');
    assert.equal(descriptors.SCHEMA_VERSION, 2);
    assert.equal(descriptors.MAX_JSON_DEPTH, 64);
    assert.deepEqual(descriptors.PURPOSES, ['validation', 'synthetic']);
    assert.equal(schema.RECEIPT_SCHEMA, descriptors.RECEIPT_SCHEMA);
    assert.equal(Object.isFrozen(descriptors.RECEIPT_KEYS), true);
    assert.equal(Object.isFrozen(descriptors.RECEIPT_SCHEMA.purposes), true);
    assert.equal(Object.values(descriptors).some(value => typeof value === 'function'), false);
    assert.throws(() => descriptors.PURPOSES.push('other'), TypeError);
});

test('JSON nesting is bounded with an exact accepted boundary', () => {
    const nested = depth => Buffer.from('['.repeat(depth) + '0' + ']'.repeat(depth) + '\n');
    assert.doesNotThrow(() => schema.parseCanonicalJson(nested(descriptors.MAX_JSON_DEPTH)));
    assert.throws(() => schema.parseCanonicalJson(nested(descriptors.MAX_JSON_DEPTH + 1)), /nesting limit/);
    assert.throws(() => schema.parseCanonicalJson(nested(10000)), /nesting limit/);
});

test('escaped duplicate keys and prototype names do not bypass canonical parsing', () => {
    assert.throws(() => schema.parseCanonicalJson(Buffer.from('{"a":1,"\\u0061":2}\n')), /duplicate/);
    const value = schema.parseCanonicalJson(Buffer.from('{"__proto__":{"receiptPolluted":true}}\n'));
    assert.equal(Object.hasOwn(value, '__proto__'), true);
    assert.equal(Object.getPrototypeOf(value), Object.prototype);
    assert.equal(Object.prototype.receiptPolluted, undefined);
});

test('synthetic receipts cannot be relabeled as validation evidence', () => {
    withFixture(({reports}) => {
        assert.equal(schema.validateReceipt(reports[0]).usable, false);
        assert.throws(() => schema.validateReceiptSet(reports), /purpose/);
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: undefined}}), /purpose/);
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'validation'}}), /purpose/);
        assert.doesNotThrow(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}}));
    });
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceipt(reports[0]), /purpose/);
    }, value => { value.purpose = 'validation'; });
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceipt(reports[0]), /purpose/);
    }, value => { value.purpose = value.preflight_result.purpose = 'unknown'; });
});

test('stale private schema and revision values are refused', () => {
    for (const revision of ['V509', 'V510', 'V511', 'V512', 'V513']) {
        withFixture(({reports}) => {
            assert.throws(() => schema.validateReceipt(reports[0]), /schema\/revision|revision mismatch/);
        }, value => { value.revision = value.preflight_result.revision = revision; });
    }
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceipt(reports[0]), /schema\/revision/);
    }, value => { value.schema_version = 1; });
});

test('receipt strings refuse invalid Unicode before filesystem aliasing', () => {
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceipt(reports[0]), /well-formed Unicode/);
    }, value => { value.argv.push(String.fromCharCode(0xd800)); });
});

test('unrelated ancestor activity does not invalidate a stable receipt tree', () => {
    withFixture(({base, reports}) => {
        let changed = false;
        const io = fileSystemAdapter({readSync: (...args) => {
            if (!changed) {
                changed = true;
                fs.writeFileSync(path.join(base, 'unrelated-sibling'), 'new sibling');
            }
            return fs.readSync(...args);
        }});
        assert.doesNotThrow(() => schema.validateReceiptSet(reports, {io, expected: {purpose: 'synthetic'}}));
        assert.equal(changed, true);
    });
});

test('replacing an ancestor still invalidates unchanged receipt files', () => {
    withFixture(({reports}) => {
        let changed = false;
        const ancestor = path.dirname(path.dirname(reports[0]));
        const receiptName = path.basename(path.dirname(reports[0]));
        const io = fileSystemAdapter({readSync: (...args) => {
            if (!changed) {
                changed = true;
                fs.renameSync(ancestor, ancestor + '-original');
                fs.mkdirSync(ancestor, {mode: 0o700});
                fs.renameSync(path.join(ancestor + '-original', receiptName), path.join(ancestor, receiptName));
            }
            return fs.readSync(...args);
        }});
        assert.throws(() => schema.validateReceipt(reports[0], {io}), /path component changed/);
        assert.equal(changed, true);
    });
});

test('receipt directory enumeration refuses immediately on a fourth entry', () => {
    withFixture(({reports}) => {
        let reads = 0;
        let closed = 0;
        const io = fileSystemAdapter({opendirSync: () => ({
            readSync: () => { reads++; return {name: 'entry-' + reads}; },
            closeSync: () => { closed++; }
        })});
        assert.throws(() => schema.validateReceipt(reports[0], {io}), /membership/);
        assert.equal(reads, 4);
        assert.equal(closed, 1);
    });
});

test('owned directory mutation after enumeration cannot pass validation or revalidation', () => {
    withFixture(({reports}) => {
        const previous = schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}});
        const directoryPath = path.dirname(reports[0]);
        let listed = false;
        let changed = false;
        const io = fileSystemAdapter({
            opendirSync: p => {
                const directory = fs.opendirSync(p, {bufferSize: 1});
                return {
                    readSync: () => directory.readSync(),
                    closeSync: () => { directory.closeSync(); if (p === directoryPath) listed = true; }
                };
            },
            readSync: (...args) => {
                if (listed && !changed) {
                    changed = true;
                    fs.writeFileSync(path.join(directoryPath, 'unexpected-entry'), 'x');
                }
                return fs.readSync(...args);
            }
        });
        assert.throws(() => schema.revalidateReceiptSet(previous, {io, expected: {purpose: 'synthetic'}}), /directory changed/);
        assert.equal(changed, true);
    });
});

test('previously read files are rechecked before a receipt is accepted', () => {
    withFixture(({reports}) => {
        const report = reports[0];
        let listed = false;
        let changed = false;
        const io = fileSystemAdapter({
            opendirSync: p => { listed = true; return fs.opendirSync(p, {bufferSize: 1}); },
            readSync: (...args) => {
                if (listed && !changed) {
                    changed = true;
                    fs.chmodSync(report, 0o600);
                    fs.appendFileSync(report, ' ');
                    fs.chmodSync(report, 0o400);
                }
                return fs.readSync(...args);
            }
        });
        assert.throws(() => schema.validateReceipt(report, {io}), /file changed/);
        assert.equal(changed, true);
    });
});

test('the first receipt is rechecked after the second receipt is read', () => {
    withFixture(({reports}) => {
        let changed = false;
        const io = fileSystemAdapter({opendirSync: p => {
            if (p === path.dirname(reports[1]) && !changed) {
                changed = true;
                fs.writeFileSync(path.join(path.dirname(reports[0]), 'late-entry'), 'x');
            }
            return fs.opendirSync(p, {bufferSize: 1});
        }});
        assert.throws(() => schema.validateReceiptSet(reports, {io, expected: {purpose: 'synthetic'}}), /directory changed/);
        assert.equal(changed, true);
    });
});

test('protected roots resolve aliases including nonexistent final components', () => {
    withFixture(({base, reports}) => {
        const alias = path.join(base, 'alias');
        fs.symlinkSync(base, alias);
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}, forbiddenRoots: [alias]}), /forbidden root/);
        const first = JSON.parse(fs.readFileSync(reports[0], 'utf8'));
        const missingRootViaAlias = path.join(alias, path.relative(base, first.run_root.path));
        assert.equal(fs.existsSync(missingRootViaAlias), false);
        assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}, forbiddenRoots: [missingRootViaAlias]}), /forbidden root/);
        assert.doesNotThrow(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}, forbiddenRoots: [path.join(alias, 'unrelated-missing')]}));
    });
});

test('PASS inventory requires matching identity digests as well as counts', () => {
    withFixture(({reports}) => {
        assert.throws(() => schema.validateReceipt(reports[0]), /inventory hashes differ/);
    }, value => { value.preflight_result.inventory_gate.actual_ids_sha256 = HASH_D; });
});

test('dangling protected-root symlinks are refused rather than treated as missing tails', () => {
    withFixture(({base, reports}) => {
        const first = JSON.parse(fs.readFileSync(reports[0], 'utf8'));
        const alias = path.join(base, 'dangling-protected-root');
        fs.symlinkSync(first.run_root.path, alias);
        assert.equal(fs.existsSync(alias), false);
        for (const protectedPath of [alias, path.join(alias, 'missing-child')])
            assert.throws(() => schema.validateReceiptSet(reports, {expected: {purpose: 'synthetic'}, forbiddenRoots: [protectedPath]}), /unresolvable protected path/);
    });
});
