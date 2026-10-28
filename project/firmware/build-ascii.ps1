param(
    [switch]$Upload,
    [string]$Port = 'COM3'
)
$ErrorActionPreference = 'Stop'
$sourceRoot = $PSScriptRoot
$targetRoot = 'C:\HollowHavenBuild'
$targetFirmware = Join-Path $targetRoot 'firmware'
$pio = 'C:\.platformio\penv\Scripts\platformio.exe'
if (-not (Test-Path -LiteralPath $pio)) { throw "PlatformIO not found: $pio" }
if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot 'platformio.ini'))) { throw 'Missing platformio.ini' }
$sharedSource = Join-Path (Split-Path $sourceRoot -Parent) 'shared'
if (-not (Test-Path -LiteralPath $sharedSource)) { throw 'Missing shared source directory' }
New-Item -ItemType Directory -Force -Path $targetFirmware | Out-Null
Write-Host 'Synchronizing saved source files to C:\HollowHavenBuild ...'
& robocopy $sourceRoot $targetFirmware /E /XD .pio .vscode build managed_components /XF sdkconfig sdkconfig.old sdkconfig.esp32dev /NFL /NDL /NJH /NJS /NP
if ($LASTEXITCODE -gt 7) { throw "Firmware copy failed: $LASTEXITCODE" }
& robocopy $sharedSource (Join-Path $targetRoot 'shared') /E /NFL /NDL /NJH /NJS /NP
if ($LASTEXITCODE -gt 7) { throw "Shared source copy failed: $LASTEXITCODE" }
$env:PLATFORMIO_CORE_DIR = 'C:\.platformio'
$pioArgs = @('run', '-d', $targetFirmware)
if ($Upload) { $pioArgs += @('-t', 'upload', '--upload-port', $Port) }
& $pio @pioArgs
if ($LASTEXITCODE -ne 0) { throw "PlatformIO failed: $LASTEXITCODE" }
