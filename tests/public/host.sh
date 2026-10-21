#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

grep -q 'module_platform_driver' "$root/starter/src/irqrec.c"
grep -q 'devm_gpiod_get' "$root/starter/src/irqrec.c"
grep -q 'devm_request_threaded_irq' "$root/starter/src/irqrec.c"
grep -q 'wait_event_interruptible' "$root/starter/src/irqrec.c"
grep -q 'poll_wait' "$root/starter/src/irqrec.c"
grep -q 'kref_get_unless_zero' "$root/starter/src/irqrec.c"

python3 "$root/tools/check_uapi.py"
python3 "$root/tools/check_visual_manifest.py"
python3 "$root/tools/check_site.py"
echo 'host structural, UAPI, and visual provenance checks passed'

