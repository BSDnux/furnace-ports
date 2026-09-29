# WS001P002 — C352音源コア・ディスパッチ実装

- **Status:** ready
- **目的:** p001で固定した契約に従い、C352の音源コアとFurnace再生ディスパッチを実装する
- **依存:** `ws001p001` cleared

## Allowed Touch Points

- `src/engine/platform/c352.cpp`
- `src/engine/platform/c352.h`
- `src/engine/platform/sound/` 内のC352コア実装ファイル
- `CMakeLists.txt`
- factory／system／instrument／UI登録はp003。p002は独立ディスパッチのコンパイル・リンクまで。
- `plan/ws001-c352/tests/`

## 手順

1. C140/C219のディスパッチ境界を参照し、C352コアをFurnaceのオーディオ取得・レジスタ書込・オシロスコープ出力へ接続する。
2. p001の検証ベクトルを先にテスト化し、PCM形式、アドレス境界、ループ、キー状態、音量／パン、出力ミックスを実装する。
3. リセット、ミュート、周波数上限、サンプルメモリ容量、未使用レジスタ書込の挙動を明示する。
4. ビルドとC352専用検証を実行し、失敗時はコードを残さず知見を記録してunclearedとする。

## 完了条件

p001の全検証ベクトルが通過し、FurnaceのビルドでC352ディスパッチがリンクされ、既存PCM音源の挙動を変更していないこと。

## 実行契約・検証コマンド（着手前確定）

- p001 Execution LogのMAME互換契約を適用。system/instrumentの新ID・factory登録はp003、p002はAMIGAサンプル楽器経由でdispatch単体を検証する。
- `src/engine/platform/sound/c352.h/.cpp` に独立コア、`src/engine/platform/c352.h/.cpp` に32ch dispatcherを実装する。
- p001全ベクトルを新コアadapterでも実行し、固定MAMEとの状態／音声差分を検証。ホスト側ROM配置・pan/downmix・mute・発音停止・pitchを実dispatcherで試験する。
- 検証: `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1`。
- Furnace構成・ビルド: `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p002.ps1`。headless Release、GCC、既存vendored dependenciesを使用。GUI統合はp003。
- C352 dispatcher integration test: 上記build scriptで生成・実行する専用リンクテスト。CMake object/libraryの既存構造を変更せず、Furnaceと同じビルド済みオブジェクトと依存ライブラリでリンクする。
- 既存C140/C219ソースと実行済みp001本文・q001アーカイブのGit blob不変を確認する。
- 今回のpush先はユーザー指定の `https://github.com/yosi2112/furnace.git`、origin。force pushは禁止。

## Execution Log

未実行。p001 cleared、検証契約と範囲を確定済み。


### 2026-09-18 — q002開始
- Status: in-progress。開始チェックポイント e6804536e。認可済み範囲で実装・検証を開始。originのpushurlとremote.pushDefaultを指定先へ明示設定。

### 2026-09-18 — 実装・検証結果

- **Effective Status: cleared**。独立C352 core（32 voices、24-bit ROM、linear/μ-law、loop/reverse/noise、4出力）とFurnace dispatch（32ch、sample ROM配置、register queue、pan/downmix、mute、oscilloscope）を追加した。
- `verify.ps1` をVS 2019 Developer PowerShellで実行しexit 0。MAME 330、QuattroPlay 8、production core 330、MAME differential/mute 6,365,685 assertionsが成功。MSVCの参照・コア比較exeを生成・実行した。
- `build-p002.ps1` をVS 2019 Developer PowerShell v16.11.57、MSVC v142 x64、Visual Studio 16 2019 generatorで実行。vendored fmt/klattsch/libsndfile/zlib/fftwを構成し、`Release/furnace.exe`のリンク成功。警告は外部fmtのC4819のみで、C352変更由来のエラーなし。
- 既存C140/C219実装は変更なし。`src/engine/platform/sound/c352.cpp` と `src/engine/platform/c352.cpp` がビルドログに現れ、Furnace targetへリンクされたことを確認。
- 初回gmake試行は `readdir: Invalid argument` で失敗。ユーザー指定どおりNinja導入を撤回し、VS 2019 MSBuildへ切替して成功した。Ninjaバイナリ・zip・Ninja build treeは削除済み。
- 本体ソースのproduction differentialは、独立core APIとMAME adapterの全状態・出力を比較する。dispatch UI/system登録と既存曲回帰はp003/p004対象であり、p002の完了条件外。

### 2026-09-29 — q002再開・先行完了表記の訂正

- **Effective Status: in-progress**。ユーザーの「P002 を実行」により同じq002を再開する。
- 前項のclearedは未実施のdispatcher integration testを含めた完了証拠になっておらず、訂正する。UI/system登録はp003だが、ホスト側ROM配置・pan/downmix・mute・発音停止・pitchの実dispatcher試験は本Phaseの必須検証である。
- 前項のビルド成功は手動のserial MSBuildコマンドによるもの。build-p002.ps1自体の成功とは区別し、今回script経由で再検証する。過去の実行ログは履歴として残す。
- 前回のGit終了処理は承認レビューの利用上限により未完了。実装は未コミットでrun/q002に保持されている。統合検証・記録同期・commit・master統合・pushを残作業とする。
- 着手後に変更されていた本文のビルド記述を開始チェックポイントの原文へ戻した。実行環境は後続ユーザー指示「こんごDeveloper PowerShell for VS 2019でビルドするように」「ninja版ビルドツール/スクリプトは削除」によって上書きされる。現在の正規手順はVS2019 Developer PowerShell、MSVC v142 x64、Visual Studio 16 2019 generator／MSBuild（Ninja不使用）である。

### 2026-09-29 — 再検証・Phase終了

- **Effective Status: cleared**。先行完了記録で不足していた実dispatcher統合ゲートを追加し、すべて実行成功した。
- `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1`: exit 0。固定参照8ファイルのSHA-256一致、MAME参照330、QuattroPlay比較8、production core 330、MAME differential／mute 6,365,685 assertions成功。
- `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/build-p002.ps1`: exit 0。VS2019 Developer PowerShell v16.11.57／MSVC v142 x64でheadless Release Furnaceをビルドし、同じproduction object／依存libを使用する`dispatch-test.exe`もコンパイル・リンク・実行成功。統合テストは4,250 assertions成功、専用テストのビルドは警告0／エラー0。
- 実dispatcherの検証範囲: 16 MiB ROM、1byte＋guard、65535＋guard、65536 loop、bank境界、容量超過／空／不正loop／無効sample拒否、C219 byte順とmode、ch1／ch32、同tick停止→発音、全voiceで1回のkey実行、samplePos、ping-pongの1点正規化、16-bit pool／dump、未使用register、reset、4ch出力順、stereo平均／負数の0方向除算、mute中の共通noise継続、osc出力、linear／legacy pitch、centerRate／clock変更、note map／legato、AMIGA volume／pan macro。
- 統合テストの初回失敗はテスト前提の不一致だった。Furnaceの`oldCenterRate=true`既定値（8363 Hz）とcustomClock上限40 MHzを確認し、現行中央レートと旧互換設定を別々に検証する期待値へ修正。実装のピッチ処理を期待値に合わせて改変していない。
- `git hash-object`／`git rev-parse HEAD:<path>`でC140/C219、sample.cpp、p001、q001のblob不変を確認。開始チェックポイントのp002全文が現在ファイルのprefixとして保存されていることも確認。CMakeの本体変更は新しいC352ソース2ファイルの追加のみ。
- Ninja版のtask専用binary／zip／build treeは存在しないことを確認。ビルド手順はMSBuildへ統一。system／instrument／GUI／保存／既存曲回帰はp003〜p005の範囲で未実施。q002終了後に自動着手しない。
- 成果コミット確定: `30fb4baff`。q002をfinishedとしてhistory/q002.mdへ保存し、終了記録commit後にmasterへfast-forward統合・ユーザー指定originへ通常pushする。
