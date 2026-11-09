# Retro Backup

Native OnionOS companion app for backing up and restoring emulator saves.

**Status:** starter scaffold / UI prototype. No backup or restore operations are implemented yet.

## Target

- Miyoo Mini Plus
- OnionOS 4.3.x
- SDL2 UI
- Docker-based ARM hard-float cross-build
- GitHub Actions CI

## Repository status

This repository currently contains:
- an SDL2 application shell with a D-pad/button-driven menu;
- host CMake build configuration;
- a Docker cross-build flow based on the Miyoo Mini toolchain;
- OnionOS Community App packaging skeleton;
- CI for source checks and ARM package build.

The target SDL2 port and toolchain are fetched by the build script. Their upstream revisions should be pinned before a public release.

## Quick start: Docker build

Requirements: Docker with BuildKit and `linux/amd64` emulation support.

```sh
./tools/build-onion.sh
```

Outputs:
- `build/onion/RetroBackup/` — staged OnionOS app directory;
- `dist/RetroBackup-onionos.zip` — installable app folder archive;
- `dist/SHA256SUMS` — archive checksum.

The script downloads the Miyoo toolchain and the SDL2 port source into `.deps/`.
The toolchain archive URL and SDL2 Git ref can be overridden with:
- `MIYOO_TOOLCHAIN_URL`
- `MIYOO_SDL2_REPOSITORY`
- `MIYOO_SDL2_REF`

For repeatable releases, replace the default SDL2 branch with a reviewed immutable commit SHA and record the toolchain archive SHA256 in the release workflow.

## Desktop development

Install SDL2 development headers and CMake, then:

```sh
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host --parallel
./build/host/retro-backup
```

On Debian/Ubuntu:

```sh
sudo apt-get install build-essential cmake libsdl2-dev
```

## Install on OnionOS

Copy `dist/RetroBackup-onionos.zip` contents to the SD card so the final layout is:

```text
/App/RetroBackup/config.json
/App/RetroBackup/launch.sh
/App/RetroBackup/retro-backup
/App/RetroBackup/lib/libSDL2-2.0.so.0
/App/RetroBackup/lib/libEGL.so
/App/RetroBackup/lib/libGLESv2.so
/App/RetroBackup/lib/libjson-c.so.5
```

Launch it from the OnionOS Apps menu. This scaffold intentionally does not patch `runtime.sh`, stop MainUI, or install a boot-time daemon.

## Controls

- D-pad / arrow keys: move selection
- A / Return: select
- B / Escape: back
- Menu: exit

The exact SDL key mapping must be confirmed on physical OnionOS 4.3 hardware.

## Current limitations

- Backup, SFTP, restore, watcher, and settings persistence are not implemented.
- UI is a functional navigation prototype, not a finished visual design.
- Device runtime and controller mapping still require hardware validation.
- The bundled runtime SDL libraries are built from the upstream Miyoo SDL port; preserve its LGPL-2.1 license and notices when distributing.

## License

Application source: GPL-3.0-or-later. Third-party components retain their own licenses.
