# Sources and attribution

The learner-facing course uses only the technical sources below. Detailed
hash, crop, attribution, and semantic-fit records are published with the course
in the provenance manifest.

## Versioned primary sources

- [Linux 6.18 platform devices and drivers](https://docs.kernel.org/6.18/driver-api/driver-model/platform.html)
- [Linux 6.18 GPIO descriptor consumer interface](https://docs.kernel.org/6.18/driver-api/gpio/consumer.html)
- [Linux 6.18 GPIO mappings](https://docs.kernel.org/6.18/driver-api/gpio/board.html)
- [Linux 6.18 gpio-sim source document](https://raw.githubusercontent.com/torvalds/linux/v6.18/Documentation/admin-guide/gpio/gpio-sim.rst)
- [Linux 6.18 generic IRQ handling](https://docs.kernel.org/6.18/core-api/genericirq.html)
- [QEMU 11.1.2 direct Linux boot](https://qemu.readthedocs.io/en/v11.1.2/system/linuxboot.html)
- [Linux Kernel Labs interrupt lecture source](https://github.com/linux-kernel-labs/linux/blob/a251a609fa4a8efb4fc41f9315d18c70a333aa1e/Documentation/teaching/lectures/interrupts.rst)
- [Upstream KASAN report for the x86 five-level-paging early-boot false positive](https://lists.openwall.net/linux-kernel/2026/10/12/2358)
- [Raspberry Pi kernel and matching-header build documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html)
- [Raspberry Pi `config.txt` and `dtoverlay` documentation](https://www.raspberrypi.com/documentation/computers/config_txt.html)
- [Raspberry Pi 4 Model B data sheet and GPIO header](https://pip-assets.raspberrypi.com/categories/545-raspberry-pi-4-model-b/documents/RP-008341-DS-1-raspberry-pi-4-datasheet.pdf?disposition=inline)

## Visual license

The three interrupt diagrams are unmodified generated artifacts from the Linux
Kernel Labs teaching source. Their provenance is pinned in
`provenance/visual-sources.json`; the teaching source is distributed from the
Linux source repository under GPL-2.0. No course-authored, Mermaid, or
AI-generated technical diagram is used.

## Original guide

`02_Linux_Interrupt_Driver_Lab.pdf` supplied the timer demonstration, real GPIO
IRQ example, wait/read/poll emphasis, build flow, and interview prompts. Course
language is newly written; the original file is not embedded or republished.

