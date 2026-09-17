# WS001P001 — C352仕様・実装契約の確定

- **Status:** ready
- **目的:** 後続実装が追加設計なしで進められるC352仕様、Furnace内契約、検証ベクトルを確定する
- **前提:** Queue `q001` が人間により認可されていること
- **成果物:** 本P書のExecution Log、必要に応じた `plan/ws001-c352/tests/` の小規模検証資産、insight追記

## Allowed Touch Points

- `plan/ws001-c352/phase001/phase.md`（Execution Logのみ）
- `plan/insights/index.md`
- `plan/ws001-c352/tests/`

## 手順

1. C352の一次資料、MAME等の信頼できる実装、既存のC140/C219実装を照合する。
2. チャンネル数、レジスタマップ、レジスタ書込順序、クロック／サンプルレート、8-bit linear・μ-lawの符号化、開始／終了／ループ、周波数、音量、パン、キーオン／オフ、4出力のミックスを表にする。
3. Furnaceの既存PCMディスパッチ、`DivSystem`、`DivInstrumentType`、サンプル深度、GUI、VGM／デバッグ登録箇所を確認し、C352の追加点と再利用点を列挙する。
4. 仕様ごとに、無音・単一サンプル・境界値・ループ・キーオフ・パン・μ-law・同時32ボイスの検証ベクトルを定義する。
5. 不確定事項は推測で埋めず、`insights/index.md`へ根拠と判断待ち状態を記録する。

## MAME調査結果（2026-09-17）

調査対象はMAME masterの [`src/devices/sound/c352.cpp`](https://github.com/mamedev/mame/blob/master/src/devices/sound/c352.cpp)、[`src/devices/sound/c352.h`](https://github.com/mamedev/mame/blob/master/src/devices/sound/c352.h)、およびC352を搭載する [`src/mame/namco/namcos11.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/namco/namcos11.cpp)／[`namcond1.cpp`](https://github.com/mamedev/mame/blob/master/src/mame/namco/namcond1.cpp) である。追加でQuattroPlayの [`src/emu/c352.c`](https://github.com/superctr/QuattroPlay/blob/master/src/emu/c352.c)／[`c352.h`](https://github.com/superctr/QuattroPlay/blob/master/src/emu/c352.h) を確認した。

### 確認できた実装仕様

| 項目 | MAMEで確認した内容 | C352実装への要求 |
|---|---|---|
| ボイス | 32ボイス | Furnaceのチャンネル数を32にする |
| サンプル | 8-bit signed linear PCMまたは8-bit μ-law | サンプル形式を明示的に変換し、μ-lawテーブルを一致させる |
| サンプルアドレス | `wave_bank`、`wave_start`、`wave_end`、`wave_loop`は各16-bit。再生位置は`bank << 16 \\| start`で24-bit ROM空間を参照 | 24-bit相当のサンプルメモリと境界検査を実装する |
| ボイスレジスタ | 1ボイスあたり8個の16-bitワード、0x00〜0xffに32ボイスを8ワード間隔で配置 | `0x00 + voice*0x10`形式へ機械的に変換せず、バイト／ワードアドレス単位を確定する |
| レジスタ項目 | `vol_f`, `vol_r`, `freq`, `flags`, `wave_bank`, `wave_start`, `wave_end`, `wave_loop` | Register SheetとFurnace内部状態を対応付ける |
| 制御 | `0x200`がcontrol、`0x202`書込で全32ボイスのKEYON／KEYOFFを実行 | キー操作を個別書込とコミット操作に分離する |
| フラグ | BUSY `0x8000`、KEYON `0x4000`、KEYOFF `0x2000`、LOOPTRG `0x1000`、LOOPHIST `0x0800`、FM `0x0400`、PHASERL `0x0200`、PHASEFL `0x0100`、PHASEFR `0x0080`、LDIR `0x0040`、LINK `0x0020`、NOISE `0x0010`、MULAW `0x0008`、FILTER `0x0004`、REVLOOP `0x0003`、LOOP `0x0002`、REVERSE `0x0001` | 実装する機能と未対応フラグを明示し、未対応ビットを黙って無視しない |
| 再生位置 | 16-bit fractional counterを`freq`で加算し、carryで次サンプルを取得 | 周波数計算とサンプル取得タイミングを専用テスト化する |
| 補間 | MAMEは前回サンプルと現サンプルをcounterで線形補間。FILTERフラグ時は補間を抑制する実装 | 補間の有無をC352固有機能として扱う |
| ループ | forward loop、reverse再生、reverse loop、LINK付き長形式を処理。非ループ終端ではKEYOFFかつBUSY解除 | 通常ループ・逆ループ・終端・LINKを別々に検証する |
| 音量／出力 | `vol_f`と`vol_r`の上位／下位byteを4出力へ割り当て、目標値へ1ずつrampする。出力はfront L/R、rear L/R | 4出力を保持し、通常のステレオ出力へのダウンミックス方針を決める |
| 位相 | front-left、rear-left、front-rightの符号反転フラグを処理 | 4出力と位相反転を検証対象にする |
| ノイズ | NOISE時はサンプルROMを読まず、16-bit状態をLFSR風に更新 | ノイズ生成器を任意機能ではなくC352固有機能として扱う |
| μ-law | MAME起動時に256エントリの変換表を生成。前半128値を段階幅1/2/4/8/16で構成し、後半はビット反転 | 変換表を再実装し、既知入力の全256値を比較する |
| サンプルレート | MAMEコメントは`input clock / (288 * 2)`。実装コードは`clock / divider`で生成し、搭載ドライバはdivider 288を指定 | クロックと実効レートの扱いは実機／既存資料で追加検証する |
| リセット | ボイス状態をゼロ化、ノイズ状態`0x1234`、control `0` | Furnace resetと再生開始時の初期状態を一致させる |

## QuattroPlay調査結果（2026-09-17）

QuattroPlayはMAME由来のC352コアを独立実装へ移植した資料であり、MAMEだけでは曖昧なホスト接続・運用上の挙動を補足する。ただし、同リポジトリのREADMEはC140/C219/C30をC352で代用してVGMログを作る旨を明記しているため、C352純正仕様と互換・代用モードを混同しない。

| 項目 | QuattroPlayで確認した内容 | 計画への反映 |
|---|---|---|
| API | `C352_init(clk)`が`clk / 288`を再生レートとして返し、`C352_update()`をそのレートで呼ぶ | p002でFurnaceのtick／サンプルレート境界を確定する |
| レジスタ | ボイスレジスタは`vol_f`、`vol_r`、`freq`、`flags`、`wave_bank`、`wave_start`、`wave_end`、`wave_loop`。`0x200`と`0x201`をcontrol1／control2として保持し、`0x202`でKEYON／KEYOFFを実行 | MAMEで不明だったcontrolが2ワード存在することを記録。用途自体は未確定のまま扱う |
| 書込・読出 | `addr < 0x100`は`addr / 8`のボイスへ書込。`addr >= 0x100`の読出は0 | ワードアドレスのエンディアン、アラインメント、読出仕様を専用テストにする |
| KEYONラッチ | KEYON時にflagsを`latch_flags`へ保存し、音量・位相・補間の更新はラッチ値を参照。再生位置はbankとstartから初期化し、counterは`0xffff` | Furnace側でも再生中のフラグ書換と発音時設定を分離する |
| 音量ramp | FILTERがラッチされている場合は目標音量を即時反映し、それ以外はクリック抑制のため1ずつ追従する（コードコメントは挙動に疑問を残す） | FILTERと音量rampの組合せを個別ベクトル化し、仮定として固定しない |
| サンプルROM | `wave_mask`でROMアクセスをマスクし、現在位置を24-bit値として扱う | Furnaceサンプルメモリの容量・マスク方針をp001の境界仕様に従わせる |
| μ-law | C352／C219互換テーブルに加え、C140用μ-lawテーブルを切替可能 | C352標準はC352テーブル、C140互換は別モードとして設計から除外または明示する |
| 出力 | 4出力を保持し、QuattroPlayはミュートマスクとrear出力抑制を持つ | Furnaceのミュートはコア外のチャンネルミュートとし、rear抑制を独自仕様にしないか明記する |
| VGM | C352書込をVGM opcode `0xe1`で記録 | Furnace VGM出力がC352を表現できるか、対応不可ならスコープ外として文書化する |
| 実運用レート | READMEでは4ch WAVのレートを`85333`または`88200`と記載 | divider 288の結果と入力クロックを組み合わせて検証する |

### QuattroPlayから追加する検証ベクトル

- `0x200`と`0x201`を個別に書き、`0x202`で実行してもcontrol値が音声へ未確定の影響を与えないことを観測する。
- KEYON後にflagsを書き換えた場合、`latch_flags`由来の出力位相・補間・ミュート条件がどこまで維持されるかを確認する。
- FILTER有／無それぞれで、音量目標への追従速度と補間の有無を比較する。
- 24-bit位置の上位bank境界、`wave_mask`相当の範囲外アクセス、サンプル終端後のKEYOFFを確認する。
- C352 μ-law全256値とC140互換μ-lawを混同しない比較テストを作る。

### MAME実装から導く検証ベクトル

- 32番目のボイスだけを発音し、31番目を越えるアドレス／チャンネルが存在しないこと。
- `wave_bank=0`で開始し、`wave_start`、`wave_end`、`wave_loop`の境界で読み出し位置を記録すること。
- 非ループ終端でBUSY解除、forward loop、reverse loop、REVERSE単独をそれぞれ確認すること。
- linear PCMとμ-lawの同一波形を比較し、256入力値の変換表を照合すること。
- `vol_f`／`vol_r`の4 byte出力、音量ramp、各位相反転フラグを確認すること。
- NOISE、FILTER、LINK、0x200 control、0x202 KEYON／KEYOFFの境界動作を確認すること。

### MAME調査上の留保

MAMEのC352デバイスは動作基準として有用だが、`m_control`の用途は実装コメント上unknownであり、LINKの意味も完全には確定していない。また、複数のMAME搭載ドライバにはC352 dividerを`TODO: verify`とする注記がある。従って、これらを仕様確定値として固定せず、p002の実装対象とp004の検証・insight管理へ引き継ぐ。

QuattroPlayも`control1`／`control2`の用途、FILTERと音量rampの関係、phase flagの一部の順序に留保を残している。従って、QuattroPlayの実装値は有力な互換基準とし、実機または追加資料で確定できない箇所は「MAME／QuattroPlay互換挙動」として明示する。

## 完了条件

- 実装者がレジスタ値と期待出力を追跡できる仕様表がある。
- C140/C219との差分およびC352固有の未対応範囲が明記されている。
- 後続p002〜p004が使用する検証ベクトルとコマンドが定義されている。
- 根拠のない仕様が残っていない。
- MAME由来の仕様と、MAME自身が未確定としている事項が区別されている。

## 検証

- 仕様表の各項目に一次資料または実装根拠が紐付いていることを目視確認。
- 検証ベクトルを用いた最小再現プログラムまたは既存テスト入力が生成できることを確認。

## Execution Log

未実行。MAME／QuattroPlayの調査はQueue起案前の事前調査として本文へ反映済み。Queue `q001` は`proposed`で、人間の実行認可待ち。
