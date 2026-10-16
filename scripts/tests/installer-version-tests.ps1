# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li

$ErrorActionPreference = 'Stop'
$script = Get-Content -Raw (Join-Path $PSScriptRoot '..\build-windows.bat')
$parser = [regex]::Match($script, '(?ms)^set "input=%SCMREVISION%"\r?\n.*?^set "VIProductVersion=[^\r\n]+')
if (-not $parser.Success) {
    throw 'Installer version parser not found'
}

$cases = @(
    @('v2.2.1.5-67-g0428dfc0c', '2.2.1.5'),
    @('v2.2.1.6', '2.2.1.6'),
    @('V2.2.1.6', '2.2.1.6'),
    @('2.2.1.6', '2.2.1.6'),
    @('v2.2.1', '2.2.1.0'),
    @('2.2.2_rc1-3-gabcdef0', '2.2.2.0')
)
$batch = Join-Path ([IO.Path]::GetTempPath()) ([IO.Path]::GetRandomFileName() + '.bat')
try {
    foreach ($case in $cases) {
        $revision, $expected = $case
        $content = "@echo off`r`nsetlocal EnableDelayedExpansion`r`nset `"SCMREVISION=$revision`"`r`n" +
            $parser.Value + "`r`necho !VIProductVersion!`r`nexit /b 0`r`n"
        [IO.File]::WriteAllText($batch, $content, [Text.Encoding]::ASCII)
        $actual = (& $env:ComSpec /d /c "`"$batch`"") -join "`n"
        if ($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $expected) {
            throw "Revision ${revision}: expected $expected, got $actual"
        }
        Write-Host "$revision -> $actual"
    }
} finally {
    Remove-Item -LiteralPath $batch -ErrorAction SilentlyContinue
}
