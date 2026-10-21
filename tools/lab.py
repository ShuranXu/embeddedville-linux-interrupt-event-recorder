#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / "release-lock.json").read_text())
STATE = ROOT / ".lab-state"
PORT = str(LOCK["runtime"]["sshPort"])
TOOLCHAIN_ROOT = Path(os.environ.get("IRQREC_TOOLCHAIN_ROOT", "/opt/embeddedville"))
QEMU_DATA_ROOT = Path(os.environ.get(
    "IRQREC_QEMU_DATA_DIR",
    str(TOOLCHAIN_ROOT.parent.parent / "usr/local/share/qemu"),
))
ACTIVITIES = {"lab1", "lab2", "lab3", "lab4", "lab5", "capstone", "all"}

def run(*command: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    print("+", " ".join(command), flush=True)
    return subprocess.run(command, cwd=ROOT, text=True, check=check)

def doctor(_: argparse.Namespace) -> None:
    problems = []
    for tool in ("bash", "make", "gcc", "qemu-system-x86_64", "ssh", "ssh-keygen", "sparse"):
        if shutil.which(tool) is None:
            problems.append(f"missing {tool}")
    kernel = TOOLCHAIN_ROOT / "linux-6.18.57/include/config/kernel.release"
    if not kernel.is_file() or kernel.read_text().strip() != LOCK["sources"]["linux"]["version"]:
        problems.append("prepared Linux 6.18.57 tree is missing")
    for artifact in (TOOLCHAIN_ROOT / "guest/bzImage", TOOLCHAIN_ROOT / "guest/initramfs.cpio.gz"):
        if not artifact.is_file():
            problems.append(f"missing {artifact}")
    if not (QEMU_DATA_ROOT / "bios-256k.bin").is_file():
        problems.append(f"missing QEMU firmware under {QEMU_DATA_ROOT}")
    if shutil.which("qemu-system-x86_64"):
        version = subprocess.run(["qemu-system-x86_64", "--version"], text=True, capture_output=True).stdout
        if LOCK["sources"]["qemu"]["version"] not in version:
            problems.append("QEMU version differs from 11.1.2 lock")
    if problems:
        raise SystemExit("doctor failed: " + "; ".join(problems))
    print("doctor: Linux 6.18.57, QEMU 11.1.2 and TCG lab dependencies are ready")

def build(_: argparse.Namespace) -> None:
    kernel_dir = f"KDIR={TOOLCHAIN_ROOT / 'linux-6.18.57'}"
    run("make", kernel_dir, "W=1")
    run("make", kernel_dir, "C=2", "W=1")

def boot(_: argparse.Namespace) -> None:
    run("bash", "tools/qemu-boot.sh")

def guest(*command: str) -> None:
    key_path_file = STATE / "ssh-key-path"
    key = Path(key_path_file.read_text().strip()) if key_path_file.is_file() else STATE / "id_ed25519"
    if not key.exists():
        raise SystemExit("guest key missing; run ./lab boot first")
    run("ssh", "-i", str(key), "-p", PORT, "-o", "BatchMode=yes", "-o", "IdentitiesOnly=yes",
        "-o", "StrictHostKeyChecking=no", "-o", "UserKnownHostsFile=/dev/null",
        "root@127.0.0.1", *command)

def run_lab(args: argparse.Namespace) -> None:
    activity = getattr(args, "activity", None) or "all"
    if activity not in ACTIVITIES:
        raise SystemExit("unknown activity; use lab1, lab2, lab3, lab4, lab5, capstone, or all")
    build(args)
    boot(args)
    guest("/bin/sh", "/workspace/tests/public/guest.sh", activity)

def test(args: argparse.Namespace) -> None:
    run("bash", "tests/public/host.sh")
    run_lab(args)
    print("test: public host and QEMU guest acceptance checks passed")

def reset(_: argparse.Namespace) -> None:
    pid_file = STATE / "qemu.pid"
    if pid_file.is_file():
        try:
            pid = int(pid_file.read_text().strip())
            cmdline = Path(f"/proc/{pid}/cmdline").read_bytes()
            if b"qemu-system-x86_64" in cmdline and str(ROOT).encode() in cmdline:
                os.kill(pid, signal.SIGTERM)
                for _ in range(30):
                    try: os.kill(pid, 0)
                    except ProcessLookupError: break
                    time.sleep(0.1)
            else:
                print(f"ignoring stale or foreign PID {pid}")
        except (ValueError, FileNotFoundError, ProcessLookupError, PermissionError):
            pass
    key_path_file = STATE / "ssh-key-path"
    if key_path_file.is_file():
        key = Path(key_path_file.read_text().strip())
        if key.parent.name.startswith("irqrec-lab-") and key.parent.parent == Path("/tmp"):
            shutil.rmtree(key.parent, ignore_errors=True)
    run("make", f"KDIR={TOOLCHAIN_ROOT / 'linux-6.18.57'}", "clean", check=False)
    shutil.rmtree(STATE, ignore_errors=True)
    print("reset: generated build and guest state removed; source and evidence preserved")

def main() -> None:
    parser = argparse.ArgumentParser(prog="./lab")
    sub = parser.add_subparsers(required=True)
    for name, function in (("doctor", doctor), ("build", build), ("boot", boot),
                           ("run", run_lab), ("test", test), ("reset", reset)):
        command = sub.add_parser(name)
        if name in {"run", "test"}:
            command.add_argument("activity", nargs="?")
        command.set_defaults(function=function)
    args = parser.parse_args()
    args.function(args)

if __name__ == "__main__":
    main()

