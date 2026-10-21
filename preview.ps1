$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$venv = Join-Path $root '.venv'

if (-not (Test-Path (Join-Path $venv 'Scripts\python.exe'))) {
    python -m venv $venv
}

$python = Join-Path $venv 'Scripts\python.exe'
& $python -m pip install --disable-pip-version-check -r (Join-Path $root 'requirements.txt')
Write-Host 'Course preview: http://127.0.0.1:8000/' -ForegroundColor Cyan
& $python -m mkdocs serve --config-file (Join-Path $root 'mkdocs.yml') --dev-addr 127.0.0.1:8000

