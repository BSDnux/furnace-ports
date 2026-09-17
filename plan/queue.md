# Queue Book

- **Queue ID:** `q001`
- **Status:** running
- **Timebox:** 90分
- **Human authorization:** 2026-09-18 JST、ユーザー指示「ws001p001を実行」。既提案の90分枠・対象Phaseの実行認可。
- **対象:** `ws001p001`
- **依存関係:** なし
- **Allowed Touch Points:**
  - `plan/ws001-c352/phase001/phase.md`（Execution Logのみ、着手後）
  - `plan/insights/index.md`（新規事実が判明した場合）
  - `plan/ledger.md`（Queue終了時の同期）
  - `plan/ws001-c352/tests/`（再現資産が必要な場合のみ）
  - `plan/queue.md`、`plan/history/q001.md`、`plan/ws001-c352/ws.md`（GNA Step 6の管理・終了同期のみ）

## 選定Phase

### `ws001p001` — C352仕様・実装契約の確定

一次資料および信頼できる既存エミュレータ実装を調査し、レジスタ、サンプル形式、アドレス／バンク、音量・パン、ループ、周波数、4系統出力、リセット・キー操作の仕様を確定する。C140/C219との差分と、Furnace APIへ落とす項目を記録し、後続Phaseの検証ベクトルを作る。

## 認可後の実行境界

人間の明示的な実行指示を受けた場合のみ、`run/q001` ブランチを作成してPhaseを開始する。MAME／QuattroPlayの事前調査結果は既にp001本文へ反映済みだが、Queue認可前のためp001は未実行である。認可がない間は、Furnaceコードおよび計画書の手順本文を変更しない。


## Lifecycle
- 2026-09-18: proposed → authorized。GNA Step 6に必要なqueue.md、history/q001.md、ws.mdの状態同期を管理操作として含む。

- 2026-09-18: authorized → running。Item ws001p001: in-progress。
