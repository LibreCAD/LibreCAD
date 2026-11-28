/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const h1 = require('./preflight-receipt');
const h2 = require('./input-capture');
const {RunnerError, ensure} = require('./formal-registry');
const {validateRequest, runFormalTransaction} = require('./formal-runner');

// --run --config ABS --run-dir ABS --preflight-receipt ABS --preflight-receipt ABS
// Parsing performs no filesystem reads or worker launches. Precedence is token
// errors, run mode, receipt cardinality, required values, duplicate paths, paths.
// Content IDs/order are checked only by the shared transaction, for both ingresses.
function parseRunArguments(argv) {
    ensure(Array.isArray(argv) && argv.length <= 64, 'CLI', 'ARG_INVALID', 'argument list must be bounded');
    const values = {};
    const receiptPaths = [];
    let run = false;
    for (let i = 0; i < argv.length; i++) {
        const option = argv[i];
        ensure(typeof option === 'string' && Buffer.byteLength(option) <= 8192 && !option.includes('\0'),
            'CLI', 'ARG_INVALID', 'argument must be a bounded NUL-free string');
        if (option === '--run') {
            ensure(!run, 'CLI', 'ARG_DUPLICATE_OPTION', 'duplicate --run');
            run = true;
        } else {
            ensure(['--config', '--run-dir', '--preflight-receipt'].includes(option),
                'CLI', 'ARG_UNKNOWN_OPTION', `unknown option: ${option}`);
            ensure(option === '--preflight-receipt' || !Object.hasOwn(values, option),
                'CLI', 'ARG_DUPLICATE_OPTION', `duplicate ${option}`);
            const value = argv[++i];
            ensure(typeof value === 'string' && !value.startsWith('--'), 'CLI', 'ARG_MISSING_VALUE', `missing value for ${option}`);
            try { h2.string(value, option, 8192); }
            catch (error) { throw new RunnerError('CLI', 'ARG_INVALID', error.message); }
            if (option === '--preflight-receipt') receiptPaths.push(value);
            else values[option] = value;
        }
    }
    ensure(run, 'CLI', 'RUN_MODE_REQUIRED', '--run is required');
    ensure(receiptPaths.length === 2, 'CLI', 'RECEIPT_COUNT', 'exactly two receipt paths are required');
    ensure(Object.hasOwn(values, '--config') && Object.hasOwn(values, '--run-dir'),
        'CLI', 'ARG_MISSING_VALUE', '--config and --run-dir are required');
    return validateRequest({configPath: values['--config'], runRoot: values['--run-dir'], receiptPaths});
}

async function cli(argv) {
    let result;
    try { result = await runFormalTransaction(parseRunArguments(argv)); }
    catch (error) {
        result = {schema_version: 1, purpose: null, status: 'REFUSED', stage: error.stage || 'CLI',
            reason: error.reason || 'ARG_INVALID', message: error.message, exit_code: 2,
            run_root: null, root_created: false, worker_launches: 0, workers: []};
    }
    process.stdout.write(h1.canonicalJson(result));
    process.exitCode = result.exit_code;
    return result;
}

if (require.main === module) cli(process.argv.slice(2));

module.exports = Object.freeze({parseRunArguments, cli});
