#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
state="$root/.lab-state"
toolchain_root="${IRQREC_TOOLCHAIN_ROOT:-/opt/embeddedville}"
kernel="$toolchain_root/guest/bzImage"
initramfs="$toolchain_root/guest/initramfs.cpio.gz"
qemu_data="${IRQREC_QEMU_DATA_DIR:-$toolchain_root/../../usr/local/share/qemu}"
port="${IRQREC_GUEST_SSH_PORT:-2222}"

mkdir -p "$state" "$root/evidence/runtime"
if [[ -s "$state/qemu.pid" ]]; then
  pid="$(cat "$state/qemu.pid")"
  if kill -0 "$pid" 2>/dev/null && tr '\0' ' ' < "/proc/$pid/cmdline" | grep -Fq "path=$root"; then
    echo "QEMU guest already running (pid $pid)."
    exit 0
  fi
  rm -f "$state/qemu.pid"
fi

[[ -s "$kernel" && -s "$initramfs" ]] || { echo "Pinned guest artifacts are missing; run ./lab doctor." >&2; exit 1; }
key_dir="$state"
probe="$state/.permission-probe"
: > "$probe"
chmod 600 "$probe"
if [[ "$(stat -c '%a' "$probe")" != 600 ]]; then
  workspace_id="$(printf '%s' "$root" | sha256sum | cut -c1-16)"
  key_dir="/tmp/irqrec-lab-$workspace_id"
  mkdir -p -m 700 "$key_dir"
fi
rm -f "$probe"
key="$key_dir/id_ed25519"
if [[ ! -s "$key" ]]; then
  ssh-keygen -q -t ed25519 -N '' -C irqrec-lab -f "$key"
fi
chmod 600 "$key"
cp "$key.pub" "$state/guest_authorized_keys"
printf '%s\n' "$key" > "$state/ssh-key-path"

qemu-system-x86_64 \
  -L "$qemu_data" \
  -accel tcg,thread=multi -machine q35 -cpu max,la57=off -smp 2 -m 2048 \
  -kernel "$kernel" -initrd "$initramfs" \
  -append "console=ttyS0 panic=-1 oops=panic nokaslr" \
  -fsdev local,id=workspace,path="$root",security_model=none \
  -device virtio-9p-pci,fsdev=workspace,mount_tag=workspace \
  -netdev "user,id=net0,hostfwd=tcp:127.0.0.1:${port}-:22" \
  -device virtio-net-pci,netdev=net0 \
  -display none -serial "file:$state/serial.log" -monitor none -no-reboot \
  </dev/null >"$state/qemu.log" 2>&1 &
pid=$!
printf '%s\n' "$pid" > "$state/qemu.pid"

ssh_opts=(-i "$key" -p "$port" -o BatchMode=yes -o IdentitiesOnly=yes -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=2)
for _ in $(seq 1 180); do
  if ! kill -0 "$pid" 2>/dev/null; then
    tail -n 100 "$state/serial.log" >&2 || true
    exit 1
  fi
  if ssh "${ssh_opts[@]}" root@127.0.0.1 'test "$(uname -r)" = 6.18.57' >/dev/null 2>&1; then
    echo "QEMU guest ready on 127.0.0.1:${port} using TCG."
    exit 0
  fi
  sleep 1
done
echo 'Timed out waiting for the QEMU guest.' >&2
exit 1

