# Knowledge recap: removal and proof

## Retain these ideas

- Close admission, mark terminal state, quiesce the producer, wake consumers,
  and only then release the device's owning reference.
- `disable_irq()` prevents new delivery; `synchronize_irq()` waits for in-flight
  hard and threaded handlers.
- Never wait for an IRQ while holding a lock that handler may require.
- A passing test is evidence for the scenario it ran, not every schedule.
- Diagnostics must remain clean while the lifecycle is stressed, not only on a
  single insertion.

## Applied checks

**An open descriptor has queued records when the fixture is removed. What should
the next two phases look like?**

Poll reports readable and terminal state. Reads drain complete records. Once
the queue and staging slot are empty, read returns EOF and poll remains terminal.

**Why inspect both userspace results and the kernel log?**

Correct output can coexist with a lockdep warning or use-after-free that has not
yet crashed. The interface result and kernel diagnostic health are separate
acceptance conditions.

**What is the minimum credible stress loop?**

Repeatedly create readers, trigger both edges, interrupt waits, unregister the
fixture with open files, close them, reload, and scan for warnings under two
vCPUs. Vary timing instead of relying on fixed sleeps alone.

## Handoff

Lab 5 deliberately exercises failure and removal before the capstone rubric is
applied.

