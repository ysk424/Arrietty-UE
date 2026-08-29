# Cesium Earth flight

最終更新: 2026-08-29（v0.10.0）

## 構成

地球飛行は`Plugins/ArriettyCesium`に分離している。既存の`ArriettyDemo`やCesiumを使わない世界は、従来どおり`ArriettyRuntime`だけの平面座標で動作する。

- `AArriettyCesiumPawn`: `AArriettyPawn`にCesium Globe AnchorとOrigin Shiftを追加する。
- `UArriettyCesiumNavigationComponent`: Arriettyのメートル座標を、その時点のWGS84接平面のEast/Northへ積分する。移動後の高度は楕円体高として適用し、地球を周回してもローカルUE座標を増大させない。
- `AArriettyCesiumGameMode`: 地球用Pawnを既定Pawnにする。
- `AArriettyCesiumCourseStart`: Globe Anchor付きの開始点、可視100 m滑走路、その先へ続く不可視2,000 m rollout面を提供する。両方に`SecretWorldRideSurface`タグがある。

計器と`Saved/arrietty_ride.csv`にはlongitude、latitude、ellipsoid heightを記録する。飛行中も対気速度・高度・HeadingなどのArrietty内部状態は従来と同じ単位を使う。

## Funafuti開始点

Secret Worldで確定した原点を使う。

| 項目 | 値 |
|---|---:|
| Longitude | `179.19678290` |
| Latitude | `-8.52398430` |
| Ellipsoid height | `34.8356 m` |
| Google Photorealistic 3D Tiles ion asset | `2275207` |

Googleタイルは写実背景であり、走行／着陸判定には使わない。着陸可能面はオープンに保存できる独自メッシュまたは`AArriettyCesiumCourseStart`の滑走路へ`SecretWorldRideSurface`タグを付ける。可視滑走路は100 m、不可視rollout面は2,000 m×12 mで、Query OnlyのVisibility traceだけをBlockする。Googleの画面クレジットは常時表示する。

## 保存済みEarth Level

`/Game/Worlds/ArriettyEarth/ArriettyEarth`を2026-08-29に作成・保存した。

- `ArriettyEarth_Georeference`: DEFAULT_GEOREFERENCE、上記Funafuti原点
- `ArriettyEarth_GoogleTiles`: ion asset `2275207`、Maximum Screen Space Error `16`、credits表示、Movable
- `ArriettyEarth_SunSky`: 青空と白い雲を表示
- `ArriettyEarth_Start`: World Z `700 cm`、ellipsoid height約`41.8356 m`
- World Settings: `ArriettyCesiumGameMode`、World Bounds Checks無効
- テンプレートFloor: 削除済み。Google Tiles非表示時に見えた格子の原因だった

低いViewportで見えた黒い帯はGoogle Tilesの地表Primitiveを下側から見たものだった。開始点を地表より上へ移し、滑走路の上面をWorld Z `700 cm`に合わせた。通常PIEの開始・停止と地表・空の表示は確認済みで、2 km rollout追加後の自転車実走は次回確認する。

2026-08-29の実機記録ではCesiumのtile countは15枚だった。数値は次回も同じ表示箇所で記録し、視界、FPS、読み込み待ちとの関係を比較する。

## 認証

Cesium for UnrealのCesium ionパネルでサインインし、プロジェクト用access tokenを設定する。tokenをソース、Markdown、ログ、`.env`へ転記しない。ローカルで生成される`Content/CesiumSettings/`はGit対象外であり、clone先では各自のtokenを設定する。

`SECRET_WORLD_GOOGLE_MAPS_API_KEY`はBlender版Secret WorldのGoogle Maps API直結用であり、Cesium ion asset `2275207`を使うUE Levelには渡さない。`OPENAI_API_KEY`も走行、Cesium、NeoStack Level編集には不要である。

実際のLevel asset作成はUE Editor内部操作になるため、手順は[`NEOSTACK_EARTH_SETUP.md`](NEOSTACK_EARTH_SETUP.md)に分離している。
