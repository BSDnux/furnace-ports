# Ledger

- **Last updated:** 2026-09-17
- **Latest milestone:** MG001（MAME／QuattroPlay調査反映済み、正式cleared前）
- **Focus WS:** `ws001-c352`
- **Last completed Queue:** なし
- **Active branch:** なし（Queue認可待ち）
- **Next action:** `q001`の人間による実行認可を得て、`run/q001`上で`ws001p001`の仕様表・検証ベクトルを正式確定する
- **Current facts:** MAMEの`c352.cpp`／`c352.h`とQuattroPlayの`src/emu/c352.c`／`c352.h`を事前調査し、p001本文へ反映済み。C352本体コード、専用テスト、ビルド成果物は未作成。
- **Open blockers:** `control1`／`control2`、LINK、FILTERと音量ramp、phase flag順序、クロックdivider、4出力のFurnace内ダウンミックスは追加検証が必要
- **Invariants:** 認可前のコード変更禁止。実行済みP書と完了Q書は改変禁止。検証なしのcleared禁止。
