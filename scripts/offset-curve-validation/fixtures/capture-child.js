/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const h1 = require('../preflight-receipt');

function makeResult(context) {
    if (context.purpose !== 'synthetic') throw new TypeError('capture-child only produces synthetic controls');
    const inventory = h1.parseCanonicalJson(fs.readFileSync(context.case_inventory_path), {maxBytes: 1048576});
    const ids = inventory.cases.map(row => row.id);
    const digest = h1.sha256(h1.canonicalJson(ids));
    const parent = fs.lstatSync(context.run_root.parent.path, {bigint: true});
    const payload = h1.canonicalJson({synthetic: true, ids});
    const payloadHash = h1.sha256(payload);
    return {
        revision: h1.REVISION, purpose: 'synthetic', status: 'PASS', candidate: context.candidate,
        source_manifest_sha256: context.source_manifest_sha256,
        inventory_gate: {status: 'PASS', expected_count: ids.length, actual_count: ids.length,
            expected_ids_sha256: digest, actual_ids_sha256: digest, missing_ids: [], extra_ids: [], duplicate_ids: []},
        patch_replays: {status: 'PASS', directions: h1.PATCH_DIRECTIONS.map(name => ({name, status: 'PASS',
            patch_sha256: payloadHash, result_sha256: payloadHash}))},
        audit_parity: {status: 'PASS', primary_sha256: payloadHash, independent_sha256: payloadHash, equal: true},
        suite_parity: {status: 'PASS', suite_count: h1.PREFLIGHT_SUITES.length, identical_repeated_outputs: true},
        suites: h1.PREFLIGHT_SUITES.map(name => ({name, status: 'PASS', attempts: [1, 2].map(ordinal => ({
            ordinal, status: 'PASS', report_bytes: payload.length, report_sha256: payloadHash, assertions: ids.length
        }))})),
        syntax: {status: 'PASS', checked_file_count: 1, checked_paths_sha256: payloadHash},
        caps: {status: 'PASS', checks: [{name: 'synthetic-control', limit: 1, measured: 1, passed: true}]},
        trace_probe: {status: 'PASS', stdout_sha256: payloadHash, stderr_sha256: payloadHash, exit_code: 0},
        run_root: {path: context.run_root.path, absent_before: true, absent_after: true, parent_stable: true,
            parent: {path: context.run_root.parent.path, device: String(parent.dev), inode: String(parent.ino)},
            write_attempts: 0, error: null}
    };
}

if (require.main === module) {
    const chunks = [];
    let length = 0;
    const buffer = Buffer.alloc(4096);
    for (;;) {
        const count = fs.readSync(0, buffer, 0, buffer.length, null);
        if (count === 0) break;
        length += count;
        if (length > 32768) throw new TypeError('stdin context exceeds byte cap');
        chunks.push(Buffer.from(buffer.subarray(0, count)));
    }
    const stdin = Buffer.concat(chunks, length);
    const context = h1.parseCanonicalJson(stdin, {maxBytes: 32768, label: 'stdin context'});
    if (context.purpose !== 'synthetic') throw new TypeError('synthetic fixture refuses validation purpose');
    const mode = process.argv[2] || 'pass';
    if (mode === 'fail') {
        process.stdout.write(Buffer.from([0x00, 0xff, 0x0a]));
        process.stderr.write('real child failed\n');
        process.exitCode = 7;
    } else if (mode === 'timeout') {
        process.stdout.write('before timeout\n');
        setInterval(() => {}, 1000);
    } else if (mode === 'overflow') {
        process.stderr.write('before overflow\n');
        process.stdout.write(Buffer.alloc(32768, 0x78));
        setInterval(() => {}, 1000);
    } else if (mode === 'signal') {
        process.stderr.write('before signal\n', () => process.kill(process.pid, 'SIGTERM'));
    } else if (mode === 'malformed') {
        process.stdout.write('not JSON\n');
    } else {
        if (mode === 'root-write') {
            fs.mkdirSync(context.run_root.path, {mode: 0o700});
            fs.writeFileSync(path.join(context.run_root.path, 'sentinel'), 'external root\n');
        } else if (mode !== 'pass') throw new TypeError('unknown capture fixture mode');
        process.stderr.write(h1.canonicalJson({keys: Object.keys(process.env).sort(), environment: {...process.env},
            stdin_bytes: stdin.length, stdin_sha256: h1.sha256(stdin),
            exec_path: process.execPath, version: process.version}));
        process.stdout.write(h1.canonicalJson(makeResult(context)));
    }
}

module.exports = Object.freeze({makeResult});
