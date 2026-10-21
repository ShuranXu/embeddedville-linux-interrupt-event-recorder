# Platform binding and GPIO descriptors

## The problem: drivers should not guess board wiring

An integer module parameter such as `gpio=17` mixes two responsibilities. The
board integrator knows which signal is connected. The device driver knows what
that signal means. Linux separates them so one driver can run on many boards.

The consumer asks for a function, not a global number:

```c
event = devm_gpiod_get(dev, "event", GPIOD_IN);
irq = gpiod_to_irq(event);
```

The string `event` is part of the binding contract. Device Tree represents it as
`event-gpios`; a lookup table represents it with `con_id = "event"`; the driver
uses the same code either way.

## Descriptor semantics

A `struct gpio_desc *` is opaque. It carries controller association, line
offset, active-low interpretation, ownership, and capability information. Code
using `gpio_get_value(17)` discards that context and depends on a legacy global
namespace. Linux 6.18 explicitly recommends the `gpiod_*` interface for new
drivers.

Request `GPIOD_IN` so direction is established at acquisition. Read through
`gpiod_get_value_cansleep()` in the threaded handler. That choice works for both
memory-mapped controllers and controllers whose access path may sleep. The
threaded context is what makes the general call legal.

## Platform matching

The driver registers `struct platform_driver` with:

- a driver name for platform-ID matching in the QEMU fixture;
- an `of_match_table` containing `embeddedville,irqrec-v1` for the optional
  Device Tree path;
- `probe()` to acquire all resources and publish the misc device;
- `remove()` to close admission and quiesce runtime activity.

The QEMU fixture is deliberately a separate module. It installs a GPIO lookup
for the line named `irqrec-event`, then registers an `irqrec` platform device.
This models board setup while keeping the actual driver free of simulation code.

## gpio-sim is simulated hardware, not a fake read path

Linux gpio-sim creates a normal gpiolib controller from configfs. The lab makes
one bank with one line named `irqrec-event`, then changes its simulated pull
between `pull-down` and `pull-up`. Those changes alter the line value and reach
the driver through the GPIO IRQ mapping.

The relevant configfs hierarchy from Linux 6.18 is:

```text
/sys/kernel/config/gpio-sim/irqrec/
├── live
└── bank0/
    ├── num_lines
    └── line0/
        └── name
```

This representation is derived directly from the upstream gpio-sim interface;
the actual names under your guest are inspected, not assumed. After setting
`live=1`, the simulator exposes a `sim_gpio0/pull` control beneath its platform
device. The lab discovers that path and records it in `/run/irqrec-sim-value`.

## Probe ordering and rollback

Publish `/dev/irqrec0` only after the GPIO and IRQ exist. Otherwise a process
can open a node whose producer is not ready. A robust probe sequence is:

1. allocate and initialize device state;
2. read optional properties with bounded defaults;
3. acquire the GPIO descriptor;
4. configure debounce or select the software fallback;
5. map and request the threaded IRQ;
6. register the misc device;
7. store driver data and report the effective configuration.

On failure, managed GPIO/IRQ resources unwind with the device. Explicitly owned
state still drops its reference. `dev_err_probe()` keeps deferred-probe and
ordinary error reporting consistent.

Continue to the [Module 2 recap](../recaps/02-platform-gpio.md), then perform
[Lab 2](../labs/02-platform.md).

