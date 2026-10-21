# Knowledge recap: ownership before code

This is the minimum model needed for the first project stage.

## Retain these ideas

- `probe()` owns the device's initial lifetime reference; each successful open
  owns another reference.
- A lock protects a state invariant. A reference count protects lifetime. They
  are not interchangeable.
- `copy_to_user()` runs in process context and may fault, so no spinlock may be
  held across it.
- A wait queue is correct only when the waiter and producer use the same
  protected readiness predicate.
- Removing the device node stops new opens; it does not invalidate existing
  file descriptors.

## Applied checks

**A reader is blocked with an empty FIFO when removal starts. What must wake it?**

Removal sets the terminal state first and then wakes the reader's wait queue.
The predicate becomes true because the device is dead, even though no event was
added. The reader can return EOF instead of sleeping forever.

**Why not free device state immediately after `misc_deregister()`?**

An already-open file still stores a pointer to that state. The final open-file
release, not node deregistration, proves that no such pointers remain.

**Which two operations must never overlap unsafely?**

Reader-list traversal in the IRQ thread and reader removal in `release()`. They
use the same device spinlock. FIFO mutation by the IRQ and FIFO consumption by
`read()` use the reader's FIFO spinlock.

## Handoff

In Lab 1, turn these statements into an ownership table and locate each state
transition in the supplied reference before modifying the starter.

