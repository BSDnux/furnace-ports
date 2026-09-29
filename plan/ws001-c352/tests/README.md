# C352 specification, core and dispatch verification

Run from the repository root with Windows PowerShell 5.1 and the VS 2019 C++ toolset installed. Both scripts enter Developer PowerShell for VS 2019 with the x64 host and target automatically:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p002.ps1
```

`verify.ps1` uses MSVC to compile the reference and production-core tests. `build-p002.ps1` requires CMake and the repository's dependency submodules, configures the `Visual Studio 16 2019` generator with `v142`, builds headless Release Furnace, then compiles, links and runs the dispatch integration test. MSBuild runs with one job by default; `-Jobs` controls the Furnace build job count. Neither script uses Ninja. No game ROM is required. Once dependencies are present, verification needs no network access. `build/` is ignored and contains generated adapters, projects, logs and binaries. Reference source bytes are marked `-text` to preserve SHA-256 across Git checkouts.

`reference/sources.json` pins upstream commits, original paths, URLs and SHA-256. Source files are unmodified. MAME files identify BSD-3-Clause and authors in their headers; `mame-BSD-3-Clause` supplies the license text. QuattroPlay files are GPL-2.0-or-later; see `QuattroPlay-COPYING`. These are test/reference assets, not Furnace build inputs.

`verify.ps1` extracts MAME voice types, register access, fetch, mixing, reset and decoder initialization without rewriting their bodies. A small adapter substitutes the 24-bit ROM interface, one-frame output sink and full-word write default. It does not emulate MAME's scheduler, machine configuration, logging or partial bus timing. The QuattroPlay adapter only stubs VGM logging and supplies synthetic ROM. Tests expose implementation differences, not hardware proof.

`vectors.cpp` specifies explicit register writes and independently calculated expected values, and runs against both the extracted MAME reference and the production C352 core through `core-adapter.hpp`. The full decoder table is compared with the existing Furnace `c219Table`, not with another copy of the MAME algorithm. `qp-vectors.c` verifies the alternate terminal, FILTER, latch, control and output behavior. `differential.cpp` compares seeded register activity, voice state and four output buses with the pinned MAME implementation, including muted voices and resets.

`dispatch-test.cpp` exercises `DivPlatformC352` directly using synthetic Furnace samples and instruments. Its generated VS project inherits the production Release compiler settings and dependency libraries and links the actual Furnace objects. It replaces the application entry point while retaining its global definitions; it does not substitute a mock engine or dispatch implementation. Build failures, link failures and nonzero test exits fail the script. Logs are written to `build/configure-vs2019-msbuild.log`, `build/furnace-vs2019-msbuild.log` and `build/dispatch-test-vs2019-msbuild.log`.

These checks cover the p002 core and dispatch layer. System selection, GUI, song serialization and existing-song regression remain later Phase acceptance work; passing this harness does not claim that coverage.

The dispatch gate covers ROM guards and bank boundaries, exact capacity and rejection,
byte ordering, note-on/off and voice 31, register write order and word dumps, loop and
sample-position mapping, quad/stereo mixing, mute/noise/scope continuity, pitch and
sample maps, and AMIGA volume/pan macros. It explicitly tests both the legacy 8363 Hz
and current 8372 Hz center references. Custom clocks use Furnace's existing limits.
The numerical run output is saved in `build/dispatch-test-results.log`.
