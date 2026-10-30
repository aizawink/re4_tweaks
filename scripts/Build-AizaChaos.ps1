param(
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [ValidateSet('', 'v143', 'v145')][string]$Toolset = ''
)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ Build Tools first.' }
$installation = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual Studio C++ x86/x64 tools not found.' }
$msbuild = Join-Path $installation 'MSBuild/Current/Bin/MSBuild.exe'
if (!$Toolset) {
    $versionFile = Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt'
    $version = [version]([IO.File]::ReadAllText($versionFile).Trim())
    if ($version.Major -eq 14 -and $version.Minor -ge 50) { $Toolset = 'v145' }
    elseif ($version.Major -eq 14 -and $version.Minor -ge 30) { $Toolset = 'v143' }
    else { throw 'Requires C++ toolset v143 or v145.' }
}
Push-Location $project
try {
    & git submodule update --init --recursive
    if ($LASTEXITCODE -ne 0) { throw 'Submodule checkout failed.' }
    $output = Join-Path $project ('artifacts/aiza-chaos-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    New-Item -ItemType Directory -Path $output | Out-Null
    $outDir = $output.Replace('\','/') + '/'
    & $msbuild (Join-Path $project 're4_tweaks.sln') /m /t:Build `
        "/p:Configuration=$Configuration" /p:Platform=Win32 `
        "/p:PlatformToolset=$Toolset" "/p:OutDir=$outDir" `
        /p:PostBuildEventUseInBuild=false
    if ($LASTEXITCODE -ne 0) { throw 'Build failed. Game DLL has not been replaced.' }
    $dll = Join-Path $output 'dinput8.dll'
    if (!(Test-Path -LiteralPath $dll)) { throw 'Build finished but dinput8.dll was not found.' }
    Copy-Item -LiteralPath (Join-Path $project 'settings/settings.ini') -Destination (Join-Path $output 'dinput8.ini')
    $modDir = Join-Path $output 're4_tweaks'
    New-Item -ItemType Directory -Path $modDir | Out-Null
    Copy-Item -LiteralPath (Join-Path $project 'settings/trainer_settings.ini') -Destination (Join-Path $modDir 'trainer.ini')
    Copy-Item -Path (Join-Path $project 'external/EnglishSubtitleData/Bin32/re4_tweaks/*') -Destination $modDir -Recurse -Force
    Copy-Item -Path (Join-Path $project 'dist/Bin32/re4_tweaks/*') -Destination $modDir -Recurse -Force
    Write-Host "BUILD COMPLETE ($Toolset): $output" -ForegroundColor Green
    Write-Host 'Game files have not been changed. See docs/AIZA_CHAOS.md for installation and testing.'
}
finally { Pop-Location }
