# GNAシステム (Git-Native Autonomous System) — AGENTS.md

## コーディングエージェント向け Gitネイティブ自律開発・実行管理プロトコル

本リポジトリでは、コーディングエージェントが複数セッションにまたがる長期的なソフトウェア開発を、安全・自律的かつ再現可能に進めるため、**GNAシステム（Git-Native Autonomous System）**を採用する。

外部SaaS（GitHub Issues / Projects等）のAPIや複雑な外部同期に依存せず、**リポジトリ内のローカルファイルおよびGitプリミティブ（Branch / Worktree / Commit / Stash）のみを基盤として動作**する。

---

## 1. コア原則と哲学

1. **広く計画し、狭く実行する（Plan broadly. Execute narrowly.）**
   - 計画階層（M/W/P）はプロジェクトの意図と構造を表す。
   - 実行認可（Q）は、人間の合意のもとで現セッション内に実行を許可された唯一の境界である。計画されていることは、実行してよいことを意味しない。
2. **履歴と事実のイミュータビリティ（不変性）**
   - 実行が開始されたP書の手順・前提や、終了したQ書の内容を事後改変してはならない。失敗や想定外の事態は「改ざん」ではなく「知見の追記・新Phaseの発行」で表現する。
3. **Gitネイティブな実行隔離と安全な巻き戻し**
   - 実装作業はコミットチェックポイントまたは作業用Gitブランチ上で隔離して行う。
   - 完了条件を満たせなかった作業（`uncleared`）の壊れたコードを作業ツリーに残してはならない。**「コードは直前のクリーン状態に巻き戻し、得られた知見のみを記録する」**。
4. **トークン効率と高速セッション復元（Context Compaction）**
   - エージェントのコンテキストウィンドウを浪費させないため、高密度な状態台帳（`ledger.md`）を維持する。セッション開始時に全計画書を読み直すことを禁ずる。
5. **完了の客観的検証（No Verification, No Clearance）**
   - コードを変更したこと自体を進捗とみなさない。事前に定義された検証コマンドのパスをもってのみ完了とする。

---

## 2. ディレクトリ構造と識別子

計画資産はすべて `plan/` 配下に格納し、Gitでバージョン管理する。

```text
plan/
├── master.md                  # M書: プロジェクト戦略・全体ゴール・Workstream一覧
├── ledger.md                  # L書: 状態台帳・高速セッション復元インデックス
├── queue.md                   # Q書: 現サイクルで実行中または直近の実行マニフェスト
├── history/                   # 完了・終了したQ書の不変アーカイブ
│   ├── q001.md
│   ├── q002.md
│   └── ...
├── insights/                  # 実行から蒸留された知見・技術制約・未解決事項
│   └── index.md
│
├── ws001-auth/                # 各Workstreamディレクトリ（ID + 任意の説明接尾辞）
│   ├── ws.md                  # W書: Workstream成果定義・Phase構造
│   ├── tests/                 # WS内で再利用する再現コード・テスト資産（Git管理）
│   ├── temp/                  # 一時スクラッチ領域（.gitignore 対象）
│   │
│   ├── phase001/              # 各Phaseディレクトリ
│   │   └── phase.md           # P書: 作業計画・検証条件・実行ログ
│   ├── phase002/
│   │   └── phase.md
│   └── ...
└── ws002-data/
    └── ...
```

### 識別子の命名と固定規則
識別子は一度発行したら、整理や美観を目的としたリナンバー（番号振り直し）を固く禁ずる。
- **Milestone Goal**: `MG001`, `MG002`, ...
- **Workstream**: `ws001`, `ws002`, ...
- **Phase**: `phase001`, `phase002`, ...（参照略記: `ws001p001`, `ws002p001`）
- **Queue Cycle**: `q001`, `q002`, ...
- **Insight**: `ins001`, `ins002`, ...

---

## 3. 文書体系とライフサイクル状態

### 3.1 M書 — Master Book (`plan/master.md`)
プロジェクト全体の戦略的正本。
- **責務**: 「最終的にどこへ向かうのか（Destination）」
- **必須記載事項**:
  - プロジェクトスコープ／明示的なスコープ外
  - 最終ゴール
  - Milestone Goals（到達状態の定義）
  - 全Workstream一覧と優先度（Primary Milestoneへの紐付け）

### 3.2 W書 — Workstream Book (`plan/wsXXX/ws.md`)
独立した開発成果の構造化計画書。
- **責務**: 「どの開発成果を達成するのか（Outcome）」
- **Workstream状態（WS Status）**:
  - `planning`: スコープやPhase構成を設計中。
  - `planned`: 構成完了、未着手。
  - `in-progress`: 配下のPhaseが実行中、または成果条件が未達。
  - `completed`: W書自体の完了条件が客観的に検証され達成された。
- **品質完了Phaseの義務付け**:
  - コードを生成・改変する全Workstreamは、最終成果判定の直前に**「品質・規約適合検証Phase」**（フォーマット、リント、全体回帰テスト、不要コード除去）を含めなければならない。

### 3.3 P書 — Phase Book (`plan/wsXXX/phaseYYY/phase.md`)
エージェントが追加の設計判断なしに自律実行できる最小手順書。
- **責務**: 「その成果の一部をどう実行・検証するのか（Procedure & Verification）」
- **Phase状態（Phase Status）**:
  - `draft`: 起案・設計中。
  - `ready`: 前提条件と検証手段が確定し、Queue選定が可能な状態。
  - `in-progress`: Queueに組み入れられ、現在作業中。
  - `cleared`: P書の完了条件が検証により完全に満たされた。
  - `uncleared`: 実行されたが、完了条件を満たせず中断・終了した。
  - `superseded`: 前提の変化により、未実行のまま別Phaseへ置換または廃止された。
- **P書の不変性とExecution Log**:
  - 着手済みのP書本体（前提、スコープ、手順）を直接書き換えて履歴を消してはならない。
  - 実行結果は、P書末尾の `## Execution Log` セクションに追記する（実行コミット、検証結果、得られた事実）。

### 3.4 Q書 — Queue Book (`plan/queue.md`)
人間から与えられた作業時間枠（Timebox）内で、エージェントが実行を許可された唯一のマニフェスト。
- **責務**: 「今回、何を実行してよいのか（Authorization Boundary）」
- **Queue自体の状態（Queue Status）**:
  - `draft`: 起案中。
  - `proposed`: 人間へ提示中（実行認可待ち）。
  - `authorized`: 人間の合意と実行指示を獲得した。
  - `running`: 実行中。
  - `finished`: 認可された全項目の処理が完了（clearedまたはuncleared）した。
  - `aborted`: 人間の指示または致命的理由により途中で中断された。
- **Queue項目の状態（Item Status）**:
  - `pending`: 認可済み、未着手。
  - `in-progress`: 作業中。
  - `cleared`: P書の検証条件を満たして完了。
  - `uncleared`: 合理的に完了できず中断（正常な終了状態の1つ）。
- **必須記載事項**:
  - Queue ID（`q001` 等）
  - 指定タイムボックス（例: 30分、2時間）
  - 人間の承認証跡（発言の引用・日時）
  - 選択されたPhase一覧および依存関係順序
  - **変更許可スコープ（Allowed Touch Points）**: 変更を許可するファイルパス・ディレクトリの明記。

### 3.5 L書 — Ledger Book (`plan/ledger.md`)
セッション間でプロジェクトの現在地を瞬時に復元するための高速状態台帳。
- **責務**: 「現在どこにいて、次に何を着手すべきか（Compacted State）」
- 最新のマイルストーン、現在のフォーカスWS、直前の完了Queue ID、アクティブブランチ、未解決ブロッカー（Insights）を数千トークン以内で読めるサマリーとして常時更新する。

---

## 4. Gitネイティブ実行＆アイソレーション規約

エージェントはコードベースを直接破壊してはならない。以下の手順を機械的に順守する。

```text
               [人間によるQueue承認]
                         │
                         ▼
        [Git Checkpoint 作成: run/qXXX]
                         │
    ┌────────────────────┴────────────────────┐
    ▼                                         ▼
[Phase 実行 (in-progress)]             [作業時間超過 / 未知の壁]
    │                                         │
    ▼                                         │
[検証コマンド実行]                             │
    ├── 合格 (Criteria Met)                   │
    │      ▼                                  │
    │   [Git Commit (wsXXXpYYY)]              │
    │   [Phase: cleared]                      │
    │   [次の ready Phase へ]                 │
    │                                         ▼
    └── 不合格 ──────────────────────> [知見抽出 (plan/insights/)]
                                              │
                                              ▼
                                       [Git Rollback: git reset --hard]
                                              │
                                              ▼
                                       [Phase: uncleared]
                                              │
                                              ▼
                                       [後続の独立Phase または Queue終了]
```

### 4.1 実行チェックポイントの作成
Queueが `authorized` になったら、直ちに専用の作業ブランチを作成する。
```bash
git checkout -b run/q001
```

### 4.2 cleared 時のコミット規約
完了条件を検証コマンドで証明できた場合のみコミットする。コミットメッセージにはPhase IDを明記する。
```text
feat(auth): implement token parser

- Phase: ws001p002
- Status: cleared
- Verification: pytest plan/ws001/tests/test_parser.py (passed)
```

### 4.3 uncleared 時のクリーンロールバック規約（最重要）
Phaseが完了条件を満たせなかった場合、または作業時間枠を超過した場合：
1. **知見の退避**: 試行した内容、エラー出力、判明した仕様制約を、該当P書の `Execution Log` および `plan/insights/index.md` に記録する。
2. **コードの完全巻き戻し**: 途中の壊れたコードを作業ツリーに残してはならない。
   ```bash
   # 実験的コードを保持したい場合はstashに退避
   git stash push -m "uncleared: ws001p002 trial"
   # または完全に直前のクリーンコミットへロールバック
   git reset --hard HEAD
   ```
3. **作業ツリーの衛生確認**: 作業ツリーがクリーン（ビルド可能、既存テスト全通過）であることを確認してから、次のQueue項目に進むか終了する。

---

## 5. エージェント標準運用サイクル

エージェントは以下のループを厳格に巡回する。ステップをスキップしてはならない。

```text
[Step 1: Resume]
    └─ plan/ledger.md を読み、現在地・直前の結果・フォーカスを把握する。

[Step 2: Plan & Refine]
    └─ 人間の意図を確認し、M書/W書を整備。近い将来の作業をP書（draft -> ready）へ分解。

[Step 3: Queue Proposal]
    └─ タイムボックスと依存関係に基づき、ready なPhaseから queue.md (proposed) を起案。
    └─ 変更許可スコープ（Allowed Touch Points）を明示する。

[Step 4: Human Authorization Boundary (人間の認可)]
    └─ 人間にQueue案を提示し、明確な実行指示・合意を得る（指示がない限りコード変更禁止）。
    └─ queue.md を authorized -> running に遷移させる。

[Step 5: Git-Isolated Execution]
    └─ run/qXXX ブランチを作成。
    └─ Phaseごとに in-progress -> 実装 -> 検証。
         ├─ 成功: コミット -> cleared
         └─ 失敗: 知見記録 -> ロールバック -> uncleared

[Step 6: Close, Synchronize & Archive]
    └─ queue.md を finished に更新。
    └─ queue.md を plan/history/qXXX.md にアーカイブ保存（以後の改変禁止）。
    └─ 各P書・W書・plan/ledger.md を最新状態に同期。
    └─ 成功ブランチをメインブランチへマージ（squash または fast-forward）。
    └─ 人間へ結果を報告し、自律停止（勝手に次のQueueを開始しない）。
```

---

## 6. 知見台帳（Insights Ledger）

外部バグトラッカーを導入する代わりに、実行中に得られた事実・障害・未解決の技術制約は `plan/insights/` に即時集約する。

### `plan/insights/index.md` のフォーマット
```markdown
# Insights Ledger

| ID | Origin | Summary | Status | Next Trigger |
|---|---|---|---|---|
| ins001 | ws001p002 | 外部APIに非公開のレート制限が存在する | open | ws001p003の設計時にリトライ機構を追加 |
| ins002 | ws002p001 | 古いテストDB fixtureがPython 3.12で非互換 | resolved | ws002p002にて更新済み |
```

- 未再現のバグや中断理由を闇雲に再試行せず、Insightとして記録して人間と共有する。
- 再現スクリプトや調査用ツールは `plan/wsXXX/tests/` にGit管理下で保存し、将来の再検証資産とする。

---

## 7. エージェントの行動制約・不変条件（Invariants）

エージェントはいかなる理由があっても以下の制約を破ってはならない。

1. **認可なきコード変更の禁止**
   - 「簡単に直せそうだから」「関連する箇所だから」という理由で、`queue.md` の `Allowed Touch Points` に含まれていないファイルを変更してはならない。発見した問題は `insights/` または計画層（M/W/P）へ差し戻す。
2. **検証なき cleared の禁止**
   - コードを変更しただけでテストや検証コマンドを実行していない場合、`cleared` を名乗ってはならない。客観的に検証できない作業はすべて `uncleared` として扱う。
3. **作業ツリー汚染の禁止**
   - セッション終了時、作業ツリーは常に「ビルド可能かつ全テストが通過するクリーンな状態」でなければならない。未完了コードをコミットしてメインの履歴を汚染してはならない。
4. **自律的なQueue継続の禁止**
   - 1つのQueueが `finished` に達したら、エージェントは必ず処理を停止し、人間に報告しなければならない。人間の新たな指示・タイムボックス合意なしに、自動的に次のQueueを開始してはならない。
5. **履歴改変の禁止**
   - 過去のQ書アーカイブ（`plan/history/`）や、実行済みP書の元の意図を上書きしてはならない。計画書は予言ではなく、その時点での理解の記録である。

