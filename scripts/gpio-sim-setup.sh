#!/usr/bin/env sh
set -eu

base=/sys/kernel/config/gpio-sim/irqrec

if ! grep -qs ' /sys/kernel/config configfs ' /proc/mounts; then
	mount -t configfs configfs /sys/kernel/config
fi
modprobe gpio-sim

if [ -d "$base" ]; then
	if [ -e "$base/live" ]; then echo 0 > "$base/live" || true; fi
	rmdir "$base/bank0/line0" 2>/dev/null || true
	rmdir "$base/bank0" 2>/dev/null || true
	rmdir "$base" 2>/dev/null || true
fi

mkdir "$base"
mkdir "$base/bank0"
echo 1 > "$base/bank0/num_lines"
mkdir "$base/bank0/line0"
echo irqrec-event > "$base/bank0/line0/name"
echo 1 > "$base/live"

value_path="$(find /sys/devices/platform -type f -path '*/sim_gpio0/pull' | head -n 1)"
if [ -z "$value_path" ]; then
	echo 'gpio-sim pull control not found' >&2
	exit 1
fi
printf '%s\n' "$value_path" > /run/irqrec-sim-value
echo pull-down > "$value_path"
printf 'gpio-sim ready: %s\n' "$value_path"

