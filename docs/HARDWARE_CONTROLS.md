# ESP32 wired control panel

最終更新: 2026-08-28

## 確定した構成

実機は従来型ESP32を搭載した開発ボードで、USB-UART変換はCH340である。
PCとはUSBケーブル1本で給電および`115200 bps`の有線シリアル通信を行う。
2026-08-28の実機確認では`COM7`だったが、Windowsが別番号を割り当てる場合がある。

- チップ: `ESP32-D0WD-V3 revision 3.1`
- USB: `USB-SERIAL CH340`
- 通信: USBシリアル、8 data bits、no parity、1 stop bit
- ファームウェア: [`Hardware/ArriettyController/ArriettyController.ino`](../Hardware/ArriettyController/ArriettyController.ino)
- Arduino FQBN: `esp32:esp32:esp32`
- Arduino ESP32 Core: `3.3.11`で実機検証

このESP32のUSB端子は標準USB HIDゲームパッドではない。WindowsではCOMポートとして認識され、
Arriettyは後述のシリアルプロトコルで状態を取得する。ESP32-S2/S3向けTinyUSB HIDの旧案は採用しない。

## 電源と配線

ジョイスティックの基板印刷`5V`はVCC端子を意味するが、本機では必ずESP32の`3V3`へ接続する。
5 Vで給電するとX/Y出力も5 V付近まで上がり、ESP32のADC入力を破損する可能性がある。
2個のジョイスティックと全ボタンはESP32のGNDを共通にする。

| 部品 | 端子 | ESP32 | ファームウェア上の意味 |
|---|---|---:|---|
| Joystick 1 | G | GND | 共通GND |
| Joystick 1 | 5V/VCC | 3V3 | 3.3 V給電 |
| Joystick 1 | module X | GPIO34 | 塗装Y軸、反転して使用 |
| Joystick 1 | module Y | GPIO35 | 塗装X軸、反転して使用 |
| Joystick 1 | SW | GPIO13 | button mask bit 6 |
| Joystick 2 | G | GND | 共通GND |
| Joystick 2 | 5V/VCC | 3V3 | 3.3 V給電 |
| Joystick 2 | module X | GPIO32 | Joystick 2 X |
| Joystick 2 | module Y | GPIO25 | Joystick 2 Y |
| Joystick 2 | SW | GPIO14 | button mask bit 7 |
| Button 1 | switch | GPIO18–GND | button mask bit 0 |
| Button 2 | switch | GPIO19–GND | button mask bit 1 |
| Button 3 | switch | GPIO21–GND | button mask bit 2 |
| Button 4 | switch | GPIO22–GND | button mask bit 3 |
| Button 5 | switch | GPIO23–GND | button mask bit 4 |
| Button 6 | switch | GPIO26–GND | button mask bit 5 |

GPIO0、2、4、5、12、15は起動用strapping pinのため使用しない。GPIO34/35は入力専用だが、
内部Pull-upを必要としないアナログ軸に適している。GPIO25はADC2のためWi-Fiと同時使用しない。
本機はUSB有線運用でWi-Fiを起動しない。

## 自転車上の配置

乗車した人から見た配置を基準とする。

| 入力 | 取付位置 |
|---|---|
| Button 1 | ハンドル中央の左 |
| Button 2 | ハンドル中央の右 |
| Button 3 | ドロップハンドル外側の左 |
| Button 4 | ドロップハンドル外側の右 |
| Button 5 | ブレーキレバー上の左 |
| Button 6 | ブレーキレバー上の右 |
| Joystick 1 | 左変速機の親指位置上 |
| Joystick 2 | 右変速機の親指位置上 |

各ボタンとSWはNormally Openで、GPIOとGNDの間に接続する。GPIO13、14、18、19、21、22、23、26は
`INPUT_PULLUP`を使用するため外付けPull-up抵抗は不要である。未押下はHigh、押下はLowで、
ファームウェアが15 msのデバウンス後に押下bitを1にする。

## ジョイスティック座標

筐体にマジックで書いた`X`と`Y`を、今後の会話、ファームウェア、Arriettyすべての基準とする。
モジュール基板の`X/Y`印刷は配線を識別するためだけに使用する。

Joystick 1は実機で次の順序を測定した。

```text
塗装X+、塗装X+、塗装Y+、塗装Y+
    ↓ 実際のADC
GPIO35減少、GPIO35減少、GPIO34減少、GPIO34減少
```

したがってJoystick 1の公開座標は次のとおりである。

```cpp
joystick1X = 4095 - analogRead(35);
joystick1Y = 4095 - analogRead(34);
```

これにより塗装X+／Y+へ動かすと、シリアルプロトコルのX／Yも正方向へ増加する。
Joystick 2はGPIO32/25の全範囲`0..4095`を確認済みであり、現在はmodule X/Yと同じ順序・極性で使用する。
極性変更はファームウェア冒頭の定数だけで行え、再配線は不要である。

起動時は約0.6秒間、4軸の中央を測定する。USB接続直後は両スティックから手を離しておく。
再校正する場合は中央へ戻して`CAL`コマンドを送る。中央にはADC 160 countsのデッドゾーンを設ける。

## USBシリアルプロトコル v1

行末はLF、数値はASCII decimalである。ESP32は起動後に次を1行送る。

```text
READY ARRIETTY-CONTROLLER/1
```

PCから使用できるコマンド:

| コマンド | 応答・動作 |
|---|---|
| `PING` | `PONG ARRIETTY-CONTROLLER/1` |
| `INFO` | 機種と通信設定 |
| `READ` | 状態パケットを1件返す |
| `STREAM ON` | 50 Hz連続送信を開始 |
| `STREAM OFF` | 連続送信を停止 |
| `CAL` | 現在位置を4軸の中央として再校正 |

状態パケット:

```text
A1,sequence,j1x,j1y,j2x,j2y,buttons
```

- `sequence`: 送信ごとに増加する符号なし番号
- `j1x`、`j1y`、`j2x`、`j2y`: `-32767..32767`、中央0、塗装または定義上の＋方向が正
- `buttons`: `0..255`のbit mask。bit 0..5はButton 1..6、bit 6はJoystick 1 SW、bit 7はJoystick 2 SW
- 連続送信周期: 20 ms（50 Hz）

例:

```text
A1,42,0,15720,-320,0,9
```

この例ではButton 1とButton 4が押されている。

## 操作割り当て

現時点のArrietty操作割り当ては次のとおりとする。Joystick 2のX/Yは飛行モードだけで操縦へ使い、
地上走行では操作を起こさない。その他の未割り当て入力も状態として報告・表示する。

| 入力 | Arrietty操作 |
|---|---|
| Button 1 | 未開始時: 実車の正面をまっすぐ見て押し、Start Ride／最新HMD姿勢を開始ライン方向へ合わせる。開始後: 実際の走行軌跡を約2 m戻る安全復帰 |
| Button 2 | 地上／人力飛行モード切替。空中では地上モードへの切替を拒否し、着陸後に切替可能 |
| Button 3 | 今回は未割り当て。入力値のみ報告・表示 |
| Button 4 | 今回は未割り当て。入力値のみ報告・表示 |
| Button 5 | 今回は未割り当て。入力値のみ報告・表示 |
| Button 6 | 押している間だけT2へ上り勾配3.0%を送り、離すと0%へ戻す |
| Joystick 1 X/Y/SW | 用途未定。入力値のみ報告・表示 |
| Joystick 2 X | 飛行時のエレベーター。`-X`で機首下げ、`+X`で機首上げ。地上走行時は未割り当て |
| Joystick 2 Y | 飛行時のエルロン。`-Y`で右翼下がり、`+Y`で左翼下がり。地上走行時は未割り当て |
| Joystick 2 SW | 用途未定。入力値のみ報告・表示 |

地上走行中のカーブはハンドルによる前輪操舵を維持する。飛行中は同じハンドル入力をラダーとして左右のヨーへ使う。

人力飛行モデルの初期値は、有効質量35 kg、滑空比30、プロペラ効率80%、最良滑空速度24 km/h、
離陸20 km/h、失速18 km/h、失速回復20.5 km/hとする。約95 Wで24 km/hの水平飛行になり、
無動力時は同速度で約0.22 m/s降下する。Joystick 2 Xで上昇すれば対気速度を失い、機首下げでは
降下エネルギーが対気速度へ戻る。失速中はバンクとラダーの効きを25%へ落とし、最低2.0 m/sで降下させる。

Button 1の安全復帰は道路中央線を推測せず、走行面上で実際に通過できた位置と向きを0.1 m間隔で記録し、
その軌跡を約2 m巻き戻す。走行開始から2 m未満なら開始位置まで戻る。走行距離の積算値は減らさない。

Button 6は物理的な非常ブレーキではなく、CYCPLUS T2のFTMS Indoor Bike Simulationへ勾配を送り、
ペダル抵抗を増やす操作である。初期値は3.0%（FTMS値`300`、0.01%単位）とする。

## CLI操作と確認済み結果

PowerShellでリポジトリのルートから実行する。

```powershell
.\Tools\ESP32-Controller.ps1 -Action Build
.\Tools\ESP32-Controller.ps1 -Action Upload
.\Tools\ESP32-Controller.ps1 -Action Test
```

スクリプトは`USB-SERIAL CH340`のCOMポートを自動検出する。必要なら`-Port COM7`を追加する。

2026-08-28の机上試験で確認済み:

- ESP32書き込みおよび書き込み後hash検証
- USB `PING/PONG`、状態パケット、50 Hz連続送信
- Joystick 1/2の4軸すべて`0..4095`到達
- 6ボタンを1個ずつ独立して検出
- Joystick 1/2のSWを独立して検出
- Joystick 1の塗装X/Y順序と正方向
- COMポートを閉じて再接続した後の通信

2026-08-28のHMD＋T2統合実走で確認済み:

- COM7自動接続とButton 1による走行開始
- SteamVRの`RightGrip`自動選択と開始ラインHeading 0°
- 最高22.08 km/h、積算809.223 mの前進
- Button 2による地上／飛行切替と最高高度12.08 m
- Button 6押下中の3.0%勾配、解放後の0.0%復帰
- Button 1再押下による走行軌跡上2.01 mの安全復帰

自転車へ取り付ける前の最終ファームウェア確認は`-Action Test`で行う。
