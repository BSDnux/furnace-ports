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

