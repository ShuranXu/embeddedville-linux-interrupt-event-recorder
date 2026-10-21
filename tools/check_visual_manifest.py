#!/usr/bin/env python3
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[1]
manifest_path = root / "provenance/visual-sources.json"
manifest = json.loads(manifest_path.read_text())
errors = []
for item in manifest["visuals"]:
    path = root / item["localPath"]
    if not path.is_file():
        errors.append(f"missing {item['localPath']}")
        continue
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != item["sha256"]:
        errors.append(f"hash mismatch {item['localPath']}")
    for field in ("sourceProject", "sourceRef", "sourceUrl", "license", "caption", "alt", "learnerInference"):
        if not item.get(field):
            errors.append(f"{item['localPath']} lacks {field}")
if errors:
    raise SystemExit("visual manifest failed:\n- " + "\n- ".join(errors))
print(f"visual manifest verified: {len(manifest['visuals'])} sourced assets")

