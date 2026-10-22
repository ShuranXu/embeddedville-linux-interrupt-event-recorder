# Lab 4 - Verify the userspace ABI

<div class="lab-launch" markdown>
[Run Lab 4 in QEMU](../start/environment.md#launch-a-guided-lab){ .md-button .md-button--primary }
[Open Codespaces](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.2?quickstart=1){ .md-button }
</div>

<p class="lab-launch-note"><strong>Workspace command:</strong> <code>./lab run lab4</code>. The Codespaces button opens or resumes the pinned course workspace.</p>

## Purpose

Make the device usable by event-loop programs and independent consumers, not
only by a blocking `cat`-style test.

## Task

Implement the 32-byte event, info/stats ioctls, per-open FIFO, blocking and
nonblocking read, staging slot, and poll callback. Then run:

```console
./lab run lab4
```

The public runtime tool performs these contract checks:

1. compiled structure sizes are exactly 32 bytes;
2. a one-byte read fails with `EINVAL`;
3. an empty nonblocking read fails with `EAGAIN`;
4. `poll` wakes after delayed gpio-sim stimulus;
5. two readers receive the same identity, edge, value, and timestamp;
6. sequence and monotonic timestamps increase across events;
7. a failed `copy_to_user` returns `EFAULT` without consuming the staged record;
8. a signal interrupts a blocking read with `EINTR`;
9. a deliberately slow reader discards oldest records and marks its next record;
10. a batched read contains only whole records; and
11. stats expose produced, debounced, delivered, and dropped counts.

## Inspect the system calls

If `strace` is present in the guest/toolchain, trace the collector:

```console
strace -tt -e openat,ioctl,poll,read,close ./build/irqrec-read /dev/irqrec0
```

The important evidence is the transition from a waiting `poll` to one or more
whole-record `read` results. A terminal screenshot is unnecessary; preserve the
selectable transcript.

## Failure experiment

Use the runtime test's invalid-pointer case or a tiny purpose-built call to
cause `EFAULT`, then retry with a valid buffer. The same staged event must remain
available and `delivered` must not advance on the failed copy.

## Completion evidence

- info ioctl output and compile-time layout assertions;
- nonblocking and poll results;
- same event identity observed by two readers;
- overflow flag and dropped counter;
- retained record after `EFAULT`.

