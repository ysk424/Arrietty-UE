# Arrietty

Arrietty は、SteamVR/OpenXR HMD、CYCPLUS T2、ステムに固定したVIVEコントローラーを使い、Unreal EngineのLevel内を走行・人力飛行するWindows向けVRアプリです。従来のBlender Extension版v0.7.9をUnreal Engine 5.8.2のC++へ移植し、機能と操作を維持しながら60 FPSを目標にした描画構成へ変更しています。現在のArrietty UE版はv0.13.0です。

## 確認済み開発環境

- Unreal Engine 5.8.2（Launcher版）
- Visual Studio 2026 Community / VS 18.0以降
- Windows SDK 10.0.26100.0
- OpenXR / SteamVR
- Windows x64

UE 5.8はVS 2026を一般開発向けに推奨しています。本プロジェクトはVS 2026形式のソリューションを生成し、インストール済みの最新リリース版MSVC（検証環境では14.51）を選択します。

## ビルド

PowerShellで次を実行します。

```powershell
cd C:\Users\azoo\git\Arrietty-UE
.\Tools\Generate-VS2026.ps1
.\Tools\Build-Arrietty.ps1 -Target Editor
.\Tools\Test-Arrietty.ps1
.\Tools\Package-Arrietty.ps1
```

その後、`Arrietty.uproject`または生成された`Arrietty.sln`を開きます。`Content/Worlds/ArriettyDemo/ArriettyDemo`を開き、Playメニューを一度`VR Preview`に設定してPlayします。日常の世界制作と走行にパッケージは不要です。`Dist\Windows\Arrietty.exe`は性能確認や配布確認が必要な場合だけ作ります。

## 世界プロジェクトを作る

ArriettyのC++、BLE、OpenXR、Pawn、計器は`Plugins/ArriettyRuntime`に分離されています。世界は別のUEプロジェクトとして作成できます。

```powershell
cd C:\Users\azoo\git\Arrietty-UE
pwsh -File .\Tools\New-ArriettyWorldProject.ps1 -Name MyForest
```

既定では`C:\Users\azoo\Documents\Unreal Projects\MyForest\MyForest.uproject`が作成され、VS2026のEditorビルドまで実行されます。`.uproject`を開いて既存Levelを編集するか、新しいLevelを`Content/Worlds`へ作成します。

走行開始位置には`ArriettyCourseStart`を置き、Actorの赤いX軸を進行方向へ向けます。地上走行させる道路にはActor TagまたはComponent Tagとして`SecretWorldRideSurface`を付け、Visibility Collisionを有効にします。詳しくは[`docs/WORLD_PROJECTS.md`](docs/WORLD_PROJECTS.md)を参照してください。

## 世界プロジェクトのC++を更新する

最初にこのリポジトリを`git pull`し、UE Editorを閉じてから実行します。

```powershell
pwsh -File .\Tools\Update-ArriettyWorldProject.ps1 `
  -Project "C:\Users\azoo\Documents\Unreal Projects\MyForest\MyForest.uproject"
```

更新対象は世界プロジェクト内の`Plugins/ArriettyRuntime`だけです。世界の`Content`とプロジェクト固有の`Config`は変更せず、更新後にVS2026プロジェクトを再生成してEditorターゲットを再ビルドします。DLLを手作業でコピーする必要はありません。

## 操作

1. SteamVRを起動し、SteamVRをアクティブなOpenXRランタイムにします。UE Editorでは走行するLevelを開き、Playメニューから`VR Preview`を選びます。
2. HMDとステム上のVIVEコントローラーを接続します。SteamVRが左／右のどちらに割り当ててもArriettyが追跡中の側を自動選択します。Garminを使う場合は、時計の`心拍転送／Broadcast Heart Rate`をBLEで開始してから走行を開始します。
3. Arriettyを起動するとOpenXR VRが自動的に開始します。開始しない場合は、画面のVR状態を確認して`Dive into Secret World`を押します。
4. テンキー`4`/`6`で開始方向、`8`/`2`で開始位置を合わせます。
5. T2を数回漕いで起こします。
6. HMDで実際の自転車の正面をまっすぐ見て、ハンドルを中央に保った状態でテンキー`0`またはButton 1／`Start Ride`を押します。その瞬間の最新OpenXR HMD姿勢を仮想走行方向へ合わせます。仮想自転車の進行方向自体はHMDの向きで上書きせず、`ArriettyCourseStart`の赤いX方向を維持します。OpenXR姿勢と操舵中央が安定するまで、押した後も約1秒は正面を見てハンドルを中央に保ちます。
7. 開始音の後に走行します。VRから戻る操作は`Back to Real World`です。`VR Preview`では`Esc`または`Exit Arrietty`でPlayを終了してUE Editorへ戻ります。パッケージ版ではArriettyのプロセスを終了します。

キー割り当ては従来版と同じです。

- `Numpad 8` / `2`: HMD正面へ前進 / 後退
- `Numpad 4` / `6`: 左 / 右旋回
- `Numpad 0`: T2接続と走行開始。走行中の再押下は走行軌跡を約2 m戻る安全復帰
- `Numpad 7`: 地上 / 飛行モード
- `Numpad 1` / `3` / `5` / `9`: P1 / P2 / P3 / P4
- `Numpad +` / `-`: P5〜P7を含む抵抗プリセットの上下移動
- `Numpad .`: 自転車の走行方向を変えず、現在のHMD仮想視界を走行方向へ再調整
- `Esc`: ログとT2接続を終了する。VR PreviewではEditorへ戻り、パッケージ版ではArriettyを閉じる

ESP32有線操作盤を接続した場合はCOMポートを自動検出します。Button 1は走行開始／開始後の約2 m安全復帰、Button 2は地上・人力飛行モード、Button 5はCodexとのPTT、Button 6は押下中だけT2へ3%上り勾配を送るブレーキです。飛行中のJoystick 2は、中央から1回倒して戻すたびにピッチまたは右ロール目標を1°ずつ増減し、SWで両方を0°へ戻します。Button 3は左ロール1°、Button 4は右ロール1°、80 ms以内のButton 3+4同時押しは機首上げ1°です。ハンドルは従来どおりラダーです。Joystick 1 SWは飛行調整を開始／確定し、Xを左右へ1回倒して戻すたびに選択中の値を1段階だけ減少／増加します。USB接続直後の約1秒は中央校正のため両スティックへ触れないでください。

PTTを使う日は、Codexが動作しているWSL/tmuxペインから最初に`./Tools/start-arrietty-voice-bridge.sh`を1回実行します。Button 5を押している間だけ自転車側のVIVEマイクを録音し、離すと日本語へ文字起こしして同じCodexセッションへEnter付きで送ります。次の最終回答だけをAI音声でPCスピーカーから読み上げます。Windowsユーザー環境変数`OPENAI_API_KEY`とFFmpegが必要です。詳細は[`Tools/ArriettyVoiceBridge/README.md`](Tools/ArriettyVoiceBridge/README.md)を参照してください。

## 維持した機能

- T2のFTMS Indoor Bike Data通知による速度・ケイデンス・パワー
- Garminを含む標準BLE Heart Rate Service `0x180D`／Heart Rate Measurement `0x2A37`による心拍数。T2検出後も10秒間探索し、見つからない場合もT2走行は継続
- ESP32-IR（`192.168.4.1:4210`）へ走行速度に応じた`LEVEL 0..6`をUDP送信し、0〜30 km/hで風量を自動調整
- FTMS Control Pointの制御権取得、通常勾配0%／Button 6押下中3%、風速0、Cw 0.51 kg/m、P1〜P7のCrr
- CSC実円盤停止判定、ケイデンス0かつ5 km/h以下の惰性停止
- SteamVRが左右どちらに割り当てても追跡中のOpenXRグリップを自動選択し、その最初の姿勢を操舵中央として校正
- Button 1／テンキー0では描画前Late Updateの影響を受けるCamera Component値ではなく、OpenXRの有効な最新HMD姿勢を取得して視界を走行方向へ絶対Yawで整列。姿勢未取得時は最大1秒再試行
- ホイールベース1.05 m、ゲイン50%、デッドゾーン1.5°、上限±15°
- 実測パワーを推進力へ変換する有効質量35 kg、翼面積15 m2、滑空比30、プロペラ効率80%の人力飛行モデル。ピッチと飛行経路角から迎角・揚力・誘導抗力を計算し、ロール時は揚力の鉛直／旋回成分を分ける
- 最良滑空速度24 km/h、離陸20 km/h、失速18 km/h、回復20.5 km/h。失速時は機首が下がり、降下して速度を回復する
- Joystick 2は倒し量に依存せず、中央から倒して戻す1ジェスチャーを1°のピッチ／右ロール目標変更に変換する。SWで両目標を0°へ戻し、ハンドルはラダーに使う
- Button 3は左ロール1°、Button 4は右ロール1°、Button 3+4同時押しは機首上げ1°としてJoystick 2を補助する
- Joystick 1で固定推進力、正の余剰出力に対するパワーブースト、ピッチ応答、ロール応答を一項目ずつ調整する飛行チューニング。パワーブーストの既定値はx10で、調整中は既定95 Wの固定推進力を使う
- 24 km/hを操舵基準速度とし、対気速度の二乗に応じてピッチ／バンクの到達速度を変える。最大角±12°／±25°は維持し、低速で鈍く、高速で速くなる
- Button 5 PTTから`gpt-4o-mini-transcribe`、WSL/tmux Codex、`gpt-4o-mini-tts`をつなぐ走行中の短文音声対話
- `SecretWorldRideSurface`タグを持つActorまたはComponentだけを走行面として使う下向きレイキャスト
- 仮想自転車の正面1.25m（従来より20cm前）、中心高1.30m（従来より20cm上）に固定する54×30cmの不透明World Space VR計器。自動車のダッシュボードと同様に進行方向と一緒に動き、頭の向きには追従しない
- 対気速度を主表示する人工水平儀、心拍、スタートからの経過時間、ケイデンス、実測／推進パワー、距離、対地高度`ALT AGL`、昇降率、Heading、Pitch、Bank、FPA、AoA、操舵効率、失速／過速度、楕円体高を含む地理座標の表示
- BLEエラー、操舵追跡喪失、コース端、飛行切替を短時間だけHMD前方へ表示
- 固定名`Saved/arrietty_ride.csv`への上書きログ

飛行モードは`Numpad 7`またはButton 2で有効にします。エネルギー計算上の20 km/h以上でJoystick 2を手前へ1回以上動かして機首上げ目標を作ると離陸します。既定の飛行速度倍率は3.0で、計器表示と世界移動は24 km/hを72 km/hとして扱います。約95 Wが水平飛行点です。140 Wでは正の余剰出力だけを既定10倍で上昇へ変換し、中立姿勢の目安は約1.04 m/s、300 mまで約5分です。出力不足や0 W時の沈下は上昇倍率で増幅しないため、獲得高度から長く滑空できます。機首上げ目標を増やすとエネルギー速度を失い、同速度18 km/h未満で失速します。回復は前へ倒して機首下げ目標にし、20.5 km/h以上へ戻します。着陸後まで飛行モードは継続し、空中で地上モードへ切り替えることはできません。コース外へ飛べますが、着陸と地上モードへの復帰には`SecretWorldRideSurface`上空へ戻る必要があります。

## Google Earth / Cesium

`Plugins/ArriettyCesium`は既存の平面世界からCesium依存を隔離し、Globe AnchorとOrigin ShiftでWGS84上を連続飛行できるようにします。Funafuti開始点の`/Game/Worlds/ArriettyEarth/ArriettyEarth`、Google Photorealistic 3D Tiles、可視100 m滑走路と不可視2 km rollout面、認証境界は[`docs/CESIUM_EARTH.md`](docs/CESIUM_EARTH.md)を参照してください。初回Level作成時の手順は[`docs/NEOSTACK_EARTH_SETUP.md`](docs/NEOSTACK_EARTH_SETUP.md)へ残しています。

## 60 FPS設計

Forward Shading、4x MSAA、Instanced Stereo、Lumen/Virtual Shadow Maps/Motion Blur無効、動的解像度70〜100%を標準設定にしています。一般フレームレート制限はOpenXRのHMD同期を妨げないよう無効化し、VIVEの90 Hzに合わせた11.1 msフレーム予算を使います。起動画面では実測平均FPSを表示し、最低目標を60 FPSとします。実際のFPSはシーン、HMD解像度、GPU負荷に依存します。

## Blender世界の移行

既存`.blend`をランタイムで直接開くことはできません。`Tools/Export-BlenderWorlds.ps1`でGLBへ一括変換し、Unreal Editorでインポートします。詳しくは[`docs/WORLD_MIGRATION.md`](docs/WORLD_MIGRATION.md)を参照してください。`ArriettyDemo` Levelには`ArriettyWorldBuilder`を明示配置し、約2.6 kmの軽量オーバル、湖、集落、5個の飛行リングをC++から生成します。他のLevelにはデモ世界を生成しません。

## 検証状態

- v0.13.0 UE 5.8.2 Editor / Win64 Development Game: ビルド成功
- v0.13.0: Garmin心拍、ESP32-IR風量制御、経過時間表示、実対気速度表示、揚力／ロール物理を統合
- Visual Studio 2026ソリューション: 生成成功（ToolsVersion 18.0）
- UE Automation: 11テスト成功（飛行チューニング、デジタル操舵、人力飛行、ESP32シリアルプロトコル、ESP32-IR風量マッピング、心拍、FTMS、CSC、制御コマンド、走行規則、VR計器Widget内容）
- NullRHIゲーム起動: `ArriettyRuntime`、`ArriettyGameMode`、`ArriettyDemo` Levelのロード成功
- 別名の世界プロジェクト生成、Runtime更新、VS2026 Editorビルド: 成功
- v0.9.0 Shipping cook/pak/archive: 成功、`Dist\Windows\Arrietty.exe`の応答確認済み。v0.10.0 Earth Levelは作成・保存済みで、Earthを含むcook/packageは未実施
- T2、HMD、VIVEコントローラー、ESP32操作盤の統合実走: `RightGrip`自動選択、開始Heading 0°、809.223 m走行、飛行、3%ブレーキと0%解除、Button 1の2.01 m安全復帰、Esc終了を確認済み（2026-08-28）
- Earth Level通常PIE: Google地表、空、可視滑走路、Joystick 2上下左右を確認。旧100 m端への衝突対策として不可視2 km rollout面を追加し、Editor再ビルドとPIE smoke test成功（2026-08-29）
- ESP32有線操作盤: 製品版書き込み、CLI 50 Hz通信、4軸全範囲、Button 1-6と両SWを確認。Button 5 PTTからVIVEマイク、Codex応答、GPT TTS、PCスピーカーまで実機合格（2026-08-30）

実機試験は[`docs/ACCEPTANCE_TEST.md`](docs/ACCEPTANCE_TEST.md)に沿って行います。

ESP32、6個の押しボタン、2個のXYジョイスティックを使うUSB有線操作盤の実配線、座標、通信仕様、CLI操作は[`docs/HARDWARE_CONTROLS.md`](docs/HARDWARE_CONTROLS.md)に記録しています。ファームウェアは[`Hardware/ArriettyController`](Hardware/ArriettyController)にあります。

## ライセンス

[MIT License](LICENSE)
