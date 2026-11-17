# Renders test_data\pdf-image\pdf-image.dxf, whose only entity shows a PDF page
# filled with red, and fails unless the result is mostly red. Qt's PDF image
# plugin fails silently, so only the pixels show whether it loaded.
param(
    [Parameter(Mandatory = $true)][string]$Program,
    [string]$Drawing = (Join-Path $PSScriptRoot '..\test_data\pdf-image\pdf-image.dxf')
)
$ErrorActionPreference = 'Stop'
$temp = if ($env:RUNNER_TEMP) { $env:RUNNER_TEMP } else { [System.IO.Path]::GetTempPath() }
$png = Join-Path $temp 'pdf-image.png'
$errors = Join-Path $temp 'pdf-image-stderr.txt'
Remove-Item -LiteralPath $png -ErrorAction SilentlyContinue
$drawingPath = (Resolve-Path -LiteralPath $Drawing).Path
$env:QT_FORCE_STDERR_LOGGING = '1'
$run = Start-Process -FilePath $Program -NoNewWindow -Wait -PassThru -RedirectStandardError $errors `
    -ArgumentList @('dxf2png', '-r', '200x200', '-o', "`"$png`"", "`"$drawingPath`"")
if (Test-Path -LiteralPath $errors) { Get-Content -LiteralPath $errors }
if ($run.ExitCode -ne 0) { throw "dxf2png exited with $($run.ExitCode)." }
if (-not (Test-Path -LiteralPath $png)) { throw 'dxf2png wrote no PNG.' }
Add-Type -AssemblyName System.Drawing
$bitmap = [System.Drawing.Bitmap]::FromFile($png)
$samples = 0
$red = 0
try {
    for ($y = 0; $y -lt $bitmap.Height; $y += 4) {
        for ($x = 0; $x -lt $bitmap.Width; $x += 4) {
            $samples++
            $pixel = $bitmap.GetPixel($x, $y)
            if ($pixel.R -gt 200 -and $pixel.G -lt 80 -and $pixel.B -lt 80) { $red++ }
        }
    }
}
finally {
    $bitmap.Dispose()
}
if (2 * $red -lt $samples) {
    throw "The PDF image did not render ($red of $samples samples red): Qt's PDF image plugin is missing or did not load."
}
Write-Host "The PDF image rendered: $red of $samples samples red."
