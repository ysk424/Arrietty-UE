# Arrietty UE architecture

最終更新: 2026-08-27

## 実行時構成

- `AArriettyPawn`: OpenXR HMD/右グリップ、テンキー、走行積算、操舵、飛行、走行面判定をゲームスレッドで処理する。
- `FArriettyBluetoothManager`: Windows Runtime BLEを専用ワーカースレッドで実行する。Unreal Objectへ触れず、MPSCキューだけでゲームスレッドへ通知する。
- `ArriettyTrainerProtocol`: FTMS/CSC解析、FTMS Control Pointコマンド、停止・飛行・操舵の純粋計算を保持する。
- `FArriettyRideLog`: `Saved/arrietty_ride.csv`を走行ごとに上書きし、各通知でflushする。
- `UArriettyControlWidget`: BlenderのNパネルに相当するデスクトップUIをC++だけで構築する。
- `UArriettyInstrumentWidget`: 黒背景を持たない緑・オレンジのVR空間計器。
- `AArriettyWorldBuilder`: 1回のHISM draw構成を中心に軽量テスト世界を作る。

## スレッド境界

BLEの探索、接続、GATT通知、Control Point応答待ちはワーカースレッドです。UnrealのActor、Component、Widget、World、CSVはゲームスレッドだけから操作します。ワーカーからのイベントには世代番号を付け、前回走行の遅延通知を破棄します。

## 座標

Arrietty表示座標は旧Blender版と同じく0°=+X、90°=+Y、Z-up、1 unit=1 mです。UEは+Yが右となるため、ワールド変換時だけ`UE Y = -Arrietty Y`、`UE Yaw = -Arrietty Heading`へ変換します。CSVとUIはArrietty座標を保ちます。

## 60 FPS

走行積算はフレームごとの`DeltaSeconds`を使い、0.25秒でクランプします。BLEは通知駆動、計器文字は10 Hz、姿勢と移動は描画フレームで更新します。路面は次の移動点だけをレイキャストし、テスト世界の道路・建物・リングはHISMでまとめています。
