param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$repo = (Resolve-Path "$root/../../..").Path
$build = "$root/build"
New-Item -ItemType Directory -Force $build | Out-Null
foreach ($s in (Get-Content "$root/reference/sources.json" -Raw | ConvertFrom-Json)) {
    if ((Get-FileHash "$root/reference/$($s.file)" -Algorithm SHA256).Hash.ToLower() -ne $s.sha256) {
        throw "Source hash mismatch: $($s.file)"
    }
}
$cpp = Get-Content "$root/reference/mame-c352.cpp" -Raw
$header = Get-Content "$root/reference/mame-c352.h" -Raw
function Slice([string]$text, [string]$from, [string]$to) {
    $a = $text.IndexOf($from)
    $b = $text.IndexOf($to, $a + $from.Length)
    if ($a -lt 0 -or $b -le $a) { throw "Missing extraction marker: $from" }
    return $text.Substring($a, $b - $a)
}
$types = Slice $header "`tenum" "`tvoid fetch_sample"
$methods = Slice $cpp 'void c352_device::fetch_sample' 'void c352_device::log_pcm'
$table = Slice $cpp "`tint j = 0;" "`t// register save state info"
$reset = $cpp.Substring($cpp.IndexOf('void c352_device::device_reset()'))
$prefix = @'
// Test-only adapter. Extracted methods retain the MAME BSD-3-Clause notice.
// copyright-holders: R. Belmont, superctr
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <iostream>
#include <stdexcept>
using u8=uint8_t; using s8=int8_t; using u16=uint16_t; using s16=int16_t;
using u32=uint32_t; using s32=int32_t; using offs_t=uint32_t;
struct sound_stream {
  int out[4]{};
  void update() {}
  int samples() const { return 1; }
  void put_int(int ch,int,int value,int) { out[ch]=value; }
};
#define COMBINE_DATA(p) (*(p) = (*(p) & ~mem_mask) | (data & mem_mask))
#define C352_LOG_PCM 0
#define logerror(...) ((void)0)
class c352_device { public:
'@
$suffix = @'
  sound_stream sink, *m_stream=&sink;
  c352_voice_t m_c352_v[32]{};
  s16 m_mulawtab[256]{};
  u16 m_random=0, m_control=0;
  std::vector<u8> rom=std::vector<u8>(1<<24);
  std::vector<u32> reads;
  u8 read_byte(u32 addr) { reads.push_back(addr & 0xffffff); return rom[addr & 0xffffff]; }
  void fetch_sample(c352_voice_t&);
  void ramp_volume(c352_voice_t&,int,u8);
  void sound_stream_update(sound_stream&);
  u16 read(offs_t);
  void write(offs_t,u16,u16 mem_mask=0xffff);
  void log_pcm(int) {}
  void device_reset();
  void init_table();
  void step() { sound_stream_update(sink); }
};
'@
$sample = Get-Content "$repo/src/engine/sample.cpp" -Raw
$existingTable = Slice $sample 'const short c219Table[256]=' 'unsigned char c219HighBitPos'
$generated = $prefix + $types + $suffix + $methods + $reset + "`nvoid c352_device::init_table() {`n" + $table + "`n}`n" + $existingTable
[IO.File]::WriteAllText("$build/oracle.hpp", $generated)
& $Compiler -std=c++17 -O0 -Wall -Wextra -Werror -I $build "$root/vectors.cpp" -o "$build/vectors.exe"
if ($LASTEXITCODE -ne 0) { throw 'Reference harness compilation failed' }
& "$build/vectors.exe"
if ($LASTEXITCODE -ne 0) { throw 'Reference vectors failed' }
$qp = Get-Content "$root/reference/QuattroPlay-c352.c" -Raw
$qp = $qp.Replace('#include "../lib/vgm.h"', '#define vgm_write(...) ((void)0)')
[IO.File]::WriteAllText("$build/qp-source.c", $qp)
Copy-Item "$root/reference/QuattroPlay-c352.h" "$build/c352.h" -Force
& $Compiler -x c -std=c11 -O0 -Wall -Wextra -Werror -I $build "$root/qp-vectors.c" -o "$build/qp-vectors.exe"
if ($LASTEXITCODE -ne 0) { throw 'QuattroPlay harness compilation failed' }
& "$build/qp-vectors.exe"
if ($LASTEXITCODE -ne 0) { throw 'QuattroPlay comparison vectors failed' }
Write-Output 'PASS: pinned source hashes and extracted MAME reference vectors'
