param([switch]$RenderDemo)

$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw 'Install Visual Studio with the Desktop development with C++ workload.'
}
$visualStudioPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudioPath) { throw 'No MSVC C++ toolchain was found.' }

Import-Module (Join-Path $visualStudioPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $visualStudioPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$ninjaPath = Join-Path $visualStudioPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
if (-not (Test-Path -LiteralPath $ninjaPath)) {
    throw 'Install the C++ CMake tools for Windows component in Visual Studio.'
}
if (-not (Test-Path -LiteralPath 'JUCE\CMakeLists.txt')) {
    & git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { throw 'Could not fetch JUCE.' }
}
& cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_MAKE_PROGRAM=$ninjaPath"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& cmake --build build --config Release --parallel 3
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
if ($RenderDemo) {
    & '.\build\ShepardRender_artefacts\Release\ShepardRender.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Offline rendering failed.' }
}
