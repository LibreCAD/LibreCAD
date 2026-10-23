/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const h1 = require('../preflight-receipt');

// A real process control, not a production gate or implementation of H7 suites.
function resultFor(context) {
    if (context.purpose !== 'synthetic') throw new TypeError('formal-child only supports synthetic controls');
    return {schema_version: 1, purpose: context.purpose, worker_id: context.worker_id, ordinal: context.ordinal,
        status: 'PASS', case_ids: context.case_ids,
        events: context.case_ids.map((caseId, i) => ({ordinal: i + 1, case_id: caseId, status: 'PASS'})), outputs: [...context.outputs]};
}

function writeOutputs(context, mode = 'pass') {
    const names = mode === 'missing-file' ? context.outputs.slice(1) : [...context.outputs];
    if (mode === 'extra-file') names.push('undeclared-extra.txt');
    for (const name of names) {
        const p = path.join(context.run_root, name);
        const bytes = h1.canonicalJson({synthetic: true, worker_id: context.worker_id, cases: context.case_ids, output: name});
        const fd = fs.openSync(p, fs.constants.O_CREAT | fs.constants.O_EXCL | fs.constants.O_WRONLY | fs.constants.O_NOFOLLOW, 0o600);
        try {
            let offset = 0;
            while (offset < bytes.length) offset += fs.writeSync(fd, bytes, offset, bytes.length - offset);
            fs.fchmodSync(fd, mode === 'wrong-mode' ? 0o600 : 0o400);
            fs.fsyncSync(fd);
        } finally { fs.closeSync(fd); }
    }
}

if (require.main === module) {
    const chunks = [];
    const buffer = Buffer.alloc(4096);
    let length = 0;
    for (;;) {
        const count = fs.readSync(0, buffer, 0, buffer.length, null);
        if (count === 0) break;
        length += count;
        if (length > 32768) throw new TypeError('formal context exceeds byte cap');
        chunks.push(Buffer.from(buffer.subarray(0, count)));
    }
    const stdin = Buffer.concat(chunks, length);
    const context = h1.parseCanonicalJson(stdin, {maxBytes: 32768});
    if (context.purpose !== 'synthetic') throw new TypeError('synthetic fixture refuses validation purpose');
    const mode = process.argv[2] || 'pass';
    if (!['pass', 'timeout', 'overflow', 'missing-file', 'extra-file', 'wrong-mode', 'empty', 'malformed',
        'status-missing', 'status-unknown', 'fail', 'events-reversed', 'outputs-duplicate', 'outputs-missing',
        'outputs-extra', 'pass-after-exit-failure', 'signal'].includes(mode)) throw new TypeError('unknown formal-child mode');
    const result = resultFor(context);
    process.stderr.write(h1.canonicalJson({synthetic: true, argv: process.argv, environment: {...process.env},
        stdin_sha256: h1.sha256(stdin)}));
    if (mode === 'timeout') setInterval(() => {}, 1000);
    else if (mode === 'overflow') { process.stdout.write(Buffer.alloc(32768, 120)); setInterval(() => {}, 1000); }
    else {
        writeOutputs(context, mode);
        if (mode === 'empty') { /* Deliberately omit a result while preserving actual outputs. */ }
        else if (mode === 'malformed') process.stdout.write('not JSON\n');
        else {
            if (mode === 'status-missing') delete result.status;
            if (mode === 'status-unknown') result.status = 'UNKNOWN';
            if (mode === 'fail') result.status = 'FAIL';
            if (mode === 'events-reversed') result.events.reverse();
            if (mode === 'outputs-duplicate') result.outputs.push(result.outputs[0]);
            if (mode === 'outputs-missing') result.outputs = [];
            if (mode === 'outputs-extra') result.outputs.push('undeclared-extra.txt');
            process.stdout.write(h1.canonicalJson(result));
        }
        if (mode === 'pass-after-exit-failure') process.exitCode = 7;
        if (mode === 'signal') process.kill(process.pid, 'SIGTERM');
    }
}

module.exports = Object.freeze({resultFor, writeOutputs});
