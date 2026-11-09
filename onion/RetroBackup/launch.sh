#!/bin/sh
APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
export LD_LIBRARY_PATH="${APP_DIR}/lib:/config/lib:/customer/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
cd "$APP_DIR" || exit 1
exec "${APP_DIR}/retro-backup"
