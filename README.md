# Arrietty

Arrietty は、SteamVR/OpenXR HMD、CYCPLUS T2、ステムに固定したVIVEコントローラーを使い、Unreal EngineのLevel内を走行・人力飛行するWindows向けVRアプリです。従来のBlender Extension版v0.7.9をUnreal Engine 5.8.2のC++へ移植し、機能と操作を維持しながら60 FPSを目標にした描画構成へ変更しています。現在のArrietty UE版はv0.9.0です。

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
2. HMDとステム上のVIVEコントローラーを接続します。SteamVRが左／右のどちらに割り当ててもArriettyが追跡中の側を自動選択します。心拍計を使う場合は、標準BLE Heart Rate Service対応センサーも装着して広告状態にします。
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

ESP32有線操作盤を接続した場合はCOMポートを自動検出します。Button 1は走行開始／開始後の約2 m安全復帰、Button 2は地上・人力飛行モード、Button 6は押下中だけT2へ3%上り勾配を送るブレーキです。飛行中は右側のJoystick 2でエルロンとエレベーターを操作します。Button 3〜5、Joystick 1のX/Y/SW、Joystick 2のSWは受信・表示のみで、ゲーム内用途はまだ割り当てていません。USB接続直後の約1秒は中央校正のため両スティックへ触れないでください。

## 維持した機能

- T2のFTMS Indoor Bike Data通知による速度・ケイデンス・パワー
- 標準BLE Heart Rate Service `0x180D`／Heart Rate Measurement `0x2A37`による心拍数。見つからない場合もT2走行は継続
- FTMS Control Pointの制御権取得、通常勾配0%／Button 6押下中3%、風速0、Cw 0.51 kg/m、P1〜P7のCrr
- CSC実円盤停止判定、ケイデンス0かつ5 km/h以下の惰性停止
- SteamVRが左右どちらに割り当てても追跡中のOpenXRグリップを自動選択し、その最初の姿勢を操舵中央として校正
- Button 1／テンキー0では描画前Late Updateの影響を受けるCamera Component値ではなく、OpenXRの有効な最新HMD姿勢を取得して視界を走行方向へ絶対Yawで整列。姿勢未取得時は最大1秒再試行
- ホイールベース1.05 m、ゲイン50%、デッドゾーン1.5°、上限±15°
- 実測パワーを推進力へ変換する有効質量35 kg、滑空比30、プロペラ効率80%の人力飛行モデル
- 最良滑空速度24 km/h、離陸20 km/h、失速18 km/h、回復20.5 km/h。失速時は機首が下がり、降下して速度を回復する
- 飛行中はJoystick 2のYをエルロン（`+Y`左翼下がり、`-Y`右翼下がり）、Xをエレベーター（`+X`機首上げ、`-X`機首下げ）に使い、ハンドルをラダーに使う
- `SecretWorldRideSurface`タグを持つActorまたはComponentだけを走行面として使う下向きレイキャスト
- 仮想自転車の正面1.05m（105cm）、中心高1.10mに固定する54×30cmの不透明World Space VR計器。自動車のダッシュボードと同様に進行方向と一緒に動き、頭の向きには追従しない
- 速度、心拍、時刻、ケイデンス、パワー、距離、周回、高度、モード、XY、P番号、FPS、走行状態の表示
- BLEエラー、操舵追跡喪失、コース端、飛行切替を短時間だけHMD前方へ表示
- 固定名`Saved/arrietty_ride.csv`への上書きログ

飛行モードは`Numpad 7`またはButton 2で有効にします。20 km/h以上でJoystick 2のXを正側へ引くと離陸します。約95 Wで24 km/hの水平飛行となり、それ以上のパワーは上昇へ、少ないパワーは降下へ使われます。エレベーターでさらに上昇すると対気速度を失い、18 km/h未満で失速します。回復はXを負側へ押して機首を下げ、20.5 km/h以上へ戻します。着陸後まで飛行モードは継続し、空中で地上モードへ切り替えることはできません。コース外へ飛べますが、着陸と地上モードへの復帰には`SecretWorldRideSurface`上空へ戻る必要があります。

## 60 FPS設計

Forward Shading、4x MSAA、Instanced Stereo、Lumen/Virtual Shadow Maps/Motion Blur無効、動的解像度70〜100%を標準設定にしています。一般フレームレート制限はOpenXRのHMD同期を妨げないよう無効化し、VIVEの90 Hzに合わせた11.1 msフレーム予算を使います。起動画面では実測平均FPSを表示し、最低目標を60 FPSとします。実際のFPSはシーン、HMD解像度、GPU負荷に依存します。

## Blender世界の移行

既存`.blend`をランタイムで直接開くことはできません。`Tools/Export-BlenderWorlds.ps1`でGLBへ一括変換し、Unreal Editorでインポートします。詳しくは[`docs/WORLD_MIGRATION.md`](docs/WORLD_MIGRATION.md)を参照してください。`ArriettyDemo` Levelには`ArriettyWorldBuilder`を明示配置し、約2.6 kmの軽量オーバル、湖、集落、5個の飛行リングをC++から生成します。他のLevelにはデモ世界を生成しません。

## 検証状態

- UE 5.8.2 Editor: ビルド成功
- Win64 Development Game / Shipping: ビルド成功
- Visual Studio 2026ソリューション: 生成成功（ToolsVersion 18.0）
- UE Automation: 8テスト成功（人力飛行、ESP32シリアルプロトコル、心拍、FTMS、CSC、制御コマンド、走行規則、VR計器Widget内容）
- NullRHIゲーム起動: `ArriettyRuntime`、`ArriettyGameMode`、`ArriettyDemo` Levelのロード成功
- 別名の世界プロジェクト生成、Runtime更新、VS2026 Editorビルド: 成功
- Shipping cook/pak/archive: 成功、`Dist\Windows\Arrietty.exe`の応答確認済み
- T2、HMD、VIVEコントローラー、ESP32操作盤の統合実走: `RightGrip`自動選択、開始Heading 0°、809.223 m走行、飛行、3%ブレーキと0%解除、Button 1の2.01 m安全復帰、Esc終了を確認済み（2026-08-28）
- ESP32有線操作盤: 完成版書き込み、CLI 50 Hz通信、UEのCOM7自動検出・接続を確認済み（2026-08-28）

実機試験は[`docs/ACCEPTANCE_TEST.md`](docs/ACCEPTANCE_TEST.md)に沿って行います。

ESP32、6個の押しボタン、2個のXYジョイスティックを使うUSB有線操作盤の実配線、座標、通信仕様、CLI操作は[`docs/HARDWARE_CONTROLS.md`](docs/HARDWARE_CONTROLS.md)に記録しています。ファームウェアは[`Hardware/ArriettyController`](Hardware/ArriettyController)にあります。

## ライセンス

[MIT License](LICENSE)
