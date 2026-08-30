// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Text.RegularExpressions;

namespace ArriettyVoiceBridge
{
    internal static class SpeechTextCleaner
    {
        public static string Prepare(string value)
        {
            var text = value ?? string.Empty;
            text = Regex.Replace(text, @"```[\s\S]*?```", " コード部分は画面をご覧ください。 ");
            text = Regex.Replace(text, @"\[([^\]]+)\]\([^\)]+\)", "$1");
            text = text.Replace("`", string.Empty).Replace("**", string.Empty).Replace("__", string.Empty);
            text = Regex.Replace(text, @"(?m)^\s{0,3}[#>*+-]+\s*", string.Empty);
            text = Regex.Replace(text, @"\s+", " ").Trim();
            if (text.Length <= 500)
            {
                return text;
            }
            var shortened = text.Substring(0, 500);
            var sentenceEnd = Math.Max(shortened.LastIndexOf('。'), shortened.LastIndexOf('！'));
            sentenceEnd = Math.Max(sentenceEnd, shortened.LastIndexOf('？'));
            return (sentenceEnd >= 80 ? shortened.Substring(0, sentenceEnd + 1) : shortened + "…").Trim();
        }
    }
}
