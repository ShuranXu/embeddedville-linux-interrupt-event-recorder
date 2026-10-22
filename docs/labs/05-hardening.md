# Lab 5 - Break and harden the lifecycle

<div class="lab-launch" markdown>
[Run Lab 5 in QEMU](../start/environment.md#launch-a-guided-lab){ .md-button .md-button--primary }
[Open Codespaces](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.2?quickstart=1){ .md-button }
</div>

<p class="lab-launch-note"><strong>Workspace command:</strong> <code>./lab run lab5</code>. The Codespaces button opens or resumes the pinned course workspace.</p>

## Purpose

Verify behavior while the device is removed, waits are interrupted, and
lifetimes overlap. This lab supplies the evidence needed for the capstone.

## Task

Run the full public scenario:

```console
./lab test
```

Then perform the removal case manually:

1. open `/dev/irqrec0` from a small poll/read program;
2. queue at least one record;
3. unload `irqrec_fixture` while the file remains open;
4. observe readable and terminal poll bits;
5. drain queued records and confirm EOF;
6. close the descriptor and unload `irqrec`.

## Diagnostic gate

Preserve and inspect:

```console
dmesg | grep -E 'irqrec|BUG:|WARNING:|KASAN:|UBSAN:|locking dependency|blocked'
cat /proc/interrupts | grep irqrec
```

The acceptance condition is both correct userspace behavior and no kernel
diagnostic finding. A warning is a failure even when the program printed the
expected record.

## Stress extension required by the capstone

Repeat load, two-reader edge delivery, fixture removal, drain, close, and unload
at least 100 times. Vary short delays and occasionally deliver a signal to a
blocked reader. Abort on the first unexpected errno, sequence regression,
timeout, warning, or leak symptom and preserve that iteration's log.

## Recovery

If the guest panics because `oops=panic`, save `.lab-state/serial.log` before
resetting. A panic is useful evidence: identify whether it occurred before IRQ
quiescence, during reader wakeup, or after the last reference was dropped.

## Completion evidence

- terminal poll plus drain/EOF transcript;
- 100-iteration stress summary;
- clean lockdep/KASAN/UBSAN/kernel-warning scan;
- preserved failure evidence for any corrected defect.

