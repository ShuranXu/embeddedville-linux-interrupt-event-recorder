# Lab 1 - Inspect the target

<div class="lab-launch" markdown>
[Run Lab 1 in QEMU](../start/environment.md#launch-a-guided-lab){ .md-button .md-button--primary }
[Open Codespaces](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.2?quickstart=1){ .md-button }
</div>

<p class="lab-launch-note"><strong>Workspace command:</strong> <code>./lab run lab1</code>. The Codespaces button opens or resumes the pinned course workspace.</p>

## Purpose

Establish that the environment and evidence boundaries are correct before
editing driver code. This prevents a host-kernel build or a mismatched QEMU
binary from producing misleading results.

## Task

1. Run the readiness check and inspect the immutable lock.
2. Build the learner scaffold with warnings and Sparse enabled.
3. Boot QEMU and compare host, build-target, and guest kernel identities.
4. Record which claim each observation can support.

```console
./lab doctor
./lab build
./lab boot
ssh -i .lab-state/id_ed25519 -p 2222 \
  -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  root@127.0.0.1 'uname -r; cat /proc/cmdline'
```

## Expected observation

- The prepared headers and guest both report `6.18.57`.
- QEMU reports `11.1.2` and supports TCG.
- Build output has no compiler or Sparse warning.
- The guest command line includes `console=ttyS0`, `oops=panic`, and `nokaslr`.

Do not paste the sample wording as evidence. Save your terminal output under
`evidence/runtime/lab1-environment.txt`.

## Engineering checkpoint

Write a five-row ownership table for device state, reader state, GPIO, IRQ, and
device node. For each row name the acquisition, normal release, and probe-failure
release. Compare it with the [first recap](../recaps/01-driver-boundaries.md).

## Recovery

If `doctor` reports a version mismatch, do not continue with the host kernel.
Use the pinned dev-container image or build the locked runtime. If SSH is not
ready, inspect `.lab-state/serial.log`, preserve it, then run `./lab reset`.

## Completion evidence

- readiness output;
- successful warning/Sparse build;
- guest version and command line;
- completed ownership table.

