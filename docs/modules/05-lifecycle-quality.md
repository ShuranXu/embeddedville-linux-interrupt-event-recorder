# Removal, observability and diagnostics

## The problem: the unhappy path defines driver quality

Probe and one successful read establish very little. Real systems unbind
devices, interrupt waits with signals, pass invalid pointers, run multiple
readers, and experience bursts faster than userspace can drain them. The final
module treats those cases as part of the interface.

## Removal has an order

The driver uses this shutdown sequence:

1. set the terminal `dead` state;
2. deregister the misc device so no new opens succeed;
3. disable the IRQ and synchronize all hard/threaded handlers;
4. wake every reader so no task remains blocked;
5. drop probe's lifetime reference;
6. let existing readers drain records and release naturally.

The order closes admission before resources disappear. `synchronize_irq()` is
called from sleepable removal context and while holding no lock the handler may
need. Calling it under the reader-list lock would create a deadlock if an IRQ
thread were waiting for the same lock.

<figure class="evidence">
  <a href="../../assets/visuals/linux-kernel-labs-context-nesting.png">
    <img src="../../assets/visuals/linux-kernel-labs-context-nesting.png" alt="Linux Kernel Labs execution-context diagram showing syscalls, exceptions, IRQ entries, and returns between user mode and kernel mode.">
  </a>
  <figcaption>Upstream Linux Kernel Labs execution-context evidence. Open/read/remove work begins in process context, while IRQ delivery enters asynchronously; synchronization must cover both. Select to enlarge.</figcaption>
</figure>

## Observability is designed, not added afterward

Use each surface for the question it answers:

- probe/remove `dev_info` records effective configuration and lifecycle;
- `/proc/interrupts` proves the registered IRQ is firing;
- the info ioctl proves ABI negotiation and active policy;
- the stats ioctl distinguishes produced, debounced, delivered, and dropped;
- dynamic debug adds temporary detail without permanent log noise;
- ftrace shows timing and call order when a counter is insufficient;
- `strace` shows blocking, `poll`, ioctl, and errno behavior from userspace.

Avoid logging every interrupt in normal operation. A burst can flood the kernel
log, perturb timing, and hide the failure being investigated.

## Diagnostics make concurrency claims falsifiable

- **Sparse** catches address-space mistakes such as missing `__user` reasoning.
- **lockdep** checks observed lock ordering and sleeping-lock misuse.
- **KASAN** detects out-of-bounds and use-after-free accesses.
- **UBSAN** detects selected undefined operations.
- **fault injection** exercises allocation failure and rollback.

Passing these tools is not a proof of all schedules. It is stronger evidence
than a happy-path run, especially when the stress test repeatedly opens, reads,
unbinds, and reloads across two virtual CPUs.

## Failure classes and recovery

| Failure | Expected behavior | Recovery evidence |
| --- | --- | --- |
| GPIO mapping missing | probe fails with specific errno; no node | fix fixture, reprobe |
| IRQ request fails | no node and all earlier resources unwind | inject failure, inspect sysfs/log |
| userspace pointer invalid | `EFAULT`, staged record retained | retry with valid buffer |
| wait interrupted | syscall reports interruption; queue unchanged | retry or close |
| reader overflows | oldest record lost only for that reader | flag and dropped count |
| device removed | HUP/ERR, queue drains, then EOF | close fd; module frees on last release |

Continue to the [Module 5 recap](../recaps/05-lifecycle-quality.md), then perform
[Lab 5](../labs/05-hardening.md).

