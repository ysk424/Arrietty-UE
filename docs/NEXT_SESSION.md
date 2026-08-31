# Next session handoff

記録日時: 2026-08-31

## 今日完了したこと

- Arrietty UE版v0.11.0として、VR計器を従来位置から20 cm前方・20 cm上方（正面1.25 m、中心高1.30 m）へ移動した。
- Joystick 2を連続アナログ入力から、中央へ戻すたびにピッチ／右ロール目標を1°増減するデジタル入力へ変更した。SWは両目標を0°へ戻す。
- Button 3単独を左ロール1°、Button 4単独を右ロール1°、80 ms以内の3+4同時押しを機首上げ1°へ割り当てた。ハンドルのラダーは変更していない。
- Button 5の`POWER x5`を廃止してPTTへ変更した。Windows音声ブリッジ、OpenAI文字起こし／音声合成、同じWSL/tmux Codexペインへの送信、現在のCodex用回答監視を追加した。
- Windows音声ブリッジのビルドと自己診断、UE 5.8.2 Editorビルド、Automation 10件、世界プロジェクト生成／更新テストに成功した。
- Joystick 1を飛行チューニングへ割り当てた。SWで開始／確定、Xの左右ジェスチャーで固定推進W、飛行速度倍率、正の余剰パワー上昇倍率、ピッチ応答、最大上下動、ロール応答の6項目を一つずつ変更する。計器は`ON GROUND`／`AIRBORNE`と`ALT AGL`／`ELLIP H`を区別する。
- Button 5 PTTは、VIVEマイクを明示選択して発話を日本語文字起こしし、同じCodexペインへの送信、回答のGPT TTS変換、PCスピーカー再生まで実機で合格した。OpenAIのストリーミングWAVはWindows再生前にRIFF/dataサイズを確定する。
- Arrietty UE版v0.10.0の地球飛行機能を`Plugins/ArriettyCesium`へ分離して追加した。既存の平面世界は`ArriettyRuntime`だけで動作する。
- `/Game/Worlds/ArriettyEarth/ArriettyEarth`を作成・保存した。Funafuti原点はlongitude `179.19678290`、latitude `-8.52398430`、ellipsoid height `34.8356 m`。
- Google Photorealistic 3D Tiles（ion asset `2275207`、SSE `16`、credits表示、Movable）、Cesium SunSky、地球用GameMode、World Bounds Checks無効を設定した。
- 重複していたCesium Georeferenceを整理し、`ArriettyEarth_Georeference` 1体を既定参照にした。
- 低い視点で見えた黒い面はGoogle Tilesの裏面、格子はテンプレートFloorだと切り分けた。Floorを除去し、CourseStartを地表より上へ移動した。
- 青空、白い雲、Google地表、可視100 m滑走路が表示される状態まで調整した。
- CourseStartに、可視100 m滑走路から続く長さ2,000 m・幅12 mの不可視`FlightRolloutSurface`を追加した。`SecretWorldRideSurface`、Query Only、Visibility Blockで、Google Tilesの衝突には依存しない。
- UE 5.8.2 `ArriettyEditor Win64 Development`を再ビルドし、Earth Levelの通常PIE開始・停止を確認した。
- 実機ではGoogle地表と空、Joystick 2の上下左右、テンキーによる開始と飛行モードまで確認した。旧100 m滑走路端で止まった問題に対して上記2 km面を追加したが、追加後の自転車実走は未確認。
- ESP32用の独立診断ファームウェアを追加し、4軸の全範囲とButton 1/2/3/4/6、Joystick 1/2 SWをGPIO直読で確認した。
- Button 5が反応しない原因はGPIOやソフトではなく、Button 5の半田切れだと実機で確認した。
- 終了前にButton 5の半田修理を完了し、製品版button maskとPTT押下／解放を実機確認した。
- 製品ファームウェアには短い押下を最低100 ms報告するラッチを追加した。診断後に製品ファームウェアをCOM7へ戻し、`PING`、状態パケット、50 Hz stream、flash hashを確認した。
- Cesium実行記録のtile countは15枚だった。Google地表が動作したことは、人力フライトシミュレーターとしての大きな到達点である。
- 実機飛行で旋回できることを確認した。今回の人力出力は最大約140 Wで、獲得高度は約3 mだった。
- 次の飛行感覚の目標は、約140 Wを維持すると明確に上昇して約300 mの高度を獲得でき、その後はペダルを休めてグライダーとして長く滑空できること。実車速度から飛行対気速度への変換にも数倍の倍率を設ける。
- 飛行速度倍率を初期値`x3.0`、正の余剰パワー上昇倍率を初期値`x10`として実装した。速度倍率は移動量・表示・FPAへ反映し、失速などの内部物理判定は等価対気速度のまま保つ。上昇倍率は正のパワー収支だけへ適用し、無出力時の滑空沈下は増幅しない。
- 140 W・等価対気速度24 km/h・上昇倍率x10で約1.04 m/s（300 mまで約4分49秒）、0 W時は約-0.22 m/sとなることを自動テストで確認した。UE 5.8.2 Editorビルド、Automation 10件、世界プロジェクト生成／更新テストに成功した。
- 初回の実機試験では初期値`速度x3.0／正の上昇x10`で40 mまで継続上昇し、J1の6項目も操作できた。300 m到達とその後の滑空は引き続き確認する。

## 終了時の状態

- Earth Levelは保存済み。PIE／VR Previewは停止済み。
- Windows音声ブリッジはUDP 49000、Codex回答監視はtmux `maro:arrietty-voice-watch`で起動した。WSLまたはWindows再起動後はCodexペインで`./Tools/start-arrietty-voice-bridge.sh`を再実行する。
- ESP32には診断版ではなく製品版ファームウェアが入っている。
- Button 5の半田修理は完了。次回、USB電源を切った状態でコネクターとGNDを確認してから実動作試験する。
- Cesium ionのproject access tokenはローカルの`Content/CesiumSettings/`だけにあり、Git対象外。値を文書、ログ、スクリーンショットへ出さない。
- `.agents/`、`.claude/`、`.neostack/`も端末固有のEditor agent設定としてGit対象外。

## 次回の順序

1. Editorを再起動して新しいRuntime DLLを読み込み、USB接続後はジョイスティックへ約1秒触れない。
2. HMD内で計器の`ON GROUND`／`AIRBORNE`、`ALT AGL`、`ELLIP H`が区別できることを確認する。
3. 飛行モードでJ1 SWを押し、6項目を一周できることと、速度倍率`x3.0`・上昇倍率`x10`が初期値であることを確認する。速度、高度、V/S、BankをCSVで比較する。
4. Joystick 2の各方向を1回ずつ中央へ戻し、1回だけ1°進むこと、倒しっぱなしで反復しないこと、SWで0°へ戻ることを確認する。Button 3/4単独と同時押しも確認する。
5. Earth LevelでButton 1開始、Button 2飛行モード、等価対気速度20 km/h以上（表示60 km/h以上）かつ機首上げ+1°からの離陸を確認し、可視滑走路の先へ進めることを確認する。
6. 140 W前後で上昇率約1 m/s、約5分で300 mへ到達することと、ペダルを止めた後に緩やかに滑空することを確認する。必要ならJ1で速度・上昇倍率を調整する。
7. 続いて失速・回復、着陸、地理座標の連続性とCesium tile countを記録する。
8. タイトル画像`C:\Users\azoo\Desktop\ARRIETTY.png`の`/Game/Brand/ARRIETTY`へのimport、Map Check、Earth Levelを含むcook/packageを完了する。

## 固定値と未完項目

- Level: `/Game/Worlds/ArriettyEarth/ArriettyEarth`
- GameMode: `/Script/ArriettyCesium.ArriettyCesiumGameMode`
- Google ion asset: `2275207`
- Google Tiles: SSE `16`、credits `true`、Movable
- CourseStart: longitude `179.19678290`、latitude `-8.52398430`、ellipsoid height 約`41.8356 m`、World Z `700 cm`
- 未完: 初期値`速度x3.0／正の上昇x10`の実機感覚、140 W前後で約300 mを獲得してから休める滑空バランス、2 km rolloutの実機再試験、タイトル画像import、Map Check、Earth cook/package、v0.11.0最終HMD＋T2受入試験。

## 既存の基準記録

2026-08-28のv0.8.0 HMD＋T2＋VIVEコントローラー＋ESP32統合実走では、SteamVRの`RightGrip`自動選択、開始Heading 0°、最高22.08 km/h、809.223 m走行、旧飛行高度最大12.08 m、Button 6の3%→0%、Button 1の2.01 m安全復帰、Esc終了をCSVで確認済み。
