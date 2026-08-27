# Next session handoff

記録日時: 2026-08-27

## 保存済み状態

- Arrietty UE版はv0.8.0。
- `ArriettyRuntime`ソースプラグインとLevel中心構成へ移行済み。
- `ArriettyDemo`には`ArriettyWorldBuilder`と`ArriettyCourseStart`を明示配置済み。
- 新規世界作成、Runtime更新、VS2026 Editor/Game/Shippingビルド、Automation 6件、Cook、Package、NoHMD Esc終了は成功済み。
- 世界作成は`New-ArriettyWorldProject.ps1`、C++更新は`Update-ArriettyWorldProject.ps1`を使う。

## 次回最初のHMD試験

ライトハウスとSteamVRを起動し、SteamVRをアクティブなOpenXR Runtimeにしてから行う。

1. `C:\Users\azoo\git\Arrietty-UE\Arrietty.uproject`を開く。
2. `/Game/Worlds/ArriettyDemo/ArriettyDemo`が開いていることを確認する。
3. Play方式を`VR Preview`にしてPlayする。
4. HMDに両眼映像、デモ道路、建物、計器が出ることを確認する。
5. 正面とハンドルを中央にしてテンキー0を押す。
6. 直進方向、操舵、コース端、飛行モードを確認する。
7. EscでVR Previewが終了し、UE Editorへ戻ることを確認する。

今回のLevel／プラグイン再構成後のHMD＋T2＋VIVEコントローラー実機試験は未実施である。異常があれば画面表示、HMD表示、開いたLevel名、操作直前の状態を記録する。

## ESP32作業開始時

1. ESP32ボードの表裏、チップ名、USB端子表記を確認する。
2. ジョイスティックの端子名と定格を確認する。
3. USB HID対応可否を確定してからGPIO番号を決める。
4. 最初はボタン1個だけをGPIO-GNDへ接続し、Pull-upとデバウンスを確認する。
5. 6ボタン、ジョイスティックX/Y、押し込みSWの順に増やす。
6. Windowsのゲームコントローラー画面で確認してからArriettyへ接続する。

設計詳細は`docs/HARDWARE_CONTROLS.md`を参照する。現時点の第一候補は標準USB HIDゲームパッドであり、F13〜F18やテンキーへの変換は予備案とする。
