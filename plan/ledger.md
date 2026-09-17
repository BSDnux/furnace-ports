# Ledger

- **Last updated:** 2026-09-18
- **Latest milestone:** MG001達成（MAME互換仕様・Furnace接続契約・検証ベクトル確定。実機未確認事項は分離管理）
- **Focus WS:** `ws001-c352`、in-progress
- **Last completed Queue:** `q001` finished、`ws001p001` cleared
- **Active branch:** `master`（run/q001をfast-forward統合。run/q001は履歴として保持）
- **Result commit:** `8fc686d32`（Phase成果）。開始チェックポイント: `a5bd354d7`
- **Verification:** `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1` exit 0。MAME 330件＋QuattroPlay 8件、固定参照8ファイルのSHA-256、GCC 8.3.0 -Werror。
- **Contract:** `plan/ws001-c352/phase001/phase.md` Execution Logの「確定契約」。P書先頭のreadyは不変の開始前記録、現在状態はExecution Log末尾のcleared。
- **Current facts:** 新コアはMAME固定版互換、C219 sample bytes/table再利用、16 MiB byte ROM、非ループguard、32voice、4系統コア出力＋既定stereo平均。Furnace本体は未実装・未変更。VGM初期非対応を明示。
- **Next action:** ws001p002のdraftを本契約に基づきready化し、新Queueの対象・変更範囲・検証環境を提示する。p002〜p005は未着手、次Queueの実行認可なし。
- **Open insights:** ins005/ins007（実機control、LINK、divider、FILTER等の未確定性）、ins010（将来VGM完全対応）。互換コア実装を止める未決定事項はなし。
- **Verification limits:** 本Phaseの参照試験は成功。Furnace全体ビルド／UI／保存／既存曲回帰は未実行、後続Phaseで必要。
- **Invariants:** 完了Q書 `plan/history/q001.md` は不変。認可なき次Queue開始・コード変更禁止。
