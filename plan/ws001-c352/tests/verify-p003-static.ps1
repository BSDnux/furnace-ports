$ErrorActionPreference='Stop'
$repo=(Resolve-Path "$PSScriptRoot/../../..").Path
$checks=0
function Check([bool]$ok,[string]$message) {
  $script:checks++
  if (!$ok) { throw $message }
}
function Read([string]$path) { [IO.File]::ReadAllText((Join-Path $repo $path)) }
function Section([string]$text,[string]$pattern) {
  $m=[regex]::Match($text,$pattern,[Text.RegularExpressions.RegexOptions]::Singleline)
  Check $m.Success "Missing section: $pattern"
  $m.Groups[1].Value
}
$gui=Read 'src/gui/guiConst.cpp'
foreach ($name in @('availableSystems','chipsSample')) {
  $items=Section $gui ("const int "+$name+'\[\]=\{(.*?)\};')
  Check (([regex]::Matches($items,'\bDIV_SYSTEM_C352\b')).Count -eq 1) "$name must contain C352 once"
}
$types=Section $gui 'const char\* insTypes.*?=\{(.*?)\};'
$entries=[regex]::Matches($types,'(?m)^  \{')
Check ($entries.Count -eq 70) '69 instruments and one null sentinel expected'
Check ($types.Contains('{"C352",ICON_FA_VOLUME_UP,ICON_FA_VOLUME_UP},')) 'Dedicated name/icon missing'
$macro=Section (Read 'src/gui/insEdit.cpp') 'case DIV_INS_C352:\s*case DIV_INS_C140:(.*?)break;'
foreach($field in @('volMacro,0,255','arpMacro','panLMacro,0,255','panRMacro,0,255','pitchMacro','phaseResetMacro')) {
  Check ($macro.Contains($field)) "C352 macro missing or wrong range: $field"
}
Check (!$macro.Contains('dutyMacro') -and !$macro.Contains('waveMacro')) 'C352 must not expose unsupported macros'
$sample=Read 'src/gui/sampleEdit.cpp'
$warnings=Section $sample 'case DIV_SYSTEM_C352:(.*?)break;'
foreach($limit in @('65536U:65535U','64 KiB','16 MiB','DIV_SAMPLE_LOOP_BACKWARD','sample->loopStart>=sample->loopEnd','sample->loopEnd>(int)sample->samples')) {
  Check ($warnings.Contains($limit)) "Sample constraint missing: $limit"
}
Check ($sample.Contains('e->song.system[i]!=DIV_SYSTEM_PCM_DAC && e->song.system[i]!=DIV_SYSTEM_C352')) 'Generic loop warning must allow C352 ping-pong'
$conf=Read 'src/gui/sysConf.cpp'
$c352=Section $conf 'case DIV_SYSTEM_C352: \{(.*?)break;'
Check ($c352.Contains('flags.set("quadOutput",quadOutput)')) 'Quad setting must persist'
Check ($c352.Contains('clock / 288')) 'Clock divider explanation missing'
Check ($conf.Contains('flags.set("customClock",customClock)')) 'Common custom clock control missing'
$debug=Read 'src/gui/debug.cpp'
Check (([regex]::Matches($debug,'case DIV_SYSTEM_C352:')).Count -eq 2) 'Chip/channel debug sections missing'
Check (!$debug.Contains('DivPlatformC352::Channel')) 'Do not cast to private C352 channel'
$export=Read 'src/gui/exportOptions.cpp'
Check ($export.Contains('minVersion==0') -and $export.Contains('this chip is not supported by the VGM format!')) 'VGM unsupported UI gate missing'
Check ((Read 'src/gui/presets/sample.cpp').Contains('CH(DIV_SYSTEM_C352, 1.0f, 0, "")')) 'C352 sample preset missing'
Check ((Read 'src/engine/legacySample.cpp').Contains('ins->type==DIV_INS_C352 ||')) 'Legacy sample protection missing'
# Frozen P002 and existing C140/C219 source must remain byte-identical.
Push-Location $repo
try {
  $frozen=@('src/engine/platform/c140.cpp','src/engine/platform/c140.h',
    'src/engine/platform/sound/c140_c219.c','src/engine/platform/sound/c140_c219.h',
    'src/engine/platform/sound/c352.cpp','src/engine/platform/sound/c352.h',
    'src/engine/platform/c352.h','src/engine/sample.cpp','src/engine/vgmOps.cpp',
    'plan/ws001-c352/tests/build-p002.ps1','plan/ws001-c352/tests/dispatch-test.cpp',
    'plan/ws001-c352/tests/verify.ps1')
  $frozen+=@(git ls-tree -r --name-only 2c0122584 -- src/engine/platform/sound/c140 plan/ws001-c352/tests/reference)
  foreach($path in $frozen) {
    $expected=git rev-parse "2c0122584:$path"
    Check ($LASTEXITCODE -eq 0) "Missing baseline: $path"
    $actual=git hash-object --path=$path $path
    Check ($LASTEXITCODE -eq 0 -and $actual -eq $expected) "Frozen source changed: $path"
  }
} finally { Pop-Location }
Write-Output "PASS: $checks GUI/static and frozen-source assertions"
