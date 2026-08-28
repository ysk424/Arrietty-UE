# Arrietty ESP32 controller firmware

従来型ESP32＋CH340を、USBシリアル接続のArrietty操作盤として使用するファームウェアです。
実配線、座標、通信仕様は[`../../docs/HARDWARE_CONTROLS.md`](../../docs/HARDWARE_CONTROLS.md)を参照してください。

リポジトリのルートから操作します。

```powershell
.\Tools\ESP32-Controller.ps1 -Action Build
.\Tools\ESP32-Controller.ps1 -Action Upload
.\Tools\ESP32-Controller.ps1 -Action Test
```

COMポートは`USB-SERIAL CH340`から自動検出します。明示する場合は`-Port COM7`を追加します。
2台のジョイスティックのX/Y/SWはArriettyへ送信しますが、ゲーム内用途はまだ割り当てていません。
