// SPDX-FileCopyrightText: 2026 ysk424
// SPDX-License-Identifier: MIT

using System;
using System.Text;
using System.Threading;

namespace ArriettyVoiceBridge
{
    internal static class Program
    {
        private const string MutexName = @"Local\ArriettyVoiceBridge-83C21DA2-0E98-45C5-A46A-C9C06EE7686C";

        private static int Main(string[] args)
        {
            Console.OutputEncoding = new UTF8Encoding(false);
            if (Array.IndexOf(args, "--self-test") >= 0)
            {
                return SelfTests.Run();
            }

            BridgeOptions options;
            try
            {
                options = BridgeOptions.Parse(args);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(exception.Message);
                return 2;
            }

            bool ownsMutex;
            using (var mutex = new Mutex(true, MutexName, out ownsMutex))
            {
                if (!ownsMutex)
                {
                    Console.Error.WriteLine("Arrietty Voice Bridge はすでに起動しています。");
                    return 3;
                }

                using (var bridge = new VoiceBridge(options))
                {
                    Console.CancelKeyPress += delegate(object sender, ConsoleCancelEventArgs eventArgs)
                    {
                        eventArgs.Cancel = true;
                        bridge.Stop();
                    };
                    return bridge.Run();
                }
            }
        }
    }
}
