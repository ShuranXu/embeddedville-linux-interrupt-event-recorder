# Knowledge recap: interrupt production

## Retain these ideas

- Hard IRQ context cannot call operations that may sleep.
- A threaded handler with `IRQF_ONESHOT` permits the general GPIO accessor and
  keeps the line masked until threaded handling finishes.
- `ktime_get_ns()` supplies monotonic ordering; wall-clock time does not.
- Debounced edges and FIFO-overflow losses are different and require different
  counters.
- One immutable event identity is copied to every active reader.

## Applied checks

**Why is the first edge allowed to be `UNKNOWN`?**

The driver has no prior accepted stable sample. Inventing a direction would make
the record look more precise than the evidence.

**Why not call `wake_up_interruptible()` before inserting the event?**

The awakened task may run immediately, test the predicate, see an empty queue,
and sleep again. Update the protected state before waking waiters.

**A burst produces 80 IRQs but only 20 accepted records. Which evidence narrows
the cause?**

Compare `/proc/interrupts` with `produced` and `debounced`. The IRQ count shows
delivery into the kernel; the debounce counter explains records intentionally
rejected by policy.

## Handoff

Lab 3 captures that evidence and confirms that two readers receive the same
sequence and timestamp for one accepted edge.

