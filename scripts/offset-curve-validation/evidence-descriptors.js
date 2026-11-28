/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

// Inert format data. Sharing these constants does not qualify an H7 checker.
const LIMITS = Object.freeze({manifestBytes: 2097152, diagnosticsBytes: 268435456,
    transportBytes: 262144, reviewBytes: 4194304, checkerConfigBytes: 65536,
    contextBytes: 524288, resultBytes: 65536, streamBytes: 1048576,
    timeoutMs: 300000, killGraceMs: 1000, dataNodes: 262144, dataDepth: 32,
    arrayItems: 8192, stringBytes: 12582912});
const FILES = Object.freeze({diagnostics: 'h3-diagnostics.json', transport: 'preflight-transport.json', review: 'review.json'});
const CHECKS = Object.freeze(['evidence-bindings', 'receipt-bindings', 'worker-diagnostics', 'ledger-bindings', 'output-bindings']);
const SCHEMA = Object.freeze({schema_version: 1, revision: 'offset-2961',
    request_keys: Object.freeze(['configPath', 'receiptPaths', 'h3Result', 'ledgerReadback',
        'preflightCaptures', 'checkerConfigPath', 'manifestPath']),
    checker_keys: Object.freeze(['schema_version', 'purpose', 'module_path', 'args', 'cwd', 'environment', 'timeout_ms', 'max_stream_bytes']),
    manifest_keys: Object.freeze(['schema_version', 'revision', 'purpose', 'kind', 'inputs', 'input_identities',
        'receipts', 'ledger', 'h3_diagnostics', 'preflight_transport', 'worker_outputs', 'checker', 'limitations']),
    result_keys: Object.freeze(['schema_version', 'purpose', 'status', 'manifest_sha256', 'ledger_sha256',
        'source_manifest_sha256', 'case_ids', 'output_count', 'checks']),
    check_keys: Object.freeze(['id', 'status']),
    context_keys: Object.freeze(['schema_version', 'revision', 'purpose', 'manifest', 'ledger',
        'source_manifest_sha256', 'case_ids', 'output_count', 'required_checks']),
    review_keys: Object.freeze(['schema_version', 'revision', 'purpose', 'kind', 'status', 'manifest_sha256',
        'ledger_sha256', 'checker', 'dispatch', 'result', 'error', 'verification_scope', 'verification_failures', 'limitations'])});

module.exports = Object.freeze({LIMITS, FILES, CHECKS, SCHEMA});
