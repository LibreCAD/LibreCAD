/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const fs = require('node:fs');
const crypto = require('node:crypto');

// A real bounded stdin-consumption probe, not a gate-success fixture.
const chunks = [], buffer = Buffer.alloc(4096);
let length = 0;
for (;;) {
    const n = fs.readSync(0, buffer, 0, buffer.length, null);
    if (!n) break;
    length += n;
    if (length > 32768) throw new TypeError('trace stdin exceeds cap');
    chunks.push(Buffer.from(buffer.subarray(0, n)));
}
const bytes = Buffer.concat(chunks, length);
process.stdout.write(`${JSON.stringify({schema_version: 1, kind: 'stdin-trace', bytes: length,
    sha256: crypto.createHash('sha256').update(bytes).digest('hex'), runtime: process.version})}\n`);
process.stderr.write('offset stdin trace completed\n');
