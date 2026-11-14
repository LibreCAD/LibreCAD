/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const {LIMITS, CHECKS} = require('../evidence-descriptors');
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const canonical = value => Buffer.from(`${JSON.stringify(value)}\n`);
const chunks = [], buffer = Buffer.alloc(4096);
let length = 0;
for (;;) {
    const count = fs.readSync(0, buffer, 0, buffer.length, null);
    if (count === 0) break;
    length += count;
    assert.ok(length <= LIMITS.contextBytes);
    chunks.push(Buffer.from(buffer.subarray(0, count)));
}
const stdin = Buffer.concat(chunks, length);
const context = JSON.parse(stdin);
assert.deepEqual(canonical(context), stdin);
// A process control, never a production validation or independent H7 checker.
assert.equal(context.purpose, 'synthetic', 'synthetic evidence checker refuses validation');
const mode = process.argv[2];
assert.ok(['pass', 'fail', 'empty', 'malformed', 'missing-status', 'unknown', 'wrong-hash', 'extra-check',
    'nonzero', 'signal', 'timeout', 'overflow', 'mutate-output', 'mutate-input', 'mutate-receipt', 'mutate-ledger', 'mutate-manifest'].includes(mode));
process.stderr.write(canonical({argv: process.argv, environment: {...process.env}, stdin_sha256: hash(stdin)}));

function bound(binding, mode) {
    const stat = fs.lstatSync(binding.path);
    assert.equal(stat.isFile(), true); assert.equal(stat.nlink, 1);
    if (mode !== undefined) assert.equal(stat.mode & 0o7777, mode);
    const bytes = fs.readFileSync(binding.path);
    assert.equal(bytes.length, binding.bytes); assert.equal(hash(bytes), binding.sha256);
    return bytes;
}
function mutate(p) {
    const mode = fs.statSync(p).mode & 0o7777;
    fs.chmodSync(p, 0o600); fs.appendFileSync(p, 'synthetic checker substitution'); fs.chmodSync(p, mode);
}
const manifest = JSON.parse(bound(context.manifest, 0o400));
assert.equal(manifest.purpose, context.purpose);
assert.equal(hash(canonical(manifest.inputs)), context.source_manifest_sha256);
for (const row of manifest.inputs.files) bound(row);
for (const row of manifest.receipts) for (const field of ['report', 'stdout', 'stderr']) bound(row[field], 0o400);
const h3 = JSON.parse(bound(manifest.h3_diagnostics, 0o400));
bound(manifest.preflight_transport, 0o400);
const ledger = JSON.parse(bound(context.ledger, 0o400));
assert.equal(h3.status, 'PASS'); assert.equal(ledger.status, 'PASS');
assert.equal(ledger.genesis.source_manifest_sha256, context.source_manifest_sha256);
assert.deepEqual(ledger.rows.map(row => row.event.case_id).sort(), context.case_ids);
assert.equal(ledger.genesis.output_count, context.output_count);
for (const row of manifest.worker_outputs) bound(row, 0o400);

if (mode === 'timeout') setInterval(() => {}, 1000);
else if (mode === 'overflow') { process.stdout.write(Buffer.alloc(2097152, 120)); setInterval(() => {}, 1000); }
else {
    if (mode === 'mutate-output') mutate(manifest.worker_outputs[0].path);
    if (mode === 'mutate-input') mutate(manifest.inputs.candidate.path);
    if (mode === 'mutate-receipt') mutate(manifest.receipts[0].stderr.path);
    if (mode === 'mutate-ledger') mutate(context.ledger.path);
    if (mode === 'mutate-manifest') mutate(context.manifest.path);
    const result = {schema_version: 1, purpose: 'synthetic', status: mode === 'fail' ? 'FAIL' : 'PASS',
        manifest_sha256: context.manifest.sha256, ledger_sha256: context.ledger.sha256,
        source_manifest_sha256: context.source_manifest_sha256, case_ids: context.case_ids,
        output_count: context.output_count, checks: CHECKS.map(id => ({id, status: mode === 'fail' ? 'FAIL' : 'PASS'}))};
    if (mode === 'missing-status') delete result.status;
    if (mode === 'unknown') result.status = 'UNKNOWN';
    if (mode === 'wrong-hash') result.manifest_sha256 = hash(Buffer.from('wrong'));
    if (mode === 'extra-check') result.checks.push({id: 'extra', status: 'PASS'});
    if (mode === 'malformed') process.stdout.write('not JSON\n');
    else if (mode !== 'empty') process.stdout.write(canonical(result));
    if (mode === 'nonzero') process.exitCode = 7;
    if (mode === 'signal') process.kill(process.pid, 'SIGTERM');
}
