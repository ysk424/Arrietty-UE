// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Diagnostics;
using System.Text;
using System.Threading;

namespace ArriettyVoiceBridge
{
    internal sealed class TmuxClient
    {
        private const string BufferName = "arrietty-voice";
        private static readonly Encoding Utf8WithoutBom = new UTF8Encoding(false);
        private readonly string _distro;
        private readonly string _target;

        public TmuxClient(string distro, string target)
        {
            _distro = distro;
            _target = target;
        }

        public void SendPrompt(string prompt)
        {
            RunWsl(new[] { "tmux", "load-buffer", "-b", BufferName, "-" }, prompt);
            RunWsl(new[] { "tmux", "paste-buffer", "-d", "-b", BufferName, "-t", _target }, null);
            // Match kidukimasu's separate physical Enter: give the Codex TUI a
            // moment to consume all pasted Unicode before submitting.
            Thread.Sleep(350);
            RunWsl(new[] { "tmux", "send-keys", "-t", _target, "Enter" }, null);
        }

        private void RunWsl(string[] command, string standardInput)
        {
            var arguments = new StringBuilder();
            AppendArgument(arguments, "-d");
            AppendArgument(arguments, _distro);
            AppendArgument(arguments, "--");
            foreach (var item in command)
            {
                AppendArgument(arguments, item);
            }

            var startInfo = new ProcessStartInfo
            {
                FileName = "wsl.exe",
                Arguments = arguments.ToString(),
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardInput = standardInput != null,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Utf8WithoutBom,
                StandardErrorEncoding = Utf8WithoutBom
            };
            using (var process = Process.Start(startInfo))
            {
                if (standardInput != null)
                {
                    // .NET Framework creates StandardInput with the Windows
                    // console encoding (Shift-JIS on this PC). tmux and the
                    // Codex terminal expect UTF-8, so bypass StreamWriter and
                    // write explicit UTF-8 bytes without a BOM.
                    var inputBytes = Utf8WithoutBom.GetBytes(standardInput);
                    process.StandardInput.BaseStream.Write(inputBytes, 0, inputBytes.Length);
                    process.StandardInput.BaseStream.Flush();
                    process.StandardInput.Close();
                }
                var error = process.StandardError.ReadToEnd();
                if (!process.WaitForExit(10000))
                {
                    process.Kill();
                    throw new TimeoutException("WSL/tmux が10秒以内に応答しませんでした。");
                }
                if (process.ExitCode != 0)
                {
                    throw new InvalidOperationException("tmux送信に失敗しました: " + error.Trim());
                }
            }
        }

        private static void AppendArgument(StringBuilder target, string value)
        {
            if (target.Length > 0) target.Append(' ');
            // wsl.exe does not use the normal CommandLineToArgvW parsing when
            // launched through .NET Framework ProcessStartInfo.Arguments on
            // this machine. Quoting even the leading "-d" makes WSL try to
            // execute it inside /bin/bash. All bridge arguments are identifiers
            // or fixed tmux tokens, so reject whitespace and pass them raw.
            if (string.IsNullOrWhiteSpace(value) ||
                value.IndexOfAny(new[] { ' ', '\t', '\r', '\n', '"' }) >= 0)
            {
                throw new ArgumentException("WSL/tmux引数に空白または引用符は使用できません: " + value);
            }
            target.Append(value);
        }
    }
}
