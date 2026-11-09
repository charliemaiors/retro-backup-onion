#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS="${ROOT}/.deps"
OUT="${ROOT}/build"
DIST="${ROOT}/dist"

TOOLCHAIN_URL="${MIYOO_TOOLCHAIN_URL:-https://github.com/steward-fu/website/releases/download/miyoo-mini/mini_toolchain-v1.0.tar.gz}"
SDL_REPO="${MIYOO_SDL2_REPOSITORY:-https://github.com/steward-fu/sdl2.git}"
SDL_REF="${MIYOO_SDL2_REF:-0631abc8e8916db6f9bc7e2afd0c22913d092a29}"

mkdir -p "${DEPS}" "${OUT}" "${DIST}"

TOOLCHAIN_EXTRACT="${DEPS}/toolchain-extract"
if [[ ! -d "${TOOLCHAIN_EXTRACT}" ]]; then
  echo "Downloading Miyoo toolchain (about 750 MB)..."
  curl -fL --retry 3 --retry-delay 2 "${TOOLCHAIN_URL}" -o "${DEPS}/toolchain.tar.gz"
  mkdir -p "${TOOLCHAIN_EXTRACT}"
  tar -xzf "${DEPS}/toolchain.tar.gz" -C "${TOOLCHAIN_EXTRACT}"
fi

MINI_DIR="$(find "${TOOLCHAIN_EXTRACT}" -type d -name mini -print -quit)"
PREBUILT_DIR="$(find "${TOOLCHAIN_EXTRACT}" -type d -name prebuilt -print -quit)"
if [[ -z "${MINI_DIR}" || -z "${PREBUILT_DIR}" ]]; then
  echo "Could not locate mini/ and prebuilt/ in the extracted Miyoo toolchain." >&2
  find "${TOOLCHAIN_EXTRACT}" -maxdepth 4 -type d -print >&2
  exit 1
fi

for required in \
  "${MINI_DIR}/bin/arm-linux-gnueabihf-gcc" \
  "${PREBUILT_DIR}/arm-linux-gnueabihf/bin/ld.bfd" \
  "${PREBUILT_DIR}/bin"; do
  if [[ ! -e "${required}" ]]; then
    echo "Missing expected Miyoo toolchain path: ${required}" >&2
    exit 1
  fi
done

if [[ ! -d "${DEPS}/sdl2/.git" ]]; then
  git clone "${SDL_REPO}" "${DEPS}/sdl2"
fi
git -C "${DEPS}/sdl2" fetch --all --tags
git -C "${DEPS}/sdl2" checkout "${SDL_REF}"

docker build --platform linux/amd64 -t retro-backup-miyoo-build -f "${DEPS}/sdl2/Dockerfile" "${DEPS}/sdl2"

docker run --rm --platform linux/amd64 \
  -v "${ROOT}:/src" \
  -v "${MINI_DIR}:/opt/mini" \
  -v "${PREBUILT_DIR}:/opt/prebuilt" \
  -v "${DEPS}/sdl2:/work/sdl2" \
  -w /src retro-backup-miyoo-build bash -lc '
    set -euo pipefail

    test -x /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd
    test -x /opt/mini/bin/arm-linux-gnueabihf-gcc
    ln -sf /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd /opt/prebuilt/arm-linux-gnueabihf/bin/ld
    ln -sf /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd /opt/prebuilt/bin/arm-linux-gnueabihf-ld
    ln -sf /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd /opt/prebuilt/bin/arm-linux-gnueabihf-ld.bfd

    export PATH=/opt/prebuilt/arm-linux-gnueabihf/bin:/opt/prebuilt/bin:/opt/mini/bin:$PATH

    # Capture json-c before building SDL2/SwiftShader. The upstream build
    # scripts can alter the mounted toolchain view, so do not search /opt
    # again after the make targets have run.
    JSONC_LIB=$(find -L /opt/mini /opt/prebuilt -type f -name "libjson-c.so.5*" -print -quit 2>/dev/null || true)
    if [[ -z "$JSONC_LIB" ]]; then
      echo "libjson-c.so.5 not found in the mounted Miyoo toolchain" >&2
      exit 1
    fi
    mkdir -p /src/build/onion/RetroBackup/lib
    cp -L "$JSONC_LIB" /src/build/onion/RetroBackup/lib/libjson-c.so.5

    cd /work/sdl2
    # Run all goals together: the upstream Makefile creates SDL2-to-SwiftShader
    # symlinks at parse time, so separate make invocations fail on existing links.
    make cfg gpu sdl2

    SDL_INC=/work/sdl2/sdl2/include
    # Upstream libtool creates libSDL2.so -> libSDL2-2.0.so.0.18.2;
    # it does not create an unversioned libSDL2-2.0.so.
    SDL_LIB=/work/sdl2/sdl2/build/.libs/libSDL2.so
    if [[ ! -f "$SDL_LIB" ]]; then
      echo "Expected SDL2 shared library not found: $SDL_LIB" >&2
      find /work/sdl2/sdl2/build -maxdepth 3 -name "libSDL2*" -ls >&2 || true
      exit 1
    fi

    # The SDL2 Mini backend intentionally carries unresolved SigmaStar
    # firmware symbols. The matching shared objects are DT_NEEDED dependencies
    # provided by OnionOS at runtime, not link-time inputs from the toolchain.
    # Allow those shared-library references while linking our executable.
    cd /src
    cmake -S . -B build/miyoo \
      -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=arm \
      -DCMAKE_C_COMPILER=/opt/mini/bin/arm-linux-gnueabihf-gcc \
      -DCMAKE_CXX_COMPILER=/opt/mini/bin/arm-linux-gnueabihf-g++ \
      -DRETRO_BACKUP_Miyoo=ON \
      -DSDL2_INCLUDE_DIR="$SDL_INC" \
      -DSDL2_LIBRARY="$SDL_LIB" \
      "-DSDL2_EXTRA_LIBRARIES=-Wl,--no-as-needed;/work/sdl2/swiftshader/build/libEGL.so;/work/sdl2/swiftshader/build/libGLESv2.so" \
      "-DCMAKE_EXE_LINKER_FLAGS=-Wl,--allow-shlib-undefined" \
      -DCMAKE_BUILD_TYPE=Release

    cmake --build build/miyoo --parallel 2

    mkdir -p build/onion/RetroBackup/lib
    cp build/miyoo/retro-backup build/onion/RetroBackup/
    cp onion/RetroBackup/config.json onion/RetroBackup/launch.sh onion/RetroBackup/README.txt build/onion/RetroBackup/
    cp /work/sdl2/sdl2/build/.libs/libSDL2-2.0.so.0 build/onion/RetroBackup/lib/
    cp -L /work/sdl2/swiftshader/build/libEGL.so /work/sdl2/swiftshader/build/libGLESv2.so build/onion/RetroBackup/lib/

    cp /work/sdl2/LICENSE build/onion/RetroBackup/THIRD_PARTY_SDL_LICENSE.txt
    chmod +x build/onion/RetroBackup/launch.sh

    command -v readelf >/dev/null
    readelf -h build/onion/RetroBackup/retro-backup > build/onion/RetroBackup/elf-header.txt
    grep -Eq "Machine:.*ARM" build/onion/RetroBackup/elf-header.txt
    rm build/onion/RetroBackup/elf-header.txt
  '

bash "${ROOT}/tools/package.sh"
