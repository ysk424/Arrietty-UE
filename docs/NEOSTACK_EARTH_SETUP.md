# NeoStack handoff: Arrietty Earth Level

記録日時: 2026-08-29

> 状態: この初回構築手順は2026-08-29に実施済み。再現用の履歴として保持する。現在値と次回作業は[`NEXT_SESSION.md`](NEXT_SESSION.md)、Level仕様は[`CESIUM_EARTH.md`](CESIUM_EARTH.md)を参照する。

この文書は午後にUE Editor内のCodexへそのまま渡す。C++変更は完了・Editorビルド成功済みである。NeoStackは`.umap`、Cesium actor設定、画像import、PIE可視確認だけに使い、ソースコードと秘密情報を変更しない。

## 目標

`/Game/Worlds/ArriettyEarth/ArriettyEarth`を作り、Funafuti International AirportからGoogle Photorealistic 3D Tiles上を飛べる状態で保存する。

## 固定値

- GameMode: `/Script/ArriettyCesium.ArriettyCesiumGameMode`
- CourseStart class: `/Script/ArriettyCesium.ArriettyCesiumCourseStart`
- Longitude: `179.19678290`
- Latitude: `-8.52398430`
- Ellipsoid height: `34.8356`
- Google Photorealistic 3D Tiles ion asset ID: `2275207`
- Title image source: `C:/Users/azoo/Desktop/ARRIETTY.png`
- Title asset target: `/Game/Brand/ARRIETTY`
- Google tiles must have `ShowCreditsOnScreen=true` and Movable mobility.
- CourseStartの赤いX／機体Heading 0°はEast向き。組み込み滑走路もEastへ100 m伸びる。

## NeoStack作業規則

1. `neostack-level-design`、画像importに該当するskill、`neostack-game-testing`を読み、最初に`help("LevelDesign")`と現在の`open_level():help()`で実シグネチャを確認する。
2. Lua状態は呼出し間で保持されない。各mutation、fresh verification、save、visual gateで`open_level()`をやり直す。
3. non-nilのmutation結果を完成証拠にしない。actor class、label、folder、主要propertyを別callで読み直す。
4. token、Google API key、OpenAI API keyを表示・保存・ログ出力しない。Cesium ion接続が未設定ならLevel編集は保存し、`ion project access token required`とだけ報告する。
5. 既存`ArriettyDemo`と既存assetを変更しない。

## 作成パス

1. assetがなければ次で新規Levelを開く。既にあれば明示loadする。

```lua
create_level("/Game/Worlds/ArriettyEarth/ArriettyEarth", {
  template = "basic",
  open = true,
})
```

2. World SettingsのGameMode Overrideを`ArriettyCesiumGameMode`へ設定する。NeoStackのLevelDesign helpでworld property設定方法を発見し、保存後にfresh readする。
3. `ArriettyEarth/Geospatial`、`ArriettyEarth/Flight`、`ArriettyEarth/Lighting`のfolderを作る。
4. 次のactorを一つずつ、明示label/folder付きで追加する。

| Label | Class | Folder | 必須設定 |
|---|---|---|---|
| `ArriettyEarth_Georeference` | `/Script/CesiumRuntime.CesiumGeoreference` | `ArriettyEarth/Geospatial` | cartographic origin、lon/lat/heightを固定値へ |
| `ArriettyEarth_GoogleTiles` | `/Script/CesiumRuntime.Cesium3DTileset` | `ArriettyEarth/Geospatial` | ion asset `2275207`、Movable、`ShowCreditsOnScreen=true`、MaximumScreenSpaceErrorはまず16 |
| `ArriettyEarth_SunSky` | `/Script/CesiumRuntime.CesiumSunSky` | `ArriettyEarth/Lighting` | Georeferenceを参照、Movable |
| `ArriettyEarth_Start` | `/Script/ArriettyCesium.ArriettyCesiumCourseStart` | `ArriettyEarth/Flight` | origin `(0,0,0)`、rotation `(0,0,0)`、Movable |

Quick AddでGoogle Photorealistic 3D Tilesを追加した場合も、最終的にlabel、asset ID、credits、mobilityをfresh readして同じ状態にする。不要な標準PlayerStartは削除し、`ArriettyEarth_Start`だけを開始点にする。

5. Title imageをimport APIで`/Game/Brand/ARRIETTY`へ読み込む。画像はタイトル／起動画面候補として保存するだけでよく、この段階で飛行HUDへ重ねない。import APIが見つからなければLevelを壊さず、画像importだけ未完として報告する。

## 検証

1. fresh callでpackageが`/Game/Worlds/ArriettyEarth/ArriettyEarth`、上記actorが各1個、folderとclassが一致することをassertする。
2. Georeferenceのlon/lat/height、Google tilesのasset ID／credits／mobility、World Settings GameModeをfresh readする。
3. `map_check()`を実行し、error 0を必須とする。warningは内容を記録する。
4. `level:save()`後、`asset_exists()`とlevel infoを別callで確認する。
5. overlayを隠したLevel screenshotを最低2枚取得する。1枚はFunafuti滑走路付近、もう1枚は上空から島とGoogle tilesのstreaming状況が分かる視点にする。
6. Desktop PIEを正確なmap pathで開始し、`AArriettyCesiumPawn`がpossessionされること、player viewにGoogleタイルまたはtoken要求状態、ネイティブ計器が見えることを確認して必ずPIEを停止する。HMD、T2、ESP32の入力試験は午後のVR試験まで行わない。

最終報告にはactor/propertyのfresh-read表、Map Check結果、save path、2枚のLevel screenshot、1枚のPIE player-view screenshot、未完項目だけを含める。
