$ErrorActionPreference = 'Stop'
$projectPath = $PSScriptRoot
$buildPath = Join-Path $projectPath 'build'
$vswherePath = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = $null
if (Test-Path -LiteralPath $vswherePath) {
    $vsPath = & $vswherePath -latest -prerelease -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
# This host has an additional Preview installation not returned by vswhere.
if (-not $vsPath) {
    $previewPath = 'C:\Program Files\Microsoft Visual Studio\2022\Preview'
    if (Test-Path -LiteralPath (Join-Path $previewPath 'VC\Tools\MSVC')) { $vsPath = $previewPath }
}
if (-not $vsPath) { throw 'Install Visual Studio 2022 C++ build tools (C compiler and Windows SDK), or use the GCC command in README.md.' }
& cmake -S $projectPath -B $buildPath -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_GENERATOR_INSTANCE=$vsPath"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& cmake --build $buildPath --config Release
if ($LASTEXITCODE -ne 0) { throw 'C build failed.' }
& ctest --test-dir $buildPath -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'C tests failed.' }
Write-Host "Receiver: $buildPath\Release\receiver.exe"
