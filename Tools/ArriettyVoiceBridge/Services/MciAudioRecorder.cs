// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace ArriettyVoiceBridge.Services
{
    internal sealed class MciAudioRecorder : IDisposable
    {
        private const string DefaultRecordingDevice = "マイク (VIVE Pro Mutimedia Audio)";
        private readonly string _recordingDevice;
        private Process _captureProcess;
        private string _recordingPath;
        private bool _disposed;

        public MciAudioRecorder()
        {
            _recordingDevice = GetEnvironmentValue("ARRIETTY_RECORDING_DEVICE") ??
                DefaultRecordingDevice;
        }

        public bool IsRecording { get; private set; }
        public string RecordingDevice { get { return _recordingDevice; } }

        public void Start()
        {
            ThrowIfDisposed();
            if (IsRecording) throw new InvalidOperationException("すでに録音中です。");

            var directory = Path.Combine(Path.GetTempPath(), "ArriettyVoiceBridge");
            Directory.CreateDirectory(directory);
            _recordingPath = Path.Combine(
                directory,
                "ptt-" + Guid.NewGuid().ToString("N") + ".wav");

            var ffmpeg = GetEnvironmentValue("ARRIETTY_FFMPEG_PATH") ?? "ffmpeg.exe";
            var startInfo = new ProcessStartInfo
            {
                FileName = ffmpeg,
                Arguments =
                    "-hide_banner -loglevel error -f dshow -audio_buffer_size 50 -i " +
                    QuoteArgument("audio=" + _recordingDevice) +
                    " -ac 1 -ar 16000 -c:a pcm_s16le -y " +
                    QuoteArgument(_recordingPath),
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardInput = true,
                RedirectStandardError = true
            };

            try
            {
                _captureProcess = Process.Start(startInfo);
                if (_captureProcess == null)
                {
                    throw new InvalidOperationException("FFmpegを起動できませんでした。");
                }

                // Do not tell UE that recording is ready until DirectShow has
                // had time to open the explicitly selected headset microphone.
                Thread.Sleep(400);
                if (_captureProcess.HasExited)
                {
                    var detail = _captureProcess.StandardError.ReadToEnd().Trim();
                    throw new InvalidOperationException(string.IsNullOrWhiteSpace(detail)
                        ? "VIVEマイクを開けませんでした。"
                        : "VIVEマイクを開けませんでした: " + detail);
                }
                IsRecording = true;
            }
            catch (Win32Exception exception)
            {
                CleanupCapture(false);
                throw new InvalidOperationException(
                    "FFmpegを起動できません。ARRIETTY_FFMPEG_PATHを確認してください。",
                    exception);
            }
            catch
            {
                CleanupCapture(false);
                throw;
            }
        }

        public string StopAndSave()
        {
            ThrowIfDisposed();
            if (!IsRecording) throw new InvalidOperationException("録音されていません。");
            IsRecording = false;
            var path = _recordingPath;
            try
            {
                if (_captureProcess == null || _captureProcess.HasExited)
                {
                    throw new InvalidOperationException("VIVEマイクの録音処理が停止しています。");
                }
                _captureProcess.StandardInput.WriteLine("q");
                _captureProcess.StandardInput.Flush();
                if (!_captureProcess.WaitForExit(5000))
                {
                    try { _captureProcess.Kill(); } catch { }
                    throw new TimeoutException("VIVEマイクの録音終了がタイムアウトしました。");
                }
                var detail = _captureProcess.StandardError.ReadToEnd().Trim();
                if (_captureProcess.ExitCode != 0)
                {
                    throw new InvalidOperationException(string.IsNullOrWhiteSpace(detail)
                        ? "VIVEマイクの録音に失敗しました。"
                        : "VIVEマイクの録音に失敗しました: " + detail);
                }
                if (string.IsNullOrWhiteSpace(path) ||
                    !File.Exists(path) ||
                    new FileInfo(path).Length <= 44)
                {
                    throw new InvalidOperationException("VIVEマイクの録音データが空です。");
                }
                CleanupCapture(true);
                return path;
            }
            catch
            {
                CleanupCapture(false);
                throw;
            }
        }

        public void Cancel()
        {
            if (_disposed) return;
            IsRecording = false;
            CleanupCapture(false);
        }

        public void Dispose()
        {
            if (_disposed) return;
            Cancel();
            _disposed = true;
        }

        private void CleanupCapture(bool keepRecording)
        {
            if (_captureProcess != null)
            {
                if (!_captureProcess.HasExited)
                {
                    try
                    {
                        _captureProcess.StandardInput.WriteLine("q");
                        _captureProcess.StandardInput.Flush();
                    }
                    catch { }
                    if (!_captureProcess.WaitForExit(1500))
                    {
                        try { _captureProcess.Kill(); } catch { }
                    }
                }
                _captureProcess.Dispose();
                _captureProcess = null;
            }
            if (!keepRecording && !string.IsNullOrWhiteSpace(_recordingPath))
            {
                try { File.Delete(_recordingPath); } catch { }
            }
            _recordingPath = null;
        }

        private static string QuoteArgument(string value)
        {
            return "\"" + value.Replace("\"", "\\\"") + "\"";
        }

        private static string GetEnvironmentValue(string name)
        {
            var value = Environment.GetEnvironmentVariable(name);
            if (!string.IsNullOrWhiteSpace(value)) return value.Trim();
            try
            {
                value = Environment.GetEnvironmentVariable(name, EnvironmentVariableTarget.User);
                if (!string.IsNullOrWhiteSpace(value)) return value.Trim();
                value = Environment.GetEnvironmentVariable(name, EnvironmentVariableTarget.Machine);
                return string.IsNullOrWhiteSpace(value) ? null : value.Trim();
            }
            catch { return null; }
        }

        private void ThrowIfDisposed() { if (_disposed) throw new ObjectDisposedException(GetType().Name); }
    }
}
