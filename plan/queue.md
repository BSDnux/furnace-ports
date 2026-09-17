# Queue Book

- **Queue ID:** q002
- **Status:** authorized
- **Timebox:** 90分（前回と同じ枠を実行上限として採用）
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
- **Item ws001p002:** pending

## Lifecycle

- draft → proposed: p001確定契約から対象と検証を具体化。
- proposed → authorized: 上記ユーザー実行指示を適用。コード着手前にP書ready化。
- system／instrument／factory／GUI登録およびVGM公開はp003以降。p002の成功をもって次Queueへ進まない。
