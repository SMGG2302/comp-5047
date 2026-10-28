param([switch]$NoBrowser)
$ErrorActionPreference = 'Stop'
$env:HH_PORT = '8766'
$url = 'http://127.0.0.1:8766'
$running = $false
try {
    $state = Invoke-RestMethod "$url/api/state" -TimeoutSec 2
    $running = $null -ne $state.config.deviceHost
} catch {}
if (-not $running) {
    $nodePath = (Get-Command node.exe -ErrorAction SilentlyContinue).Source
    if (-not $nodePath) { throw 'Node.js 18+ is required. Install Node.js and try again.' }
    $dataPath = Join-Path $PSScriptRoot 'data'
    New-Item -ItemType Directory -Force -Path $dataPath | Out-Null
    $serverPath = Join-Path $PSScriptRoot 'server.js'
    $process = Start-Process -FilePath $nodePath -ArgumentList @('"' + $serverPath + '"') -WorkingDirectory $PSScriptRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dataPath 'server.log') -RedirectStandardError (Join-Path $dataPath 'server-error.log')
    $process.Id | Set-Content -LiteralPath (Join-Path $dataPath 'server.pid')
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        Start-Sleep -Milliseconds 200
        try { $state = Invoke-RestMethod "$url/api/state" -TimeoutSec 1; $running = $null -ne $state.config.deviceHost; if ($running) { break } } catch {}
        if ($process.HasExited) { break }
    }
    if (-not $running) { throw "HollowHaven failed to start. Check $dataPath\server-error.log" }
}
if (-not $NoBrowser) { Start-Process $url }
