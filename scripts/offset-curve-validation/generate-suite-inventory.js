/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

'use strict';

const api = require('./suite-inventory');
function writeGenerated(filePath, value) {
    const fs = require('node:fs');
    const bytes = api.canonical(value);
    api.ensure(bytes.length <= api.LIMITS.definitionBytes, 'generated configuration exceeds byte cap');
    const fd = fs.openSync(filePath, fs.constants.O_CREAT | fs.constants.O_EXCL | fs.constants.O_NOFOLLOW | fs.constants.O_WRONLY, 0o600);
    try {
        let offset = 0;
        while (offset < bytes.length) { const count = fs.writeSync(fd, bytes, offset, bytes.length - offset); api.ensure(count > 0, 'zero configuration write'); offset += count; }
        fs.fchmodSync(fd, 0o400); fs.fsyncSync(fd);
    } finally { fs.closeSync(fd); }
    api.ensure(api.pinFile(filePath, api.LIMITS.definitionBytes).sha256 === api.hash(bytes), 'generated configuration readback differs');
}

async function generateAndPublish(options) {
    options = api.snapshotGeneratorOptions(options);
    const result = await api.generateSuiteInventory(options);
    // Three exclusive publications, not an atomic multi-file transaction; failures retain their prefix.
    writeGenerated(options.definitionsPath, result.definitions);
    writeGenerated(options.registryPath, result.registry);
    writeGenerated(options.inventoryPath, result.inventory);
    return result;
}

async function cli(argv) {
    api.ensure(argv.length === 6 && argv[0] === '--definitions' && argv[2] === '--registry' && argv[4] === '--inventory',
        'expected --definitions ABS --registry ABS --inventory ABS');
    const result = await generateAndPublish({purpose: 'validation', definitionsPath: argv[1], registryPath: argv[3], inventoryPath: argv[5], groups: null,
        environment: {...process.env}});
    process.stdout.write(api.canonical({schema_version: 1, status: 'GENERATED_NOT_QUALIFIED',
        case_count: result.inventory.cases.length, capture: result.capture,
        definitions: api.pinFile(argv[1], api.LIMITS.definitionBytes), registry: api.pinFile(argv[3]), inventory: api.pinFile(argv[5])}));
    return result;
}

if (require.main === module) cli(process.argv.slice(2)).catch(error => { process.stderr.write(`${error.stack || error}\n`); process.exitCode = 1; });
module.exports = Object.freeze({writeGenerated, generateAndPublish, cli});
