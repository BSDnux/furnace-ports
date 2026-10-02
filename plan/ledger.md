# Ledger

- **Last updated:** 2026-10-02
- **Latest milestone:** MG001／MG002／MG003達成。C352コア、dispatcher、システム／楽器／GUI／文書を統合・検証済み。最終品質判定MG004は未達。
- **Focus WS:** `ws001-c352`、in-progress（P004／P005未実行）
- **Last completed Queue:** `q003` finished、`ws001p003` cleared。archive: `plan/history/q003.md`。
- **Active branch:** `run/q003`（ローカル成果・終了記録を保持。master統合／pushは自動承認審査で拒否され、未実行）
- **Result commit:** P003 `45977c5fb`、開始checkpoint `2c0122584`。P002 `30fb4baff`、P001 `8fc686d32`。
- **Verification:** VS2019 Developer PowerShell／MSVC v142 x64／MSBuild。verify.ps1 exit 0（参照330＋比較8＋実装330＋MAME差分6,365,685、参照SHA-256一致）。build-p002.ps1 exit 0（headless Release、実dispatcher 4,250件）。build-p003.ps1 exit 0（GUI Release、統合898件、GUI/static／frozen-source 74件）。P003再現手順はtests/README-p003.md。
- **Current facts:** C352 system ID `0xe8`／instrument type `68`、32ch、8-bit+C219 mask、VGM version 0。専用楽器とAMIGA fallback、sample map／macros、sample削除保護、JSON、FUI sample埋込、.fur別engine再読込、clock／quad flags、stereo→quad切替を検証済み。
- **Verification limits:** GUIはReleaseビルド・compiled chooser・静的検証・ImGui debug frame。手動GUI操作、大規模既存曲回帰、最終品質検証はP004／P005に残す。
- **Next action:** 自動承認審査がmaster統合と外部pushを拒否。検証済みC352実装・文書・q003記録をmasterへ統合し、https://github.com/yosi2112/furnace.git のmasterへ通常pushすることについて、ユーザーの明示承認待ち。P004／次Queueは開始しない。
- **Open insights:** ins005／ins007（実機control、LINK、divider、FILTER等）、ins010（将来VGM完全対応）。ins014はGUIビルドでMomo localeを有効にして解決。
- **Contract:** P001 Execution Logの確定契約、P003 Execution Log末尾のclearedが現在状態。P書本文のreadyは着手前の不変記録。
- **Invariants:** q001〜q003 archiveと実行済みP書本文は不変。C140/C219／decoder／C352 core／P002参照・検証資産はblob不変。
- **User constraints:** push先はyosi2112/furnace。Developer PowerShell for VS 2019／MSVC、Ninja不使用。サブエージェント禁止。
- **Execution history:** 2026-09-30開始、GUIビルド途中で中断。2026-10-02「途中終了したコマンドから再開」で続行し実装・検証完了。統合／pushは上記の理由で保留。中断待機期間を除き4時間枠内。
