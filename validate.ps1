param([switch]$Runtime)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$venv = Join-Path $root '.venv'
$python = Join-Path $venv 'Scripts\python.exe'

if (-not (Test-Path $python)) {
    python -m venv $venv
    & $python -m pip install --disable-pip-version-check -r (Join-Path $root 'requirements.txt')
}

Push-Location $root
try {
    & $python -m mkdocs build --strict
    if ($LASTEXITCODE -ne 0) { throw 'MkDocs strict build failed.' }
    & $python tools/check_uapi.py
    if ($LASTEXITCODE -ne 0) { throw 'UAPI validation failed.' }
    & $python tools/check_visual_manifest.py
    if ($LASTEXITCODE -ne 0) { throw 'Visual provenance validation failed.' }
    & $python tools/check_site.py
    if ($LASTEXITCODE -ne 0) { throw 'Built-site validation failed.' }

    $overlay = Get-Content -Raw hardware/rpi/irqrec-overlay.dts
    foreach ($marker in @(
        'compatible = "embeddedville,irqrec-v1"',
        'event-gpios = <&gpio 17 0>',
        'debounce-us = <5000>',
        'queue-depth = <64>'
    )) {
        if (-not $overlay.Contains($marker)) { throw "Raspberry Pi overlay marker missing: $marker" }
    }

    Write-Host 'Course, UAPI, provenance, site, and Raspberry Pi checks passed.' -ForegroundColor Green
    if ($Runtime) {
        wsl bash -lc "cd '$($root.Replace('\', '/').Replace('C:', '/mnt/c'))' && ./lab test capstone"
        if ($LASTEXITCODE -ne 0) { throw 'QEMU runtime validation failed.' }
    }
}
finally {
    Pop-Location
}

