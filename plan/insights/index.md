# Insights Ledger

| ID | Origin | Summary | Status | Next Trigger |
|---|---|---|---|---|
| ins001 | initial survey | Furnace masterにはC352の既存実装・文書・識別子が見当たらない | open | ws001p001で実装の新規追加範囲を確定 |
| ins002 | initial survey | C140/C219は `c140.cpp` と共有サウンド実装 `c140_c219.c` を基盤にしている | open | ws001p002で再利用可否と分離境界を判断 |
| ins003 | initial survey | 初期計画資産はGNASysに作成されたが、現在はFurnaceの`plan/`へ移動済み | resolved | Furnaceリポジトリ内のplanを正本として運用 |
| ins004 | ws001p001 / MAME | MAMEはC352を32ボイス、8-bit linear／μ-law、24-bit ROM、4出力として実装し、0x200 controlと0x202 KEYON／KEYOFF実行レジスタを持つ | resolved | ws001p002のコア設計へ反映 |
| ins005 | ws001p001 / MAME | MAME実装ではcontrol用途とLINKの意味が未確定。搭載ドライバのdividerにもverify注記がある | open | ws001p002〜p004で追加根拠・実機挙動を確認 |
| ins006 | ws001p001 / QuattroPlay | QuattroPlayはcontrol1／control2、KEYON時のflagsラッチ、C352／C140 μ-law切替、VGM opcode `0xe1`、`clk/288`レートを実装している | resolved | ws001p002の互換設計とp004の検証へ反映 |
| ins007 | ws001p001 / QuattroPlay | FILTER時の音量ramp抑制、control用途、phase flag順序は実装・コメント上も完全確定していない | open | ws001p002〜p004で追加根拠・実機挙動を確認 |
