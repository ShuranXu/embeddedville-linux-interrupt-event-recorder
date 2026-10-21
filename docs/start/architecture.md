# Architecture evidence

The project crosses four contracts: hardware description, driver binding,
event production, and userspace consumption. This page uses upstream Linux
teaching material and kernel interfaces as the visual evidence; it does not
introduce a course-authored architecture drawing.

## Interrupt delivery begins below the driver

<figure class="evidence">
  <a href="../../assets/visuals/linux-kernel-labs-pic.png">
    <img src="../../assets/visuals/linux-kernel-labs-pic.png" alt="Linux Kernel Labs diagram showing several device IRQ lines entering an interrupt controller, which signals the CPU through INTR, with NMI shown separately.">
  </a>
  <figcaption>Upstream Linux Kernel Labs, <em>Interrupts</em>: devices signal an interrupt controller; the controller signals the CPU. The driver requests an abstract Linux IRQ and does not program this topology directly. Select the image to enlarge it.</figcaption>
</figure>

gpio-sim replaces only the physical electrical source. It still registers a
GPIO controller, creates an IRQ mapping, and sends the event through Linux IRQ
infrastructure. Consequently, the test exercises `gpiod_to_irq()` and
`request_threaded_irq()` rather than calling the handler directly.

## Binding is a match, not self-discovery

Linux 6.18 defines the platform-driver shape as:

```c
struct platform_driver {
        int (*probe)(struct platform_device *);
        void (*remove)(struct platform_device *);
        void (*shutdown)(struct platform_device *);
        struct device_driver driver;
        const struct platform_device_id *id_table;
        /* ... */
};
```

This is a shortened upstream structure excerpt. It is visual evidence of the
contract: the platform core owns matching and calls `probe()`/`remove()`; the
driver does not create the device it claims to drive. In the lab, a separate
fixture supplies the platform device and GPIO lookup. On a real board, firmware
description such as Device Tree fills that role.

## Data travels through independent ownership domains

Use these upstream interfaces as checkpoints:

| Boundary | Linux interface | Owner in this project |
| --- | --- | --- |
| Hardware description to driver | `devm_gpiod_get(dev, "event", GPIOD_IN)` | QEMU fixture / board firmware |
| GPIO controller to IRQ core | `gpiod_to_irq(desc)` | gpiolib and irqdomain |
| IRQ core to driver | `devm_request_threaded_irq(...)` | irqrec platform driver |
| Driver to open reader | per-open `kfifo` and wait queue | `struct irqrec_reader` |
| Kernel to process | `read`, `poll`, versioned ioctl | public UAPI |

The key design decision is the fourth row: one FIFO per open file. A global FIFO
would turn readers into competitors. Per-open FIFOs turn them into independent
subscribers while keeping event production device-wide.

