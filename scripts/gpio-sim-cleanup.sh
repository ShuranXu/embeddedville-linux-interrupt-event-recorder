#!/usr/bin/env sh
set -eu
base=/sys/kernel/config/gpio-sim/irqrec
rmmod irqrec_fixture 2>/dev/null || true
rmmod irqrec 2>/dev/null || true
if [ -d "$base" ]; then
	if [ -e "$base/live" ]; then echo 0 > "$base/live" || true; fi
	rmdir "$base/bank0/line0" 2>/dev/null || true
	rmdir "$base/bank0" 2>/dev/null || true
	rmdir "$base" 2>/dev/null || true
fi
rm -f /run/irqrec-sim-value

