# C352対応計画（Master Book）

- **Document:** M書
- **Status:** planning
- **対象リポジトリ:** `E:\programing work\furnace`
- **計画資産:** `E:\programing work\furnace\plan`

## プロジェクトスコープ

Furnace TrackerへNamco C352 PCM音源を追加し、作曲・再生・サンプル管理・デバッグ・ドキュメント・回帰検証までを一貫して利用可能にする。C352は32ボイス、8-bit linear / μ-law PCM、4系統出力を持つ音源として知られているが、実装契約はPhase 001で一次資料および既存エミュレータ実装を照合して確定する。

参照する既存実装は、同じPCM系統の `src/engine/platform/c140.cpp`、`src/engine/platform/sound/c140_c219.c`、およびC140/C219のシステム・インストゥルメント文書とする。外部仕様の実装基準としてMAMEの [`c352.cpp`](https://github.com/mamedev/mame/blob/master/src/devices/sound/c352.cpp)／[`c352.h`](https://github.com/mamedev/mame/blob/master/src/devices/sound/c352.h)、互換実装・運用基準としてQuattroPlayの [`c352.c`](https://github.com/superctr/QuattroPlay/blob/master/src/emu/c352.c)／[`c352.h`](https://github.com/superctr/QuattroPlay/blob/master/src/emu/c352.h) を使用する。

## 明示的なスコープ外

- C352以外の音源の挙動変更
- 外部SaaSへのIssue・PR・同期処理
- 未検証の実機固有仕様を推測で実装すること
- Queue認可前のFurnaceコード変更

## 最終ゴール

C352を独立したFurnaceシステムとして選択でき、サンプルを割り当てた32チャンネルで、音量・パン・ピッチ・ループ・キーオン／オフ・μ-law形式を仕様どおり再生できる。UI、保存／読込、ダンプ／デバッグ、ビルドおよび既存回帰テストが成立し、C352固有の制約が文書化されている。

## Milestone Goals

| ID | 到達状態 | 主Workstream |
|---|---|---|
| MG001 | C352の仕様・Furnace内データ契約・検証ベクトルが確定 | ws001 |
| MG002 | C352のエミュレーション／ディスパッチが実装され単体検証済み | ws001 |
| MG003 | システム登録、UI、サンプル／インストゥルメント、文書が統合済み | ws001 |
| MG004 | 品質・規約適合検証を通過し、成果を引き渡し可能 | ws001 |

## Workstream一覧

| ID | 内容 | 優先度 | 対応Milestone | 状態 |
|---|---|---:|---|---|
| ws001 | Namco C352 Furnace対応 | Primary | MG001–MG004 | planned |
