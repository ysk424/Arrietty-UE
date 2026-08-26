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
2. HMDと右VIVEコントローラーを接続します。
3. `Dive into Secret World`を押します。
4. テンキー`4`/`6`で開始方向、`8`/`2`で開始位置を合わせます。
5. T2を数回漕いで起こします。
6. ハンドルを中央に保ち、テンキー`0`または`Start Ride`を押します。
7. 開始音の後に走行します。終了は`Back to Real World`です。

キー割り当ては従来版と同じです。

- `Numpad 8` / `2`: HMD正面へ前進 / 後退
- `Numpad 4` / `6`: 左 / 右旋回
- `Numpad 0`: T2接続と走行開始。走行中の再押下では止まりません
- `Numpad 7`: 地上 / 飛行モード
- `Numpad 1` / `3` / `5` / `9`: P1 / P2 / P3 / P4
- `Numpad +` / `-`: P5〜P7を含む抵抗プリセットの上下移動

## 維持した機能

- T2のFTMS Indoor Bike Data通知による速度・ケイデンス・パワー
- FTMS Control Pointの制御権取得、勾配0%、風速0、Cw 0.51 kg/m、P1〜P7のCrr
- CSC実円盤停止判定、ケイデンス0かつ5 km/h以下の惰性停止
- 右OpenXRグリップの最初の姿勢を操舵中央として校正
- ホイールベース1.05 m、ゲイン50%、デッドゾーン1.5°、上限±15°
- 速度10 km/h超の1 km/hにつき高度1 mとなる飛行モード
- `SecretWorldRideSurface`タグを持つActorまたはComponentだけを走行面として使う下向きレイキャスト
- 緑・オレンジの透明背景VR計器、右コントローラー位置の一度だけの校正
- 速度、時刻、ケイデンス、パワー、距離、周回、高度、モード、XY、P番号の表示
- 固定名`Saved/arrietty_ride.csv`への上書きログ

## 60 FPS設計

Forward Shading、4x MSAA、Instanced Stereo、Lumen/Virtual Shadow Maps/Motion Blur無効、動的解像度70〜100%、16.67 msフレーム予算、60 FPS上限を標準設定にしています。起動画面にも実測平均FPSを表示します。60 FPSはシーン、HMD解像度、GPU負荷に依存するため、任意のインポート世界で絶対に保証される値ではありません。目標を下回る場合は動的解像度が先に働きます。

## Blender世界の移行

既存`.blend`をランタイムで直接開くことはできません。`Tools/Export-BlenderWorlds.ps1`でGLBへ一括変換し、Unreal Editorでインポートします。詳しくは[`docs/WORLD_MIGRATION.md`](docs/WORLD_MIGRATION.md)を参照してください。リポジトリの標準起動では、約2.6 kmの軽量オーバル、湖、集落、5個の飛行リングをC++から生成するため、外部アセットなしでも動作確認できます。

## 検証状態

- UE 5.8.2 Editor: ビルド成功
- Win64 Development Game / Shipping: ビルド成功
- Visual Studio 2026ソリューション: 生成成功（ToolsVersion 18.0）
- UE Automation: 4テスト成功（FTMS、CSC、制御コマンド、走行規則）
- NullRHIゲーム起動: `ArriettyGameMode`と`ArriettyWorld`マップのロード成功
- Shipping cook/pak/archive: 成功、`Dist\Windows\Arrietty.exe`の応答確認済み
- T2、HMD、右コントローラーを組み合わせた実機受入試験: 未実施

実機試験は[`docs/ACCEPTANCE_TEST.md`](docs/ACCEPTANCE_TEST.md)に沿って行います。

## ライセンス

GPL-3.0-or-later
