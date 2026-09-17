# ws001p001 specification evidence

Run from the repository root (Windows PowerShell 5.1 or PowerShell 7; GCC on PATH):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1
```

`-Compiler <absolute path to g++.exe>` overrides compiler discovery. No network access or game ROM is required. `build/` is ignored and contains generated adapters and executables only. Reference source bytes are marked `-text` to preserve SHA-256 across Git checkouts.

`reference/sources.json` pins upstream commits, original paths, URLs and SHA-256. Source files are unmodified. MAME files identify BSD-3-Clause and authors in their headers; `mame-BSD-3-Clause` supplies the license text. QuattroPlay files are GPL-2.0-or-later; see `QuattroPlay-COPYING`. These are test/reference assets, not Furnace build inputs.

`verify.ps1` extracts MAME voice types, register access, fetch, mixing, reset and decoder initialization without rewriting their bodies. A small adapter substitutes the 24-bit ROM interface, one-frame output sink and full-word write default. It does not emulate MAME's scheduler, machine configuration, logging or partial bus timing. The QuattroPlay adapter only stubs VGM logging and supplies synthetic ROM. Tests expose implementation differences, not hardware proof.

`vectors.cpp` specifies explicit register writes and independently calculated expected values. The full decoder table is compared with the existing Furnace `c219Table`, not with another copy of the MAME algorithm. `qp-vectors.c` verifies the alternate terminal, FILTER, latch, control and output behavior. No assertion is a test of a yet-unimplemented Furnace C352 core.

For p002/p004: run this command unchanged as the reference gate, then run the same writes and observations against the new core adapter. Compare exact signed samples, addresses, counters, flags and outputs; audio-only similarity is insufficient. The adapter comparison gate must be added in the future authorized Queue, not claimed as passing by p001. p003/p004 integration acceptance cases and proposed commands are in the Phase Execution Log.
