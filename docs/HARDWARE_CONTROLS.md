# ESP32 bicycle control panel

最終更新: 2026-08-27

## 結論

6個の押しボタンと2軸ジョイスティックを同時に使うため、USBキーボードではなく標準USB HIDゲームパッドとして実装する。押しボタンはButton 1〜6、ジョイスティックはX/Y軸、ジョイスティックに押し込み接点があればButton 7として報告する。

物理スイッチ自身が文字やキー番号を返すわけではない。各スイッチはESP32のGPIOをHigh/Lowにし、ファームウェアがチャタリングを除去してUSB HIDレポートへ変換する。

F13〜F18はキーボード方式の予備案としては使えるが、ジョイスティックのアナログ値を表現できない。テンキー1〜6はNumLock、UE Editorのフォーカス、既存のArriettyキー割当と衝突するため使用しない。

## ESP32の確認

到着済みボードの表面印刷、搭載チップ名、USBコネクタ横の表記を最初に確認する。ピン番号はボードが判明するまで決めない。

- ESP32-S2/S3: USB OTG Deviceを使用でき、TinyUSBでHIDキーボード、マウス、ゲームパッドなどを実装できる。
- 従来のESP32: ネイティブUSB Deviceを持たない。開発ボードのUSB端子は通常USB-UART変換で、PCからはCOMポートに見える。
- ESP32-C3: 内蔵USB Serial/JTAGは固定機能で、任意のHID descriptorには変更できない。

S2/S3開発ボードにUSB端子が2個ある場合、一般に`USB`または`OTG`側がHID用で、`UART`側が書き込み・ログ用である。実際の表記と回路はボード資料で確認する。充電専用ではなく通信対応USBケーブルを使う。

ESP32-S3のUSB Device StackはHIDを含む複数クラスをサポートし、公式にTinyUSB HIDのキーボード／マウス例がある。

- [Espressif USB Device Stack](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/usb_device.html)
- [Espressif USB FAQ](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/usb.html)

## 配線原則

押しボタンはNormally Open（通常開）として、各ボタンを別々のGPIOとGNDの間に接続する。GPIOは内部Pull-upを有効にし、離した状態をHigh、押した状態をLowとして読む。

```text
GPIO ---- push switch ---- GND
```

- 6ボタンは6本のGPIOを使う。ダイオード行列は不要。
- ファームウェアで10〜20 ms程度のデバウンスを行う。
- ESP32のGPIOへ5 Vを入力しない。
- Boot/strapping、USB D+/D-、Flash/PSRAM使用ピンを避ける。
- 自転車へ固定する前にブレッドボードで全入力を確認する。

一般的なアナログジョイスティックは`VCC`、`GND`、`VRx`、`VRy`、任意の`SW`を持つ。

- VCCは原則3.3 Vとし、正確なモジュール仕様を到着後に確認する。
- VRx/VRyはADC対応GPIOへ接続する。
- SWは通常のボタンと同じくGPIO＋内部Pull-upで読む。
- 起動後、手を触れない中央位置を約1秒測定して中心値とする。
- 中央付近に5〜10%のデッドゾーンを設け、必要なら平均化する。
- 軸反転はファームウェアまたはUE設定で変更可能にする。

## USB HID report

最初のプロトタイプでは次の単純なレポートを目標とする。

```text
X axis    signed 16 bit
Y axis    signed 16 bit
Buttons   bit 0..6
Rate      100 Hz程度、または状態変化時＋定期送信
```

Windowsの「USBゲーム コントローラーのセットアップ」で、UEを起動する前にX/Y軸と各ボタンを確認する。

UE 5.8.2では、XInputとして扱われない一般USB HID機器はRawInputでVID/PID、軸、ボタンを設定できる。RawInputはUE 5.8ではDeprecated表示もあるため、実機列挙後にRawInputとGameInputのどちらを採用するか確定する。ファームウェアをキーボードに固定する判断は、その確認後に行う。

- [UE 5.8 RawInput Plugin](https://dev.epicgames.com/documentation/en-us/unreal-engine/rawinput-plugin-in-unreal-engine)

## 仮の操作割当

| HID入力 | Arrietty操作 |
|---|---|
| Button 1 | Start Ride／HMD正面合わせ |
| Button 2 | 地上／飛行モード切替 |
| Button 3 | 抵抗プリセットを下げる |
| Button 4 | 抵抗プリセットを上げる |
| Button 5 | 計器パネル表示切替 |
| Button 6 | Back to Real World／Dive into Secret World |
| Button 7 | ジョイスティック押し込み。用途は保留 |
| X/Y | 走行前の開始方向・開始位置調整、またはUI操作。用途は実機感触を見て確定 |

右VIVEコントローラーによるハンドル操舵は現状維持する。アナログジョイスティックを走行中の操舵へ直結すると、意図しない進路変更が起きるため最初の実装では行わない。
