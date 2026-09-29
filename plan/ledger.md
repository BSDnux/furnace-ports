# Ledger

- **Last updated:** 2026-09-29
- **Latest milestone:** MG001達成。MG002に向けp002のコア／dispatcher実装・検証を完了。system／UI統合はp003。
- **Focus WS:** `ws001-c352`、in-progress
- **Last completed Queue:** `q002` finished、`ws001p002` cleared（archive: plan/history/q002.md）
- **Active branch:** `master`（終了記録commit後にrun/q002をfast-forward統合する。run/q002は履歴として保持）
- **Result commit:** p002 `30fb4baff`、開始チェックポイント `e6804536e`。p001は`8fc686d32`。
- **Verification:** VS2019 Developer PowerShell v16.11.57／MSVC v142 x64。`verify.ps1` exit 0（参照330＋比較8＋実装330＋MAME差分6,365,685、参照SHA-256一致）。`build-p002.ps1` exit 0（headless Release Furnace、実dispatcher統合4,250 assertions）。両scriptは`plan/ws001-c352/tests/`。
- **Contract:** `plan/ws001-c352/phase001/phase.md` Execution Logの「確定契約」。P書先頭のreadyは不変の開始前記録、現在状態はExecution Log末尾のcleared。
- **Current facts:** P002 cleared。32voice独立コア／dispatcher、C219 byte再利用、16 MiB ROM、4系統＋既定stereo、pitch／macro／muteまで実dispatcherで検証済み。先行した完了記録はExecution Logで訂正し、実検証結果を追記。
- **Next action:** q003はproposed。P003計画をready化済み、実装認可待ち。認可後はregistry／asset／GUI／save-reloadをVS2019 MSBuildで検証する。p004／p005は未着手。
- **Open insights:** ins005/ins007（実機control、LINK、divider、FILTER等の未確定性）、ins010（将来VGM完全対応）。互換コア実装を止める未決定事項はなし。
- **Verification limits:** headless全体ビルドとcore／dispatch試験は成功。UI／保存／既存曲回帰は未実施。既存C140/C219コードはblob不変。
- **Invariants:** 完了Q書 `plan/history/q001.md`、`plan/history/q002.md` は不変。認可なき次Queue開始・コード変更禁止。
- **User constraints:** push先はyosi2112/furnace。ビルドはDeveloper PowerShell for VS 2019／MSVC、Ninja版は使用しない。2026-09-29以降サブエージェント使用禁止。
- **Planning:** `plan/ws001-c352/phase003/phase.md`にP003の固定ID、Allowed Touch Points、実行順、受入条件を記録。`plan/queue.md`はq003 proposedで停止。
