$ErrorActionPreference = 'Stop'
$pidFile = Join-Path $PSScriptRoot 'data\server.pid'
if (Test-Path -LiteralPath $pidFile) {
    $serverProcessId = [int](Get-Content -LiteralPath $pidFile)
    $process = Get-CimInstance Win32_Process -Filter "ProcessId=$serverProcessId" -ErrorAction SilentlyContinue
    $expected = Join-Path $PSScriptRoot 'server.js'
    if ($process -and $process.Name -eq 'node.exe' -and $process.CommandLine.Contains($expected)) {
        Stop-Process -Id $serverProcessId
        Write-Host 'HollowHaven stopped.'
    }
}
