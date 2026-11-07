/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

// Inert shared format data only. No imports, parsing, I/O or validation helpers.
// An independent checker derives and checks outcomes with its own implementation.
const LIMITS = Object.freeze({ledgerBytes: 16777216, h3Bytes: 268435456, dataNodes: 262144,
    dataDepth: 32, arrayItems: 8192, stringBytes: 12582912, directoryEntries: 128});
const SCHEMA = Object.freeze({schema_version: 1, revision: 'offset-2961',
    ledger_keys: Object.freeze(['schema_version', 'revision', 'purpose', 'status', 'genesis', 'genesis_sha256', 'rows', 'row_count', 'tail_sha256']),
    genesis_keys: Object.freeze(['schema_version', 'revision', 'purpose', 'candidate', 'runtime', 'source_manifest_sha256',
        'config', 'registry', 'run_root', 'receipts', 'case_inventory_sha256', 'worker_count', 'case_count', 'output_count']),
    binding_keys: Object.freeze(['path', 'bytes', 'sha256']),
    receipt_keys: Object.freeze(['ordinal', 'receipt_id', 'report', 'stdout', 'stderr']),
    row_keys: Object.freeze(['event', 'sha256']),
    event_keys: Object.freeze(['ordinal', 'previous_sha256', 'genesis_sha256', 'worker_ordinal', 'worker_id', 'event_ordinal',
        'case_id', 'status', 'source', 'result_sha256', 'dispatch_mode', 'stdin_sha256', 'stdout_sha256', 'stderr_sha256', 'outputs']),
    output_keys: Object.freeze(['name', 'bytes', 'sha256'])});

module.exports = Object.freeze({LIMITS, SCHEMA});
