# Threaded IRQ and debounce

## The problem: an edge arrives in a constrained context

Interrupt latency matters, but so does correctness. A hard IRQ handler cannot
sleep, allocate casually, copy to userspace, or call a GPIO accessor that may
sleep. The event recorder does not need hard-context work, so it requests a
threaded handler with a default primary handler and `IRQF_ONESHOT`.

<figure class="evidence">
  <a href="../../assets/visuals/linux-kernel-labs-interrupt-phases.png">
    <img src="../../assets/visuals/linux-kernel-labs-interrupt-phases.png" alt="Linux Kernel Labs diagram dividing interrupt processing into critical, immediate, and deferred phases, with IRQ acknowledgement and later deferred execution.">
  </a>
  <figcaption>Upstream Linux Kernel Labs interrupt phases. A threaded handler moves the driver's substantial work into scheduled context while the IRQ core controls masking and acknowledgement. Select to enlarge.</figcaption>
</figure>

The registration is intentionally explicit:

```c
devm_request_threaded_irq(dev, irq, NULL, irqrec_irq_thread,
        IRQF_ONESHOT | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
        dev_name(dev), idev);
```

With `handler == NULL`, the IRQ core supplies the small primary handler that
wakes the thread. `IRQF_ONESHOT` keeps the line masked until the threaded
function finishes. The thread may call `gpiod_get_value_cansleep()`.

## Timestamp and edge identity

Use `ktime_get_ns()` for a monotonic timestamp. Wall-clock time can move when a
clock is corrected and is therefore unsuitable for event ordering or interval
measurement. The sequence number is device-wide and increments once per
accepted edge. All readers receive the same sequence/timestamp identity.

The driver derives the edge from the last accepted stable value. The first
record may be `UNKNOWN` because there is no previous accepted sample. Reporting
that uncertainty is better than inventing a rising or falling edge.

## Debounce is a policy with observable loss

The driver first asks the controller to apply `gpiod_set_debounce()`. A
controller may reject it because no hardware debounce exists. That capability
failure activates the documented software fallback: ignore events that arrive
within `debounce_us` of the last accepted event and increment the `debounced`
counter.

This simple interval filter is deterministic and testable, but it is not a full
state-machine debouncer. It can suppress a legitimate rapid transition. The
course exposes the interval and counter so a system integrator can reason about
the tradeoff instead of hiding it.

## Producing for many readers

The IRQ thread creates one immutable event, then traverses the reader list under
the device spinlock. For each reader it takes the FIFO spinlock, applies that
reader's overflow policy, and queues a copy. It wakes the reader after releasing
the FIFO lock.

The critical sections contain no sleeping operation. GPIO access and timestamp
creation happen before list locking. Userspace copying happens later in process
context. Lock order is always device-reader-list lock before reader-FIFO lock;
the read path never attempts the reverse pair.

## Runtime evidence

Use `/proc/interrupts` to show that the Linux IRQ count changes. Use the JSON
Lines collector to show sequence, timestamp, value, and derived edge. Use two
collectors to show identical event identity. These observations answer different
questions; no single screenshot proves the whole mechanism.

Continue to the [Module 3 recap](../recaps/03-threaded-irq.md), then perform
[Lab 3](../labs/03-irq.md).

