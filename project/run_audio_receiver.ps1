param([string]$DeviceIP = '192.168.4.1')
$ErrorActionPreference = 'Stop'
$audioExe = Join-Path $PSScriptRoot 'build\Release\audio_receiver.exe'
$audioFolder = Join-Path $PSScriptRoot 'recordings'
New-Item -ItemType Directory -Force $audioFolder | Out-Null
Write-Host 'Connect to HollowHaven-ESP32. A WAV file arrives after each completed event recording (about 30 seconds). Ctrl+C stops.'
& $audioExe --host $DeviceIP --output-dir $audioFolder
exit $LASTEXITCODE
