#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

set -euo pipefail

if [[ -z "${TMUX_PANE:-}" ]]; then
    echo "tmux内のCodexペインから実行してください。" >&2
    exit 2
fi
if [[ -z "${WSL_DISTRO_NAME:-}" ]]; then
    echo "WSL_DISTRO_NAME が見つかりません。" >&2
    exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "$script_dir/.." && pwd)"
powershell_start="$(wslpath -w "$script_dir/ArriettyVoiceBridge/start.ps1")"

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$powershell_start" \
    -TmuxTarget "$TMUX_PANE" \
    -WslDistro "$WSL_DISTRO_NAME"

watcher="$script_dir/arrietty_codex_voice.py"
log_file="/tmp/arrietty-codex-voice-${UID}.log"
session_name="$(tmux display-message -p -t "$TMUX_PANE" '#{session_name}')"
watch_window="$session_name:arrietty-voice-watch"
watch_command="exec python3 '$watcher' --watch --cwd '$project_dir' >>'$log_file' 2>&1"
if tmux list-windows -t "$session_name" -F '#{window_name}' | rg -qx 'arrietty-voice-watch'; then
    tmux respawn-window -k -t "$watch_window" "$watch_command"
else
    tmux new-window -d -t "$session_name:" -n arrietty-voice-watch "$watch_command"
fi
watcher_pid="$(tmux list-panes -t "$watch_window" -F '#{pane_pid}' | head -n 1)"
tmux select-window -t "$TMUX_PANE"
echo "Codex answer watcher started in tmux $watch_window (PID $watcher_pid, log $log_file)."
