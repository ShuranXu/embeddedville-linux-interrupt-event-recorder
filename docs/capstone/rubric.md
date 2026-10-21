# Capstone acceptance rubric

All critical rows must pass. Scoring is provided for review clarity; a critical
failure cannot be offset by extra points elsewhere.

| Area | Evidence | Points | Critical |
| --- | --- | ---: | :---: |
| Environment | Exact Linux/QEMU locks and clean W=1/Sparse build | 10 | Yes |
| Binding | Independent fixture, platform match, descriptor, IRQ mapping | 15 | Yes |
| IRQ path | Both edges, threaded context, monotonic identity, debounce | 15 | Yes |
| UAPI | Fixed 32-byte layouts, version/info/stats, reserved zero space | 15 | Yes |
| Reader semantics | block/nonblock, batching, poll, signals, `EFAULT` retry | 15 | Yes |
| Multi-reader/overflow | independent FIFOs, stated loss policy and counters | 10 | Yes |
| Removal | HUP/ERR, drain then EOF, reference-safe last close | 15 | Yes |
| Diagnostics | stress run and clean lockdep/KASAN/UBSAN/oops scan | 5 | Yes |

## Passing decision

The capstone passes at 80/100 or higher with every critical row satisfied. The
review uses the exact source snapshot that produced the evidence. A result from
a different ref, kernel, or QEMU version must be rerun.

## Non-credit additions

Optional Raspberry Pi execution can demonstrate board integration but earns no
completion points. Polished screenshots, extra frameworks, or a large README do
not compensate for a missing concurrency or teardown test.

