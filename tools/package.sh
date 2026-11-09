#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP="${ROOT}/build/onion/RetroBackup"
DIST="${ROOT}/dist"
[[ -x "${APP}/retro-backup" ]] || { echo "Missing target executable; run tools/build-onion.sh first" >&2; exit 1; }
mkdir -p "${DIST}"
python3 - "${ROOT}/build/onion" "${DIST}/RetroBackup-onionos.zip" <<'PYCODE'
import pathlib, sys, zipfile
source = pathlib.Path(sys.argv[1]) / "RetroBackup"
target = pathlib.Path(sys.argv[2])
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as archive:
    for path in source.rglob("*"):
        if path.is_file():
            archive.write(path, path.relative_to(source.parent))
PYCODE
(cd "${DIST}" && sha256sum RetroBackup-onionos.zip > SHA256SUMS)
echo "Created ${DIST}/RetroBackup-onionos.zip"
