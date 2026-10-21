# Lab 3 - Capture GPIO interrupts

<div class="lab-launch" markdown>
[Run Lab 3 in QEMU](../start/environment.md#launch-a-guided-lab){ .md-button .md-button--primary }
[Open Codespaces](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.0?quickstart=1){ .md-button }
</div>

<p class="lab-launch-note"><strong>Workspace command:</strong> <code>./lab run lab3</code>. The Codespaces button opens or resumes the pinned course workspace.</p>

## Purpose

Turn gpio-sim line transitions into stable, timestamped records and distinguish
IRQ delivery from debounce policy.

## Task

Request both edges through a threaded IRQ. In the threaded function, read the
logical value, apply the debounce decision, create one event identity, fan it
out to active readers, then wake them.

With the modules loaded, locate the pull control:

```console
pull="$(cat /run/irqrec-sim-value)"
before="$(grep irqrec /proc/interrupts)"
echo pull-up > "$pull"
sleep 0.02
echo pull-down > "$pull"
after="$(grep irqrec /proc/interrupts)"
printf 'before: %s\nafter:  %s\n' "$before" "$after"
```

In a second terminal, run:

```console
./build/irqrec-read /dev/irqrec0
```

## Expected observation

The IRQ count increases. The collector prints JSON Lines with increasing
sequence and monotonic timestamps. Probe samples the starting value, so pull-up
produces rising and pull-down produces falling from the first delivered edge.

## Debounce experiment

Toggle the pull twice inside the configured interval, then once after it. Read
`IRQREC_IOC_GET_STATS` through the test tool. The fast transition should increase
`debounced`; the later transition should increase `produced`.

Do not assert an exact timestamp gap: scheduling under TCG is not a physical
timing instrument. Assert monotonic ordering and the policy boundary.

## Completion evidence

- before/after `/proc/interrupts` rows;
- at least one rising and falling parsed record;
- monotonic sequence/timestamp check;
- produced/debounced statistics that explain the experiment.

