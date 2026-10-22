#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
activity="${1:-all}"
driver="$root/starter/src/irqrec.c"

require() { grep -q "$1" "$driver" || { echo "missing ${2:-$1} for $activity" >&2; exit 1; }; }

require 'module_platform_driver' 'platform-driver registration'
case "$activity" in
  lab1) ;;
  lab2) require 'devm_gpiod_get' 'descriptor GPIO acquisition'; require 'devm_request_threaded_irq' 'threaded IRQ request'; require 'misc_register' 'device-node registration' ;;
  lab3) require 'devm_gpiod_get' 'descriptor GPIO acquisition'; require 'devm_request_threaded_irq' 'threaded IRQ request'; require 'ktime_get_ns' 'monotonic timestamping'; require 'gpiod_set_debounce' 'hardware debounce attempt'; require 'IRQREC_FEATURE_SW_DEBOUNCE' 'software debounce fallback reporting'; require 'wake_up_interruptible' 'event notification' ;;
  lab4) require 'struct irqrec_reader' 'per-open reader state'; require 'wait_event_interruptible' 'blocking read'; require 'poll_wait' 'poll support'; require 'copy_to_user' 'bounded userspace copy'; require 'IRQREC_IOC_GET_INFO' 'versioned ioctl handling'; require 'IRQREC_EVENT_F_OVERFLOW' 'overflow contract' ;;
  lab5|capstone|all) require 'kref_get_unless_zero' 'referenced lifetime'; require 'synchronize_irq' 'IRQ teardown synchronization'; require 'misc_deregister' 'device-node removal'; require 'EPOLLHUP' 'terminal poll state'; require 'EPOLLERR' 'terminal poll state' ;;
  *) echo "unknown activity: $activity" >&2; exit 2 ;;
esac

python3 "$root/tools/check_uapi.py"
python3 "$root/tools/check_visual_manifest.py"
python3 "$root/tools/check_site.py"
echo "host structural, UAPI, and visual provenance checks passed for $activity"

