param(
    [string]$DeviceIP = '192.168.4.1',
    [string]$OutputPath = ''
)
$ErrorActionPreference = 'Stop'
$receiverPath = Join-Path $PSScriptRoot 'build\Release\receiver.exe'
if (-not (Test-Path -LiteralPath $receiverPath)) { throw 'Run build_receiver.ps1 first.' }
if (-not $OutputPath) {
    $OutputPath = Join-Path $PSScriptRoot ('experiment_' + (Get-Date -Format 'yyyyMMdd_HHmmss') + '.csv')
}
Write-Host 'Connect this PC to Wi-Fi HollowHaven-ESP32 first. No Internet is expected on that hotspot.'
& $receiverPath --host $DeviceIP --output $OutputPath
exit $LASTEXITCODE
