#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "starter/include/irqrec_uapi.h").read_text()
required = {
    "IRQREC_ABI_VERSION": r"#define\s+IRQREC_ABI_VERSION\s+1U",
    "event structure": r"struct\s+irqrec_event_v1",
    "info ioctl": r"IRQREC_IOC_GET_INFO",
    "stats ioctl": r"IRQREC_IOC_GET_STATS",
    "overflow flag": r"IRQREC_EVENT_F_OVERFLOW",
}
missing = [name for name, pattern in required.items() if not re.search(pattern, header)]
if missing:
    raise SystemExit("UAPI check failed: " + ", ".join(missing))
print("UAPI contract markers present; compiled size assertions run in irqrec-test")

