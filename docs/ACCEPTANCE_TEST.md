# 実機受入試験

最終更新: 2026-08-30

## 前提

- SteamVR/OpenXRランタイムを最新版にする。
- HTC VIVE HMD、ステム固定コントローラー`LHR-9EFF8645`、CYCPLUS T2を起動する。コントローラーのSteamVR左右割り当てはどちらでもよい。心拍試験では標準BLE Heart Rate Service対応センサーも広告状態にする。
- Windows Bluetoothで他のトレーニングアプリがT2を使用していないことを確認する。
- `Arrietty.uproject`を開き、`ArriettyDemo` Levelを選び、Play方式を`VR Preview`にする。
- Windowsユーザー環境変数`OPENAI_API_KEY`を設定し、Codexが動くWSL/tmuxペインで`./Tools/start-arrietty-voice-bridge.sh`を実行する。`Arrietty Voice Bridge started`を確認する。

## ESP32有線操作盤

> 2026-08-29にButton 5の半田切れを修理した。次回はUSB電源を切ってコネクターとGNDを確認してから再接続し、Button 5のbutton maskとPTT edgeを最初に再試験する。

1. 両ジョイスティックを中央に置いてUSBケーブルを接続し、約1秒触れない。
2. `pwsh -File .\Tools\ESP32-Controller.ps1 -Action Test`が`ARRIETTY CONTROLLER TEST PASSED`になることを確認する。
3. Arrietty起動後、デスクトップUIの`ESP32 Wired Controller`が`CONNECTED: COMx at 115200 bps`になることを確認する。
4. 未開始時のButton 1で走行開始し、開始後に2 m以上走ってもう一度押すと、道路中央を推測せず実走軌跡上の約2 m前の位置・向きへ戻ることを確認する。
5. Button 2を押して地上→人力飛行へ切り替わることを確認する。空中では再押下しても地上モードにならず、着陸後は地上モードへ戻せることを確認する。
6. Button 6を押している間、T2 ControlとVR計器が`BRAKE 3.0%`／`BRK 3.0%`となって抵抗が増え、離してから約1秒以内に`FLAT`／`BRK 0.0%`へ戻ることを確認する。
7. 飛行中にButton 3を単独で押すたび左ロール目標が1°、Button 4を単独で押すたび右ロール目標が1°進むことを確認する。Button 3+4をほぼ同時に押すとロール目標を変えず、機首上げ目標だけが1°進むことを確認する。
8. Joystick 1/2の各X/Yを動かし、UIの生値が中央付近の0から正負へ変化することを確認する。飛行中はJoystick 2を1回倒して中央へ戻すたび、倒し量に関係なく該当方向のピッチ／右ロール目標が1°だけ変わること、倒しっぱなしでは反復しないこと、地上モードでは目標が変わらないことを確認する。
9. Joystick 1/2のSWを押してbutton maskのbit 6/7を確認する。Joystick 2 SWでピッチ／右ロール目標が両方0°へ戻り、Joystick 1 SWはゲーム操作を起こさないことを確認する。
10. Button 5を押すと`PTT LISTENING`となり、話して離すと`PTT SENT TO CODEX`になることを確認する。VIVEマイクの文字起こしが現在のtmux Codexペインへ入りEnterが送られ、次の最終回答だけがPCスピーカーからAI音声で再生されることを確認する。

## 試験

1. VR Preview開始時に両眼表示が自動開始する。`Back to Real World`でデスクトップ表示へ戻り、`Dive into Secret World`で再開する。
2. テンキー8/2がHMD正面、4/6が保存済み進行方向を動かす。
3. 実際の自転車の正面をHMDでまっすぐ見て、ハンドル中央でテンキー0またはButton 1を押し、そのまま1秒保つ。仮想視界とBikeのUE Forwardが`ArriettyCourseStart`の赤いX方向で1°以内に一致し、T2制御が`FLAT P5`になって開始音が鳴る。Steering Sourceは`LeftGrip`または`RightGrip`となり、直進時のRaw/Applied操舵角が0°付近になる。ログに`View aligned from live OpenXR pose`と`residual`が記録され、残差が±1°以内であることを確認する。
4. 20 km/hで1秒走り、仮想距離が約5.56 m増える。
5. 顔を左右へ向けても進路は変わらず、ハンドルだけで曲がる。
6. コントローラー追跡を隠すと移動が止まり、復帰すると再開する。
7. 惰性で5 km/h以下かCSC円盤停止になると表示速度と移動が0になる。低速で漕ぐと再発進する。
8. テンキー7またはButton 2で人力飛行モードにし、20 km/h以上で機首上げ目標を作ると離陸する。24 km/hを基準に、低速ではピッチ／バンクの到達が鈍く、高速では速くなるが最大ピッチ±12°／最大バンク±25°を超えない。ハンドルは地上の前輪操舵ではなく飛行中のラダーとしてヨーを動かす。18 km/h未満で`STALL`警告が出て、機首下げ目標かつ20.5 km/h以上にすると回復する。コース外へ飛べるが、空中で地上モードへ切り替えられず、`SecretWorldRideSurface`上へ着陸後に切り替えられる。
9. 緑色の外枠を持つ不透明な54×30cmの計器が仮想自転車の正面1.25 m、中心高1.30 mに表示される。頭を右へ振ると計器は視界の左へ移るが、板自体はHMDへ追従回転せず、仮想自転車の進行方向に固定される。
10. 計器に対気速度を主表示する人工水平儀、心拍、ケイデンス、実測／推進パワー、距離、時刻、高度、昇降率、Heading、Pitch、Bank、FPA、AoA、操舵効率、失速／過速度状態が表示される。Cesium Levelではlongitude、latitude、ellipsoid heightも表示される。心拍計がない場合もT2走行は継続する。
11. 操舵コントローラー追跡喪失、コース端、BLEエラーでは短時間のHMD前方警告が表示される。
12. P1〜P7を切り替え、成功応答後だけ表示P番号が変わる。
13. `Back to Real World`後、`Saved\arrietty_ride.csv`最終行が`BACK_TO_REAL_WORLD`である。
14. 10分走行し、画面の平均FPSが59.5以上で、VIVEの通常90 Hz動作ではSteamVR再投影が常態化せず、重大なhitchやクラッシュがない。
15. `Esc`または`Exit Arrietty`でT2接続が停止する。VR PreviewではPlayが終了してEditorへ戻り、パッケージ版ではプロセスが正常終了する。
16. 仮想視界の方向がずれた場合はテンキー`.`を押し、自転車のHeadingを変えず表示Offsetが0°付近へ戻る。

## 現在の環境上の注意

2026-08-28のv0.8.0統合実走では、開始Heading 0°、操舵Source `RightGrip`、最高22.08 km/h、距離809.223 m、
旧速度連動飛行の高度最大12.08 m、Button 6の3.0%→0.0%、Button 1の2.01 m安全復帰をCSVで確認した。
v0.11.0のButton 5 PTTは、実機Button 5、VIVEマイク、Codex応答、GPT TTS、PCスピーカーの往復で合格済み。1°デジタル操舵、Button 3/4補助、パネル位置変更はAutomation 9件、Editorビルドまで確認済みであり、実機HMDで再確認する。
SteamVRが`LeftGrip`へ割り当てる回の自動選択はコードとビルドで対応済みだが、実機割り当てが左になった回に再確認する。

2026-08-29のEarth Level部分試験では、Google地表と空、旧Joystick 2アナログ操舵、テンキー開始／飛行モードを確認した。旧100 m滑走路端で止まったため、CourseStartに不可視2 km rollout面を追加し、Editorビルドと通常PIE smoke testに成功した。追加後のHMD＋T2実走とv0.11.0入力／PTTは未実施である。

2026-08-27のヘッドレス起動では、UE 5.8.2によるOpenXR 1.1の初回作成はSteamVR 2.16.7に拒否されましたが、エンジンがOpenXR 1.0へフォールバックし、インスタンス初期化は成功しました。NullRHIではHMD描画やコントローラー姿勢までは検証できないため、SteamVRを最新版にして実機試験を行ってください。
