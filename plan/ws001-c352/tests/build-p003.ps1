param([ValidateRange(1,64)][int]$Jobs=1)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/verify-p003-static.ps1"
. "$PSScriptRoot/enter-vs2019.ps1"
$repo=(Resolve-Path "$PSScriptRoot/../../..").Path
$build="$PSScriptRoot/build/furnace-gui-vs2019-msbuild"
New-Item -ItemType Directory -Force $build | Out-Null
$options=@('-G','Visual Studio 16 2019','-A','x64','-T','v142',
  '-DCMAKE_POLICY_VERSION_MINIMUM=3.5','-DBUILD_GUI=ON','-DCONSOLE_SUBSYSTEM=ON',
  '-DUSE_SDL2=ON','-DUSE_SNDFILE=ON','-DUSE_RTMIDI=OFF','-DUSE_BACKWARD=OFF',
  '-DWITH_PORTAUDIO=OFF','-DWITH_ASIO=OFF','-DWITH_JACK=OFF','-DWITH_OGG=OFF',
  '-DWITH_MPEG=OFF','-DWITH_JSON=ON','-DWITH_LOCALE=ON','-DUSE_MOMO=ON','-DWITH_RENDER_SDL=ON',
  '-DWITH_RENDER_OPENGL=OFF','-DWITH_RENDER_OPENGL1=OFF','-DWITH_RENDER_DX9=OFF',
  '-DWITH_RENDER_DX11=OFF')
$ErrorActionPreference='Continue' # Windows PowerShell treats native stderr as ErrorRecords.
$configureLog="$PSScriptRoot/build/configure-gui-vs2019-msbuild.log"
$buildLog="$PSScriptRoot/build/furnace-gui-vs2019-msbuild.log"
& cmake -S $repo -B $build @options *> $configureLog
$result=$LASTEXITCODE
$ErrorActionPreference='Stop'
if ($result -ne 0) { Get-Content $configureLog -Tail 50; throw 'Furnace configure failed' }
$ErrorActionPreference='Continue'
& cmake --build $build --config Release --target furnace -- "/m:$Jobs" *> $buildLog
$result=$LASTEXITCODE
$ErrorActionPreference='Stop'
if ($result -ne 0) { Get-Content $buildLog -Tail 65; throw 'Furnace build failed' }
Write-Output 'PASS: GUI Release Furnace build'

# Reuse the exact production GUI objects, definitions and dependency libraries from
# CMake's VS project. Only main.obj is replaced: integration-test.cpp includes
# src/main.cpp with its entry point renamed, preserving the application's globals.
[xml]$project=Get-Content -LiteralPath "$build/furnace.vcxproj" -Raw
$xmlNs='http://schemas.microsoft.com/developer/msbuild/2003'
$ns=New-Object System.Xml.XmlNamespaceManager($project.NameTable)
$ns.AddNamespace('m',$xmlNs)
$releaseCondition="'`$(Configuration)|`$(Platform)'=='Release|x64'"
$release=$project.SelectNodes('/m:Project/m:ItemDefinitionGroup',$ns) |
  Where-Object { $_.Condition -eq $releaseCondition }
if (@($release).Count -ne 1) { throw 'Expected one Release x64 configuration' }
$objectRoot=Join-Path $build 'furnace.dir/Release/'
$objects=@()
foreach ($source in $project.SelectNodes('/m:Project/m:ItemGroup/m:ClCompile',$ns)) {
  if ([IO.Path]::GetFullPath($source.Include) -eq (Join-Path $repo 'src/main.cpp')) { continue }
  $objectName=$source.SelectNodes('m:ObjectFileName',$ns) |
    Where-Object { !$_.Condition -or $_.Condition -eq $releaseCondition } |
    Select-Object -Last 1
  if ($objectName) {
    $objectPath=$objectName.InnerText.Replace('$(IntDir)',$objectRoot)
  } else {
    $objectPath=Join-Path $objectRoot ([IO.Path]::GetFileNameWithoutExtension($source.Include)+'.obj')
  }
  if ($objectPath.Contains('$(')) { throw "Unresolved object path: $objectPath" }
  if (!(Test-Path -LiteralPath $objectPath -PathType Leaf)) { throw "Missing production object: $objectPath" }
  $objects+=[IO.Path]::GetFullPath($objectPath)
}
if (!$objects.Count) { throw 'No production objects found' }

function Set-ProjectValue($node,[string]$name,[string]$value) {
  $child=$node.SelectSingleNode("m:$name",$ns)
  if (!$child) {
    $child=$project.CreateElement($name,$xmlNs)
    $null=$node.AppendChild($child)
  }
  $child.InnerText=$value
}

# Keep toolchain/import settings while removing CMake regeneration, application
# sources, resources and project references; they were built by the command above.
foreach ($group in @($project.SelectNodes('/m:Project/m:ItemGroup',$ns))) {
  if (!$group.SelectSingleNode('m:ProjectConfiguration',$ns)) {
    $null=$group.ParentNode.RemoveChild($group)
  }
}
$testOut=Join-Path $build 'integration-test/'
$testInt=Join-Path $testOut 'obj/'
New-Item -ItemType Directory -Force $testInt | Out-Null
foreach ($node in $project.SelectNodes('/m:Project/m:PropertyGroup/m:OutDir',$ns)) { $node.InnerText=$testOut }
foreach ($node in $project.SelectNodes('/m:Project/m:PropertyGroup/m:IntDir',$ns)) { $node.InnerText=$testInt }
foreach ($node in $project.SelectNodes('/m:Project/m:PropertyGroup/m:TargetName',$ns)) { $node.InnerText='integration-test' }
foreach ($node in $project.SelectNodes('/m:Project/m:PropertyGroup/m:ProjectName',$ns)) { $node.InnerText='integration-test' }
$project.SelectSingleNode('/m:Project/m:PropertyGroup/m:ProjectGuid',$ns).InnerText='{DC291654-BEA4-4793-9D9E-64B613CD0619}'
Set-ProjectValue $release.ClCompile 'PrecompiledHeader' 'NotUsing'
Set-ProjectValue $release.ClCompile 'PrecompiledHeaderFile' ''
Set-ProjectValue $release.ClCompile 'PrecompiledHeaderOutputFile' ''
Set-ProjectValue $release.ClCompile 'ForcedIncludeFiles' ''
Set-ProjectValue $release.Link 'AdditionalDependencies' ($release.Link.AdditionalDependencies+';'+($objects -join ';'))
Set-ProjectValue $release.Link 'ImportLibrary' (Join-Path $testOut 'integration-test.lib')
Set-ProjectValue $release.Link 'ProgramDataBaseFile' (Join-Path $testOut 'integration-test.pdb')
$testItems=$project.CreateElement('ItemGroup',$xmlNs)
$testSource=$project.CreateElement('ClCompile',$xmlNs)
$testSource.SetAttribute('Include',(Join-Path $PSScriptRoot 'integration-test.cpp'))
$null=$testItems.AppendChild($testSource)
$null=$project.DocumentElement.AppendChild($testItems)
$testProject=Join-Path $build 'integration-test.vcxproj'
$project.Save($testProject)

$testLog="$PSScriptRoot/build/integration-test-vs2019-msbuild.log"
$ErrorActionPreference='Continue'
& MSBuild $testProject /nologo /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 *> $testLog
$result=$LASTEXITCODE
$ErrorActionPreference='Stop'
if ($result -ne 0) { Get-Content $testLog -Tail 65; throw 'C352 registry/save-reload test build failed' }
$resultLog="$PSScriptRoot/build/integration-test-results.log"
$ErrorActionPreference='Continue'
Push-Location $testOut
try {
  & (Join-Path $testOut 'integration-test.exe') *> $resultLog
  $result=$LASTEXITCODE
} finally {
  Pop-Location
}
$ErrorActionPreference='Stop'
Get-Content -LiteralPath $resultLog
if ($result -ne 0) { throw "C352 registry/save-reload tests failed (exit $result)" }
Write-Output 'PASS: C352 registry/save-reload tests with production Furnace objects'
