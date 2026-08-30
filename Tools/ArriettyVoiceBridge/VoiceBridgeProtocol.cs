// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Collections.Generic;
using System.Text;
using System.Web.Script.Serialization;

namespace ArriettyVoiceBridge
{
    internal enum BridgeMessageType
    {
        Unknown,
        PttDown,
        PttUp,
        PttCancel,
        Answer
    }

    internal sealed class BridgeMessage
    {
        public BridgeMessageType Type;
        public string Id;
        public string Text;
    }

    internal static class VoiceBridgeProtocol
    {
        public const string PttDown = "ARRIETTY_VOICE/1 PTT_DOWN";
        public const string PttUp = "ARRIETTY_VOICE/1 PTT_UP";
        public const string PttCancel = "ARRIETTY_VOICE/1 PTT_CANCEL";

        public static BridgeMessage Parse(byte[] data)
        {
            var value = Encoding.UTF8.GetString(data).Trim();
            if (value == PttDown) return new BridgeMessage { Type = BridgeMessageType.PttDown };
            if (value == PttUp) return new BridgeMessage { Type = BridgeMessageType.PttUp };
            if (value == PttCancel) return new BridgeMessage { Type = BridgeMessageType.PttCancel };
            if (!value.StartsWith("{", StringComparison.Ordinal))
            {
                return new BridgeMessage { Type = BridgeMessageType.Unknown };
            }

            try
            {
                var root = new JavaScriptSerializer().DeserializeObject(value) as Dictionary<string, object>;
                object protocol;
                object type;
                object text;
                object id;
                if (root != null &&
                    root.TryGetValue("protocol", out protocol) && Equals(protocol as string, "ARRIETTY_VOICE/1") &&
                    root.TryGetValue("type", out type) && Equals(type as string, "answer") &&
                    root.TryGetValue("text", out text))
                {
                    root.TryGetValue("id", out id);
                    return new BridgeMessage
                    {
                        Type = BridgeMessageType.Answer,
                        Id = id as string,
                        Text = text as string
                    };
                }
            }
            catch (ArgumentException)
            {
                // Invalid datagrams are intentionally ignored.
            }
            return new BridgeMessage { Type = BridgeMessageType.Unknown };
        }
    }
}
