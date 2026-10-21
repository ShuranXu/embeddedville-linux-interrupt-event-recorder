#!/usr/bin/env sh
set -eu
cd /workspace
activity="${1:-all}"

./scripts/gpio-sim-cleanup.sh || true
./scripts/gpio-sim-setup.sh

case "$activity" in
  lab1)
    uname -r | grep -Fx 6.18.57
    echo 'pinned QEMU target is ready'
    exit 0
    ;;
  lab2|lab3|lab4|lab5|capstone|all) ;;
  *) echo "unknown activity: $activity" >&2; exit 2 ;;
esac

insmod ./irqrec_fixture.ko
insmod ./irqrec.ko
test -c /dev/irqrec0
grep -q irqrec /proc/interrupts

case "$activity" in
  lab2) test -L /sys/bus/platform/devices/irqrec/driver ;;
  lab3) grep -q irqrec /proc/interrupts; ./build/irqrec-test > evidence/runtime/public-test.txt; cat evidence/runtime/public-test.txt ;;
  lab4|lab5|capstone|all) ./build/irqrec-test > evidence/runtime/public-test.txt; cat evidence/runtime/public-test.txt ;;
esac

cat /proc/interrupts > evidence/runtime/proc-interrupts.txt
{
	echo "value-control=$(cat /run/irqrec-sim-value)"
	find /sys/kernel/config/gpio-sim/irqrec -maxdepth 3 -type f -print
} > evidence/runtime/gpio-sim-state.txt

case "$activity" in
  lab5|capstone|all)
	./build/irqrec-read /dev/irqrec0 > evidence/runtime/removal-reader.txt 2>&1 &
	reader_pid=$!
	sleep 1
	rmmod irqrec_fixture
	for _ in $(seq 1 20); do
		if ! kill -0 "$reader_pid" 2>/dev/null; then break; fi
		sleep 0.1
	done
	if kill -0 "$reader_pid" 2>/dev/null; then
		kill "$reader_pid" 2>/dev/null || true
		echo 'reader did not wake after device removal' >&2
		exit 1
	fi
	wait "$reader_pid"
	grep -q 'device removed' evidence/runtime/removal-reader.txt
	test ! -e /dev/irqrec0
	rmmod irqrec

	for iteration in $(seq 1 100); do
		insmod ./irqrec_fixture.ko
		insmod ./irqrec.ko
		test -c /dev/irqrec0
		test -L /sys/bus/platform/devices/irqrec/driver
		rmmod irqrec_fixture
		test ! -e /dev/irqrec0
		rmmod irqrec
	done
	echo 'one hundred bind/unbind and load/unload cycles passed' > evidence/runtime/lifecycle-stress.txt
	;;
  *)
	rmmod irqrec_fixture
	test ! -e /dev/irqrec0
	rmmod irqrec
	;;
esac

log="$(dmesg 2>/dev/null || true)"
if printf '%s\n' "$log" | grep -Eiq 'BUG:|WARNING:|KASAN:|UBSAN:|kernel panic|possible circular locking dependency|INFO: task .* blocked'; then
	printf '%s\n' "$log" >&2
	exit 1
fi

printf '%s\n' "$log" | tail -n 80 > evidence/runtime/dmesg.txt
echo 'guest runtime and removal checks passed'

