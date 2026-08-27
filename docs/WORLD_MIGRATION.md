# Blender world migration

## 一括エクスポート

```powershell
cd C:\Users\azoo\git\Arrietty-UE
.\Tools\Export-BlenderWorlds.ps1
```

既定では旧リポジトリの`C:\Users\azoo\git\Arrietty\test_data`を読み、`SourceAssets\BlenderExports`へGLBを作ります。BlenderのカスタムプロパティもglTF extrasへ含めます。

## Unrealへのインポート

対象となる世界の`.uproject`をUnreal Editorで開き、Content BrowserへGLBをドラッグします。Arrietty開発プロジェクトへ一括インポートする場合は、Output Logから次も実行できます。

```text
py "C:/Users/azoo/git/Arrietty-UE/Tools/import_worlds.py"
```

またはContent BrowserへGLBをドラッグします。単位はmからcmへ変換されるため、Import Uniform Scaleは既定値を使い、インポート後に既知の100 m直線で寸法を確認します。

## 走行面

道路や滑走路のActor TagまたはComponent Tagへ`SecretWorldRideSurface`を追加します。タグ付き面が1つでもある世界では、タグのない面を地面として使いません。次のXY位置にタグ付き面がなければ移動を一時停止します。

走行面はVisibilityチャンネルのレイキャストに応答するCollision設定にします。Levelへ`ArriettyCourseStart`を1個置き、開始地点へ移動して赤いX軸を最初の進行方向へ向けます。Level固有のGameMode Overrideを設定している場合は解除するか、`ArriettyGameMode`を指定します。

## 60 FPS向け確認

- 同一素材の静的メッシュはNaniteではなくISM/HISM結合を優先する。
- 透明素材、動的ライト、影を落とす小物を減らす。
- インポート直後に`stat unit`、`stat gpu`、画面左のArrietty FPS表示を確認する。
- 目標未達時は、まずHMD向けScreen Percentageと影を調整し、走行物理の更新頻度は落とさない。
