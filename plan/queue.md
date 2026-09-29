# Queue Book

- **Queue ID:** q003
- **Status:** proposed
- **Timebox:** 4時間（P003実装・検証の上限案。GUI依存取得を含む）
- **Human authorization:** 2026-09-29 JST「P003の計画具体化して」「作業を進めて」。このQueueでは計画の具体化のみを認可済みとし、コード実行の認可は未取得。
- **対象:** ws001p003 — C352システム・UI・資産統合
- **依存関係:** ws001p002 cleared、成果commit `3a08cafe0`
- **Item ws001p003:** pending（Queue認可待ち）

## Allowed Touch Points

- `src/engine/sysDef.h`, `src/engine/sysDef.cpp`
- `src/engine/dispatchContainer.cpp`
- `src/engine/instrument.h`, `src/engine/instrument.cpp`
- `src/engine/engine.cpp`, `src/engine/legacySample.cpp`, `src/engine/fileOps/json.cpp`
- `src/engine/platform/c352.cpp`（C352 instrument type接続のみ。core arithmetic／ROM実装は変更しない）
- `src/gui/gui.h`, `src/gui/guiConst.cpp`, `src/gui/settings/allSettings.cpp`
- `src/gui/insEdit.cpp`, `src/gui/doAction.cpp`, `src/gui/sampleEdit.cpp`
- `src/gui/sysConf.cpp`, `src/gui/sysMiscInfo.cpp`, `src/gui/debug.cpp`
- `src/gui/presets/sample.cpp`, `src/gui/presets/arcadeSystems.cpp`
- `doc/4-instrument/README.md`, `doc/4-instrument/c352.md`
- `doc/7-systems/README.md`, `doc/7-systems/c352.md`
- `papers/format.md`, `papers/newIns.md`
- `plan/ws001-c352/tests/`（P003 registry/save-reload/build assets。P001 reference原本とP002検証資産は不変）
- `plan/ws001-c352/phase003/phase.md`, `plan/ws001-c352/ws.md`, `plan/queue.md`, `plan/ledger.md`, `plan/insights/index.md`, `plan/history/q003.md`

## Explicitly excluded

- `src/engine/vgmOps.cpp` — VGM version 0／非対応を維持。
- `src/engine/platform/sound/c352.cpp`、C140/C219の既存core・dispatcher・decoder — P002成果と既存音源を不変にする。
- P004の大規模既存曲回帰、P005の最終品質判定。
- Ninjaの導入・実行。

## Proposed execution order

1. file ID `0xe8`、instrument type `68`、enum衝突、既存C140/C219 blobをpreflightで確認。
2. system enum／definition、dispatch factory、instrument feature／sample lifecycleを接続。
3. C352 instrument、sample depth／loop warnings、`quadOutput`／custom clock flagsを接続。
4. GUI chooser／macro editor／system config／debug／presetを接続。
5. docsとformat tableを更新し、VGM非対応を明記。
6. `verify.ps1`、`build-p002.ps1`、新設`build-p003.ps1`（VS2019 MSBuild GUI／registry／save-reload）を実行。
7. 合格後のみP003成果commit、q003 archive、master統合、指定originへの通常pushを行う。失敗時はinsight記録とrollbackを行い、unclearedとする。

## Proposed verification

- Registry: C352がsystem chooser／file map／dispatch factoryに一意に登録され、32 channels、8-bit+C219 mask、VGM version 0となる。
- Asset: dedicated instrumentのsample map、macro、sample usage/deletion protection、JSON／legacy pathが機能する。
- Persistence: `.fur` save/reloadでsystem ID、32 channels、C352 instrument、sample bytes／depth／loop、`customClock`、`quadOutput`が一致する。
- GUI: C352 chooser、macro editor、sample constraints、system configuration、debug register sheetが表示される。
- Regression: C140/C219 IDs `0xce`／`0xcf`、channel count、既存P002 gateが変わらない。
- Build: Developer PowerShell for VS 2019、MSVC v142 x64、Visual Studio 16 2019／MSBuild。GUIはSDL submodule初期化後に構成する。Ninjaは使用しない。

## Authorization boundary

P003の計画はready化したが、q003は`proposed`で停止する。ユーザーがP003の実行を明示するまで、上記Allowed Touch Pointsへのコード変更、ビルド、commit、pushを開始しない。
