# Optional Raspberry Pi 4 exercise

This is a detailed hardware transfer exercise, not a course requirement. Complete
and preserve the QEMU evidence first. The target below is deliberately narrow so
the commands are repeatable:

| Item | Supported choice |
|---|---|
| Board | Raspberry Pi 4 Model B |
| OS | Raspberry Pi OS 64-bit |
| Kernel | Running distribution kernel with matching `linux-headers-rpi-v8` |
| Input | BCM GPIO17, physical header pin 11 |
| Ground | Physical header pin 6 |
| Bias | External 10 kΩ pull-up to 3.3 V, physical pin 1 |
| Stimulus | Normally-open push button from GPIO17 to ground |

[Raspberry Pi's official Pi 4 data sheet](https://pip-assets.raspberrypi.com/categories/545-raspberry-pi-4-model-b/documents/RP-008341-DS-1-raspberry-pi-4-datasheet.pdf?disposition=inline)
is the authority for the header layout; confirm pins 1, 6, and 11 there before
wiring. The [official kernel guide](https://www.raspberrypi.com/documentation/computers/linux_kernel.html)
notes that Linux 6.18 no longer supports a 32-bit BCM2711 kernel, so this
exercise specifies a 64-bit OS.

!!! danger "Electrical boundary"
    Shut the Pi down and remove power before wiring. Raspberry Pi GPIO is 3.3 V
    logic and is not 5 V tolerant. Never connect pin 2 or pin 4 (5 V) to GPIO17.
    If your board, HAT, or signal source differs from the table, stop and consult
    its electrical documentation rather than adapting this recipe by guesswork.

## 1. Finish the QEMU path

On the development host, retain a known-good software baseline:

```console
./lab test
```

This proves the ABI and lifecycle independently of wiring. Hardware results add
electrical and board-specific evidence; they do not replace the emulator tests.

## 2. Prepare the Pi

Boot the Pi, open a terminal, and record the exact environment:

```console
uname -a
getconf LONG_BIT
cat /etc/os-release
```

`getconf LONG_BIT` must print `64`. Install the matching kernel headers, compiler,
and Device Tree compiler:

```console
sudo apt update
sudo apt install --yes build-essential linux-headers-rpi-v8 device-tree-compiler git
test -e "/lib/modules/$(uname -r)/build/Makefile"
```

If the final command fails, reboot after a full system update and install the
headers again. Do not build against headers for a different `uname -r`.

Clone the tagged public course on the Pi. The remaining commands assume:

```console
git clone --branch interrupt-event-recorder-v1.0.2 --depth 1 \
  https://github.com/ShuranXu/embeddedville-linux-interrupt-event-recorder.git
cd embeddedville-linux-interrupt-event-recorder
```

## 3. Wire the button while powered off

Run `sudo poweroff`, wait for activity LEDs to stop, unplug power, and make these
connections:

| From | Through | To | Purpose |
|---|---|---|---|
| Physical pin 1, 3.3 V | 10 kΩ resistor | Physical pin 11, GPIO17 | Stable released/high state |
| Physical pin 11, GPIO17 | Normally-open button | Physical pin 6, GND | Pressed/low stimulus |

With this circuit, button press is a falling edge and release is a rising edge.
Inspect the wiring against the official data sheet, then restore power.

## 4. Build only the hardware driver and tools

The QEMU-only fixture must not be loaded on the Pi. `BUILD_FIXTURE=0` excludes it:

```console
make clean KDIR="/lib/modules/$(uname -r)/build"
make KDIR="/lib/modules/$(uname -r)/build" BUILD_FIXTURE=0 modules userspace
file irqrec.ko build/irqrec-read build/irqrec-test
modinfo ./irqrec.ko | sed -n '1,12p'
```

Compile the supplied overlay:

```console
dtc -@ -I dts -O dtb \
  -o hardware/rpi/irqrec.dtbo \
  hardware/rpi/irqrec-overlay.dts
dtc -I dtb -O dts -W no-gpios_property \
  hardware/rpi/irqrec.dtbo | sed -n '1,80p'
```

The decompiled check must show `compatible = "embeddedville,irqrec-v1"`,
GPIO number 17 (shown as `0x11`), `debounce-us = <5000>` (shown as `0x1388`),
and `queue-depth = <64>` (shown as `0x40`). The GPIO-controller phandle remains
an external fixup in a standalone overlay and is resolved by Pi firmware.

## 5. Install and activate the Device Tree overlay

Raspberry Pi OS reads `/boot/firmware/config.txt`; the
[official configuration guide](https://www.raspberrypi.com/documentation/computers/config_txt.html)
defines `dtoverlay=` and the reboot requirement. Back up the file, install the
compiled overlay, add one activation line, and reboot:

```console
sudo cp -a /boot/firmware/config.txt \
  "/boot/firmware/config.txt.irqrec-backup-$(date +%Y%m%d-%H%M%S)"
sudo install -m 0644 hardware/rpi/irqrec.dtbo \
  /boot/firmware/overlays/irqrec.dtbo
grep -qxF 'dtoverlay=irqrec' /boot/firmware/config.txt || \
  printf '%s\n' 'dtoverlay=irqrec' | sudo tee -a /boot/firmware/config.txt
sudo reboot
```

After reconnecting, verify that firmware instantiated the platform device before
loading the module:

```console
find /proc/device-tree -name compatible -exec sh -c \
  'grep -alq "embeddedville,irqrec-v1" "$1" && echo "${1%/compatible}"' sh {} \;
```

The command should print a path ending in `/irqrec`.

## 6. Load, bind, and observe real interrupts

From the project directory:

```console
sudo insmod ./irqrec.ko
ls -l /dev/irqrec0
readlink /sys/bus/platform/devices/irqrec/driver
grep -E 'irqrec|GPIO' /proc/interrupts
sudo dmesg | tail -n 30
```

The driver link should end in `/bus/platform/drivers/irqrec`, and the kernel log
should include `irqrec0 ready`. Start the reader:

```console
sudo ./build/irqrec-read /dev/irqrec0
```

Press and release the button at least five times. Confirm that sequence numbers
increase, timestamps are monotonic, and falling/rising edges alternate. In a
second terminal, compare the IRQ counter before and after more presses:

```console
grep -E 'irqrec|GPIO' /proc/interrupts
sudo ./build/irqrec-test /dev/irqrec0
```

Keep the terminal transcript as hardware evidence. Switch bounce is physical,
so also note whether rapid contacts increment the driver's `debounced` statistic.

## 7. Exercise removal safely

Start a blocking reader in one terminal, then remove the module from another:

```console
sudo rmmod irqrec
```

The blocked process must wake, `/dev/irqrec0` must disappear, and `dmesg` must
contain no warning or oops. Reload for another run with `sudo insmod ./irqrec.ko`.

## 8. Optional persistent installation

Only after the manual run succeeds:

```console
sudo install -D -m 0644 irqrec.ko \
  "/lib/modules/$(uname -r)/extra/irqrec.ko"
sudo depmod -a
printf '%s\n' irqrec | sudo tee /etc/modules-load.d/irqrec.conf
sudo reboot
```

After reboot, repeat the binding, device-node, interrupt-counter, and reader
checks from step 6. A future kernel update requires rebuilding the module against
the new running kernel headers.

## 9. Full rollback

Remove the automatic module setting and overlay line, then delete only the files
installed by this exercise:

```console
sudo rm -f /etc/modules-load.d/irqrec.conf
sudo rm -f "/lib/modules/$(uname -r)/extra/irqrec.ko"
sudo depmod -a
sudo sed -i '/^[[:space:]]*dtoverlay=irqrec[[:space:]]*$/d' \
  /boot/firmware/config.txt
sudo rm -f /boot/firmware/overlays/irqrec.dtbo
sudo reboot
```

After reboot, the Device Tree search in step 5 should print nothing and
`/dev/irqrec0` should not exist.

## Transfer questions

1. Which parts of the QEMU evidence remain valid on the Pi, and which claims now
   require physical measurement?
2. Why is the BCM GPIO number stored in Device Tree rather than in `irqrec.c`?
3. What failure do matching kernel headers prevent?

The answers should distinguish reusable driver behavior from board description,
electrical safety, bounce, and latency.
