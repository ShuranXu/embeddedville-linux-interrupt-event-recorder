# Knowledge recap: binding and GPIO

## Retain these ideas

- The platform device describes that hardware exists; the platform driver
  implements its behavior.
- The driver requests a function named `event`, not a board-specific integer.
- `devm_gpiod_get(..., GPIOD_IN)` establishes ownership and direction.
- `gpiod_to_irq()` maps the descriptor through the GPIO controller's IRQ domain.
- gpio-sim supplies a real gpiolib/IRQ path; the separate fixture supplies the
  QEMU board description.

## Applied checks

**Why is registering a platform device inside `irqrec.c` a design failure?**

It makes the driver claim that its hardware exists. Real enumeration belongs to
firmware, a bus, or board setup. Keeping the fixture separate lets the same
driver bind through Device Tree later.

**The lookup table names `irqrec-event`, while the driver asks for `event`. Is
that inconsistent?**

No. `irqrec-event` identifies the simulated line; `event` is the consumer
function (`con_id`). The lookup table maps one to the other.

**What does a successful module load fail to prove?**

It does not prove that the driver bound, acquired a descriptor, mapped an IRQ,
or published its node. Inspect the platform binding, probe log, device node,
and `/proc/interrupts` separately.

## Handoff

Lab 2 creates the simulated controller first, registers the fixture second, and
loads the driver last so each boundary is observable.

