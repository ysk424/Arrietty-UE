# Arrietty UE architecture

最終更新: 2026-08-28

## 実行時構成

- `ArriettyWorldHost`: `.uproject`ごとの最小ゲームモジュール。プロジェクト名とVS2026 Targetだけを所有する。
- `ArriettyRuntime`: 世界プロジェクト間で更新・再ビルドできるMITライセンスのRuntimeプラグイン。
- `AArriettyPawn`: OpenXR HMD/左右自動選択グリップ、テンキー、走行積算、操舵、飛行、走行面判定をゲームスレッドで処理する。
- `FArriettyBluetoothManager`: Windows Runtime BLEを専用ワーカースレッドで実行する。Unreal Objectへ触れず、MPSCキューだけでゲームスレッドへ通知する。
- `FArriettySerialController`: COM1〜64からArrietty ESP32を`PING`で識別し、USBシリアルを専用ワーカースレッドで50 Hz受信する。切断時は自動再探索し、MPSCキューだけでゲームスレッドへ通知する。
- `ArriettyControllerProtocol`: `A1`状態パケットの構文、軸範囲、8-bitボタンmaskを検証する純粋パーサー。
- `ArriettyTrainerProtocol`: FTMS/CSC解析、FTMS Control Pointコマンド、停止・飛行・操舵の純粋計算を保持する。
- `FArriettyRideLog`: `Saved/arrietty_ride.csv`を走行ごとに上書きし、各通知でflushする。
- `UArriettyControlWidget`: BlenderのNパネルに相当するデスクトップUIをC++だけで構築する。
- `UArriettyInstrumentWidget`: 黒背景を持たない緑・オレンジのVR空間計器。
- `AArriettyCourseStart`: Level内で自転車の開始位置と開始方向を示す`PlayerStart`。
- `AArriettyWorldBuilder`: `ArriettyDemo` Levelにだけ明示配置し、HISM中心の軽量テスト世界を作る。

## Level境界

ArriettyはEditorで現在開かれているLevelをそのまま走行します。`AArriettyGameMode`は`AArriettyCourseStart`を優先してPawnを生成し、PawnはそのUE座標と赤いX軸方向をArriettyの開始位置・Headingへ変換します。走行開始時はこのHeadingをHMDの物理的な向きで上書きせず、`VROrigin`だけを回して仮想視界を開始ライン方向へ合わせます。`AArriettyWorldBuilder`の自動生成は行わないため、通常のLevelへデモ道路や建物が混入しません。

世界プロジェクト固有の`.umap`、`.uasset`、`Config`は`Update-ArriettyWorldProject.ps1`の更新対象外です。更新されるのは`Plugins/ArriettyRuntime`の管理ファイルだけです。

## スレッド境界

BLEの探索、接続、GATT通知、Control Point応答待ちはワーカースレッドです。UnrealのActor、Component、Widget、World、CSVはゲームスレッドだけから操作します。ワーカーからのイベントには世代番号を付け、前回走行の遅延通知を破棄します。

ESP32のCOM探索と読取も専用ワーカースレッドです。ゲームスレッドの`AArriettyPawn`はキューを毎フレーム排出し、Button 1/2は立ち上がり、Button 6は押下と解放の両方を操作へ変換します。Button 1は開始後に直近の走行軌跡を約2 m巻き戻し、Button 6は押下中だけT2のFTMS勾配を3%にします。Button 3〜5と2台のJoystickのX/Y/SWは状態へ保存してUIへ表示しますが、ゲーム内用途はまだ割り当てません。走行中のカーブはハンドル操舵を維持します。

## 座標

Arrietty表示座標は旧Blender版と同じく0°=+X、90°=+Y、Z-up、1 unit=1 mです。UEは+Yが右となるため、ワールド変換時だけ`UE Y = -Arrietty Y`、`UE Yaw = -Arrietty Heading`へ変換します。CSVとUIはArrietty座標を保ちます。

## 60 FPS

走行積算はフレームごとの`DeltaSeconds`を使い、0.25秒でクランプします。BLEは通知駆動、計器文字は10 Hz、姿勢と移動は描画フレームで更新します。路面は次の移動点だけをレイキャストし、テスト世界の道路・建物・リングはHISMでまとめています。
