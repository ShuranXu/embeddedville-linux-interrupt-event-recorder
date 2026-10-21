# Linux Interrupt Driver Project

Build a GPIO interrupt event recorder that behaves like a small production
driver rather than a one-file demonstration. The finished system binds through
the Linux platform bus, consumes a descriptor-based GPIO, handles both edges in
a threaded IRQ, and exposes a versioned event stream through `/dev/irqrec0`.

<div class="course-meta">
  <div><strong>Level</strong>Applied bridge course</div>
  <div><strong>Time</strong>10-12 focused hours</div>
  <div><strong>Required hardware</strong>None</div>
  <div><strong>Execution target</strong>Linux 6.18.57 on QEMU 11.1.2 TCG</div>
</div>

## The engineering problem

The source guide proves two important ideas: a timer can demonstrate an event
path, and a GPIO can generate a real interrupt. A useful driver must answer a
larger set of questions. How is the device discovered? Which component owns the
GPIO mapping? What happens if two processes read at different speeds? What does
an old program do after the event structure changes? Can a blocked reader exit
cleanly while the device is removed?

This course turns those questions into one coherent project. You will build and
verify the mechanisms instead of memorizing API names.

## What you will be able to demonstrate

By the end, you can:

- explain the ownership boundary between platform description and driver code;
- acquire a GPIO through the descriptor interface and map it to an IRQ;
- choose a threaded interrupt path and defend its execution-context rules;
- design fixed-width, versioned records and ioctls across a kernel/userspace ABI;
- implement blocking reads, nonblocking reads, batching, `poll`, and independent
  per-open queues;
- define and observe overflow, debounce, signal, and removal behavior;
- use gpio-sim, `/proc/interrupts`, dynamic debug, tracing, Sparse, lockdep,
  KASAN, and UBSAN as evidence rather than decoration.

## The course journey

| Stage | Question answered | Increment added to the final product |
| --- | --- | --- |
| 1. Driver boundaries | What turns a demo into a maintainable kernel interface? | Explicit lifecycle and ownership model |
| 2. Devices and GPIO | How does a driver receive hardware description without hard-coding a pin? | Bound platform device and descriptor |
| 3. Interrupts and time | Where may the handler sleep, and how is noise filtered? | Timestamped, debounced edge production |
| 4. Event ABI | How do several applications consume events without stealing from one another? | Versioned UAPI and per-open FIFO |
| 5. Lifecycle quality | How does the driver fail, unload, and explain itself safely? | Observable removal and stress evidence |

Each module ends in a detailed recap with applied reasoning. There are no
module quizzes. The five labs build toward one evaluated capstone.

!!! info "Lab environment"
    QEMU is the authoritative target. gpio-sim supplies a real kernel GPIO chip
    and IRQ mapping, so the driver follows the same subsystem APIs it would use
    on a board. This proves software behavior, concurrency, ABI, and teardown;
    it does **not** prove Raspberry Pi voltage, wiring, electrical noise, or
    physical timing. The optional board appendix is not part of completion.

## Start

Open the [project roadmap](start/roadmap.md) to see how each lab advances the
same driver, then follow the [environment workflow](start/environment.md).
When the local toolchain is available, your first command is:

```console
./lab doctor
```

