# Project roadmap

The five labs are successive, demonstrable increments of one product: a Linux
GPIO interrupt event recorder exposed as `/dev/irqrec0`. You do not build five
unrelated examples. Keep the working result from each lab and extend it in the
next one.

## From first boot to usable driver

<div class="roadmap-grid" markdown>

<article class="roadmap-step" markdown>
<span class="roadmap-number">Lab 1</span>

### [Inspect the target](../labs/01-target.md)

**Demo:** The compiler, Linux headers, QEMU binary, and guest match the pinned
release; compiler and Sparse checks are clean.

**Builds:** A trustworthy baseline and an ownership plan for resources the
driver will acquire.
</article>

<article class="roadmap-step" markdown>
<span class="roadmap-number">Lab 2</span>

### [Bind a platform device](../labs/02-platform.md)

**Demo:** The device binds, the driver receives the named `event` GPIO,
`/dev/irqrec0` appears, and an IRQ is visible in `/proc/interrupts`.

**Builds:** A hardware-independent GPIO-to-IRQ input boundary with hardware
description outside the driver.
</article>

<article class="roadmap-step" markdown>
<span class="roadmap-number">Lab 3</span>

### [Capture GPIO interrupts](../labs/03-irq.md)

**Demo:** gpio-sim transitions produce ordered rising and falling records with
monotonic timestamps; debounce decisions are observable.

**Builds:** The threaded IRQ event producer used by the finished recorder.
</article>

<article class="roadmap-step" markdown>
<span class="roadmap-number">Lab 4</span>

### [Verify the userspace ABI](../labs/04-abi.md)

**Demo:** Reads, `poll`, ioctls, batching, overflow, and two independent
readers obey the published ABI.

**Builds:** A stable `/dev/irqrec0` interface usable by event-loop applications.
</article>

<article class="roadmap-step" markdown>
<span class="roadmap-number">Lab 5</span>

### [Harden the lifecycle](../labs/05-hardening.md)

**Demo:** Readers wake during removal, events drain, terminal poll state is
reported, and 100 lifecycle cycles leave no kernel diagnostic.

**Builds:** Predictable behavior while userspace and device lifetimes overlap.
</article>

<article class="roadmap-step roadmap-capstone" markdown>
<span class="roadmap-number">Final product</span>

### [Capstone demonstration](../capstone/project.md)

**Demo:** One evidence run joins binding, edge capture, ABI behavior, overflow,
two readers, removal, and diagnostic checks.

**Builds:** A complete interrupt event recorder ready for evaluation and
optional transfer to Raspberry Pi.
</article>

</div>

## How to move through the project

For each stage:

1. read only the linked project briefing and recap needed by that lab;
2. implement the increment in the learner scaffold;
3. use the lab's **Run in QEMU** button and capture the stated observation;
4. fix the failure experiment before continuing; and
5. carry the same source forward to the next lab.

The QEMU result is the required path. The optional
[Raspberry Pi exercise](../support/raspberry-pi.md) transfers the completed
driver to physical hardware only after the emulator evidence passes.

## Execution choices

[Set up and run QEMU](environment.md#launch-a-guided-lab){ .md-button .md-button--primary }
[Open the pinned Codespace](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.2?quickstart=1){ .md-button }
[Open Lab 1](../labs/01-target.md){ .md-button }

The Codespaces link opens or resumes the immutable `interrupt-event-recorder-v1.0.2`
course workspace. QEMU runs with software TCG, so no physical board or nested
virtualization is required.

## Final demonstration

At the end of the roadmap, the driver must bind without a hard-coded GPIO,
report rising and falling records, support independent readers and overflow
reporting, and shut down safely during removal. The
[acceptance rubric](../capstone/rubric.md) lists the evidence used to evaluate
that integrated demonstration; the optional board exercise does not replace
the QEMU result.
