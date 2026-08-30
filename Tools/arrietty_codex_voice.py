#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 ysk424
# SPDX-License-Identifier: MIT

"""Relay completed Codex answers from WSL to the Windows voice bridge."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import time


PROTOCOL = "ARRIETTY_VOICE/1"
DEFAULT_PORT = 49000


def _windows_host_candidates() -> list[str]:
    configured = os.environ.get("ARRIETTY_VOICE_HOST", "").strip()
    candidates: list[str] = [configured] if configured else []
    candidates.append("127.0.0.1")
    try:
        route = subprocess.run(
            ["ip", "route", "show", "default"],
            check=False,
            capture_output=True,
            text=True,
            timeout=2,
        ).stdout.split()
        if "via" in route:
            candidates.append(route[route.index("via") + 1])
    except (OSError, subprocess.SubprocessError, ValueError, IndexError):
        pass
    try:
        for line in Path("/etc/resolv.conf").read_text(encoding="utf-8").splitlines():
            if line.startswith("nameserver "):
                candidates.append(line.split()[1])
                break
    except OSError:
        pass
    return list(dict.fromkeys(value for value in candidates if value))


def send_answer(message_id: str, answer: str, port: int = DEFAULT_PORT) -> None:
    packet = json.dumps(
        {
            "protocol": PROTOCOL,
            "type": "answer",
            "id": message_id,
            "text": answer[:4000],
        },
        ensure_ascii=False,
        separators=(",", ":"),
    ).encode("utf-8")
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sender:
        for host in _windows_host_candidates():
            try:
                sender.sendto(packet, (host, port))
            except OSError:
                continue


def _session_cwd(path: Path) -> str | None:
    try:
        with path.open("r", encoding="utf-8") as source:
            first = json.loads(source.readline())
        if first.get("type") == "session_meta":
            return first.get("payload", {}).get("cwd")
    except (OSError, json.JSONDecodeError):
        pass
    return None


def _newest_session(cwd: str) -> Path | None:
    session_root = Path.home() / ".codex" / "sessions"
    try:
        candidates = sorted(
            session_root.glob("*/*/*/*.jsonl"),
            key=lambda path: path.stat().st_mtime,
            reverse=True,
        )
    except OSError:
        return None
    normalized_cwd = str(Path(cwd).resolve())
    for path in candidates[:30]:
        session_cwd = _session_cwd(path)
        if session_cwd and str(Path(session_cwd).resolve()) == normalized_cwd:
            return path
    return None


def _assistant_final(record: dict) -> tuple[str, str] | None:
    if record.get("type") != "response_item":
        return None
    payload = record.get("payload", {})
    if (
        payload.get("type") != "message"
        or payload.get("role") != "assistant"
        or payload.get("phase") != "final"
    ):
        return None
    parts = [
        item.get("text", "")
        for item in payload.get("content", [])
        if item.get("type") == "output_text"
    ]
    text = "\n".join(part for part in parts if part).strip()
    return (payload.get("id", "codex-final"), text) if text else None


def watch(cwd: str, port: int) -> int:
    session = _newest_session(cwd)
    if session is None:
        print("Codex session JSONL was not found for " + cwd, file=sys.stderr)
        return 2
    print("Watching Codex answers: " + str(session), flush=True)
    with session.open("r", encoding="utf-8") as source:
        source.seek(0, os.SEEK_END)
        while True:
            line = source.readline()
            if not line:
                time.sleep(0.25)
                continue
            try:
                final = _assistant_final(json.loads(line))
            except json.JSONDecodeError:
                continue
            if final:
                send_answer(final[0], final[1], port)


def notify(payload_text: str, port: int) -> int:
    try:
        payload = json.loads(payload_text)
    except json.JSONDecodeError:
        return 2
    if payload.get("type") != "agent-turn-complete":
        return 0
    answer = payload.get("last-assistant-message", "")
    if answer:
        send_answer(payload.get("turn-id", "codex-notify"), answer, port)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--watch", action="store_true")
    mode.add_argument("--notify", action="store_true")
    parser.add_argument("--cwd", default=os.getcwd())
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("payload", nargs="?")
    args = parser.parse_args()
    if args.watch:
        return watch(args.cwd, args.port)
    if not args.payload:
        return 2
    return notify(args.payload, args.port)


if __name__ == "__main__":
    raise SystemExit(main())
