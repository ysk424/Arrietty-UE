# Arrietty

Arrietty は、SteamVR/OpenXR HMD、CYCPLUS T2、ステムに固定した右VIVEコントローラーを使い、Unreal Engine内の世界を走行・飛行するWindows向けVRアプリです。従来のBlender Extension版v0.7.9をUnreal Engine 5.8.2のC++へ移植し、機能と操作を維持しながら60 FPSを目標にした描画構成へ変更しています。

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

その後、`Arrietty.uproject`または生成された`Arrietty.sln`を開きます。エディターのPlayで起動できます。パッケージ版は`Dist\Windows\Arrietty.exe`です。

## 操作

1. SteamVRを起動し、SteamVRをアクティブなOpenXRランタイムにします。
2. HMDと右VIVEコントローラーを接続します。心拍計を使う場合は、標準BLE Heart Rate Service対応センサーも装着して広告状態にします。
3. Arriettyを起動するとOpenXR VRが自動的に開始します。開始しない場合は、画面のVR状態を確認して`Dive into Secret World`を押します。
4. テンキー`4`/`6`で開始方向、`8`/`2`で開始位置を合わせます。
5. T2を数回漕いで起こします。
6. HMDで自転車の正面を向き、ハンドルを中央に保ってテンキー`0`または`Start Ride`を押します。この時点のHMD正面を自転車正面として再センターします。OpenXR座標が安定してからハンドル中央を記録するため、押した後も約1秒はハンドルを中央に保ちます。
7. 開始音の後に走行します。VRから戻る操作は`Back to Real World`、アプリ終了は`Esc`または`Exit Arrietty`です。

キー割り当ては従来版と同じです。

- `Numpad 8` / `2`: HMD正面へ前進 / 後退
- `Numpad 4` / `6`: 左 / 右旋回
- `Numpad 0`: T2接続と走行開始。走行中の再押下では止まりません
- `Numpad 7`: 地上 / 飛行モード
- `Numpad 1` / `3` / `5` / `9`: P1 / P2 / P3 / P4
- `Numpad +` / `-`: P5〜P7を含む抵抗プリセットの上下移動
- `Numpad .`: 現在向いているHMD方向を自転車正面として再調整
- `Esc`: ログとT2接続を終了してArriettyを閉じる

## 維持した機能

- T2のFTMS Indoor Bike Data通知による速度・ケイデンス・パワー
- 標準BLE Heart Rate Service `0x180D`／Heart Rate Measurement `0x2A37`による心拍数。見つからない場合もT2走行は継続
- FTMS Control Pointの制御権取得、勾配0%、風速0、Cw 0.51 kg/m、P1〜P7のCrr
- CSC実円盤停止判定、ケイデンス0かつ5 km/h以下の惰性停止
- 右OpenXRグリップの最初の姿勢を操舵中央として校正
- ホイールベース1.05 m、ゲイン50%、デッドゾーン1.5°、上限±15°
- 速度10 km/h超の1 km/hにつき高度1 mとなる飛行モード
- `SecretWorldRideSurface`タグを持つActorまたはComponentだけを走行面として使う下向きレイキャスト
- 自転車のステム付近に固定したWorld Space VR計器、右コントローラー位置の一度だけの校正
- 速度、心拍、時刻、ケイデンス、パワー、距離、周回、高度、モード、XY、P番号、FPS、走行状態の表示
- BLEエラー、操舵追跡喪失、コース端、飛行切替を短時間だけHMD前方へ表示
- 固定名`Saved/arrietty_ride.csv`への上書きログ

飛行モードは従来版と同じく`Numpad 7`で切り替え、10 km/hを超えた速度1 km/hにつき1 m上昇します。水平移動は地上・飛行とも`SecretWorldRideSurface`上に制限されます。コース端で停止するのはUnreal Engineの制限ではなく、Arriettyが従来版から維持している走行規則です。

## 60 FPS設計

Forward Shading、4x MSAA、Instanced Stereo、Lumen/Virtual Shadow Maps/Motion Blur無効、動的解像度70〜100%を標準設定にしています。一般フレームレート制限はOpenXRのHMD同期を妨げないよう無効化し、VIVEの90 Hzに合わせた11.1 msフレーム予算を使います。起動画面では実測平均FPSを表示し、最低目標を60 FPSとします。実際のFPSはシーン、HMD解像度、GPU負荷に依存します。

## Blender世界の移行

既存`.blend`をランタイムで直接開くことはできません。`Tools/Export-BlenderWorlds.ps1`でGLBへ一括変換し、Unreal Editorでインポートします。詳しくは[`docs/WORLD_MIGRATION.md`](docs/WORLD_MIGRATION.md)を参照してください。リポジトリの標準起動では、約2.6 kmの軽量オーバル、湖、集落、5個の飛行リングをC++から生成するため、外部アセットなしでも動作確認できます。

## 検証状態

- UE 5.8.2 Editor: ビルド成功
- Win64 Development Game / Shipping: ビルド成功
- Visual Studio 2026ソリューション: 生成成功（ToolsVersion 18.0）
- UE Automation: 5テスト成功（心拍、FTMS、CSC、制御コマンド、走行規則）
- NullRHIゲーム起動: `ArriettyGameMode`と`ArriettyWorld`マップのロード成功
- Shipping cook/pak/archive: 成功、`Dist\Windows\Arrietty.exe`の応答確認済み
- T2、HMD、右コントローラーを組み合わせた実機走行: 起動、正面整合、直進、Esc終了を確認済み（2026-08-27）

実機試験は[`docs/ACCEPTANCE_TEST.md`](docs/ACCEPTANCE_TEST.md)に沿って行います。

## ライセンス

[MIT License](LICENSE)
