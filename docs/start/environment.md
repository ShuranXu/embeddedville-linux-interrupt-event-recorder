# Environment and workflow

## Reproducible target

The course locks the material environment in `release-lock.json`:

- Linux `6.18.57` with modules, gpio-sim, configfs, tracing, lockdep, KASAN,
  UBSAN, and fault injection;
- QEMU `11.1.2` using software TCG, so nested virtualization is unnecessary;
- BusyBox `1.37.0` and Dropbear `2026.94` in a small initramfs;
- a read-only toolchain image selected by content digest.

The QEMU CPU contract is `max,la57=off`. Five-level paging is disabled because
the pinned x86 KASAN combination otherwise has an upstream-reported early-boot
false positive before any course module loads. The suite does not filter that
diagnostic; the target is configured so a later KASAN line remains a real
failure.

QEMU direct kernel boot uses the upstream `-kernel`, `-initrd`, `-append`, and
serial-console interfaces. The guest mounts this workspace through private 9p
sharing and accepts SSH only through `127.0.0.1:2222`.

!!! note "What the pin guarantees"
    The version lock makes two learners use the same headers and virtual target.
    It does not make scheduling deterministic. Tests assert ordering and
    invariants, not exact nanosecond intervals.

## Commands

```console
./lab doctor   # confirm exact tools and guest artifacts
./lab build    # W=1 module build, Sparse, userspace tools, static checks
./lab boot     # start or reuse this workspace's QEMU guest
./lab run <lab> # build, boot, and execute one guided lab (lab1 through lab5)
./lab test     # run the public acceptance suite
./lab reset    # stop only this guest and remove generated build state
```

`reset` preserves source and captured evidence. The helper verifies the PID and
command line before stopping a process, so a stale PID file cannot terminate an
unrelated QEMU instance.

## Launch a guided lab

The **Run in QEMU** button on each lab returns here because QEMU is a process,
not a browser page. Run the matching command from the repository root; it
builds or reuses the pinned guest and executes that stage's
public scenario.

| Guided lab | Command | Main observation |
| --- | --- | --- |
| [Lab 1 - Inspect the target](../labs/01-target.md) | `./lab run lab1` | Exact tool and guest identities |
| [Lab 2 - Bind a platform device](../labs/02-platform.md) | `./lab run lab2` | Platform binding, GPIO mapping, and IRQ registration |
| [Lab 3 - Capture GPIO interrupts](../labs/03-irq.md) | `./lab run lab3` | Rising/falling records and debounce behavior |
| [Lab 4 - Verify the userspace ABI](../labs/04-abi.md) | `./lab run lab4` | Per-open queues, reads, poll, ioctls, and overflow |
| [Lab 5 - Harden the lifecycle](../labs/05-hardening.md) | `./lab run lab5` | Removal behavior and clean lifecycle stress |

!!! info "Open the hosted workspace"
    [Open or resume the pinned Codespace](https://codespaces.new/ShuranXu/embeddedville-linux-interrupt-event-recorder/tree/interrupt-event-recorder-v1.0.2?quickstart=1){ .md-button .md-button--primary }

    The link selects the immutable course tag and uses the repository's pinned
    dev-container image. Save work under `/workspaces`, commit or export work
    you need to keep, and stop the Codespace when you finish a session.

## First readiness check

```console
$ ./lab doctor
doctor: Linux 6.18.57, QEMU 11.1.2 and TCG lab dependencies are ready
```

That output is an example of the expected result. Your own terminal is the evidence.
If it differs, stop and use the troubleshooting page rather than continuing
with an unknown kernel.

## Captured release evidence

The validated release run produced this selectable evidence on the
pinned target; learners must still produce their own run:

```text
ABI=1 event_size=32 depth=64 debounce_us=50000
multi_reader_sequence=1 produced=76 delivered=3 dropped=9 debounced=1 batch_bytes=128
30:         75          2  irq_sim   0  irqrec
device removed
one hundred bind/unbind and load/unload cycles passed
```

The first two lines are the public ABI/behavior tool, the third is the bound
device's genuine `/proc/interrupts` row, and the last two are teardown evidence.
Exact counts can differ with scheduling; the relationships and invariants may
not.

## Workspace boundaries

This learner repository contains the `starter/` scaffold and public tooling.
The complete reference implementation, protected tests, source mappings, and
release QA records remain outside the public repository.

## Hosted-session expectations

Codespaces opens an immutable course ref in a private forwarded-port
environment. Closing a browser tab does not necessarily stop compute. Stopping
normally preserves saved workspace files; rebuilding may remove state outside
`/workspaces`; deleting or retention expiry can remove unpushed work. Personal
accounts include monthly compute and storage quotas; active compute and stored
Codespaces consume those quotas, and continued use can require a payment method
and spending limit. Review [GitHub's current Codespaces billing documentation](https://docs.github.com/en/billing/concepts/product-billing/github-codespaces)
because provider policy can change independently of this course.

For a local documentation preview, run `preview.ps1` or `./preview.sh` from the
repository root and open `http://127.0.0.1:8000/`.

