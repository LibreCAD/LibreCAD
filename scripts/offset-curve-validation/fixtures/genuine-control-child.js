/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const path = require('node:path');
const h1 = require('../preflight-receipt');
const {makeResult} = require('./capture-child');

// Envelope/retention controls only. These are not qualification suite results.
function control(context, mode = 'pass') {
    if (context.purpose !== 'synthetic') throw new TypeError('genuine envelope control refuses validation purpose');
    const directory = path.join(process.cwd(), `synthetic-control-${h1.sha256(h1.canonicalJson(context.run_root)).slice(0, 32)}`);
    fs.mkdirSync(directory, {mode: 0o700});
    const artifacts = [];
    for (let i = 0; i < (mode === 'combined-count' ? 33 : 28); i++) {
        const p = path.join(directory, `control-${String(i).padStart(2, '0')}.json`);
        const bytes = h1.canonicalJson({synthetic_control: true, ordinal: i, context_sha256: h1.sha256(h1.canonicalJson(context))});
        const fd = fs.openSync(p, fs.constants.O_WRONLY | fs.constants.O_CREAT | fs.constants.O_EXCL | fs.constants.O_NOFOLLOW, 0o600);
        try { fs.writeFileSync(fd, bytes); fs.fchmodSync(fd, 0o400); fs.fsyncSync(fd); }
        finally { fs.closeSync(fd); }
        artifacts.push({role: `synthetic-control:${i}`, path: p, bytes: bytes.length, sha256: h1.sha256(bytes)});
    }
    const envelope = {schema_version: 1, purpose: 'synthetic', kind: 'genuine-preflight-evidence',
        context_sha256: h1.sha256(h1.canonicalJson(context)), artifacts,
        limitations: ['Synthetic envelope control, not genuine qualification evidence.']};
    const first = artifacts[0];
    if (mode === 'missing') fs.unlinkSync(first.path);
    else if (mode === 'wrong-hash') first.sha256 = '0'.repeat(64);
    else if (mode === 'wrong-size') first.bytes++;
    else if (mode === 'hardlink') fs.linkSync(first.path, path.join(directory, 'hardlink'));
    else if (mode === 'symlink') { fs.renameSync(first.path, `${first.path}.original`); fs.symlinkSync(`${first.path}.original`, first.path); }
    else if (mode === 'mode') fs.chmodSync(first.path, 0o600);
    else if (mode === 'extra-file') fs.writeFileSync(path.join(directory, 'extra'), 'extra', {mode: 0o400});
    else if (mode === 'duplicate-path') artifacts[1].path = first.path;
    else if (mode === 'wrong-context') envelope.context_sha256 = '0'.repeat(64);
    else if (mode === 'wrong-purpose') envelope.purpose = 'validation';
    else if (mode === 'extra-key') envelope.extra = true;
    else if (mode === 'oversized') first.bytes = 8388609;
    else if (mode === 'total-cap') artifacts.forEach(ref => { ref.bytes = 8388608; });
    else if (mode === 'too-many') {
        while (artifacts.length < 65) artifacts.push({...first, role: `synthetic-control:${artifacts.length}`});
    } else if (mode === 'input-overlap') Object.assign(first, {path: context.candidate.path,
        bytes: context.candidate.bytes, sha256: context.candidate.sha256});
    else if (mode === 'h3-overlap') first.path = path.join(process.cwd(), 'worker-parent', 'run', 'a.json');
    else if (mode === 'h4-overlap') first.path = path.join(process.cwd(), 'ledger-parent', 'ledger', 'joined-events.json');
    else if (mode === 'h5-overlap') first.path = path.join(process.cwd(), 'evidence-parent', 'attempt', 'external.json');
    else if (mode === 'parent-link') {
        fs.renameSync(directory, `${directory}.original`); fs.symlinkSync(`${directory}.original`, directory);
    } else if (!['pass', 'noncanonical', 'duplicate-key', 'ordinary', 'combined-count', 'ordinary-text',
        'ordinary-marker', 'ordinary-json-marker'].includes(mode)) throw new TypeError('unknown synthetic envelope control mode');
    let stderr = h1.canonicalJson(envelope);
    if (mode === 'noncanonical') stderr = Buffer.from(`${JSON.stringify(envelope, null, 2)}\n`);
    if (mode === 'duplicate-key') stderr = Buffer.from(stderr.toString().replace('"schema_version":1,', '"schema_version":1,"schema_version":1,'));
    if (mode === 'ordinary') stderr = h1.canonicalJson({synthetic_control: true});
    if (mode === 'ordinary-text') stderr = Buffer.from('opaque synthetic stderr without an evidence envelope\n');
    if (mode === 'ordinary-marker') stderr = Buffer.from('ordinary diagnostic mentions genuine-preflight-evidence without an envelope\n');
    if (mode === 'ordinary-json-marker') stderr = h1.canonicalJson({kind: 'ordinary-control', message: 'genuine-preflight-evidence'});
    return {result: makeResult(context), stderr};
}

if (require.main === module) {
    const chunks = [], buffer = Buffer.alloc(4096);
    let length = 0;
    for (;;) {
        const count = fs.readSync(0, buffer, 0, buffer.length, null);
        if (count === 0) break;
        length += count;
        if (length > 32768) throw new TypeError('control stdin context exceeds byte cap');
        chunks.push(Buffer.from(buffer.subarray(0, count)));
    }
    const bytes = Buffer.concat(chunks, length);
    const context = h1.parseCanonicalJson(bytes, {maxBytes: 32768});
    const out = control(context, process.argv[2] || 'pass');
    process.stderr.write(out.stderr); process.stdout.write(h1.canonicalJson(out.result));
}

module.exports = Object.freeze({control});
