# 実機受入試験

最終更新: 2026-08-28

## 前提

- SteamVR/OpenXRランタイムを最新版にする。
- HTC VIVE HMD、ステム固定コントローラー`LHR-9EFF8645`、CYCPLUS T2を起動する。コントローラーのSteamVR左右割り当てはどちらでもよい。心拍試験では標準BLE Heart Rate Service対応センサーも広告状態にする。
- Windows Bluetoothで他のトレーニングアプリがT2を使用していないことを確認する。
- `Arrietty.uproject`を開き、`ArriettyDemo` Levelを選び、Play方式を`VR Preview`にする。

## ESP32有線操作盤

1. 両ジョイスティックを中央に置いてUSBケーブルを接続し、約1秒触れない。
2. `pwsh -File .\Tools\ESP32-Controller.ps1 -Action Test`が`ARRIETTY CONTROLLER TEST PASSED`になることを確認する。
3. Arrietty起動後、デスクトップUIの`ESP32 Wired Controller`が`CONNECTED: COMx at 115200 bps`になることを確認する。
4. 未開始時のButton 1で走行開始し、開始後に2 m以上走ってもう一度押すと、道路中央を推測せず実走軌跡上の約2 m前の位置・向きへ戻ることを確認する。
5. Button 2を1回ずつ押し、地上→飛行→地上と切り替わることを確認する。
6. Button 6を押している間、T2 ControlとVR計器が`BRAKE 3.0%`／`BRK 3.0%`となって抵抗が増え、離してから約1秒以内に`FLAT`／`BRK 0.0%`へ戻ることを確認する。
7. Button 3〜5はbutton maskだけが変化し、ゲーム操作を起こさないことを確認する。
8. Joystick 1/2の各X/Yを動かし、UIの値が中央付近の0から正負へ変化することを確認する。
9. 両SWを押し、UIのbutton maskでbit 6/7が変化することを確認する。Joystickのゲーム内用途は未定のため、値の受信以外の動作は期待しない。

## 試験

1. VR Preview開始時に両眼表示が自動開始する。`Back to Real World`でデスクトップ表示へ戻り、`Dive into Secret World`で再開する。
2. テンキー8/2がHMD正面、4/6が保存済み進行方向を動かす。
3. HMDの物理的な向きに関係なく、ハンドル中央でテンキー0を押してそのまま1秒保つ。仮想視界とBikeのUE Forwardが`ArriettyCourseStart`の赤いX方向で一致し、T2制御が`FLAT P5`になって開始音が鳴る。Steering Sourceは`LeftGrip`または`RightGrip`となり、直進時のRaw/Applied操舵角が0°付近になる。
4. 20 km/hで1秒走り、仮想距離が約5.56 m増える。
5. 顔を左右へ向けても進路は変わらず、ハンドルだけで曲がる。
6. コントローラー追跡を隠すと移動が止まり、復帰すると再開する。
7. 惰性で5 km/h以下かCSC円盤停止になると表示速度と移動が0になる。低速で漕ぐと再発進する。
8. テンキー7で、15 km/hなら高度5 m、20 km/hなら10 mになり、飛行中はコース外へ移動できる。コース外でテンキー7を押すと飛行が継続し、コース上空へ戻ると地上モードへ切り替えられる。
9. 緑色の外枠を持つ不透明な54×30cmの計器が仮想自転車の正面に表示される。頭を右へ振ると計器は視界の左へ移るが、板自体はHMDへ追従回転せず、仮想自転車の進行方向に固定される。
10. 計器に速度、心拍、ケイデンス、パワー、距離、時刻、高度、モード、FPS、状態が表示される。心拍計がない場合もT2走行は継続する。
11. 操舵コントローラー追跡喪失、コース端、BLEエラーでは短時間のHMD前方警告が表示される。
12. P1〜P7を切り替え、成功応答後だけ表示P番号が変わる。
13. `Back to Real World`後、`Saved\arrietty_ride.csv`最終行が`BACK_TO_REAL_WORLD`である。
14. 10分走行し、画面の平均FPSが59.5以上で、VIVEの通常90 Hz動作ではSteamVR再投影が常態化せず、重大なhitchやクラッシュがない。
15. `Esc`または`Exit Arrietty`でT2接続が停止する。VR PreviewではPlayが終了してEditorへ戻り、パッケージ版ではプロセスが正常終了する。
16. 仮想視界の方向がずれた場合はテンキー`.`を押し、自転車のHeadingを変えず表示Offsetが0°付近へ戻る。

## 現在の環境上の注意

2026-08-28の統合実走では、開始Heading 0°、操舵Source `RightGrip`、最高22.08 km/h、距離809.223 m、
飛行高度最大12.08 m、Button 6の3.0%→0.0%、Button 1の2.01 m安全復帰をCSVで確認した。
SteamVRが`LeftGrip`へ割り当てる回の自動選択はコードとビルドで対応済みだが、実機割り当てが左になった回に再確認する。

2026-08-27のヘッドレス起動では、UE 5.8.2によるOpenXR 1.1の初回作成はSteamVR 2.16.7に拒否されましたが、エンジンがOpenXR 1.0へフォールバックし、インスタンス初期化は成功しました。NullRHIではHMD描画やコントローラー姿勢までは検証できないため、SteamVRを最新版にして実機試験を行ってください。
