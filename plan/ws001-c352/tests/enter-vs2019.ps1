$ErrorActionPreference='Stop'
$vswhere="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
if (!(Test-Path $vswhere)) { throw 'Visual Studio Installer / vswhere is required' }
$vsPath=& $vswhere -latest -version '[16.0,17.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vsId=& $vswhere -latest -version '[16.0,17.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property instanceId
if (!$vsPath -or !$vsId) { throw 'VS 2019 C++ tools are required' }
Import-Module "$vsPath/Common7/Tools/Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstanceId $vsId -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
if ($env:VisualStudioVersion -ne '16.0') { throw 'Expected Developer PowerShell for VS 2019' }
Write-Output "Developer PowerShell for VS 2019: $vsPath, $env:VSCMD_ARG_TGT_ARCH"
