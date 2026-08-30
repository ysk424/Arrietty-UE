// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.IO;
using System.Media;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace ArriettyVoiceBridge.Services
{
    internal sealed class OpenAiAudioClient
    {
        private const string TranscriptionEndpoint = "https://api.openai.com/v1/audio/transcriptions";
        private const string SpeechEndpoint = "https://api.openai.com/v1/audio/speech";
        private const string DefaultTranscriptionModel = "gpt-4o-mini-transcribe";
        private const string DefaultTtsModel = "gpt-4o-mini-tts";
        private const string DefaultTtsVoice = "coral";
        private static readonly HttpClient HttpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(90) };

        static OpenAiAudioClient()
        {
            // This standalone .NET Framework executable otherwise negotiates
            // the legacy process default on this machine, while OpenAI's API
            // requires a modern TLS channel.
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
        }

        public static bool HasApiKey()
        {
            return !string.IsNullOrWhiteSpace(GetEnvironmentValue("OPENAI_API_KEY"));
        }

        public async Task<string> TranscribeJapaneseAsync(string wavePath)
        {
            var key = RequireApiKey();
            var model = GetEnvironmentValue("ARRIETTY_TRANSCRIBE_MODEL") ?? DefaultTranscriptionModel;
            var prompt = GetEnvironmentValue("ARRIETTY_TRANSCRIBE_PROMPT");
            var bytes = File.ReadAllBytes(wavePath);
            using (var response = await SendWithRetryAsync(delegate
            {
                var request = new HttpRequestMessage(HttpMethod.Post, TranscriptionEndpoint);
                request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", key);
                request.Headers.UserAgent.ParseAdd("arrietty-voice-bridge/1.0");
                var form = new MultipartFormDataContent();
                var audio = new ByteArrayContent(bytes);
                audio.Headers.ContentType = new MediaTypeHeaderValue("audio/wav");
                form.Add(audio, "file", Path.GetFileName(wavePath));
                form.Add(new StringContent(model, Encoding.UTF8), "model");
                form.Add(new StringContent("ja", Encoding.UTF8), "language");
                if (!string.IsNullOrWhiteSpace(prompt))
                {
                    form.Add(new StringContent(prompt, Encoding.UTF8), "prompt");
                }
                form.Add(new StringContent("json", Encoding.UTF8), "response_format");
                request.Content = form;
                return request;
            }))
            {
                var body = await response.Content.ReadAsStringAsync();
                EnsureSuccess(response, body);
                var root = new JavaScriptSerializer().DeserializeObject(body) as Dictionary<string, object>;
                object text;
                if (root == null || !root.TryGetValue("text", out text) || !(text is string))
                {
                    throw new InvalidOperationException("文字起こし結果がありませんでした。");
                }
                return (string)text;
            }
        }

        public async Task SpeakJapaneseAsync(string text)
        {
            var key = RequireApiKey();
            var model = GetEnvironmentValue("ARRIETTY_TTS_MODEL") ?? DefaultTtsModel;
            var voice = GetEnvironmentValue("ARRIETTY_TTS_VOICE") ?? DefaultTtsVoice;
            var body = new JavaScriptSerializer().Serialize(new Dictionary<string, object>
            {
                { "model", model },
                { "voice", voice },
                { "input", text },
                { "instructions", "落ち着いた明瞭な日本語で、短く読み上げてください。" },
                { "response_format", "wav" }
            });
            using (var response = await SendWithRetryAsync(delegate
            {
                var request = new HttpRequestMessage(HttpMethod.Post, SpeechEndpoint);
                request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", key);
                request.Headers.UserAgent.ParseAdd("arrietty-voice-bridge/1.0");
                request.Content = new StringContent(body, Encoding.UTF8, "application/json");
                return request;
            }))
            {
                var bytes = await response.Content.ReadAsByteArrayAsync();
                if (!response.IsSuccessStatusCode)
                {
                    EnsureSuccess(response, Encoding.UTF8.GetString(bytes));
                }
                FinalizeStreamingWave(bytes);
                var path = Path.Combine(Path.GetTempPath(), "ArriettyVoiceBridge", "tts-" + Guid.NewGuid().ToString("N") + ".wav");
                Directory.CreateDirectory(Path.GetDirectoryName(path));
                File.WriteAllBytes(path, bytes);
                try
                {
                    using (var player = new SoundPlayer(path))
                    {
                        player.Load();
                        player.PlaySync();
                    }
                }
                finally
                {
                    try { File.Delete(path); } catch { }
                }
            }
        }

        internal static void FinalizeStreamingWave(byte[] bytes)
        {
            if (bytes == null || bytes.Length < 12 ||
                !HasChunkId(bytes, 0, "RIFF") ||
                !HasChunkId(bytes, 8, "WAVE"))
            {
                throw new InvalidOperationException("OpenAI TTSの応答が有効なRIFF/WAVEではありません。");
            }

            // The speech endpoint can stream a PCM WAVE whose RIFF and data
            // lengths are 0xffffffff because they are not known when the
            // response header is emitted. SoundPlayer requires finalized
            // lengths, so fill them in after the complete body is available.
            WriteUInt32LittleEndian(bytes, 4, (uint)(bytes.Length - 8));

            var chunkOffset = 12;
            while (chunkOffset <= bytes.Length - 8)
            {
                var declaredSize = ReadUInt32LittleEndian(bytes, chunkOffset + 4);
                var payloadOffset = chunkOffset + 8;
                if (HasChunkId(bytes, chunkOffset, "data"))
                {
                    WriteUInt32LittleEndian(bytes, chunkOffset + 4, (uint)(bytes.Length - payloadOffset));
                    return;
                }

                if (declaredSize == uint.MaxValue)
                {
                    break;
                }
                var nextOffset = (long)payloadOffset + declaredSize + (declaredSize & 1U);
                if (nextOffset > bytes.Length)
                {
                    break;
                }
                chunkOffset = (int)nextOffset;
            }

            throw new InvalidOperationException("OpenAI TTSのWAVE応答にdataチャンクがありません。");
        }

        private static bool HasChunkId(byte[] bytes, int offset, string id)
        {
            return offset >= 0 && offset <= bytes.Length - 4 &&
                bytes[offset] == (byte)id[0] &&
                bytes[offset + 1] == (byte)id[1] &&
                bytes[offset + 2] == (byte)id[2] &&
                bytes[offset + 3] == (byte)id[3];
        }

        private static uint ReadUInt32LittleEndian(byte[] bytes, int offset)
        {
            return (uint)(bytes[offset] |
                bytes[offset + 1] << 8 |
                bytes[offset + 2] << 16 |
                bytes[offset + 3] << 24);
        }

        private static void WriteUInt32LittleEndian(byte[] bytes, int offset, uint value)
        {
            bytes[offset] = (byte)value;
            bytes[offset + 1] = (byte)(value >> 8);
            bytes[offset + 2] = (byte)(value >> 16);
            bytes[offset + 3] = (byte)(value >> 24);
        }

        private static async Task<HttpResponseMessage> SendWithRetryAsync(
            Func<HttpRequestMessage> requestFactory)
        {
            const int maximumAttempts = 3;
            Exception lastException = null;
            for (var attempt = 1; attempt <= maximumAttempts; ++attempt)
            {
                using (var request = requestFactory())
                {
                    try
                    {
                        return await HttpClient.SendAsync(request);
                    }
                    catch (TaskCanceledException exception)
                    {
                        lastException = exception;
                    }
                    catch (HttpRequestException exception)
                    {
                        lastException = exception;
                    }
                }
                if (attempt < maximumAttempts)
                {
                    await Task.Delay(TimeSpan.FromMilliseconds(400 * attempt));
                }
            }

            if (lastException is TaskCanceledException)
            {
                throw new TimeoutException(
                    "OpenAI APIが3回の試行で応答しませんでした: " + DescribeException(lastException),
                    lastException);
            }
            throw new InvalidOperationException(
                "OpenAI APIへ3回の試行で接続できませんでした: " + DescribeException(lastException),
                lastException);
        }

        private static string DescribeException(Exception exception)
        {
            var parts = new List<string>();
            for (var current = exception; current != null; current = current.InnerException)
            {
                parts.Add(current.GetType().Name + ": " + current.Message);
            }
            return string.Join(" -> ", parts);
        }

        private static void EnsureSuccess(HttpResponseMessage response, string body)
        {
            if (response.IsSuccessStatusCode) return;
            var detail = string.Empty;
            try
            {
                var root = new JavaScriptSerializer().DeserializeObject(body) as Dictionary<string, object>;
                object errorValue;
                object messageValue;
                var error = root != null && root.TryGetValue("error", out errorValue)
                    ? errorValue as Dictionary<string, object>
                    : null;
                if (error != null && error.TryGetValue("message", out messageValue)) detail = messageValue as string;
            }
            catch (ArgumentException) { }
            throw new InvalidOperationException(string.IsNullOrWhiteSpace(detail)
                ? "OpenAI API error: " + (int)response.StatusCode + " " + response.ReasonPhrase
                : "OpenAI API error: " + detail);
        }

        private static string RequireApiKey()
        {
            var value = GetEnvironmentValue("OPENAI_API_KEY");
            if (string.IsNullOrWhiteSpace(value)) throw new InvalidOperationException("OPENAI_API_KEY が設定されていません。");
            return value.Trim();
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
    }
}
