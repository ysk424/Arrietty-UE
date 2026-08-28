# Arrietty UE architecture

最終更新: 2026-08-28

## 実行時構成

- `ArriettyWorldHost`: `.uproject`ごとの最小ゲームモジュール。プロジェクト名とVS2026 Targetだけを所有する。
- `ArriettyRuntime`: 世界プロジェクト間で更新・再ビルドできるMITライセンスのRuntimeプラグイン。
- `AArriettyPawn`: OpenXR HMD/左右自動選択グリップ、テンキー、走行積算、操舵、飛行、走行面判定をゲームスレッドで処理する。
- `FArriettyBluetoothManager`: Windows Runtime BLEを専用ワーカースレッドで実行する。Unreal Objectへ触れず、MPSCキューだけでゲームスレッドへ通知する。
- `FArriettySerialController`: COM1〜64からArrietty ESP32を`PING`で識別し、USBシリアルを専用ワーカースレッドで50 Hz受信する。切断時は自動再探索し、MPSCキューだけでゲームスレッドへ通知する。
- `ArriettyControllerProtocol`: `A1`状態パケットの構文、軸範囲、8-bitボタンmaskを検証する純粋パーサー。
- `ArriettyTrainerProtocol`: FTMS/CSC解析、FTMS Control Pointコマンド、停止・操舵、人力飛行のエネルギー・抗力・失速計算を保持する。
- `FArriettyRideLog`: `Saved/arrietty_ride.csv`を走行ごとに上書きし、各通知でflushする。
- `UArriettyControlWidget`: BlenderのNパネルに相当するデスクトップUIをC++だけで構築する。
- `UArriettyInstrumentWidget`: 黒背景を持たない緑・オレンジのVR空間計器。
- `AArriettyCourseStart`: Level内で自転車の開始位置と開始方向を示す`PlayerStart`。
- `AArriettyWorldBuilder`: `ArriettyDemo` Levelにだけ明示配置し、HISM中心の軽量テスト世界を作る。

## Level境界

ArriettyはEditorで現在開かれているLevelをそのまま走行します。`AArriettyGameMode`は`AArriettyCourseStart`を優先してPawnを生成し、PawnはそのUE座標と赤いX軸方向をArriettyの開始位置・Headingへ変換します。走行開始時はこのHeadingをHMDの物理的な向きで上書きせず、`VROrigin`だけを回して仮想視界を開始ライン方向へ合わせます。`AArriettyWorldBuilder`の自動生成は行わないため、通常のLevelへデモ道路や建物が混入しません。

世界プロジェクト固有の`.umap`、`.uasset`、`Config`は`Update-ArriettyWorldProject.ps1`の更新対象外です。更新されるのは`Plugins/ArriettyRuntime`の管理ファイルだけです。

HMD再センタリングはCamera Componentのゲームスレッド姿勢を使わず、`IXRTrackingSystem::GetCurrentPose`から有効な最新OpenXR追跡姿勢を取得する。追跡空間でのHMD正面をPawnローカル`+X`へ合わせる`VROrigin`相対Yawを絶対値で設定するため、過去の補正は累積しない。姿勢が無効なら50 ms間隔で最大20回再試行し、補正後のHMD正面と自転車正面の残差が1°以下になるまで走行開始を待つ。これによりOpenXRの描画前Late UpdateとゲームスレッドCamera姿勢の差による斜め向きを防ぐ。

## スレッド境界

BLEの探索、接続、GATT通知、Control Point応答待ちはワーカースレッドです。UnrealのActor、Component、Widget、World、CSVはゲームスレッドだけから操作します。ワーカーからのイベントには世代番号を付け、前回走行の遅延通知を破棄します。

ESP32のCOM探索と読取も専用ワーカースレッドです。ゲームスレッドの`AArriettyPawn`はキューを毎フレーム排出し、Button 1/2は立ち上がり、Button 6は押下と解放の両方を操作へ変換します。Button 1は開始後に直近の走行軌跡を約2 m巻き戻し、Button 6は押下中だけT2のFTMS勾配を3%にします。Button 3〜5、Joystick 1のX/Y/SW、Joystick 2のSWは状態へ保存してUIへ表示しますが、ゲーム内用途はまだ割り当てません。Joystick 2のX/Yは飛行中だけエレベーター／エルロンへ使います。ハンドル入力は地上で前輪操舵、飛行中はラダーになります。

## 人力飛行

飛行モードではT2の表示速度を移動速度にせず、測定パワーをプロペラ効率80%で推進力へ変換します。有効質量35 kg、滑空比30、最良滑空速度24 km/hの抗力曲線を使い、推進力・抗力・重力・昇降率のエネルギー収支から対気速度を積分します。約95 Wが24 km/hの水平飛行点です。Joystick 2 Xのエレベーターで上昇率を増すと対気速度が減り、機首下げでは高度を速度へ変換します。

20 km/h以上かつ機首上げ入力で離陸し、18 km/h未満で失速、機首下げかつ20.5 km/h以上で回復します。Joystick 2 Yは`+Y`左翼下がり／`-Y`右翼下がりのバンク目標となり、バンクによる旋回率とハンドルのラダー旋回率を合成します。空中で地上モードへ直接切り替えることはできず、`SecretWorldRideSurface`上へ着陸してから切り替えます。

## 座標

Arrietty表示座標は旧Blender版と同じく0°=+X、90°=+Y、Z-up、1 unit=1 mです。UEは+Yが右となるため、ワールド変換時だけ`UE Y = -Arrietty Y`、`UE Yaw = -Arrietty Heading`へ変換します。CSVとUIはArrietty座標を保ちます。

## 60 FPS

走行積算はフレームごとの`DeltaSeconds`を使い、0.25秒でクランプします。BLEは通知駆動、計器文字は10 Hz、姿勢と移動は描画フレームで更新します。路面は次の移動点だけをレイキャストし、テスト世界の道路・建物・リングはHISMでまとめています。
