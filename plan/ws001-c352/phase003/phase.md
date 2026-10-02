# WS001P003 — C352システム・UI・資産統合

- **Status:** ready
- **目的:** P002で検証済みのC352 dispatcherをFurnaceのシステム、専用インストゥルメント、サンプル管理、GUI、`.fur`保存／再読込へ接続する
- **依存:** `ws001p002` cleared (`30fb4baff`)
- **実行境界:** Queue `q003` の認可後にのみコード変更・ビルド・外部pushを行う。今回の計画具体化では実装しない。

## 成果定義

1. C352を新規Furnace systemとして選択でき、32 PCMチャンネルが作成される。
2. C352専用インストゥルメントを選択でき、既存の`DivInstrumentAmiga` sample map／macro保存形式を再利用してサンプルを割り当てられる。
3. 8-bit PCMとC219形式のサンプル深度、64 KiB bank／16 MiB ROM、loop／ping-pong／非対応条件がUIの説明と警告に一致する。
4. `quadOutput`とcustom clockをsystem flagsに保存し、再読込後も保持される。既定の2出力downmixと4出力の意味を表示する。
5. `.fur`を保存して別の`DivEngine`へ再読込したとき、system file ID、32チャンネル、instrument type／sample map、sample depth／bytes／loop、system flagsが一致する。
6. C352はVGM出力対象に追加せず、非対応を明示する。既存C140／C219のfile IDと動作を変更しない。

## 固定識別子と互換方針

- `DIV_SYSTEM_C352`は既存enum値を変更しないよう`DIV_SYSTEM_MAX`直前へ追加する。`.fur` system file IDは事前調査で未使用の`0xe8`を候補とし、実行開始時に全`sysDefs`との衝突を再検査する。
- `DIV_INS_C352`は既存enum値を変更しないよう`DIV_INS_MAX`直前へ追加する。instrument file typeは候補`68`を再検査し、`papers/newIns.md`へ追記する。
- C352 instrument固有のPODは新設せず、P002 dispatcherが使用する`DivInstrumentAmiga`の`initSample`／note mapを再利用する。C352専用UIではvolume／pan／pitch／phase-reset macroを表示し、C352で意味を持たないFM／wave項目は表示しない。
- C352 system definitionは32/32/32 channels、`DIV_CH_PCM`、primary `DIV_INS_C352`、alternate `DIV_INS_AMIGA`、sample mask `(1U<<DIV_SAMPLE_DEPTH_8BIT)|(1U<<DIV_SAMPLE_DEPTH_C219)`、VGM version `0`とする。clock既定25401600 Hz、divider 288、rate 88200 HzはP002契約を引き継ぐ。
- `src/engine/vgmOps.cpp`は変更しない。VGM未対応をsystem definitionのversion 0、export画面／文書の説明で表現する。推測のopcodeやheaderを追加しない。

## Allowed Touch Points

### Engine登録・資産・保存

- `src/engine/sysDef.h` / `src/engine/sysDef.cpp` — system enum、file-ID map、32ch definition、sample mask、VGM version 0。
- `src/engine/dispatchContainer.cpp` — C352 header includeと`DivPlatformC352`生成case。
- `src/engine/instrument.h` / `src/engine/instrument.cpp` — C352 instrument enumとsample／macro feature classification。
- `src/engine/engine.cpp` — sample使用判定・削除保護など、sample mapを走査するC352 type分岐。
- `src/engine/legacySample.cpp` / `src/engine/fileOps/json.cpp` — C352 instrumentをsample-capable typeとして扱う既存リストへの追加。
- `src/engine/platform/c352.cpp` — dedicated `DIV_INS_C352`をAMIGA fallbackと同じsample map経路へ接続し、macro scale／type判定をC352契約に合わせる。P002のcore arithmetic、ROM layout、既存C140/C219は変更しない。

### GUI・プリセット・デバッグ

- `src/gui/gui.h` / `src/gui/guiConst.cpp` / `src/gui/settings/allSettings.cpp` — C352 instrument color、名称／icon、system／sample-capable list、既定色設定。
- `src/gui/insEdit.cpp` — C352のVolume、Arpeggio、Panning L/R、Pitch、Phase Reset macro表示。
- `src/gui/doAction.cpp` — C352 instrumentをsample assignment／sample map操作の対象へ追加。
- `src/gui/sampleEdit.cpp` — C352の8-bit／C219 depth、65535＋guard、65536 loop、bank境界、backward loop非対応等の警告とloop hint。
- `src/gui/sysConf.cpp` — C352のstereo／quad output flag表示、`quadOutput`保存、custom clockの表示。未対応のhardware controlは設定項目にしない。
- `src/gui/sysMiscInfo.cpp` — C352の表示名・短縮名。
- `src/gui/debug.cpp` — C352のsystem／register dump表示。P002のChannelがprivateであるため、専用private castを追加せず、共通dispatch情報とregister sheetを使う。
- `src/gui/presets/sample.cpp` / `src/gui/presets/arcadeSystems.cpp` — C352単体presetと、必要ならC352を含むNamco preset。既存presetの順序・内容を変更しない。

### 文書・検証資産

- `doc/4-instrument/README.md` / `doc/4-instrument/c352.md` — instrument操作、sample map、macro範囲、保存形式。
- `doc/7-systems/README.md` / `doc/7-systems/c352.md` — 32 voices、clock／divider、4 output、ROM／loop制約、VGM非対応。
- `papers/format.md` / `papers/newIns.md` — `.fur` system ID `0xe8`、instrument type `68`、sample depth maskの記録。
- `plan/ws001-c352/tests/` — P003 registry／save-reload test、VS2019 GUI／headless build script、再現用synthetic sample fixture。P001 reference原本とP002 testは変更しない。

### 管理ファイル

- `plan/ws001-c352/phase003/phase.md`（着手後はExecution Logへの追記のみ）
- `plan/ws001-c352/ws.md`、`plan/queue.md`、`plan/ledger.md`、`plan/insights/index.md`（GNA状態同期）
- `plan/history/q003.md`（q003終了時の不変アーカイブ）

## 実行手順

1. **Preflight:** `master`がP002 `3a08cafe0`を含みcleanであること、`0xe8`／`68`の衝突がないこと、既存C140/C219のblobが不変であることを確認する。SDL submoduleとGUI依存を確認し、Ninjaは使用しない。
2. **Engine registration:** enumをMAX直前へ追加し、system definition、file-ID map、dispatch factory、instrument feature listsを接続する。VGMはversion 0のままにする。
3. **C352 asset path:** dedicated instrumentからP002 dispatcherのsample mapへ接続し、C352 sample depth／loop／ROM制約を既存sample lifecycleへ追加する。sample削除保護とJSON／legacy importの漏れを確認する。
4. **GUI:** system chooser、instrument chooser／macro editor、sample warnings、system configuration、debug/register view、presetsを追加する。`quadOutput`はflagsへ保存し、未対応のLINK／FM／VGMを操作項目として公開しない。
5. **Documentation:** system／instrument／format docsを実装と同じ数値・ID・制約で更新する。既存C140/C219の説明とfile IDを差し替えない。
6. **Verification:** P002のcore／dispatch gateを再実行し、registry test、save/reload test、headless Release build、GUI Release buildを順に実行する。失敗時はPhaseをunclearedとして知見を記録し、実装を直前のcheckpointへ戻す。

## 検証コマンドと受入条件

- `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1` — P002参照／core gateが再現可能でexit 0。
- `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p002.ps1` — VS2019 Developer PowerShell、MSVC v142 x64、headless Release、Ninja不使用でexit 0。
- `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p003.ps1` — GUI依存を含むVS 2019 `Visual Studio 16 2019`／MSBuild Releaseを構成し、registry／save-reload testを同じproduction objectsでリンク・実行する。SDLが未初期化なら`git submodule update --init extern/SDL`後に再実行する。
- Registry assertions: `systemFromFileFur(0xe8)==DIV_SYSTEM_C352`、definition channels/min/max=32、sample mask=8-bit+C219、VGM version=0、dispatch type=`DivPlatformC352`、possible instrument listにC352、既存C140/C219 IDs unchanged。
- Save/reload fixture: C352 system 1個、32 channels、flags `customClock=38102400`と`quadOutput=true`、C352 instrument with initSample／note map／volume／pan macros、8-bit sampleとC219 sample、forward／ping-pong loopを保存し、別engineへload後に全項目とsample bytes／depth／loopが一致する。
- GUI/static assertions: system chooserとinstrument chooserにC352が1回ずつ表示され、sample editorの警告文がP002の実制約と一致し、debugは共通register sheetを表示し、VGM exportはC352をunsupportedとして扱う。
- Regression: C140／C219の既存fixtureを保存・再読込し、system file IDs `0xce`／`0xcf`、channel count、sample behaviorが変わらない。P002 C352 testと既存headless buildも成功する。

## 非対象

- `src/engine/vgmOps.cpp`のC352 opcode／header追加、実機固有のLINK／FM／FILTER仕様の推測公開。
- P004の大規模既存曲回帰、P005の最終静的解析・品質判定。
- 既存C140／C219 dispatcher、core、sample decoderの改変。

## 計画具体化記録

### 2026-09-29 — P003 ready化

- P002成果物、現行enum／file-ID map、C140/C219のGUI・保存経路を照合し、登録・資産・UI・文書・検証のAllowed Touch Pointsを固定した。
- C352は既存enum値をシフトさせず末尾追加、file ID `0xe8`／instrument type `68`を実行時に再検査する。既存sample／macro PODを再利用し、P003で新しいバイナリPODを増やさない。
- GUIビルドはSDL submoduleを必要条件とし、ユーザー指定どおりDeveloper PowerShell for VS 2019／MSBuildを使用する。Ninjaは使用しない。
- User instruction「P003の計画具体化して」「作業を進めて」を計画作成の認可として適用。実装開始はq003の別認可後とする。

## Execution Log

未実行。q003は提案中で、コード変更・ビルド・pushは未実施。

### 2026-09-30 — q003開始

- Status: in-progress。ユーザー指示「q003を実行。」により認可。run/q003、checkpoint 2c0122584。


- 中間検証: verify.ps1 exit 0（参照330＋比較8＋実装330＋MAME差分6,365,685）、build-p002.ps1 -Jobs 4 exit 0（headless Release＋実dispatcher 4,250件）。GUI依存SDL／jsonを固定submodule commitで初期化し、build-p003.ps1実行中。
- GUI/static gate 74件通過。C352のquad切替は既存DivDispatchContainer::CHECK_MISSING_BUFSで不足bufferを補うため、追加の再初期化は不要。既存経路を統合試験で確認する。


- GUI初回ビルドの修正: 共通DivDispatch経由で参照不可のprotected debug flagsを公開output count表示に置換。headless設定由来のWITH_LOCALE=OFFでは既存insEditのngettextが未宣言となるため、GUI検証構成をWITH_LOCALE=ON／USE_MOMO=ONに修正。既存翻訳コードは変更しない。初回ログはtests/build/furnace-gui-vs2019-first-failure.log。


### 2026-10-02 — 中断から再開

- ユーザー指示「途中終了したコマンドから再開」。実行中のコンパイラは存在せず、Momo有効化後のGUIビルドが途中終了していることをログで確認。既存buildディレクトリを使ってbuild-p003.ps1を再実行する。
### 2026-10-02 — 検証完了

- Status: cleared。q003のP003受入条件を満たした。
- `verify.ps1`: exit 0。参照330、比較8、実装330、MAME差分6,365,685件。固定参照SHA-256一致（2026-09-30実行。以後core／参照資産は不変）。
- `build-p002.ps1 -Jobs 4`: exit 0。VS2019 MSBuild headless Release、実dispatcher 4,250 assertions（2026-09-30の最終ソースで再実行済み）。
- `build-p003.ps1 -Jobs 4`: exit 0（2026-10-02）。VS2019／MSVC v142 x64、GUI Release、GUI/static／frozen-source 74件、production objectsによるregistry／save-reload／asset／dispatch 898件成功。
- C352 ID 0xe8／instrument 68、32ch、sample mask、VGM非対応、compiled chooser、ImGui debug frame、全180 sample-map entries、macro／sample bytes／depth／loop、customClock／quadOutput、FUI埋込、JSON、sample削除保護、stereo→quadの共通buffer確保を検証。
- C140/C219 ID 0xce／0xcf、24／16ch、保存／再読込とROMロードも通過。既存C140/C219、decoder、C352 core、vgmOps、P002検証資産のGit blobは不変。
- 中断後の制限環境ではMSBuildが診断なしで終了したが、必要権限で再実行しビルド成功。統合試験の初回実行はWindows stack overflow（exit -1073741571）。試験内のDivSong／DivInstrumentをheapへ移して解消し、上記898件が成功した。
- 検証限界: GUIはReleaseビルド、compiled table／source assertions、ImGui debug frameで確認。手動GUI操作・大規模既存曲回帰・最終品質判定は未実施でP004／P005に残す。
- 成果コミット: このExecution Logを含む `feat(c352): integrate system, instruments and GUI`。確定hashはq003終了記録とledgerへ記録する。
