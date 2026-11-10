# Retro Backup

Native OnionOS companion app for backing up and restoring emulator saves.

**Status:** 0.4 - manual SFTP snapshots, remote history and verified restore implemented. Interval scheduling is next.

## Features

- D-pad/button-driven SDL2 interface.
- SFTP configuration via `config.ini`, SSH key authentication and strict pinned host-key verification.
- Manual snapshot of the complete OnionOS `Saves` directory.
- SHA-256 manifest and temporary remote upload directory published by rename after successful transfer.
- Remote history lists only completed timestamped backup directories; `.incomplete-*` uploads are ignored.
- Restore downloads the selected archive, verifies its SHA-256 digest, validates archive paths and rejects links/special files before extraction.
- Restore stages a new `Saves` tree beside the current one, then renames the existing tree to `Saves.rollback-<timestamp>` before activating the restored tree. The rollback directory is intentionally retained.
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
5. Copy `id_ed25519` and `known_hosts` into `/App/RetroBackup/`. The app copies the private key into a temporary mode-0600 file before invoking OpenSSH, which also works around FAT/exFAT permission limitations. Keep the SD card physically secure.
6. Edit `/App/RetroBackup/config.ini`:

   ```ini
   host=sftp.example.net
   port=22
   username=retro-backup
   remote_path=/retro-backup
   identity_file=id_ed25519
   known_hosts_file=known_hosts
   saves_path=../../Saves
   ```

7. Launch Retro Backup, open **Settings**, and press **A** to test the SFTP connection.

Relative credential paths are resolved beside the application. Absolute paths are also accepted. The app requires OpenSSH-compatible `sftp`, `tar` and `sha256sum` executables on OnionOS. The configured remote directory must already exist and be writable.

## Backup and restore behavior

Backups include the complete `Saves` directory, not just the active RetroArch profile. Close games and avoid changing save files while a backup or restore is running; the app does not lock emulator save files.

The restore screen requires selecting a backup and pressing **A** a second time to confirm. A successful restore leaves the previous directory alongside it as `Saves.rollback-<timestamp>`; delete that directory manually only after confirming the restored games work. The app does not delete remote backups or apply retention.

The history view reads directory metadata from the SFTP server. Completed backup folders use names like `YYYYMMDDTHHMMSSZ-PID`; temporary `.incomplete-*` folders are not shown.

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

- Backup and restore operations currently run synchronously in the UI; long transfers temporarily block input. Background execution and interval scheduling are the next implementation step.
- The connection test validates authentication and host-key trust, but does not yet verify remote directory write permissions.
- SFTP, tar and sha256sum availability and SFTP server listing behavior still require validation on physical Miyoo hardware and against a real server.
- UI/device controller mapping still requires hardware validation.
- The bundled runtime SDL libraries are built from the upstream Miyoo SDL port; preserve its LGPL-2.1 license and notices when distributing.

## License

Application source: GPL-3.0-or-later. Third-party components retain their own licenses.
