// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Text.RegularExpressions;

namespace ArriettyVoiceBridge
{
    internal sealed class BridgeOptions
    {
        private static readonly Regex TmuxTargetPattern = new Regex(
            @"^(%[0-9]+|[A-Za-z0-9_.-]+:[0-9]+\.[0-9]+)$",
            RegexOptions.CultureInvariant);
        private static readonly Regex DistroPattern = new Regex(
            @"^[A-Za-z0-9_. -]+$",
            RegexOptions.CultureInvariant);

        public string TmuxTarget { get; private set; }
        public string WslDistro { get; private set; }
        public int Port { get; private set; }

        public static BridgeOptions Parse(string[] args)
        {
            var options = new BridgeOptions { Port = 49000 };
            for (var index = 0; index < args.Length; ++index)
            {
                var name = args[index];
                if (name == "--tmux-target")
                {
                    options.TmuxTarget = ReadValue(args, ref index, name);
                }
                else if (name == "--wsl-distro")
                {
                    options.WslDistro = ReadValue(args, ref index, name);
                }
                else if (name == "--port")
                {
                    int port;
                    if (!int.TryParse(ReadValue(args, ref index, name), out port) || port < 1024 || port > 65535)
                    {
                        throw new ArgumentException("--port は 1024..65535 で指定してください。");
                    }
                    options.Port = port;
                }
                else
                {
                    throw new ArgumentException("不明な引数です: " + name);
                }
            }

            if (!IsValidTmuxTarget(options.TmuxTarget))
            {
                throw new ArgumentException("--tmux-target には現在の TMUX_PANE（例: %0）が必要です。");
            }
            if (string.IsNullOrWhiteSpace(options.WslDistro) || !DistroPattern.IsMatch(options.WslDistro))
            {
                throw new ArgumentException("--wsl-distro には WSL_DISTRO_NAME が必要です。");
            }
            return options;
        }

        public static bool IsValidTmuxTarget(string target)
        {
            return !string.IsNullOrWhiteSpace(target) && TmuxTargetPattern.IsMatch(target);
        }

        private static string ReadValue(string[] args, ref int index, string name)
        {
            if (++index >= args.Length)
            {
                throw new ArgumentException(name + " の値がありません。");
            }
            return args[index];
        }
    }
}
