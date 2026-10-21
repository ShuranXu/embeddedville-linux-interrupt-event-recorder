# Troubleshooting

Use the narrowest evidence source that crosses the failing boundary.

## `./lab doctor` fails

Do not build against whatever headers happen to be installed. Confirm the
content-digested dev-container image or construct the runtime from
`release-lock.json`. QEMU must report 11.1.2 and expose TCG; the prepared kernel
release must be 6.18.57.

## gpio-sim setup cannot find `sim_gpio0/pull`

Check that configfs is mounted, `gpio-sim` is loaded, and `live` is `1`:

```console
mount | grep configfs
lsmod | grep gpio_sim
find /sys/kernel/config/gpio-sim/irqrec -maxdepth 3 -type f -print
dmesg | tail -n 80
```

Set `live=0` before trying to alter the bank structure. The simulator freezes
its configuration while live.

## Fixture loads but driver does not bind

Inspect `/sys/bus/platform/devices/irqrec`, its `modalias`, and the driver ID
table. Then inspect the probe error. A missing line-name lookup should fail
cleanly; it should not be bypassed with an integer GPIO parameter.

## `/proc/interrupts` does not change

Confirm the driver requested both edge flags and that the simulated pull value
actually changes. Rewriting `pull-up` while already high is not an edge. Alternate
`pull-up` and `pull-down`, and check whether debounce intentionally filtered the
record after the IRQ count increased.

## `poll` wakes but `read` returns `EAGAIN`

Poll and read probably evaluate different predicates, or the ready record was
consumed by shared global state. Both paths must inspect the same reader's
staging slot/FIFO. A second reader must have a different FIFO.

## Module unload hangs

Look for `synchronize_irq()` called while holding the reader-list or FIFO lock.
An in-flight handler may need that lock. Also verify that every blocked reader
is woken after terminal state is visible.

## QEMU exits

Preserve `.lab-state/serial.log` before `./lab reset`. With `oops=panic`, a
kernel warning may terminate the guest intentionally. The last stack trace is
the primary evidence; the SSH failure is only a consequence.

