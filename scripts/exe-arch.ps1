# exe-arch.ps1
# Print the architecture a Windows program was built for: x86, AMD64 or ARM64.
#
# Why this exists:
#   An installer has to match the program it packages. The machine that builds
#   the program does not say what the program is: 64-bit Windows also builds
#   32-bit programs. The machine type in the header of the executable does.
#
# Usage:
#   .\exe-arch.ps1 <path of an .exe or a .dll>

param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Path
)

$ErrorActionPreference = 'Stop'

$file = (Resolve-Path -LiteralPath $Path).ProviderPath
$stream = [System.IO.File]::OpenRead($file)
try {
    $reader = New-Object System.IO.BinaryReader($stream)
    if ($reader.ReadUInt16() -ne 0x5A4D) {
        throw "[exe-arch] $file is not a Windows executable"
    }
    # The DOS header gives the offset of the PE header at 0x3C. The PE header
    # starts with "PE\0\0", and the machine type follows it.
    $stream.Position = 0x3C
    $stream.Position = $reader.ReadUInt32()
    if ($reader.ReadUInt32() -ne 0x00004550) {
        throw "[exe-arch] $file has no PE header"
    }
    $machine = $reader.ReadUInt16()
}
finally {
    $stream.Close()
}

switch ($machine) {
    0x014C  { 'x86' }
    0x8664  { 'AMD64' }
    0xAA64  { 'ARM64' }
    default { throw ('[exe-arch] {0} is built for machine type 0x{1:X4}, which is not known here' -f $file, $machine) }
}
