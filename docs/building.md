# Building

The target build follows the upstream Miyoo Mini/Plus SDL2 port instructions:
https://github.com/steward-fu/sdl2

The upstream project documents a Docker-based build using `mini_toolchain-v1.0`, `make cfg`, `make gpu`, and `make sdl2`, and reports support for OnionOS 4.3.1-1.

`tools/build-onion.sh` automates that sequence and compiles the Retro Backup executable against the generated SDL2 library.

## Reproducibility

The initial scaffold uses the SDL2 repository's `master` branch by default to make first bring-up easy. Before producing a public release:
1. select a reviewed immutable SDL2 commit SHA;
2. pin the toolchain archive with a verified SHA256;
3. record the SDL2 fork's LGPL-2.1 license and notices;
4. verify all runtime shared libraries with `readelf`/`ldd` against the device filesystem;
5. run the app on physical OnionOS hardware.

The host CI job checks source-level compilation with distro SDL2. The Miyoo job checks target compilation and package creation.
