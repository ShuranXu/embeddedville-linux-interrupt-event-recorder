# Capstone - GPIO interrupt event recorder

## Project outcome

Deliver one Linux platform driver and userspace collector that turn a simulated
GPIO input into a robust, versioned event stream. The QEMU run is authoritative;
no physical board is required or accepted as a substitute for missing QEMU
evidence.

## Required interface

Device node: `/dev/irqrec0`, mode `0660`.

Event v1 is exactly 32 bytes and contains:

- `u64` sequence, device-wide and monotonically increasing;
- `u64` monotonic timestamp in nanoseconds;
- `u8` stable logical value;
- `u8` edge (`UNKNOWN`, `RISING`, or `FALLING`);
- `u16` flags, including `OVERFLOW`;
- twelve reserved zero bytes.

`IRQREC_IOC_GET_INFO` returns ABI version, event size, FIFO depth, debounce
interval, and feature flags. `IRQREC_IOC_GET_STATS` returns device production
and debounce counts plus per-open delivery and drop counts.

## Required behavior

- Bind as an `irqrec` platform driver and support
  `embeddedville,irqrec-v1` Device Tree matching.
- Obtain the `event` GPIO descriptor, map it to an IRQ, request both edges with
  a threaded handler, and use hardware debounce or the stated software fallback.
- Allocate one FIFO and wait queue per open. Readers are independent subscribers.
- Support blocking, `O_NONBLOCK`, batched whole-record reads, signals, `poll`,
  and compatible ioctls.
- On overflow, discard the oldest queued record for that reader, preserve the
  newest, increment `dropped`, and mark the following record.
- On removal, reject new opens, stop IRQ delivery, wake readers, unregister the
  node, allow queued records to drain, and retain state until the last release.

## Deliverables

1. Driver, QEMU fixture, UAPI header, and collector source.
2. Successful `./lab test` output tied to the locked environment.
3. Selectable evidence transcripts for binding, `/proc/interrupts`, two-reader
   identity, overflow, removal, and diagnostics.
4. A short engineering note defending lock ownership, overflow policy, ABI
   compatibility, and teardown order.

## Deliberate exclusions

Do not add networking, a database, a graphical dashboard, power management,
DMA, or a custom QEMU device. Those could be later products, but they do not
help prove this project's interrupt and lifecycle contract.

