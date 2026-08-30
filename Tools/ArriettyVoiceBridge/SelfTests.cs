// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Text;
using ArriettyVoiceBridge.Services;

namespace ArriettyVoiceBridge
{
    internal static class SelfTests
    {
        public static int Run()
        {
            try
            {
                Assert(VoiceBridgeProtocol.Parse(Encoding.UTF8.GetBytes(VoiceBridgeProtocol.PttDown)).Type == BridgeMessageType.PttDown, "PTT_DOWN");
                Assert(VoiceBridgeProtocol.Parse(Encoding.UTF8.GetBytes(VoiceBridgeProtocol.PttUp)).Type == BridgeMessageType.PttUp, "PTT_UP");
                var answer = VoiceBridgeProtocol.Parse(Encoding.UTF8.GetBytes("{\"protocol\":\"ARRIETTY_VOICE/1\",\"type\":\"answer\",\"id\":\"a\",\"text\":\"了解です。\"}"));
                Assert(answer.Type == BridgeMessageType.Answer && answer.Text == "了解です。", "answer JSON");
                Assert(BridgeOptions.IsValidTmuxTarget("%0"), "tmux pane target");
                Assert(!BridgeOptions.IsValidTmuxTarget("%0; rm"), "tmux target rejection");
                Assert(SpeechTextCleaner.Prepare("**結果**: [詳細](https://example.com) `ok`") == "結果: 詳細 ok", "speech cleanup");
                var streamingWave = MakeStreamingWaveHeader();
                OpenAiAudioClient.FinalizeStreamingWave(streamingWave);
                Assert(BitConverter.ToUInt32(streamingWave, 4) == 36U, "finalized RIFF length");
                Assert(BitConverter.ToUInt32(streamingWave, 40) == 0U, "finalized data length");
                Console.WriteLine("ARRIETTY VOICE BRIDGE SELF-TEST PASSED");
                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("SELF-TEST FAILED: " + exception.Message);
                return 1;
            }
        }

        private static byte[] MakeStreamingWaveHeader()
        {
            var bytes = new byte[44];
            Buffer.BlockCopy(Encoding.ASCII.GetBytes("RIFF"), 0, bytes, 0, 4);
            Buffer.BlockCopy(BitConverter.GetBytes(uint.MaxValue), 0, bytes, 4, 4);
            Buffer.BlockCopy(Encoding.ASCII.GetBytes("WAVEfmt "), 0, bytes, 8, 8);
            Buffer.BlockCopy(BitConverter.GetBytes(16U), 0, bytes, 16, 4);
            Buffer.BlockCopy(Encoding.ASCII.GetBytes("data"), 0, bytes, 36, 4);
            Buffer.BlockCopy(BitConverter.GetBytes(uint.MaxValue), 0, bytes, 40, 4);
            return bytes;
        }

        private static void Assert(bool condition, string name)
        {
            if (!condition) throw new InvalidOperationException(name);
        }
    }
}
