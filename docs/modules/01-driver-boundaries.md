# From demo to driver

## The problem: producing an event is the easy part

A timer callback can increment a counter and wake a process. A GPIO handler can
do the same after a physical edge. Neither fact defines a durable driver. The
difficult work sits around the callback: ownership, concurrency, failure, and
the meaning of an open file while a device disappears.

The original `irqdemo.c` is useful because it isolates the wait/read/poll idea.
The original `gpio_irq.c` adds a real IRQ path. This course preserves those
lessons and changes the architecture: platform binding replaces module
parameters, GPIO descriptors replace integer GPIO numbers, and per-open queues
replace a single latest-event counter.

## Four lifetimes exist at once

1. **Module lifetime** begins when code is loaded and ends after exit.
2. **Platform-device lifetime** begins at registration/discovery and surrounds
   a successful `probe()`/`remove()` pair.
3. **IRQ registration lifetime** begins after the IRQ request succeeds and ends
   only after the source is disabled and in-flight handlers are synchronized.
4. **Open-file lifetime** begins in `open()` and may extend beyond `remove()`.

The final point is where demonstration drivers commonly fail. A file descriptor
is a userspace reference to kernel state. Unregistering `/dev/irqrec0` prevents
new opens, but it does not revoke existing file descriptions. If `remove()`
frees the device immediately, their next `read()` dereferences freed memory.

The reference design gives the device a `kref`. Probe owns one reference. Every
successful open takes one. Remove marks the device dead, blocks new opens,
stops IRQ production, wakes readers, and drops the probe reference. The last
release performs the final free.

## File operations are behavioral promises

`file_operations` is not merely a routing table. Each method defines observable
semantics:

- `open`: allocate a private subscription and reject a removed device;
- `read`: return whole records, block when empty, honor `O_NONBLOCK`, propagate
  signals, and return EOF only after removal and queue drain;
- `poll`: register the same wait queue used by `read`, report readability for
  this reader, and report terminal state after removal;
- `unlocked_ioctl`: copy fixed-width versioned metadata, never internal pointers;
- `release`: detach this subscription before freeing it and dropping the device
  reference.

`file->private_data` begins as the miscdevice pointer when the misc layer calls
open. The driver replaces it with the allocated reader only after that reader
is fully initialized and linked. Error paths undo resources in reverse order.

## Concurrency begins before SMP

Even a one-vCPU guest has concurrency between process context, the threaded IRQ,
signals, and removal. With two vCPUs, those paths may truly overlap. Assign one
lock to each invariant:

| State | Synchronization | Reason |
| --- | --- | --- |
| reader list membership | device spinlock | IRQ traversal and release cannot sleep |
| one reader's FIFO/flags/counters | reader spinlock | shared with IRQ producer |
| staging one userspace read | reader mutex | copy_to_user may fault or sleep |
| device lifetime | `kref` | ownership, not mutual exclusion |
| blocking condition | wait queue + protected predicate | avoids lost wakeups |

Never hold a spinlock across `copy_to_user()`. The reference implementation pops
the head into a per-reader staging slot, copies from the stable slot, and clears
it only after a successful copy. An `-EFAULT` therefore does not silently consume
the record.

## Evidence to seek

The lifecycle is credible when you can leave a descriptor open, unregister the
platform device, observe `POLLHUP|POLLERR`, drain any queued records, receive
EOF, and finally unload the driver without warnings. A clean insertion/removal
log alone does not exercise that case.

Continue to the [Module 1 recap](../recaps/01-driver-boundaries.md), then perform
[Lab 1](../labs/01-target.md).

