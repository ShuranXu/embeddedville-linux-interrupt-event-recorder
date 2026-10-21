# Linux Interrupt Event Recorder

Build a production-style Linux GPIO interrupt event recorder against a pinned
Linux 6.18.57/QEMU 11.1.2 environment. Five guided labs extend one driver from
platform binding through IRQ handling, a versioned userspace ABI, independent
per-open queues, and removal-safe teardown.

[Open or resume the course workspace in GitHub Codespaces](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.1?quickstart=1)

## Start the project

In Codespaces, the toolchain is already installed. Run:

```console
./lab doctor
./preview.sh
```

Open the forwarded private port `8000`, follow the project roadmap, and use
the command shown in each lab. The complete public workflow is:

```console
./lab build
./lab boot
./lab run lab1
./lab run lab2
./lab run lab3
./lab run lab4
./lab run lab5
./lab test capstone
./lab submit --activity irqrec-capstone
./lab reset
```

The starter intentionally does not contain the completed driver. Public tests
describe observable behavior; protected completion checks are run by the
EmbeddedVille learner platform.

## Local documentation preview

Windows PowerShell:

```powershell
.\preview.ps1
```

Linux/macOS:

```console
./preview.sh
```

Browse `http://127.0.0.1:8000/`.

## Repository boundaries

- `docs/` contains the course and sourced technical visuals.
- `starter/` is the learner-owned driver and UAPI scaffold.
- `fixture/`, `userspace/`, `runtime/`, and `scripts/` provide the QEMU lab.
- `tests/public/` contains learner-visible checks only.
- `provenance/visual-sources.json` records every technical visual's exact
  upstream source, version, license, hash, caption, and learning purpose.

The reference implementation and protected evaluator are maintained outside
this public repository.

