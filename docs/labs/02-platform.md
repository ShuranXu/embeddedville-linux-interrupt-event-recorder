# Lab 2 - Bind a platform device

<div class="lab-launch" markdown>
[Run Lab 2 in QEMU](../start/environment.md#launch-a-guided-lab){ .md-button .md-button--primary }
[Open Codespaces](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.1?quickstart=1){ .md-button }
</div>

<p class="lab-launch-note"><strong>Workspace command:</strong> <code>./lab run lab2</code>. The Codespaces button opens or resumes the pinned course workspace.</p>

## Purpose

Prove that the driver binds through the platform core and receives a GPIO
mapping from outside its implementation.

## Task

Start from the learner scaffold. Add the platform ID and optional Device Tree
match, then acquire the `event` descriptor in probe. Keep the QEMU fixture in a
separate translation unit.

In the guest:

```console
cd /workspace
./scripts/gpio-sim-setup.sh
insmod irqrec_fixture.ko
insmod irqrec.ko
readlink /sys/bus/platform/devices/irqrec/driver
cat /run/irqrec-sim-value
ls -l /dev/irqrec0
grep irqrec /proc/interrupts
```

## Expected observation

The platform device links to the `irqrec` driver, `/dev/irqrec0` exists with
mode `0660`, and `/proc/interrupts` contains the requested IRQ. The pull-control
path belongs to a gpio-sim platform device.

## Failure experiment

Unload everything, remove the simulated chip, then load the fixture and driver
without the named line. Probe must fail with a GPIO mapping error and must not
leave `/dev/irqrec0` behind. Restore the simulator with the setup script.

## Engineering checkpoint

Explain why the following three names differ:

- platform device/driver match name: `irqrec`;
- GPIO consumer function: `event`;
- simulated GPIO line name: `irqrec-event`.

If the explanation does not assign each name to an owner, revisit the
[binding recap](../recaps/02-platform-gpio.md).

## Completion evidence

- sysfs driver link;
- successful probe log with effective IRQ and debounce mode;
- absence of a node in the missing-mapping experiment;
- restored clean binding.

