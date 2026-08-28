# Next session handoff

記録日時: 2026-08-28

## 保存済み状態

- Arrietty UE版はv0.8.0。
- `ArriettyRuntime`ソースプラグインとLevel中心構成へ移行済み。
- `ArriettyDemo`には`ArriettyWorldBuilder`と`ArriettyCourseStart`を明示配置済み。
- 新規世界作成、Runtime更新、VS2026 Editor/Game/Shippingビルド、Automation 7件、Cook、Package、NoHMD Esc終了は成功済み。
- 世界作成は`New-ArriettyWorldProject.ps1`、C++更新は`Update-ArriettyWorldProject.ps1`を使う。

## 確認済みHMD統合試験

ライトハウスとSteamVRを起動し、SteamVRをアクティブなOpenXR Runtimeにしてから行う。

1. `C:\Users\azoo\git\Arrietty-UE\Arrietty.uproject`を開く。
2. `/Game/Worlds/ArriettyDemo/ArriettyDemo`が開いていることを確認する。
3. Play方式を`VR Preview`にしてPlayする。
4. HMDに両眼映像、デモ道路、建物、計器が出ることを確認する。
5. ハンドルを中央にしてテンキー0を押す。SteamVRの左右割り当てにかかわらず操舵Sourceが`LeftGrip`または`RightGrip`になり、仮想視界が開始ライン方向を向くことを確認する。
6. 直進方向、操舵、コース端、飛行モードを確認する。
7. ESP32のButton 1を再度押し、直前の走行軌跡を約2 m戻ることを確認する。
8. Button 2で地上／飛行を切り替える。
9. Button 6を押している間だけT2が3%上り勾配となり、離すと0%へ戻ることを確認する。
10. EscでVR Previewが終了し、UE Editorへ戻ることを確認する。

2026-08-28にHMD＋T2＋VIVEコントローラー＋ESP32操作盤で実施済み。SteamVRが`RightGrip`へ割り当てたコントローラーを自動選択し、開始Heading 0°、最高22.08 km/h、809.223 m走行、飛行高度最大12.08 m、3%ブレーキと0%解除、Button 1による2.01 m安全復帰、Esc終了をCSVで確認した。異常があれば画面表示、HMD表示、開いたLevel名、操作直前の状態を記録する。

## ESP32操作盤

- 実機はESP32-D0WD-V3＋CH340であり、標準USB HIDではなく`115200 bps`のUSBシリアルを使用する。
- 6ボタン、2個のXYジョイスティック、両SW、Joystick 1の塗装X/Yと正方向まで机上試験済み。
- 配線、座標、プロトコル、CLIは`docs/HARDWARE_CONTROLS.md`に確定記録済み。
- ファームウェアは`Hardware/ArriettyController`、操作CLIは`Tools/ESP32-Controller.ps1`。
- 完成版ファームウェアをCOM7の実機へ書き込み済み。CLIの`PING/PONG`、状態パケット、50 Hz連続送信と、UEからの自動接続を確認済み。
- 2台のJoystickのX/Y/SWはUEへ受信・表示するだけで、ゲーム内用途は未定。
- Button 1は開始／開始後の約2 m軌跡復帰、Button 2は地上・飛行トグル、Button 6は押下中3%勾配ブレーキ。Button 3〜5は未割り当て。
- 走行中のカーブはハンドル操舵を使い、Joystickとは結び付けない。
- ステム固定VIVEコントローラーはSteamVRの左／右割り当てを固定せず、追跡できている`LeftGrip`／`RightGrip`を自動選択する。
- USB接続直後の約1秒はジョイスティックへ触れず、起動時中央校正を完了させる。
