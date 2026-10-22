#!/usr/bin/env python3
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
LOCK = json.loads((ROOT / "release-lock.json").read_text())
STATE = ROOT / ".lab-state"
PORT = os.environ.get("IRQREC_GUEST_SSH_PORT", str(LOCK["runtime"]["sshPort"]))
TOOLCHAIN_ROOT = Path(os.environ.get("IRQREC_TOOLCHAIN_ROOT", "/opt/embeddedville"))
QEMU_DATA_ROOT = Path(os.environ.get(
    "IRQREC_QEMU_DATA_DIR",
    str(TOOLCHAIN_ROOT.parent.parent / "usr/local/share/qemu"),
))
ACTIVITY_ALIASES = {
    "lab1": "lab1", "irqrec-lab-target": "lab1",
    "lab2": "lab2", "irqrec-lab-platform": "lab2",
    "lab3": "lab3", "irqrec-lab-irq": "lab3",
    "lab4": "lab4", "irqrec-lab-abi": "lab4",
    "lab5": "lab5", "irqrec-lab-hardening": "lab5",
    "capstone": "capstone", "irqrec-capstone": "capstone", "all": "all",
}

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
    requested = getattr(args, "activity", None) or "all"
    activity = ACTIVITY_ALIASES.get(requested)
    if activity is None:
        raise SystemExit("unknown activity; use an irqrec-lab-* ID, lab1 through lab5, irqrec-capstone, capstone, or all")
    build(args)
    boot(args)
    guest("/bin/sh", "/workspace/tests/public/guest.sh", activity)

def test(args: argparse.Namespace) -> None:
    requested = getattr(args, "activity", None) or "all"
    activity = ACTIVITY_ALIASES.get(requested)
    if activity is None:
        raise SystemExit("unknown activity; use an irqrec-lab-* ID, lab1 through lab5, irqrec-capstone, capstone, or all")
    run("bash", "tests/public/host.sh", activity)
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

def submission_snapshot() -> tuple[Path, dict[str, object], str, str]:
    mapping = {
        "starter/src/irqrec.c": "src/irqrec.c",
        "starter/include/irqrec_uapi.h": "include/irqrec_uapi.h",
        "Makefile": "Makefile",
    }
    files = []
    for source, archive_path in mapping.items():
        data = (ROOT / source).read_bytes()
        files.append({"path": archive_path, "bytes": len(data),
                      "sha256": hashlib.sha256(data).hexdigest(), "type": "file"})
    manifest = {"schemaVersion": 1, "files": files}
    canonical = json.dumps(manifest, separators=(",", ":"), sort_keys=True).encode()
    evidence = ROOT / "evidence"
    evidence.mkdir(exist_ok=True)
    output = evidence / "irqrec-capstone-submission.zip"
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for source, archive_path in mapping.items():
            info = zipfile.ZipInfo(archive_path, (1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o100644 << 16
            archive.writestr(info, (ROOT / source).read_bytes())
    return output, manifest, hashlib.sha256(canonical).hexdigest(), hashlib.sha256(output.read_bytes()).hexdigest()

def post_json(url: str, payload: dict[str, object], token: str | None = None) -> dict[str, object]:
    headers = {"Content-Type": "application/json"}
    if token:
        headers["Authorization"] = "Bearer " + token
    request = urllib.request.Request(url, data=json.dumps(payload).encode(), headers=headers, method="POST")
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)

def submit(args: argparse.Namespace) -> None:
    if args.activity != "irqrec-capstone":
        raise SystemExit("only --activity irqrec-capstone is submitted; guided labs are formative")
    output, manifest, manifest_digest, snapshot_digest = submission_snapshot()
    origin = os.environ.get("EMBEDDEDVILLE_ORIGIN", "https://embeddedville.com")
    session = post_json(origin + "/api/workspace-submission-sessions", {
        "courseId": "linux-interrupt-event-recorder",
        "activityId": "irqrec-capstone",
        "repository": "ShuranXu/embeddedville-linux-interrupt-event-recorder",
        "starterVersion": "interrupt-event-recorder-v1.0.2",
        "baseRef": "interrupt-event-recorder-v1.0.2",
        "headCommit": None,
        "dirty": True,
        "manifestSha256": manifest_digest,
        "snapshotSha256": snapshot_digest,
        "snapshotBytes": output.stat().st_size,
    })
    print("Approve this exact snapshot:", session["verificationUri"])
    while True:
        request = urllib.request.Request(
            origin + f"/api/workspace-submission-sessions/{session['id']}/status",
            headers={"Authorization": "Bearer " + str(session["pollToken"])},
        )
        with urllib.request.urlopen(request, timeout=30) as response:
            status = json.load(response)["status"]
        if status == "approved":
            break
        if status in {"denied", "expired", "consumed"}:
            raise SystemExit("pairing " + status)
        time.sleep(2)
    exchanged = post_json(
        origin + f"/api/workspace-submission-sessions/{session['id']}/exchange",
        {}, str(session["pollToken"]),
    )
    boundary = "----embeddedville-irqrec-boundary"
    body = (
        f"--{boundary}\r\nContent-Disposition: form-data; name=\"manifest\"\r\n\r\n"
        f"{json.dumps(manifest, separators=(',', ':'), sort_keys=True)}\r\n"
        f"--{boundary}\r\nContent-Disposition: form-data; name=\"snapshot\"; "
        f"filename=\"{output.name}\"\r\nContent-Type: application/zip\r\n\r\n"
    ).encode() + output.read_bytes() + f"\r\n--{boundary}--\r\n".encode()
    request = urllib.request.Request(
        origin + f"/api/workspace-submission-sessions/{session['id']}/submit",
        data=body,
        headers={
            "Authorization": "Bearer " + str(exchanged["submissionToken"]),
            "Content-Type": "multipart/form-data; boundary=" + boundary,
        },
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        print(json.dumps(json.load(response), indent=2))

def main() -> None:
    parser = argparse.ArgumentParser(prog="./lab")
    sub = parser.add_subparsers(required=True)
    for name, function in (("doctor", doctor), ("build", build), ("boot", boot),
                           ("run", run_lab), ("test", test), ("reset", reset)):
        command = sub.add_parser(name)
        if name in {"run", "test"}:
            command.add_argument("activity", nargs="?")
        command.set_defaults(function=function)
    submit_command = sub.add_parser("submit")
    submit_command.add_argument("--activity", required=True)
    submit_command.set_defaults(function=submit)
    args = parser.parse_args()
    args.function(args)

if __name__ == "__main__":
    main()

