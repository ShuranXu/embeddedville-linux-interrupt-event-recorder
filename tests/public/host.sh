#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
activity="${1:-all}"
driver="$root/starter/src/irqrec.c"

require() { grep -q "$1" "$driver" || { echo "missing ${2:-$1} for $activity" >&2; exit 1; }; }

check_platform() { require 'devm_gpiod_get' 'descriptor GPIO acquisition'; require 'devm_request_threaded_irq' 'threaded IRQ request'; require 'misc_register' 'device-node registration'; }
check_irq() { require 'ktime_get_ns' 'monotonic timestamping'; require 'gpiod_set_debounce' 'hardware debounce attempt'; require 'IRQREC_FEATURE_SW_DEBOUNCE' 'software debounce fallback reporting'; require 'wake_up_interruptible' 'event notification'; }
check_abi() { require 'struct irqrec_reader' 'per-open reader state'; require 'wait_event_interruptible' 'blocking read'; require 'poll_wait' 'poll support'; require 'copy_to_user' 'bounded userspace copy'; require 'IRQREC_IOC_GET_INFO' 'versioned ioctl handling'; require 'IRQREC_EVENT_F_OVERFLOW' 'overflow contract'; }
check_hardening() { require 'kref_get_unless_zero' 'referenced lifetime'; require 'synchronize_irq' 'IRQ teardown synchronization'; require 'misc_deregister' 'device-node removal'; require 'POLLHUP' 'terminal poll state'; require 'POLLERR' 'terminal poll state'; }

require 'module_platform_driver' 'platform-driver registration'
case "$activity" in
  lab1) ;;
  lab2) check_platform ;;
  lab3) check_platform; check_irq ;;
  lab4) check_platform; check_irq; check_abi ;;
  lab5|capstone|all) check_platform; check_irq; check_abi; check_hardening ;;
  *) echo "unknown activity: $activity" >&2; exit 2 ;;
esac

python3 "$root/tools/check_uapi.py"
python3 "$root/tools/check_visual_manifest.py"
python3 "$root/tools/check_site.py"
echo "host structural, UAPI, and visual provenance checks passed for $activity"
