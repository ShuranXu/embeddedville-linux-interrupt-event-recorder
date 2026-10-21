# Knowledge recap: userspace contract

## Retain these ideas

- Fixed-width types, explicit versioning, reserved fields, and compile-time size
  checks make the binary layout reviewable.
- Read accepts whole-record capacities and returns only whole records.
- Blocking read and poll must use the same readiness predicate.
- Each open file has a private FIFO, wait queue, counters, and staging record.
- Overflow discards that reader's oldest queued record, preserves the newest,
  increments `dropped`, and marks the following record.

## Applied checks

**Why does `GET_STATS` mix device-wide and per-open counters?**

Production and debounce occur once at the device. Delivery and overflow occur
independently for each subscription. The distinction helps identify whether an
event was filtered before fan-out or lost by one slow consumer.

**Why is a one-byte read rejected instead of returning one byte?**

Partial binary records cannot be interpreted safely. `EINVAL` exposes caller
misuse and preserves record framing.

**How does the staging slot protect `-EFAULT` behavior?**

The record leaves the FIFO but remains owned by the reader until the full copy
succeeds. A failed copy does not advance delivered statistics or expose the
next record.

## Handoff

Lab 4 uses ioctl, nonblocking read, blocking poll, batched read, and two-reader
tests to verify the contract from userspace.

