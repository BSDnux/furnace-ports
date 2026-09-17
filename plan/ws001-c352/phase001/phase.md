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

### 2026-09-18 — q001開始
- Status: in-progress。承認証跡: ユーザー「ws001p001を実行」。run/q001、チェックポイント a5bd354d7 で実行。本文は開始時点の記録として保存し、以下の確定契約が事前調査の留保・相違を補足する。
- 検証コマンド（実行前定義）: `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1`。固定版参照のハッシュ照合、参照コードの抽出コンパイル、数値ベクトルとの比較を行う。

### 確定契約 — 根拠と互換プロファイル

本節はFurnace用の実装契約であり、C352実機の完全な仕様書ではない。実機測定・メーカーのデータシートによる裏付けは今回取得していない。不明点を推測で補わず、既存実装の観測可能な挙動と製品側の設計選択を区別する。

| 根拠ID | 固定した資料 | 参照箇所 |
|---|---|---|
| M1 | [MAME c352.cpp](https://github.com/mamedev/mame/blob/92a3f13f9664e29d5c8ffba9634e714d623f6baf/src/devices/sound/c352.cpp) | `fetch_sample`, `sound_stream_update`, `read/write`, `device_start/reset` |
| M2 | [MAME c352.h](https://github.com/mamedev/mame/blob/92a3f13f9664e29d5c8ffba9634e714d623f6baf/src/devices/sound/c352.h) | フラグ、voice構造、24-bit ROM interface |
| M3 | [MAME namcos11.cpp](https://github.com/mamedev/mame/blob/92a3f13f9664e29d5c8ffba9634e714d623f6baf/src/mame/namco/namcos11.cpp) | `C352(config, "c352", 25401600, 288)`、4出力ルーティング |
| Q1 | [QuattroPlay c352.c](https://github.com/superctr/QuattroPlay/blob/448f316945d50a5aec5d1ed9607e04b4a0a3ee9c/src/emu/c352.c) | `C352_init/write/read/update`, decoder |
| Q2 | [QuattroPlay c352.h](https://github.com/superctr/QuattroPlay/blob/448f316945d50a5aec5d1ed9607e04b4a0a3ee9c/src/emu/c352.h) | latch、出力型、未確定コメント |
| F1 | Furnace base `54ce9df2c`、`src/engine/platform/c140.cpp/.h`、`sound/c140_c219.c` | PCMディスパッチ、サンプル配置、C219 decoder |
| F2 | 同base、`src/engine/sample.cpp/.h` | `c219Table[256]`、`dataC219`、depth=12 |
| F3 | 同base、`dispatch.h`、`platform/qsound.cpp`、`engine.cpp::autoPatchbay` | 16-bit register pool、複数出力API、自動配線 |
| F4 | 同base、`sysDef.cpp/.h`、`instrument.cpp/.h`、`dispatchContainer.cpp`、`vgmOps.cpp`、`src/gui/` | 登録・保存・GUI・VGM接続点 |

原本とSHA-256は `../tests/reference/sources.json` に保存。MAMEを数値基準に採用する。QuattroPlayの異なる挙動は混ぜない。p002が別の挙動を採用する場合は、この契約を上書きせず、新しいPhase／insightで変更理由を記録する。

| 差異 | MAME互換として採用 | QuattroPlayで観測した差 |
|---|---|---|
| フラグ | 再生中も現在のflagsを参照 | 位相・補間はKEYON時ラッチ |
| FILTER | 補間のみ無効。rampは継続 | 更新機会に音量を即時反映 |
| 非ループ終端 | 読み出した終端値を直ちに0にする | その時点では終端値を保持 |
| 再KEYON | sample/last_sampleを0に初期化 | sample/last_sampleを初期化しない |
| control | 0x200保持・読出可、0x201無視・読出0 | 両方保持、両方読出0 |
| 出力 | 各寄与を右シフト8、合計を右シフト3、signed16へ折り返す | 未スケールのdouble加算 |

この選択により本文の「Furnace側でもflagsラッチへ分離」という事前提案は採用しない。実機のcontrol、FM、LOOPTRG、LINKの意味、divider、FILTER、位相配線に関する未確定性はins005/ins007に残す。

### 確定契約 — レジスタと数値処理

数値は特記なき限り16進。外部APIは `write(wordAddress, uint16Data)`。レジスタ数をbyteアドレスと混同しない。内部値にバイト順はない。バイト列として保存・表示する際は明示的な変換を用いる。

| ワードアドレス | 内容／契約 | 根拠 |
|---|---|---|
| `v*8+0`, `v*8+1` | v=0..31。front/rear volume、上位byte=L、下位byte=R、各0..255 | M1 read/write/mix |
| `v*8+2` | freq=0..ffff、16-bit fractionへの1出力フレーム当たり加算値 | M1 mix |
| `v*8+3` | 現在flagsを保持。readでBUSY/LDIR/LOOPHIST等も観測 | M1/M2 |
| `v*8+4..7` | bank/start/end/loop。いずれも16-bit、ROM byteアドレスは24-bitにマスク | M1/M2 |
| `0100..01ff` | 今回は読出0、書込無効果。内部状態のミラーは推測しない | M1 |
| `0200` | control保持、音声への効果なし。部分書込はmask合成 | M1 |
| `0201` | 読出0・書込無効果。QuattroPlay control2は不採用 | M1/Q1 |
| `0202` | full-word書込だけが全voiceのKEYON/KEYOFFを実行。値は無関係、読出0 | M1 |
| その他 | 読出0・書込無効果、配列外アクセス禁止 | M1＋ホスト安全契約 |

- 発音書込順はvolume、freq、bank/start/end/loop、flags、最後に0202。複数voiceの予約後、tick末尾に0202を1回出す。同一tick内の停止→再発音は最終状態をKEYONのみとして予約し、KEYOFFを残さない。低レベルで両bitが立つ場合はMAME通りKEYOFFが勝つ。
- KEYONはpos=`bank<<16|start`、counter=ffff、sample/last_sample/current volumes=0、BUSY=1、KEYON/LOOPHIST=0。KEYOFFはBUSY/KEYOFF=0、counter=ffff。resetは32voice/controlを0、noise seed=1234。（M1）
- `next=counter+freq`のbit16が立つとfetchを1回、`(next^counter)&18000`が非0なら各volumeを目標へ1だけ動かす。counterは下位16-bit。freq=0はKEYON直後なら無音でfetchしない。freq=ffffも厳密には1.0倍ではない。（M1）
- linear decodeはsigned byte×256（負数の左シフトに依存しない移植とする）。C352 μ-lawはF2の全256値と一致する。00=0、7f=31232、80=-32、ff=-31264。G.711/C140変換とは別物。`dataC219`をROMへbyte順のままコピーし、再生時にC352テーブルでdecodeする。（M1/Q1/F2）
- FILTER=0では前回／今回サンプルをfractionで線形補間、FILTER=1では今回値。signed負数の丸めは参照結果の算術右シフト相当として再現する。各出力の寄与は`(signed sample * current volume)>>8`、32voiceをint32で合計し、`sum>>3`の下位16-bitをsigned値にする。コア段でclampに変更しない。（M1）
- PHASEFL=0100はFL、PHASERL=0200はRL、PHASEFR=0080はFRとRRを反転。RR独立bitはない。NOISE=0010時、chip共通16-bit状態をfetchごとに `(state>>1)^((-(state&1))&fff6)` で更新し、ROMは読まない。voice走査は0→31、ミュートしても走査・noise更新は継続する。（M1、ミュートはホスト契約）
- BUSY/KEYON/KEYOFF/LOOPHIST/LDIRは上記状態機械に従う。FM=0400、LOOPTRG=1000は保存・読出のみで音響効果未実装と明記。LINK=0020は下記参照挙動のみ、実機long-format対応を名乗らない。（M1/M2）

### 確定契約 — アドレスとループ

| 条件 | fetch後の処理 | 根拠 |
|---|---|---|
| 非ループ、pos下位16bit != end | REVERSE=0ならpos+1、1ならpos-1 | M1 |
| 非ループ、pos下位16bit == end | BUSY解除、KEYOFF設定、sample=0。従ってstart=endの非ループは無音 | M1、数値ベクトル |
| LOOP=1/REVERSE=0、end | 同一bankのloopへ移動、LOOPHIST設定、終端サンプルも発音 | M1 |
| LOOP=1/REVERSE=1 | endでLDIR=1、loopでLDIR=0、変更後の向きへ1進む。端点は往復ごとに1回 | M1 |
| LINK+LOOP、通常終端 | pos=`wave_start<<16|wave_loop`、LOOPHIST設定。REVLOOP分岐が先 | M1 |
| bank跨ぎ | increment/decrementは位置全体に作用、ROM読出は24-bitマスク。LINKのbankにも同じマスク | M1/M2 |

Furnaceサンプル配置は製品側の制約として以下に固定する。

- ROM=16 MiB、1byte/サンプル、ゼロ初期化、bank=64 KiB。通常のサンプルをbank跨ぎで置かない。C219の偶数アドレス制約、`^1` byte入替、group bankは流用しない。
- 非ループは最大65535音声byte＋ゼロguard 1byte。start=先頭、end=先頭+長さ（guard位置）。これによりMAMEの終端消去でも最後の音声byteを発音できる。1byteサンプルもstart/endを別にする。
- forward loopはFurnaceの半開区間`[loopStart,loopEnd)`を、loop=`base+loopStart`、end=`base+loopEnd-1`へ写像。最大65536byte。範囲外loop、空サンプルは発音しない。長過ぎる／ROMに収まらないサンプルは未ロード＋警告とし、暗黙の切詰めはしない。
- reverse単発のGUI公開は初期統合の対象外（コアとpokeでは対応）。ping-pong loopはLOOP|REVERSEで再現し、同一点ループはforwardへ正規化する。LINKの自動サンプル連結は初期対象外。これらはhardware制限ではなく、Furnace初期実装の範囲である。

### 確定契約 — Furnace API／追加・再利用点

| 接続箇所 | 後続Phaseの契約 | 根拠 |
|---|---|---|
| `platform/sound/c352.*`, `platform/c352.cpp/.h` | 新コア・新DivPlatformC352。C140/C219コアを改変せず、32 Channel/osc buffers、16-bit値のwrite queue。C140のmacro/note-map/porta実装パターンを再利用 | F1、32voice/レジスタ差 |
| クロック | 既定25401600 Hz、divider=288固定、rate=整数除算88200 Hz。customClockも同divider。発音レートRからfreq=`clamp(round(R/rate*65536),0,65535)`。M3の構成を選ぶ製品方針で、全基板の実測値ではない | M1/M3/Q1 |
| `getRegisterPool*`, `poke`, dumpWrites | poolはuint16[0x203]、size=0x203エントリ、depth=16。poke/dumpはワードアドレス・16-bit値を維持。部分書込はコア試験用API、通常dispatchはfull-word | F3、M1 |
| `getOutputCount/acquire` | コア常時FL/FR/RL/RR。system flag `quadOutput=false`では2出力、L=(FL+RL)/2、R=(FR+RR)/2をint32で加算し0方向へ除算。trueでは4出力を同順序で公開。gain/postAmp=1.0 | F3、製品方針 |
| 初期volume/pan | channel volume=255、FL/FR/RL/RR pan=255。通常PANNINGはL/R両面に適用。SURROUND_PANNINGはout=0..3で個別値0..255。volume乗算は整数`vol*pan/255` | F1、dispatch.h API、製品方針 |
| 音量・マクロ | 新DIV_INS_C352、sample mapを使用。volume/arp/pitch/panL/panR/phaseResetをC140同等に扱う。panL/Rマクロは両面へ適用。C352 control macroは初期公開しない（NOISE/FILTER/phase/LINKはpoke試験対象） | F1/F4、公開範囲の選択 |
| ミュート | chip状態・共通noiseは進め、対象voiceの出力寄与のみ0。oscは対象voiceの4寄与の平均、signed16範囲へclamp。空／無効sampleは停止 | M1/F1、製品方針 |
| `sysDef.h/.cpp`, `dispatchContainer.cpp`, `CMakeLists.txt` | DIV_SYSTEM_C352を末尾へ追加、32 PCM channel、DIV_INS_C352＋AMIGA fallback。sample depth maskは8BITとC219。新ソースをビルド・生成factoryへ追加 | F4 |
| ファイル識別子 | base時点で未使用のsystem file ID `0xe8`、instrument ID `68`を予約候補として固定。p003着手時に衝突を再検査、衝突があれば新計画へ戻す。既存IDの番号変更禁止 | F4、baseで検索確認 |
| `instrument.h/.cpp` | enum=68、sample map/length機能フラグを有効にする。既存sample depth=12を流用し、新decoder形式IDは追加しない | F2/F4 |
| `gui/insEdit.cpp`, `guiConst.cpp`, `gui.h`, `settings/allSettings.cpp` | PCM instrument editor、名称・色・アイコン（既存汎用PCMアイコンを再利用）、システム一覧へ追加 | F4 |
| `gui/sysConf.cpp`, `sampleEdit.cpp`, `doAction.cpp`, `sysMiscInfo.cpp`, `debug.cpp`, `presets/sample.cpp`, `presets/arcadeSystems.cpp` | custom clock/quadOutput設定、長さ・loop・ROM容量警告、sample instrument判定、デバッグ32voice、プリセット登録 | F4 |
| `doc/7-systems`, `doc/4-instrument`, ファイル形式文書 | 音源／楽器・ID・非対応範囲・quad配線を文書化。autoPatchbayは出力番号とデバイス番号をそのまま結ぶため、4ch modeのrearをステレオへ自動合算しない | F3/F4 |
| `vgmOps.cpp` | 既存hasC352とheader欄はゼロplaceholderのみ。初期統合ではVGM非対応を明示、sysDef最小VGM版は0にして選択不可。opcode e1だけ追加して「対応済み」としない。完全対応は別途ROM block/clock/divider/word endianの検証を伴うPhaseが必要 | F4/Q1 |

C140は24voice、C219は16voice、既存dispatchは8-bitレジスタ＋2出力。C352は32voice、8ワード/voice、グローバルキー実行、4出力、固有ramp・loop・LINKを持つため、同じクラスのモード追加ではなく独立実装とする。C219の256値decoderテーブルとsample形式は再利用できるが、C140のG.711変換・16-bit ROM配置は再利用しない。

### 検証ベクトルと後続検証ゲート

以下は `../tests/vectors.cpp` にレジスタ列・期待値として実装済み。既定flagsはFILTER=1、freq=ffff、4volume=ff、初回counter=ffff。個別の前提は各テストの`key`呼出しで上書きする。

| ケース | 代表的な期待値 | 使用Phase |
|---|---|---|
| reset/無音 | 4出力0、control=0、noise=1234 | p002/p004 |
| 単一・終端 | 非ループstart=endなら無音。guardを置きend=start+1なら最初の7fが15/出力、次にBUSY解除 | p002/p004 |
| linear境界 | 7f→32512、80→-32768 | p002/p004 |
| μ-law全256値 | F2 tableとの全要素一致、80=-32、ff=-31264 | p002/p004 |
| register境界／commit | voice31 base=f8、mask合成1234→ab34、partial0202無効果、full0202でBUSY | p002/p004 |
| freq／補間 | freq=8000の初回counter=7fff、PCM40の初回出力3、2frameで1fetch。freq=0はfetch0 | p002/p004 |
| forward/reverse/ping-pong | ROM読出列0,1,2,1,2／3,2,1,0／0,1,2,1,0,1 | p002/p004 |
| LINK／24-bit境界 | bank1/start2/end2/loop7→pos20007。ffffffの次のROM読出は0 | p002/p004 |
| noise | seed1234→091a→048d→fdb0、ROM読出なし | p002/p004 |
| stop／同時KEYON+KEYOFF | 次frame無音、同時指定時の最終flags=0004 | p002/p004 |
| ramp／4出力／位相 | PCM40でFL=80、RR=40に収束後の出力1024,0,0,512。位相反転は-1024／-512、RL独立も確認 | p002/p004 |
| voice31／32voice | voice31単独PCM40初回8、32voice PCM7f初回508、255更新後-1532（wrap） | p002/p004 |
| 実装差異 | QuattroPlayのFILTER即時volume=255、終端保持、live位相無反映、control読出0、rate88200 | p001比較根拠 |

実行可能な参照ゲート: `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1`。依存はPowerShell＋GCCのみ、ROMはテスト内で生成。p001は参照実装を検証するもので、未作成のFurnace C352コアの検証ではない。

p002/p004では上記コマンドを継続使用し、新コアに同じ入力列を渡す比較アダプタを認可スコープ内で作成する。ホスト契約の追加ベクトルは次で固定する: 1byte非ループのguard配置、65535+guardのbank末尾、65536byte loop、65536byte非ループ拒否、ROM満杯／空／不正loop拒否、byte順保持、4ch→2chの(100,200,300,400)→(200,300)、負の奇数の0方向除算、ミュート中noise継続、word register dump保存。

p003/p004統合ゲートはCMake構成済みの専用ビルド先に対し `cmake --build <configured-build> --config Release`。同build内に新設するC352比較テストを登録した後 `ctest --test-dir <configured-build> -C Release --output-on-failure -R c352` を使う。現時点では対象CTestは存在しないため、これを実行済み・合格とは扱わない。既存testディレクトリ／CMakeには今回使用可能なC352 gateがない。

UI／保存ゲート: C352を選択→32ch表示→8-bitとC219 sampleを割当→ch1/ch32の発音、stop、forward/ping-pong loop、pan、pitchを確認→quadOutput両設定で再生→.fur保存→再読込でsystem ID・楽器・sample bytes・clock・quadOutputが同一。VGM画面はC352非対応を表示。従来C140/C219曲の再読込・renderが不変であることをp004/p005で確認する。実際のfixtureとビルド依存は各Phaseのready化時に用意し、今回は未実行。

### 2026-09-18 — 検証結果・Phase終了

- **Effective Status: cleared**。先頭のStatus=readyと「未実行」は実行前スナップショットとして保持し、現状態は本Execution Log末尾を正本とする。
- Windows PowerShell 5.1 + GCC 8.3.0で `powershell -NoProfile -ExecutionPolicy Bypass -File plan/ws001-c352/tests/verify.ps1` がexit 0。MAME 330件＋QuattroPlay 8件、計338件の数値assertionが成功。両harnessは `-Wall -Wextra -Werror` でコンパイル成功、固定参照8ファイルのSHA-256一致。
- 仕様表の全行についてM1〜M3/Q1〜Q2/F1〜F4または明示的な製品設計判断へ追跡できることを目視確認。資料間の不一致は互換プロファイルで解決し、実機未確認事項をins005/ins007へ分離。
- 完了条件対応: レジスタ／期待出力表＝上記数値契約、C140/C219差分＝API接続表と差分説明、再現可能ベクトル＝tests、未確定と根拠の区別＝互換プロファイル／insights。全5条件を満たす。
- チェックポイント a5bd354d7 のP書全文が現在のP書先頭に一致することを検証し、本文改変なしを確認。`git diff --no-index --check` で追記差分を確認。`git status --porcelain -- src CMakeLists.txt` は空。
- Furnace本体・ビルド設定は未変更。Furnace全体ビルド、既存曲回帰、UI／保存試験は本Phaseの検証対象外で未実行。実装済み音源としてのclearedではない。
- 実行コミットは次のPhase ID付き成果コミットと、Queue終了記録から追跡する。p002以降は未認可・未着手。

- 成果コミット確定: 8fc686d32。q001はfinishedとしてhistory/q001.mdへ保存。状態同期後にmasterへfast-forward統合。
