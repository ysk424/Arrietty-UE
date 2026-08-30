// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;
using ArriettyVoiceBridge.Services;

namespace ArriettyVoiceBridge
{
    internal sealed class VoiceBridge : IDisposable
    {
        private const int MinimumRecordingMilliseconds = 180;
        private const string VoicePromptSuffix =
            "\n\n［音声応答モード：自転車で走行中です。日本語で結論から2文以内にしてください。危険を伴う操作は実行せず、短く確認してください。］";

        private readonly object _stateLock = new object();
        private readonly BridgeOptions _options;
        private readonly MciAudioRecorder _recorder = new MciAudioRecorder();
        private readonly OpenAiAudioClient _audioClient = new OpenAiAudioClient();
        private readonly TmuxClient _tmux;
        private readonly Stopwatch _recordingTimer = new Stopwatch();
        private readonly HashSet<string> _receivedAnswerIds = new HashSet<string>(StringComparer.Ordinal);

        private UdpClient _listener;
        private bool _isRecording;
        private bool _isTranscribing;
        private bool _awaitingResponse;
        private bool _stopRequested;

        public VoiceBridge(BridgeOptions options)
        {
            _options = options;
            _tmux = new TmuxClient(options.WslDistro, options.TmuxTarget);
        }

        public int Run()
        {
            try
            {
                _listener = new UdpClient(new IPEndPoint(IPAddress.Loopback, _options.Port));
            }
            catch (SocketException exception)
            {
                Console.Error.WriteLine("UDPポートを開けません: " + exception.Message);
                return 4;
            }

            Console.WriteLine("Arrietty Voice Bridge ready");
            Console.WriteLine("  UDP: 127.0.0.1:" + _options.Port);
            Console.WriteLine("  tmux: " + _options.WslDistro + " " + _options.TmuxTarget);
            Console.WriteLine("  microphone: " + _recorder.RecordingDevice);
            Console.WriteLine(OpenAiAudioClient.HasApiKey()
                ? "  OpenAI API key: ready"
                : "  OpenAI API key: NOT SET");

            while (!_stopRequested)
            {
                try
                {
                    IPEndPoint sender = null;
                    var datagram = _listener.Receive(ref sender);
                    HandleMessage(VoiceBridgeProtocol.Parse(datagram), sender);
                }
                catch (ObjectDisposedException)
                {
                    break;
                }
                catch (SocketException exception)
                {
                    if (!_stopRequested)
                    {
                        Console.Error.WriteLine("UDP受信エラー: " + exception.Message);
                    }
                }
            }
            return 0;
        }

        public void Stop()
        {
            _stopRequested = true;
            if (_listener != null)
            {
                _listener.Close();
            }
            CancelRecording();
        }

        public void Dispose()
        {
            Stop();
            _recorder.Dispose();
            if (_listener != null)
            {
                _listener.Dispose();
            }
        }

        private void HandleMessage(BridgeMessage message, IPEndPoint sender)
        {
            switch (message.Type)
            {
            case BridgeMessageType.PttDown:
                BeginRecording(sender);
                break;
            case BridgeMessageType.PttUp:
                EndRecording(sender);
                break;
            case BridgeMessageType.PttCancel:
                CancelRecording();
                SendStatus(sender, "CANCELLED");
                break;
            case BridgeMessageType.Answer:
                ReceiveAnswer(message.Id, message.Text);
                break;
            }
        }

        private void BeginRecording(IPEndPoint client)
        {
            lock (_stateLock)
            {
                if (_isRecording || _isTranscribing)
                {
                    SendStatus(client, "ERROR", "音声ブリッジは処理中です。");
                    return;
                }
                if (!OpenAiAudioClient.HasApiKey())
                {
                    Console.Error.WriteLine("OPENAI_API_KEY がWindows環境に設定されていません。");
                    SendStatus(client, "ERROR", "OPENAI_API_KEY が設定されていません。");
                    SafeBeep(300, 250);
                    return;
                }
                try
                {
                    _recorder.Start();
                    _recordingTimer.Restart();
                    _isRecording = true;
                    Console.WriteLine("PTT: recording");
                    SendStatus(client, "RECORDING");
                }
                catch (Exception exception)
                {
                    Console.Error.WriteLine("録音開始エラー: " + exception.Message);
                    SendStatus(client, "ERROR", "録音開始エラー: " + exception.Message);
                    SafeBeep(300, 250);
                }
            }
        }

        private void EndRecording(IPEndPoint client)
        {
            string wavePath;
            long duration;
            lock (_stateLock)
            {
                if (!_isRecording || _isTranscribing)
                {
                    SendStatus(client, "ERROR", "録音中ではありません。");
                    return;
                }
                _isRecording = false;
                _recordingTimer.Stop();
                duration = _recordingTimer.ElapsedMilliseconds;
                try
                {
                    wavePath = _recorder.StopAndSave();
                }
                catch (Exception exception)
                {
                    Console.Error.WriteLine("録音終了エラー: " + exception.Message);
                    SendStatus(client, "ERROR", "録音終了エラー: " + exception.Message);
                    SafeBeep(300, 250);
                    return;
                }
                if (duration < MinimumRecordingMilliseconds)
                {
                    TryDelete(wavePath);
                    Console.WriteLine("PTT: too short, ignored");
                    SendStatus(client, "ERROR", "録音時間が短すぎます。");
                    return;
                }
                _isTranscribing = true;
                SendStatus(client, "TRANSCRIBING");
            }

            Task.Run(async delegate
            {
                try
                {
                    Console.WriteLine("PTT: transcribing " + duration + " ms");
                    var transcript = (await _audioClient.TranscribeJapaneseAsync(wavePath)).Trim();
                    if (transcript.Length == 0)
                    {
                        throw new InvalidOperationException("音声を認識できませんでした。");
                    }
                    if (transcript.Length > 1200)
                    {
                        transcript = transcript.Substring(0, 1200);
                    }

                    lock (_stateLock)
                    {
                        _awaitingResponse = true;
                    }
                    try
                    {
                        _tmux.SendPrompt(transcript + VoicePromptSuffix);
                    }
                    catch
                    {
                        lock (_stateLock)
                        {
                            _awaitingResponse = false;
                        }
                        throw;
                    }
                    Console.WriteLine("PTT -> Codex: " + transcript);
                    SendStatus(client, "SENT");
                    SafeBeep(1320, 100);
                }
                catch (Exception exception)
                {
                    Console.Error.WriteLine("文字起こし／tmux送信エラー: " + exception.Message);
                    SendStatus(client, "ERROR", exception.Message);
                    SafeBeep(300, 300);
                }
                finally
                {
                    TryDelete(wavePath);
                    lock (_stateLock)
                    {
                        _isTranscribing = false;
                    }
                }
            });
        }

        private void CancelRecording()
        {
            lock (_stateLock)
            {
                if (_isRecording || _recorder.IsRecording)
                {
                    _recorder.Cancel();
                    _recordingTimer.Reset();
                    _isRecording = false;
                    Console.WriteLine("PTT: cancelled");
                }
            }
        }

        private void ReceiveAnswer(string id, string answer)
        {
            string speech;
            lock (_stateLock)
            {
                if (!_awaitingResponse || string.IsNullOrWhiteSpace(answer))
                {
                    return;
                }
                if (!string.IsNullOrEmpty(id) && !_receivedAnswerIds.Add(id))
                {
                    return;
                }
                if (_receivedAnswerIds.Count > 64)
                {
                    _receivedAnswerIds.Clear();
                }
                _awaitingResponse = false;
                speech = SpeechTextCleaner.Prepare(answer);
            }
            if (speech.Length == 0)
            {
                return;
            }

            Task.Run(async delegate
            {
                try
                {
                    Console.WriteLine("Codex -> TTS: " + speech);
                    await _audioClient.SpeakJapaneseAsync(speech);
                }
                catch (Exception exception)
                {
                    Console.Error.WriteLine("音声合成エラー: " + exception.Message);
                    SafeBeep(300, 300);
                }
            });
        }

        private static void SafeBeep(int frequency, int duration)
        {
            try { Console.Beep(frequency, duration); }
            catch { }
        }

        private void SendStatus(IPEndPoint client, string status, string detail = null)
        {
            if (client == null || _listener == null)
            {
                return;
            }
            var safeDetail = string.IsNullOrWhiteSpace(detail)
                ? string.Empty
                : " " + detail.Replace('\r', ' ').Replace('\n', ' ').Trim();
            var bytes = System.Text.Encoding.UTF8.GetBytes(
                "ARRIETTY_VOICE/1 STATUS " + status + safeDetail);
            try
            {
                _listener.Send(bytes, bytes.Length, client);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("UE状態通知エラー: " + exception.Message);
            }
        }

        private static void TryDelete(string path)
        {
            try { if (!string.IsNullOrEmpty(path)) File.Delete(path); }
            catch { }
        }
    }
}
