# Arrietty UE architecture

最終更新: 2026-08-29

## 実行時構成

- `ArriettyWorldHost`: `.uproject`ごとの最小ゲームモジュール。プロジェクト名とVS2026 Targetだけを所有する。
- `ArriettyRuntime`: 世界プロジェクト間で更新・再ビルドできるMITライセンスのRuntimeプラグイン。
- `ArriettyCesium`: Cesium依存をRuntime本体から隔離し、WGS84移動、Globe Anchor、Origin Shift、地球用GameMode／CourseStartを提供するproject plugin。
- `AArriettyPawn`: OpenXR HMD/左右自動選択グリップ、テンキー、走行積算、操舵、飛行、走行面判定をゲームスレッドで処理する。
- `FArriettyBluetoothManager`: Windows Runtime BLEを専用ワーカースレッドで実行する。Unreal Objectへ触れず、MPSCキューだけでゲームスレッドへ通知する。
- `FArriettySerialController`: COM1〜64からArrietty ESP32を`PING`で識別し、USBシリアルを専用ワーカースレッドで50 Hz受信する。切断時は自動再探索し、MPSCキューだけでゲームスレッドへ通知する。
- `ArriettyControllerProtocol`: `A1`状態パケットの構文、軸範囲、8-bitボタンmaskを検証する純粋パーサー。
- `ArriettyTrainerProtocol`: FTMS/CSC解析、FTMS Control Pointコマンド、停止・操舵、人力飛行のエネルギー・抗力・失速計算を保持する。
- `FArriettyRideLog`: `Saved/arrietty_ride.csv`を走行ごとに上書きし、各通知でflushする。
- `UArriettyControlWidget`: BlenderのNパネルに相当するデスクトップUIをC++だけで構築する。
- `UArriettyInstrumentWidget`: 人工水平儀、飛行角、速度警告、実測／推進パワー、地理座標を表示する緑・オレンジのVR空間計器。
- `AArriettyCourseStart`: Level内で自転車の開始位置と開始方向を示す`PlayerStart`。
- `AArriettyWorldBuilder`: `ArriettyDemo` Levelにだけ明示配置し、HISM中心の軽量テスト世界を作る。

## Level境界

ArriettyはEditorで現在開かれているLevelをそのまま走行します。`AArriettyGameMode`は`AArriettyCourseStart`を優先してPawnを生成し、PawnはそのUE座標と赤いX軸方向をArriettyの開始位置・Headingへ変換します。走行開始時はこのHeadingをHMDの物理的な向きで上書きせず、`VROrigin`だけを回して仮想視界を開始ライン方向へ合わせます。`AArriettyWorldBuilder`の自動生成は行わないため、通常のLevelへデモ道路や建物が混入しません。

世界プロジェクト固有の`.umap`、`.uasset`、`Config`は`Update-ArriettyWorldProject.ps1`の更新対象外です。更新されるのは`Plugins/ArriettyRuntime`の管理ファイルだけです。

HMD再センタリングはCamera Componentのゲームスレッド姿勢を使わず、`IXRTrackingSystem::GetCurrentPose`から有効な最新OpenXR追跡姿勢を取得する。追跡空間でのHMD正面をPawnローカル`+X`へ合わせる`VROrigin`相対Yawを絶対値で設定するため、過去の補正は累積しない。姿勢が無効なら50 ms間隔で最大20回再試行し、補正後のHMD正面と自転車正面の残差が1°以下になるまで走行開始を待つ。これによりOpenXRの描画前Late UpdateとゲームスレッドCamera姿勢の差による斜め向きを防ぐ。

## スレッド境界

BLEの探索、接続、GATT通知、Control Point応答待ちはワーカースレッドです。UnrealのActor、Component、Widget、World、CSVはゲームスレッドだけから操作します。ワーカーからのイベントには世代番号を付け、前回走行の遅延通知を破棄します。

ESP32のCOM探索と読取も専用ワーカースレッドです。ゲームスレッドの`AArriettyPawn`はキューを毎フレーム排出し、Button 1/2は立ち上がり、Button 5/6は押下と解放の両方を操作へ変換します。Button 1は開始後に直近の走行軌跡を約2 m巻き戻し、Button 5はUDPでWindows音声ブリッジへPTT edgeを送り、Button 6は押下中だけT2のFTMS勾配を3%にします。Joystick 2は0.45の開始閾値と0.20の復帰閾値を持つedge検出で、倒しっぱなしを反復させず1ジェスチャーを1°の目標変更へ変換します。Button 3/4は80 msの同時押し判定を通し、単独では左／右ロール、同時では機首上げです。Joystick 2 SWはピッチ／右ロール目標を0°へ戻します。ハンドル入力は地上で前輪操舵、飛行中は従来どおりラダーになります。

## 人力飛行

飛行モードではT2の表示速度を移動速度にせず、測定パワーをプロペラ効率80%で推進力へ変換します。有効質量35 kg、滑空比30、最良滑空速度24 km/hの抗力曲線を使い、推進力・抗力・重力・昇降率のエネルギー収支から対気速度を積分します。約95 Wが24 km/hの水平飛行点です。積算されたピッチ目標で上昇率を増すと対気速度が減り、機首下げでは高度を速度へ変換します。

20 km/h以上かつ正の機首上げ目標で離陸し、18 km/h未満で失速、機首下げ目標かつ20.5 km/h以上で回復します。公開するロール目標は右を正とし、既存飛行モデルの内部エルロン符号へ変換して、バンク旋回率とハンドルのラダー旋回率を合成します。空中で地上モードへ直接切り替えることはできず、`SecretWorldRideSurface`上へ着陸してから切り替えます。

ピッチ／バンクの目標角は1°刻みで安全上限±12°／±25°を維持し、そこへ到達する角速度だけを`(airspeed / 24 km/h)^2`で変える。通常の操舵効率は0.20〜1.75へclampし、失速中は25%へ落とす。FPAは対気速度と鉛直速度から求め、AoAは`Pitch - FPA`として計器とCSVへ出す。エンジンブーストは廃止し、推進力は常にT2の実測パワーから求める。

## Codex音声ブリッジ

UnrealはButton 5の押下／解放をloopback UDP 49000へ送るだけで、マイク、APIキー、WSLプロセスへ直接触れません。Windowsの`ArriettyVoiceBridge`が16 kHz mono WAV録音、OpenAI文字起こし、`wsl.exe`経由の`tmux load-buffer`／`paste-buffer`／`send-keys Enter`、回答WAVの再生を担当します。Codex回答はユーザー設定の`notify`で受け、起動済みで設定を再読込できない現セッションだけsession JSONL監視をfallbackとして使います。PTTから送った質問の次の最終回答だけを読み上げます。

## Cesium座標

`UArriettyNavigationComponent`は非平面世界用の拡張境界であり、通常世界では従来変換へfallbackする。`UArriettyCesiumNavigationComponent`は前フレームのGlobe Anchor ECEF位置に、現在地のENU接平面で得たEast/North差分を加え、新しいlongitude/latitudeへ変換する。高さとEast-South-Up姿勢はGlobe Anchorへ適用し、`UCesiumOriginShiftComponent`がPawn近傍へUE原点を移す。これによりArrietty内部の距離積算をメートルのまま維持しながら世界を周回できる。

## 座標

Arrietty表示座標は旧Blender版と同じく0°=+X、90°=+Y、Z-up、1 unit=1 mです。UEは+Yが右となるため、ワールド変換時だけ`UE Y = -Arrietty Y`、`UE Yaw = -Arrietty Heading`へ変換します。CSVとUIはArrietty座標を保ちます。

## 60 FPS

走行積算はフレームごとの`DeltaSeconds`を使い、0.25秒でクランプします。BLEは通知駆動、計器文字は10 Hz、姿勢と移動は描画フレームで更新します。路面は次の移動点だけをレイキャストし、テスト世界の道路・建物・リングはHISMでまとめています。
