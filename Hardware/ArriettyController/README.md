# Arrietty ESP32 controller firmware

従来型ESP32＋CH340を、USBシリアル接続のArrietty操作盤として使用するファームウェアです。
実配線、座標、通信仕様は[`../../docs/HARDWARE_CONTROLS.md`](../../docs/HARDWARE_CONTROLS.md)を参照してください。

リポジトリのルートから操作します。

```powershell
.\Tools\ESP32-Controller.ps1 -Action Build
.\Tools\ESP32-Controller.ps1 -Action Upload
.\Tools\ESP32-Controller.ps1 -Action Test
.\Tools\ESP32-Controller.ps1 -Action ButtonTest
```

COMポートは`USB-SERIAL CH340`から自動検出します。明示する場合は`-Port COM7`を追加します。
`ButtonTest`は内蔵テストモードへ一時的に切り替え、画面の指示どおりButton 1〜6を
1個ずつ押してGPIO、生入力、デバウンス済み入力を確認する。完了時およびエラー終了時には
通常モードへ戻す。電源投入時も常に通常モードで起動する。

自転車から画面が見えない場合は音で操作できる。短い1音で次のButtonを押し、2回の高音でPASSして
離す。6個完了は3音の上昇音、失敗は3回の低音で知らせる。
確認済みのButtonを省略する場合は、例として`-ButtonSequence 3,4,6`を追加する。

テスト中はUEやシリアルモニターを終了し、COMポートを空けておく。
2台のジョイスティックのX/Y/SWはArriettyへ送信しますが、ゲーム内用途はまだ割り当てていません。
