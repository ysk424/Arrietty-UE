# Arrietty Voice Bridge

Button 5 を押している間だけ自転車側のVIVEマイクを 16 kHz mono WAV で録音し、離すと
`gpt-4o-mini-transcribe` で日本語へ変換して、起動元の WSL/tmux Codex ペインへ貼り付けて
Enter を送る。Codex の最終回答は `gpt-4o-mini-tts`（既定 voice: `coral`）で短く読み上げる。

APIキーはコードや設定ファイルへ保存せず、Windowsユーザー環境変数 `OPENAI_API_KEY` を使う。
録音にはWindowsから実行できる `ffmpeg.exe` も必要となる。

```powershell
[Environment]::SetEnvironmentVariable("OPENAI_API_KEY", "YOUR_API_KEY", "User")
```

新しい Windows Terminal / WSL を開き、Codex が動く tmux ペインで一度だけ起動する。

```bash
./Tools/start-arrietty-voice-bridge.sh
```

録音入力は既定で `マイク (VIVE Pro Mutimedia Audio)` に固定されます。
別の入力を使う場合だけ `ARRIETTY_RECORDING_DEVICE` を設定してください。
`ARRIETTY_FFMPEG_PATH` で FFmpeg の実行ファイルも指定できます。

`ARRIETTY_TRANSCRIBE_MODEL`、`ARRIETTY_TRANSCRIBE_PROMPT`、`ARRIETTY_TTS_MODEL`、
`ARRIETTY_TTS_VOICE` を Windows ユーザー環境変数へ設定すると既定値を上書きできる。

現在のCodexプロセスには後から `notify` を読み込ませられないため、起動スクリプトは現在の
session JSONLだけを読む監視プロセスもWSL内へ起動する。次回以降はCodexの公式 `notify`
設定も利用できる。どちらから届いても、PTTで送った質問の次の回答だけを読み上げる。

音声合成はAI音声であり、人間の録音音声ではない。
