#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS="${ROOT}/.deps"
OUT="${ROOT}/build"
DIST="${ROOT}/dist"
TOOLCHAIN_URL="${MIYOO_TOOLCHAIN_URL:-https://github.com/steward-fu/website/releases/download/miyoo-mini/mini_toolchain-v1.0.tar.gz}"
SDL_REPO="${MIYOO_SDL2_REPOSITORY:-https://github.com/steward-fu/sdl2.git}"
SDL_REF="${MIYOO_SDL2_REF:-master}"

mkdir -p "${DEPS}" "${OUT}" "${DIST}"
if [[ ! -d "${DEPS}/mini_toolchain-v1.0" ]]; then
  echo "Downloading Miyoo toolchain..."
  curl -fL "${TOOLCHAIN_URL}" -o "${DEPS}/toolchain.tar.gz"
  mkdir -p "${DEPS}/mini_toolchain-v1.0"
  tar -xzf "${DEPS}/toolchain.tar.gz" -C "${DEPS}/mini_toolchain-v1.0" --strip-components=1
fi
if [[ ! -d "${DEPS}/sdl2/.git" ]]; then
  git clone "${SDL_REPO}" "${DEPS}/sdl2"
fi
git -C "${DEPS}/sdl2" fetch --all --tags
git -C "${DEPS}/sdl2" checkout "${SDL_REF}"

docker build --platform linux/amd64 -t retro-backup-miyoo-build -f "${DEPS}/sdl2/Dockerfile" "${DEPS}/sdl2"
docker run --rm --platform linux/amd64 \
  -v "${ROOT}:/src" \
  -v "${DEPS}/mini_toolchain-v1.0/mini:/opt/mini" \
  -v "${DEPS}/mini_toolchain-v1.0/prebuilt:/opt/prebuilt" \
  -v "${DEPS}/sdl2:/work/sdl2" \
  -w /src retro-backup-miyoo-build bash -lc '
    set -euo pipefail
    ln -sf /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd /opt/prebuilt/arm-linux-gnueabihf/bin/ld
    ln -sf /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd /opt/prebuilt/bin/arm-linux-gnueabihf-ld
    ln -sf /opt/prebuilt/arm-linux-gnueabihf/bin/ld.bfd /opt/prebuilt/bin/arm-linux-gnueabihf-ld.bfd
    export PATH=/opt/prebuilt/arm-linux-gnueabihf/bin:/opt/prebuilt/bin:/opt/mini/bin:$PATH
    cd /work/sdl2
    make cfg
    make gpu
    make sdl2
    SDL_INC=/work/sdl2/sdl2/include
    SDL_LIB=/work/sdl2/sdl2/build/.libs/libSDL2-2.0.so
    test -f "$SDL_LIB"
    cd /src
    cmake -S . -B build/miyoo \
      -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=arm \
      -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
      -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
      -DRETRO_BACKUP_Miyoo=ON \
      -DSDL2_INCLUDE_DIR="$SDL_INC" \
      -DSDL2_LIBRARY="$SDL_LIB" \
      -DCMAKE_BUILD_TYPE=Release
    cmake --build build/miyoo --parallel 2
    mkdir -p build/onion/RetroBackup/lib
    cp build/miyoo/retro-backup build/onion/RetroBackup/
    cp onion/RetroBackup/config.json onion/RetroBackup/launch.sh onion/RetroBackup/README.txt build/onion/RetroBackup/
    cp /work/sdl2/sdl2/build/.libs/libSDL2-2.0.so.0 build/onion/RetroBackup/lib/
    cp -L /work/sdl2/swiftshader/build/libEGL.so /work/sdl2/swiftshader/build/libGLESv2.so build/onion/RetroBackup/lib/
    JSONC_LIB=$(find /opt/mini /opt/prebuilt /work/sdl2 -name "libjson-c.so.5*" -print -quit)
    test -n "$JSONC_LIB"
    cp -L "$JSONC_LIB" build/onion/RetroBackup/lib/libjson-c.so.5
    cp /work/sdl2/LICENSE build/onion/RetroBackup/THIRD_PARTY_SDL_LICENSE.txt
    chmod +x build/onion/RetroBackup/launch.sh
    file build/onion/RetroBackup/retro-backup
    readelf -h build/onion/RetroBackup/retro-backup | grep -E "Class:|Machine:"
    readelf -h build/onion/RetroBackup/retro-backup | grep -q "Machine:.*ARM"
  '
"${ROOT}/tools/package.sh"
