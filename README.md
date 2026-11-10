# Retro Backup

Native OnionOS companion app for backing up and restoring emulator saves.

**Status:** 0.2 - SFTP configuration and connection test implemented. Backup and restore operations are the next milestones.

## Target

- Miyoo Mini Plus
- OnionOS 4.3.x
- SDL2 UI
- Docker-based ARM hard-float cross-build
- GitHub Actions CI

## Features

- D-pad/button-driven SDL2 interface.
- SFTP connection configuration via `config.ini`.
- SSH key authentication with strict pinned host-key verification.
- SFTP connection test with an 8-second connect timeout and bounded overall execution.
- Host CMake build, ARM cross-build and OnionOS App Folder packaging.

## Build

Requirements: Docker with BuildKit and `linux/amd64` emulation support.

```sh
bash ./tools/build-onion.sh
```

Outputs:
- `build/onion/RetroBackup/` — staged OnionOS app directory;
- `dist/RetroBackup-onionos.zip` — installable app folder archive;
- `dist/SHA256SUMS` — archive checksum.

The script downloads the Miyoo toolchain and SDL2 port source into `.deps/`. The toolchain archive URL and SDL2 Git ref can be overridden with `MIYOO_TOOLCHAIN_URL`, `MIYOO_SDL2_REPOSITORY` and `MIYOO_SDL2_REF`.

## Configure SFTP

1. Copy the package contents to the SD card so the app is installed at `/App/RetroBackup`.
2. On a trusted computer, create a dedicated key pair (do not use your personal login key):

   ```sh
   ssh-keygen -t ed25519 -f id_ed25519 -N ""
   ```

3. Add the public key to the SFTP account's `authorized_keys`.
4. Obtain the server host key and verify its fingerprint with the server administrator through a trusted channel. Then create the pinned host-key file:

   ```sh
   ssh-keyscan -p 22 sftp.example.net > known_hosts
   ssh-keygen -lf known_hosts
   ```

   Do not trust the `ssh-keyscan` output until its fingerprint has been independently verified.
5. Copy `id_ed25519` and `known_hosts` into `/App/RetroBackup/`. Set the private key to mode 600 where possible.
6. Edit `/App/RetroBackup/config.ini`:

   ```ini
   host=sftp.example.net
   port=22
   username=retro-backup
   remote_path=/retro-backup
   identity_file=id_ed25519
   known_hosts_file=known_hosts
   ```

7. Launch Retro Backup, open **Settings**, and press **A** to test the SFTP connection.

Relative credential paths are resolved beside the application. Absolute paths are also accepted. The app requires an OpenSSH-compatible `sftp` executable on the OnionOS device; if it is not available, the connection test reports that explicitly.

## Desktop development

Install SDL2 development headers and CMake, then:

```sh
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host --parallel
./build/host/retro-backup
```

On Debian/Ubuntu: `sudo apt-get install build-essential cmake libsdl2-dev`.

## Install layout

```text
/App/RetroBackup/config.json
/App/RetroBackup/config.ini
/App/RetroBackup/launch.sh
/App/RetroBackup/retro-backup
/App/RetroBackup/id_ed25519       # device-local; never distribute
/App/RetroBackup/known_hosts      # device-local pinned server keys
/App/RetroBackup/lib/...
```

The app does not patch `runtime.sh`, stop MainUI, or install a boot-time daemon.

## Current limitations

- Backup, history, restore and interval scheduler are not implemented yet.
- SFTP connectivity depends on an OpenSSH-compatible `sftp` client being present in OnionOS; this must be confirmed on physical hardware.
- The connection test validates authentication and host-key trust, but does not yet verify remote directory write permissions.
- UI/device controller mapping still requires hardware validation.
- The bundled runtime SDL libraries are built from the upstream Miyoo SDL port; preserve its LGPL-2.1 license and notices when distributing.

## License

Application source: GPL-3.0-or-later. Third-party components retain their own licenses.
