# FIFO, blocking I/O, poll and ioctl

## The problem: an ABI outlives its implementation

Once a userspace program ships, structure layout and error behavior become a
contract. Kernel pointers, `long`, implicit padding, and enum width are not
stable wire formats. The project uses Linux fixed-width UAPI types and explicit
reserved space.

```c
struct irqrec_event_v1 {
        __u64 sequence;
        __u64 timestamp_ns;
        __u8  value;
        __u8  edge;
        __u16 flags;
        __u32 reserved[3];
}; /* exactly 32 bytes */
```

`IRQREC_IOC_GET_INFO` reports the ABI version, event size, FIFO depth, debounce
interval, and feature flags. A collector checks these values before interpreting
records. `IRQREC_IOC_GET_STATS` separates device-wide production/debounce from
reader-specific delivery/drop counts.

## Read semantics

The driver accepts only a nonzero byte count that is a multiple of the event
size. A valid call may return several complete records. It never returns a
partial record.

When the reader's queue is empty:

- `O_NONBLOCK` returns `-EAGAIN`;
- a blocking descriptor sleeps with `wait_event_interruptible()`;
- a signal returns `-ERESTARTSYS`, which the VFS presents as an interrupted
  system call when it is not restarted;
- a removed device returns EOF after the queue and staging slot are empty.

The wait predicate is checked before sleeping and again after wakeup. This is
what prevents the classic lost-wakeup race between testing the queue and adding
the task to the wait queue.

## Poll must describe the same state as read

`poll_wait(file, &reader->wait, wait)` registers interest; it does not sleep by
itself. The callback then computes readiness from the same per-reader predicate
used by `read()`.

| Reader state | Poll result | Following read |
| --- | --- | --- |
| record staged or FIFO nonempty | `POLLIN|POLLRDNORM` | at least one full record |
| empty and device active | no readable bit | blocks or `EAGAIN` |
| removed, records remain | readable plus terminal bits | drains records |
| removed and empty | `POLLHUP|POLLERR` | EOF |

Readiness is level-triggered: it remains true while unread data exists. This
makes ordinary `poll` and `epoll` loops predictable.

## Independent readers and overflow

A global FIFO would distribute events: reader A could consume the record before
reader B sees it. Each open therefore allocates a private `kfifo`. Slow reader B
can overflow without affecting reader A.

When a FIFO is full, the driver discards its oldest queued record, increments
that reader's `dropped` counter, preserves the new record, and sets
`IRQREC_EVENT_F_OVERFLOW` on the next queued record. Preserving the newest state
is appropriate for an edge-monitoring stream; another domain might choose to
reject new data. The important point is that the policy is explicit and visible.

## Fault-safe delivery

`copy_to_user()` may fail. Popping first and then copying loses a record on
`-EFAULT`; holding a spinlock across the copy is illegal. The per-reader staging
slot resolves this: the FIFO head is moved into stable kernel memory, the copy is
attempted without the spinlock, and the slot is cleared only on success.

Continue to the [Module 4 recap](../recaps/04-event-abi.md), then perform
[Lab 4](../labs/04-abi.md).

