# P003 integration verification

Run from the repository root in PowerShell:

```powershell
git submodule update --init extern/SDL extern/json
powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p002.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p003.ps1
```

Both build scripts enter Developer PowerShell for VS 2019 and use Visual Studio 16 2019, MSVC v142 x64 and MSBuild. `-Jobs 4` is optional; the default is one MSBuild worker. Ninja is not used. SDL and JSON must be at the repository's pinned submodule commits.

`build-p003.ps1` first runs `verify-p003-static.ps1`, then builds the GUI Release target with the SDL renderer, JSON export and the bundled Momo locale support enabled. It links `integration-test.cpp` against the same production objects and libraries, replacing only the application's entry point. It neither starts an audio device nor changes the user's Furnace configuration. Synthetic fixtures and logs go under the ignored `build/` directory.

The integration executable checks:

- unique C352 system ID `0xe8`, instrument type 68, 32 PCM channels, dedicated and generic Sample instrument choices, 8-bit/C219 render mask, and VGM unsupported status;
- compiled GUI chooser tables and a rendered ImGui frame using the production chip/channel debug functions;
- `.fur` save/reload through separate engines, including sample bytes, depth, loop endpoints and modes, all 180 sample-map entries, volume/pan/arp/pitch/phase-reset macros, clock and quad flags;
- standalone instrument sample embedding, JSON sample-map export, unused-sample deletion protection and ROM loading;
- dedicated C352 sample-map playback and macro scaling, default stereo, restored quad output, and the common container's lazy allocation when switching from stereo to quad;
- C140/C219 IDs, channel counts, instrument/sample persistence and factory/ROM paths.

`verify-p003-static.ps1` covers editor macro ranges, sample warnings, shared clock controls, preset registration, legacy sample classification, VGM export UI gating and frozen-source Git blob equality against the q003 checkpoint. GUI interaction and visual layout are not manually tested by this script; the GUI build and debug-frame smoke test complement its source assertions. Broad existing-song regression and final quality clearance remain P004/P005 work.

Logs: `build/configure-gui-vs2019-msbuild.log`, `build/furnace-gui-vs2019-msbuild.log`, `build/integration-test-vs2019-msbuild.log`, `build/integration-test-results.log`.

Fixtures: `build/furnace-gui-vs2019-msbuild/integration-test/fixture-232.fur` (C352), `fixture-206.fur` (C140), `fixture-207.fur` (C219).
