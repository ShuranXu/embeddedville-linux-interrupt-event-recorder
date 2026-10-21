#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 -m venv "$root/.venv"
"$root/.venv/bin/python" -m pip install --disable-pip-version-check -r "$root/requirements.txt"
printf 'Course preview: http://127.0.0.1:8000/\n'
exec "$root/.venv/bin/python" -m mkdocs serve --config-file "$root/mkdocs.yml" --dev-addr 127.0.0.1:8000

