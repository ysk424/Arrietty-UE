# Next session handoff

記録日時: 2026-08-29

## 今日完了したこと

- Arrietty UE版v0.10.0の地球飛行機能を`Plugins/ArriettyCesium`へ分離して追加した。既存の平面世界は`ArriettyRuntime`だけで動作する。
- `/Game/Worlds/ArriettyEarth/ArriettyEarth`を作成・保存した。Funafuti原点はlongitude `179.19678290`、latitude `-8.52398430`、ellipsoid height `34.8356 m`。
- Google Photorealistic 3D Tiles（ion asset `2275207`、SSE `16`、credits表示、Movable）、Cesium SunSky、地球用GameMode、World Bounds Checks無効を設定した。
- 重複していたCesium Georeferenceを整理し、`ArriettyEarth_Georeference` 1体を既定参照にした。
- 低い視点で見えた黒い面はGoogle Tilesの裏面、格子はテンプレートFloorだと切り分けた。Floorを除去し、CourseStartを地表より上へ移動した。
- 青空、白い雲、Google地表、可視100 m滑走路が表示される状態まで調整した。
- CourseStartに、可視100 m滑走路から続く長さ2,000 m・幅12 mの不可視`FlightRolloutSurface`を追加した。`SecretWorldRideSurface`、Query Only、Visibility Blockで、Google Tilesの衝突には依存しない。
- UE 5.8.2 `ArriettyEditor Win64 Development`を再ビルドし、Earth Levelの通常PIE開始・停止を確認した。
- 実機ではGoogle地表と空、Joystick 2の上下左右、テンキーによる開始と飛行モードまで確認した。旧100 m滑走路端で止まった問題に対して上記2 km面を追加したが、追加後の自転車実走は未確認。
- ESP32用の独立診断ファームウェアを追加し、4軸の全範囲とButton 1/2/3/4/6、Joystick 1/2 SWをGPIO直読で確認した。
- Button 5が反応しない原因はGPIOやソフトではなく、Button 5の半田切れだと実機で確認した。
- 製品ファームウェアには短い押下を最低100 ms報告するラッチを追加した。診断後に製品ファームウェアをCOM7へ戻し、`PING`、状態パケット、50 Hz stream、flash hashを確認した。

## 終了時の状態

- Earth Levelは保存済み。PIE／VR Previewは停止済み。
- ESP32には診断版ではなく製品版ファームウェアが入っている。
- Button 5の半田修理は未完。作業のためボタン側コネクターは外してある。USB電源を切った状態で修理・再接続してから試験する。
- Cesium ionのproject access tokenはローカルの`Content/CesiumSettings/`だけにあり、Git対象外。値を文書、ログ、スクリーンショットへ出さない。
- `.agents/`、`.claude/`、`.neostack/`も端末固有のEditor agent設定としてGit対象外。

## 次回の順序

1. USBを外した状態でButton 5を半田修理し、ボタンコネクターとGNDを再接続する。
2. USB接続後はジョイスティックへ約1秒触れず、`pwsh -File .\Tools\ESP32-Controller.ps1 -Action Test`を実行する。
3. 診断版へ書き換えず、製品版のbutton maskでButton 1、2、5、6を確認する。Button 5の修理が間に合わない場合は1、2、6だけで続行する。
4. Earth Levelを通常PIEで開き、Button 1で開始、Button 2で飛行モード、Joystick 2でPitch／Bankが動くことを確認する。
5. 20 km/h以上で離陸し、可視滑走路の先へ進めることを確認する。2 km面の終端まで走り切る必要はない。
6. 続いてSteamVR/OpenXR、HMD、T2を使い、Button 5 `POWER x1/x5`、失速・回復、着陸、地理座標の連続性を確認する。
7. タイトル画像`C:\Users\azoo\Desktop\ARRIETTY.png`の`/Game/Brand/ARRIETTY`へのimport、Map Check、Earth Levelを含むcook/packageを完了する。

## 固定値と未完項目

- Level: `/Game/Worlds/ArriettyEarth/ArriettyEarth`
- GameMode: `/Script/ArriettyCesium.ArriettyCesiumGameMode`
- Google ion asset: `2275207`
- Google Tiles: SSE `16`、credits `true`、Movable
- CourseStart: longitude `179.19678290`、latitude `-8.52398430`、ellipsoid height 約`41.8356 m`、World Z `700 cm`
- 未完: Button 5半田修理、2 km rolloutの実機再試験、Button 1/2の製品版再試験、タイトル画像import、Map Check、Earth cook/package、v0.10.0最終HMD＋T2受入試験。

## 既存の基準記録

2026-08-28のv0.8.0 HMD＋T2＋VIVEコントローラー＋ESP32統合実走では、SteamVRの`RightGrip`自動選択、開始Heading 0°、最高22.08 km/h、809.223 m走行、旧飛行高度最大12.08 m、Button 6の3%→0%、Button 1の2.01 m安全復帰、Esc終了をCSVで確認済み。
