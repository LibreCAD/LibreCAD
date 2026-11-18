/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const REVISION = 'offset-2961';
const SCHEMA_VERSION = 2;
const PURPOSES = Object.freeze(['validation', 'synthetic']);
const MAX_JSON_DEPTH = 64;
const MAX_REPORT_BYTES = 65536;
const MAX_STREAM_BYTES = 67108864;
const RECEIPT_DIRECTORY_MODE = 0o700;
const RECEIPT_FILE_MODE = 0o400;
const PREFLIGHT_SUITES = Object.freeze([
    'receipt-schema',
    'capture-readback',
    'formal-runner-boundary',
    'genesis-ledger-p0f-joins'
]);
const PATCH_DIRECTIONS = Object.freeze([
    'BASE_TO_PRODUCT',
    'PRODUCT_TO_CANDIDATE',
    'CANDIDATE_TO_PRODUCT_REVERSE',
    'BASE_TO_CANDIDATE'
]);

const RECEIPT_KEYS = Object.freeze([
    'schema_version', 'revision', 'purpose', 'receipt_id', 'candidate', 'source_manifest_sha256',
    'argv', 'cwd', 'environment_sha256', 'child', 'stdout', 'stderr',
    'preflight_result', 'run_root', 'status'
]);
const CANDIDATE_KEYS = Object.freeze(['path', 'bytes', 'sha256', 'git_blob']);
const CHILD_KEYS = Object.freeze(['exit_code', 'signal', 'error']);
const ERROR_KEYS = Object.freeze(['code', 'message']);
const STREAM_KEYS = Object.freeze(['path', 'bytes', 'sha256', 'file_sha256']);
const ROOT_PROOF_KEYS = Object.freeze([
    'path', 'absent_before', 'absent_after', 'parent_stable', 'parent', 'write_attempts', 'error'
]);
const PARENT_KEYS = Object.freeze(['path', 'device', 'inode']);
const RECEIPT_SUMMARY_KEYS = Object.freeze([
    'ordinal', 'receipt_id', 'receipt_directory', 'receipt_directory_mode',
    'report_path', 'report_bytes', 'report_sha256', 'report_mode',
    'stdout_path', 'stdout_bytes', 'stdout_sha256', 'stdout_mode',
    'stderr_path', 'stderr_bytes', 'stderr_sha256', 'stderr_mode',
    'run_root_path', 'root_absent_before', 'root_absent_after', 'root_write_attempts',
    'run_root_parent_path', 'run_root_parent_device', 'run_root_parent_inode'
]);

const PREFLIGHT_RESULT_KEYS = Object.freeze([
    'revision', 'purpose', 'status', 'candidate', 'source_manifest_sha256', 'inventory_gate',
    'patch_replays', 'audit_parity', 'suite_parity', 'suites', 'syntax', 'caps',
    'trace_probe', 'run_root'
]);
const INVENTORY_KEYS = Object.freeze([
    'status', 'expected_count', 'actual_count', 'expected_ids_sha256',
    'actual_ids_sha256', 'missing_ids', 'extra_ids', 'duplicate_ids'
]);
const PATCH_REPLAYS_KEYS = Object.freeze(['status', 'directions']);
const PATCH_DIRECTION_KEYS = Object.freeze(['name', 'status', 'patch_sha256', 'result_sha256']);
const AUDIT_PARITY_KEYS = Object.freeze(['status', 'primary_sha256', 'independent_sha256', 'equal']);
const SUITE_PARITY_KEYS = Object.freeze(['status', 'suite_count', 'identical_repeated_outputs']);
const SUITE_KEYS = Object.freeze(['name', 'status', 'attempts']);
const SUITE_ATTEMPT_KEYS = Object.freeze(['ordinal', 'status', 'report_bytes', 'report_sha256', 'assertions']);
const SYNTAX_KEYS = Object.freeze(['status', 'checked_file_count', 'checked_paths_sha256']);
const CAPS_KEYS = Object.freeze(['status', 'checks']);
const CAP_CHECK_KEYS = Object.freeze(['name', 'limit', 'measured', 'passed']);
const TRACE_KEYS = Object.freeze(['status', 'stdout_sha256', 'stderr_sha256', 'exit_code']);

function deepFreeze(value) {
    if (value && typeof value === 'object' && !Object.isFrozen(value)) {
        for (const key of Object.keys(value)) deepFreeze(value[key]);
        Object.freeze(value);
    }
    return value;
}

const RECEIPT_SCHEMA = deepFreeze({
    schema_version: SCHEMA_VERSION,
    revision: REVISION,
    purposes: [...PURPOSES],
    max_json_depth: MAX_JSON_DEPTH,
    receipt_keys: [...RECEIPT_KEYS],
    candidate_keys: [...CANDIDATE_KEYS],
    child_keys: [...CHILD_KEYS],
    child_error_keys: [...ERROR_KEYS],
    stream_keys: [...STREAM_KEYS],
    run_root_keys: [...ROOT_PROOF_KEYS],
    parent_keys: [...PARENT_KEYS],
    receipt_summary_keys: [...RECEIPT_SUMMARY_KEYS],
    max_report_bytes: MAX_REPORT_BYTES,
    max_stream_bytes: MAX_STREAM_BYTES,
    directory_mode: RECEIPT_DIRECTORY_MODE,
    file_mode: RECEIPT_FILE_MODE
});

const PREFLIGHT_RESULT_SCHEMA = deepFreeze({
    revision: REVISION,
    purposes: [...PURPOSES],
    keys: [...PREFLIGHT_RESULT_KEYS],
    inventory_gate_keys: [...INVENTORY_KEYS],
    patch_replays_keys: [...PATCH_REPLAYS_KEYS],
    patch_direction_keys: [...PATCH_DIRECTION_KEYS],
    patch_directions: [...PATCH_DIRECTIONS],
    audit_parity_keys: [...AUDIT_PARITY_KEYS],
    suite_parity_keys: [...SUITE_PARITY_KEYS],
    suite_keys: [...SUITE_KEYS],
    suite_attempt_keys: [...SUITE_ATTEMPT_KEYS],
    suite_names: [...PREFLIGHT_SUITES],
    syntax_keys: [...SYNTAX_KEYS],
    caps_keys: [...CAPS_KEYS],
    cap_check_keys: [...CAP_CHECK_KEYS],
    trace_probe_keys: [...TRACE_KEYS],
    root_proof_keys: [...ROOT_PROOF_KEYS],
    parent_keys: [...PARENT_KEYS]
});

module.exports = Object.freeze({
    REVISION,
    SCHEMA_VERSION,
    PURPOSES,
    MAX_JSON_DEPTH,
    MAX_REPORT_BYTES,
    MAX_STREAM_BYTES,
    RECEIPT_DIRECTORY_MODE,
    RECEIPT_FILE_MODE,
    PREFLIGHT_SUITES,
    PATCH_DIRECTIONS,
    RECEIPT_KEYS,
    CANDIDATE_KEYS,
    CHILD_KEYS,
    ERROR_KEYS,
    STREAM_KEYS,
    ROOT_PROOF_KEYS,
    PARENT_KEYS,
    RECEIPT_SUMMARY_KEYS,
    PREFLIGHT_RESULT_KEYS,
    INVENTORY_KEYS,
    PATCH_REPLAYS_KEYS,
    PATCH_DIRECTION_KEYS,
    AUDIT_PARITY_KEYS,
    SUITE_PARITY_KEYS,
    SUITE_KEYS,
    SUITE_ATTEMPT_KEYS,
    SYNTAX_KEYS,
    CAPS_KEYS,
    CAP_CHECK_KEYS,
    TRACE_KEYS,
    RECEIPT_SCHEMA,
    PREFLIGHT_RESULT_SCHEMA,
});
