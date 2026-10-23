/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

// Expectations come from roadmap section 13.1, not the production validator.
// Expectations describe the actual owned API protocol, not an invented common
// runner envelope. A throwing boundary has no returned terminal status.
function row(id, condition, stage, reason, options = {}) {
    return Object.freeze({id, condition, stage, reason, status: 'REFUSED', phase: 'pre-root',
        ingress: 'transaction', protocol: 'result', scope: 'H3.runFormalTransaction',
        state: 'executable', original: true, ...options});
}

const report = Object.freeze({phase: 'post-root', ingress: 'owned-api', protocol: 'throw', status: null,
    scope: 'H5.readEvidenceManifest', exceptionName: 'EvidenceError'});
const joined = Object.freeze({phase: 'post-root', ingress: 'owned-api', protocol: 'throw', status: null,
    scope: 'H4.buildJoinedLedger', exceptionName: 'LedgerError'});
const evidence = Object.freeze({phase: 'post-root', ingress: 'owned-api', original: false,
    scope: 'H5.runP0fEvidence'});
const cli = Object.freeze({ingress: 'cli'});
const added = Object.freeze({original: false});
const worker = Object.freeze({original: false, phase: 'post-root', status: 'FAIL'});

const INVENTORY = Object.freeze([
    row('H6-CLI-01', 'Missing --run', 'CLI', 'RUN_MODE_REQUIRED', cli),
    row('H6-CLI-02', 'Unknown option', 'CLI', 'ARG_UNKNOWN_OPTION', cli),
    row('H6-CLI-03', 'Duplicate --run', 'CLI', 'ARG_DUPLICATE_OPTION', cli),
    row('H6-CLI-04', 'Missing option value', 'CLI', 'ARG_MISSING_VALUE', cli),
    row('H6-CLI-05', 'Zero receipt paths', 'CLI', 'RECEIPT_COUNT', cli),
    row('H6-CLI-06', 'One receipt path', 'CLI', 'RECEIPT_COUNT', cli),
    row('H6-CLI-07', 'Three receipt paths', 'CLI', 'RECEIPT_COUNT', cli),
    row('H6-CLI-08', 'Repeated path', 'CLI', 'RECEIPT_PATH_DUPLICATE', cli),
    row('H6-CLI-09', 'Distinct paths with duplicate IDs', 'RECEIPT_JOIN', 'RECEIPT_ID_DUPLICATE', cli),
    row('H6-CLI-10', 'Reversed expected pair', 'RECEIPT_JOIN', 'RECEIPT_ORDER_MISMATCH', cli),
    row('H6-CLI-11', 'Relative receipt path', 'CLI', 'ARG_RELATIVE_PATH', cli),
    row('H6-ID-01', 'Missing report file', 'RECEIPT_IO', 'RECEIPT_NOT_FOUND'),
    row('H6-ID-02a', 'Synthetic stale V506 revision', 'IDENTITY', 'REVISION_MISMATCH'),
    row('H6-ID-02b', 'Synthetic stale V507 revision', 'IDENTITY', 'REVISION_MISMATCH'),
    row('H6-ID-02c', 'Synthetic stale V508 revision', 'IDENTITY', 'REVISION_MISMATCH'),
    row('H6-ID-02d', 'Synthetic stale V509 revision', 'IDENTITY', 'REVISION_MISMATCH'),
    row('H6-ID-02e', 'Synthetic stale V510 revision', 'IDENTITY', 'REVISION_MISMATCH'),
    row('H6-ID-02f', 'Synthetic stale V511 revision', 'IDENTITY', 'REVISION_MISMATCH'),
    row('H6-ID-03a', 'Synthetic stale V509 candidate tuple', 'IDENTITY', 'CANDIDATE_MISMATCH'),
    row('H6-ID-03b', 'Synthetic stale V510 candidate tuple', 'IDENTITY', 'CANDIDATE_MISMATCH'),
    row('H6-ID-03c', 'Synthetic stale V511 candidate tuple', 'IDENTITY', 'CANDIDATE_MISMATCH'),
    row('H6-ID-04', 'Synthetic stale manifest tuple', 'IDENTITY', 'MANIFEST_MISMATCH'),
    row('H6-ID-05', 'Other candidate field mismatch', 'IDENTITY', 'CANDIDATE_MISMATCH'),
    row('H6-ID-06', 'Other manifest field mismatch', 'IDENTITY', 'MANIFEST_MISMATCH'),
    row('H6-ID-07', 'Environment digest mismatch', 'IDENTITY', 'ENVIRONMENT_MISMATCH'),
    row('H6-ID-08', 'Runtime executable mismatch', 'IDENTITY', 'NODE_MISMATCH'),
    row('H6-ID-09', 'Runner source mismatch', 'IDENTITY', 'RUNNER_MISMATCH'),
    row('H6-ID-10', 'Canonical cwd mismatch', 'IDENTITY', 'CWD_MISMATCH'),
    row('H6-ID-11', 'Expected reserved output-root mismatch', 'IDENTITY', 'RUN_ROOT_MISMATCH'),
    row('H6-TREE-01a', 'Invalid UTF-8 report', 'RECEIPT_PARSE', 'REPORT_PARSE_FAILED'),
    row('H6-TREE-01b', 'Malformed JSON report', 'RECEIPT_PARSE', 'REPORT_PARSE_FAILED'),
    row('H6-TREE-02a', 'Duplicate report key', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02b', 'Reordered report key', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02c', 'Missing report key', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02d', 'Extra report key', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02e', 'Report BOM', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02f', 'Report CR', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02g', 'Report double LF', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-02h', 'Other noncanonical report bytes', 'RECEIPT_CANONICAL', 'REPORT_NONCANONICAL'),
    row('H6-TREE-03a', 'Non-PASS receipt', 'RECEIPT_RESULT', 'CHILD_NOT_PASS'),
    row('H6-TREE-03b', 'Nonzero child exit', 'RECEIPT_RESULT', 'CHILD_NOT_PASS'),
    row('H6-TREE-03c', 'Child signal', 'RECEIPT_RESULT', 'CHILD_NOT_PASS'),
    row('H6-TREE-03d', 'Child spawn error', 'RECEIPT_RESULT', 'CHILD_NOT_PASS'),
    row('H6-TREE-04a', 'Zero stdout result lines', 'RECEIPT_STREAM', 'STDOUT_SHAPE'),
    row('H6-TREE-04b', 'Multiple stdout result lines', 'RECEIPT_STREAM', 'STDOUT_SHAPE'),
    row('H6-TREE-04c', 'Noncanonical stdout result line', 'RECEIPT_STREAM', 'STDOUT_SHAPE'),
    row('H6-TREE-05a', 'Persisted genuine report-byte binding substitution', 'READBACK', 'MANIFEST_MISMATCH', report),
    row('H6-TREE-05b', 'Persisted report-hash binding substitution', 'READBACK', 'MANIFEST_MISMATCH', report),
    row('H6-TREE-05c', 'Persisted report-size binding substitution', 'READBACK', 'MANIFEST_MISMATCH', report),
    row('H6-TREE-05d', 'Stdout sidecar binding mismatch', 'RECEIPT_BINDING', 'FILE_BINDING_MISMATCH'),
    row('H6-TREE-05e', 'Stderr sidecar binding mismatch', 'RECEIPT_BINDING', 'FILE_BINDING_MISMATCH'),
    row('H6-TREE-05f', 'Report basename/ID mismatch', 'RECEIPT_BINDING', 'FILE_BINDING_MISMATCH'),
    row('H6-TREE-06a', 'Wrong report mode', 'FILESYSTEM', 'FILE_METADATA_MISMATCH'),
    row('H6-TREE-06b', 'Nonregular report file', 'FILESYSTEM', 'FILE_METADATA_MISMATCH'),
    row('H6-TREE-07a', 'Missing tree entry', 'FILESYSTEM', 'RECEIPT_TREE_MEMBERSHIP'),
    row('H6-TREE-07b', 'Extra tree entry', 'FILESYSTEM', 'RECEIPT_TREE_MEMBERSHIP'),
    row('H6-TREE-08a', 'Report symlink', 'FILESYSTEM', 'PATH_NOT_REGULAR_SINGLE_LINK'),
    row('H6-TREE-08b', 'Report hardlink', 'FILESYSTEM', 'PATH_NOT_REGULAR_SINGLE_LINK'),
    row('H6-TREE-08c', 'Aliased receipt storage directory', 'FILESYSTEM', 'PATH_NOT_REGULAR_SINGLE_LINK'),
    row('H6-TREE-09a', 'Reserved root overlaps receipt storage', 'PATH_POLICY', 'PATH_POLICY_VIOLATION'),
    row('H6-TREE-09b', 'Ancestor/descendant reserved roots', 'PATH_POLICY', 'PATH_POLICY_VIOLATION'),
    row('H6-TREE-09c', 'Protected source-path overlap', 'PATH_POLICY', 'PATH_POLICY_VIOLATION'),
    row('H6-RACE-01', 'Parent changes before exclusive creation', 'ROOT_PREFLIGHT', 'PARENT_IDENTITY_CHANGED'),
    row('H6-RACE-02a', 'Report changes between validations', 'REVALIDATION', 'RECEIPT_CHANGED'),
    row('H6-RACE-02b', 'Sidecar changes between validations', 'REVALIDATION', 'RECEIPT_CHANGED'),
    row('H6-RACE-03', 'Readback differs without changing disk', 'REVALIDATION', 'READBACK_MISMATCH'),
    row('H6-RACE-04', 'External root wins exclusive creation', 'ROOT_CAS', 'ROOT_ALREADY_EXISTS'),
    row('H6-JOIN-01', 'Same receipt tree/path through exported transaction', 'CLI', 'RECEIPT_PATH_DUPLICATE'),
    row('H6-JOIN-02', 'Duplicate receipt ID through transaction', 'RECEIPT_JOIN', 'RECEIPT_ID_DUPLICATE'),
    row('H6-JOIN-03', 'Pair order mismatch through transaction', 'RECEIPT_JOIN', 'RECEIPT_ORDER_MISMATCH'),
    row('H6-JOIN-04', 'Valid persisted prefix references wrong bytes despite matching digest strings',
        'JOIN', 'OUTPUT_MISMATCH', joined),
    row('H6-ID-02g', 'Synthetic staleV512 revision', 'IDENTITY', 'REVISION_MISMATCH', added),
    row('H6-ID-03d', 'Synthetic staleV512 candidate tuple', 'IDENTITY', 'CANDIDATE_MISMATCH', added),
    row('H6-ID-02h', 'Synthetic pre-consolidationV513 revision', 'IDENTITY', 'REVISION_MISMATCH', added),
    row('H6-ID-03e', 'Synthetic pre-consolidationV513 candidate tuple', 'IDENTITY', 'CANDIDATE_MISMATCH', added),
    row('H6-WORKER-01', 'Missing worker result', 'WORKER', 'WORKER_RESULT_MISSING', worker),
    row('H6-WORKER-02', 'Missing worker status property', 'WORKER', 'WORKER_STATUS_MISSING', worker),
    row('H6-WORKER-03', 'Unknown worker status', 'WORKER', 'WORKER_STATUS_UNKNOWN', worker),
    row('H6-WORKER-04', 'Explicit worker FAIL', 'WORKER', 'WORKER_FAILED', worker),
    row('H6-WORKER-05', 'Missing worker output inventory', 'WORKER', 'WORKER_OUTPUT_MISSING', worker),
    row('H6-WORKER-06', 'Extra worker output inventory', 'WORKER', 'WORKER_OUTPUT_UNEXPECTED', worker),
    row('H6-WORKER-07', 'Duplicate worker output inventory', 'WORKER', 'WORKER_OUTPUT_DUPLICATE', worker),
    row('H6-WORKER-08', 'Undefined worker status property', 'WORKER', 'WORKER_STATUS_MISSING', worker),
    row('H6-EVIDENCE-01', 'Missing required evidence helper/export', 'CONFIG', 'DEPENDENCY_MISSING', evidence),
    row('H6-EVIDENCE-02', 'Dummy source tuple', 'JOIN', 'DISPATCH_MISMATCH', evidence),
    row('H6-EVIDENCE-03', 'Absent evidence manifest is published successfully', 'COMPLETE', 'H5_EVIDENCE_COMPLETE',
        {...evidence, status: 'PASS'}),
    row('H6-EVIDENCE-04', 'Output changed after evidence dispatch', 'REVALIDATION', 'OUTPUT_CHANGED',
        {...evidence, status: 'FAIL'}),
    row('H6-CONTROL-01', 'Positive real-CLI control', 'COMPLETE', 'H3_DISPATCH_COMPLETE',
        {...worker, ingress: 'cli', status: 'PASS'}),
    row('H6-CONTROL-02', 'Positive shared-transaction control', 'COMPLETE', 'H3_DISPATCH_COMPLETE',
        {...worker, status: 'PASS'}),
    row('H6-CONTROL-03', 'Real CLI forwards reordered option pairs and receipt argv', 'COMPLETE', 'H3_DISPATCH_COMPLETE',
        {...worker, ingress: 'cli', status: 'PASS'}),
    row('H6-POST-01', 'Post-root genesis mutation preserves valid prefix', 'READBACK', 'LEDGER_MISMATCH',
        {...joined, original: false, scope: 'H4.revalidateJoinedLedger'}),
    row('H6-POST-02', 'Post-root ledger mutation preserves valid prefix', 'READBACK', 'LEDGER_MISMATCH',
        {...joined, original: false, scope: 'H4.revalidateJoinedLedger'}),
    row('H6-POST-03', 'Post-root evidence manifest mutation preserves valid prefix', 'REVALIDATION', 'MANIFEST_CHANGED',
        {...report, original: false, scope: 'H5.revalidateEvidenceManifest'})
]);

const IDS = Object.freeze([...new Set(INVENTORY.map(entry => entry.id))].sort());
const ORIGINAL_IDS = Object.freeze(INVENTORY.filter(entry => entry.original).map(entry => entry.id).sort());
const EXECUTABLE = Object.freeze(INVENTORY.filter(entry => entry.state === 'executable'));
const PENDING = Object.freeze(INVENTORY.filter(entry => entry.state === 'pending'));

module.exports = Object.freeze({INVENTORY, IDS, ORIGINAL_IDS, EXECUTABLE, PENDING, COUNT: IDS.length});
