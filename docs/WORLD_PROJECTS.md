# Arrietty world projects

最終更新: 2026-08-27

## 新規作成

PowerShell 7（`pwsh`）でArriettyリポジトリから実行します。

```powershell
pwsh -File .\Tools\New-ArriettyWorldProject.ps1 -Name MyForest
```

`-Name`には英字で始まる英数字とアンダースコアだけを使用できます。既定の保存先は`Documents\Unreal Projects`です。変更する場合は`-DestinationRoot`を指定します。既存ディレクトリは上書きしません。

生成されるプロジェクトには、プロジェクト固有の`ArriettyWorldHost`、Arriettyのソース付きRuntimeプラグイン、VR向けConfig、`ArriettyDemo` Levelが含まれます。ビルドを後回しにするときだけ`-SkipBuild`を指定できます。

## Levelを走行可能にする

1. `Content/Worlds`の下にLevelを作成または保存する。
2. `ArriettyCourseStart`を1個置き、赤いX軸を出発方向へ向ける。
3. 道路のActorまたはPrimitive Componentへ`SecretWorldRideSurface`タグを追加する。
4. 道路がVisibility CollisionへBlock応答することを確認する。
5. World SettingsのGameMode Overrideを未指定にするか`ArriettyGameMode`へ設定する。
6. SteamVRを起動し、Levelを開いたままPlayメニューの`VR Preview`を実行する。

タグ付き走行面がLevel内に1つもなければ、地上モードはZ=0の平面を走行します。タグ付き面が1つでもあれば、その面が存在する場所だけを地上走行できます。飛行モードはこの範囲制限を受けません。

## C++更新

UE Editorを閉じ、Arriettyリポジトリを更新してから実行します。

```powershell
git -C C:\Users\azoo\git\Arrietty-UE pull
pwsh -File C:\Users\azoo\git\Arrietty-UE\Tools\Update-ArriettyWorldProject.ps1 `
  -Project "C:\Users\azoo\Documents\Unreal Projects\MyForest\MyForest.uproject"
```

スクリプトは`Plugins/ArriettyRuntime`の管理対象ソースだけを同期し、プラグインの現バージョンを表示します。`Content`と`Config`は保持されます。続いてVS2026プロジェクトを再生成し、対象プロジェクトのEditorターゲットを再ビルドします。

世界固有のC++を追加する場合は`ArriettyRuntime`を直接編集せず、別のProject ModuleまたはPluginへ置きます。更新スクリプトが管理するプラグインファイルへ独自変更を加えると、次回更新で置き換わります。
