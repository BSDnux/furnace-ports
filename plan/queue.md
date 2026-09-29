# Queue Book

- **Queue ID:** q002
- **Status:** running
- **Timebox:** 当初90分。2026-09-29の再実行指示でP002の残作業を再開し、完了まで継続する（先行指示「終わるまで続き」を適用）。
- **Human authorization:** 2026-09-18 JST「https://github.com/yosi2112/furnace ←今後ここにPushするように。つぎはP002です。」P002の実行指示として認可。
- **対象:** ws001p002
- **依存関係:** ws001p001 cleared (8fc686d32)
- **Allowed Touch Points:**
  - src/engine/platform/c352.cpp, c352.h
  - src/engine/platform/sound/c352.cpp, c352.h
  - CMakeLists.txt（C352ソースの追加）
  - plan/ws001-c352/tests/（実行・比較・ビルド検証資産。reference原本は不変）
  - plan/ws001-c352/phase002/phase.md（着手後はExecution Logのみ）
  - plan/queue.md, plan/history/q002.md, plan/ledger.md, plan/ws001-c352/ws.md, plan/insights/index.md（GNA管理・状態同期）
- **Administrative operations:** run/q002作成・検証後commit・masterへfast-forward・originの指定URLへ通常push。既存gitlinkのdependency初期化、tests/build/配下のビルド生成。
- **Item ws001p002:** cleared

## Lifecycle

- draft → proposed: p001確定契約から対象と検証を具体化。
- proposed → authorized: 上記ユーザー実行指示を適用。コード着手前にP書ready化。
- system／instrument／factory／GUI登録およびVGM公開はp003以降。p002の成功をもって次Queueへ進まない。

## Result

- 前回のfinished/cleared表記は統合検証・Git終了処理に先行していたため、2026-09-29にrunningへ訂正して再開した。過去アーカイブは作成されていなかったため同じq002を使用。
- 2026-09-29 JST「P002 を実行」を再開認可として適用。Allowed Touch Pointsは変更なし。
- ws001p002: **cleared**。VS2019でverify.ps1／build-p002.ps1がともにexit 0。参照330、比較8、実装330、MAME差分6,365,685、実dispatcher統合4,250 assertions成功。headless Release Furnace／統合テストのリンク成功。
- 既存C140/C219、sample.cpp、p001、q001のGit blob不変。P002着手前本文を保存。終了処理としてcommit、archive、masterへfast-forward、指定originへ通常pushを行う。
