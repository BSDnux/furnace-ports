# Insights Ledger

| ID | Origin | Summary | Status | Next Trigger |
|---|---|---|---|---|
| ins001 | initial survey / ws001p001 | C352 core/system/instrumentは未実装。ただしvgmOps.cppにはゼロ値のhasC352/header placeholderが存在。p001 Execution Logに追加点を確定 | resolved | p003でID衝突を再検査して登録 |
| ins002 | initial survey / ws001p001 | C140/C219 dispatchからmacro/sample-mapパターンを再利用。C352は独立コア・dispatch、C219 μ-law全256値だけ一致を検証済み | resolved | p002で契約どおり分離実装 |
| ins003 | initial survey | 初期計画資産はGNASysに作成されたが、現在はFurnaceの`plan/`へ移動済み | resolved | Furnaceリポジトリ内のplanを正本として運用 |
| ins004 | ws001p001 / MAME | MAMEはC352を32ボイス、8-bit linear／μ-law、24-bit ROM、4出力として実装し、0x200 controlと0x202 KEYON／KEYOFF実行レジスタを持つ | resolved | ws001p002のコア設計へ反映 |
| ins005 | ws001p001 / MAME | MAME実装ではcontrol用途とLINKの意味が未確定。搭載ドライバのdividerにもverify注記がある | open | ws001p002〜p004で追加根拠・実機挙動を確認 |
| ins006 | ws001p001 / QuattroPlay | QuattroPlayはcontrol1／control2、KEYON時のflagsラッチ、C352／C140 μ-law切替、VGM opcode `0xe1`、`clk/288`レートを実装している | resolved | ws001p002の互換設計とp004の検証へ反映 |
| ins007 | ws001p001 / QuattroPlay | FILTER時の音量ramp抑制、control用途、phase flag順序は実装・コメント上も完全確定していない | open | ws001p002〜p004で追加根拠・実機挙動を確認 |
| ins008 | ws001p001 / reference execution | MAMEとQuattroPlayは終端sample消去、KEYON初期化、FILTER時ramp、flagsラッチ、control読出、出力スケールが異なる。MAME固定版を互換基準に採用 | resolved | p002/p004で参照ゲートと新コアを比較 |
| ins009 | ws001p001 / sample contract | MAME互換では非ループstart=endが無音。Furnace配置は音声末尾の後にguardを置きendをguard位置とする。64KiB bankを越える通常sampleは初期対応外 | resolved | p002でguard／容量／不正loop検証を追加 |
| ins010 | ws001p001 / integration | VGMはC352 header placeholderのみで実用対応なし。初期統合は非対応を明示。完全対応はe1以外にROM block・divider・clock・endian検証が必要 | open | VGM対応を別Phaseとして認可する場合 |
| ins011 | ws001p001 / F2 | dataC219とc219Tableは流用可。ただしsample.cppの汎用C219→16bit変換は負側を単純符号反転する経路があり、C352ハードdecoderの負側-32差をそのまま表す関数ではない | resolved | p002はdataC219 byte＋C352 decoderを使用、既存C219変換は変更しない |

## q001 evidence notes (2026-09-18)

- ins005/ins007は実機仕様の未確定性として残す。互換実装を進める上ではp001 Execution LogのMAMEプロファイルと明示的な非対応範囲を適用する。
- 参照版: MAME `92a3f13f9664e29d5c8ffba9634e714d623f6baf`、QuattroPlay `448f316945d50a5aec5d1ed9607e04b4a0a3ee9c`。SHA-256と再実行コマンドは `plan/ws001-c352/tests/`。
- 初回の通常権限ではGit refs書込・HTTPS取得が環境制限で失敗した。認可された同じ操作を昇格実行して成功。仕様上の障害ではなく、コードのロールバックは不要。
