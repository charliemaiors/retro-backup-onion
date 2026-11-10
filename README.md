# Retro Backup

Native OnionOS companion app for backing up and restoring emulator saves.

**Status:** 0.5 - startup backup, manual backup, remote history and verified restore.

## Features

- D-pad/button-driven SDL2 interface.
- SFTP configuration via `config.ini`, SSH key authentication and strict pinned host-key verification.
- Automatically starts one backup when the app opens; no interval scheduler.
- Manual backup and retry from the Backup screen, sharing the same worker to prevent overlapping runs.
- Snapshot of the complete OnionOS `Saves` directory, SHA-256 manifest and temporary remote upload directory published by rename after successful transfer.
- Remote history lists completed timestamped backup directories.
- Restore verifies the SHA-256 digest, rejects unsafe archive paths and links/special files, extracts to staging, and retains the previous `Saves` as `Saves.rollback-<timestamp>`.

## Build

Requirements: Docker with BuildKit and `linux/amd64` emulation support.

```sh
bash ./tools/build-onion.sh
```

Outputs: `build/onion/RetroBackup/`, `dist/RetroBackup-onionos.zip`, and `dist/SHA256SUMS`.

## Configure SFTP

Install the app at `/App/RetroBackup`. Create a dedicated SSH key, add its public key to the SFTP account, and install the private key plus a host-key file whose fingerprint you verified independently.

Edit `/App/RetroBackup/config.ini`:

```ini
host=sftp.example.net
port=22
username=retro-backup
remote_path=/retro-backup
identity_file=id_ed25519
known_hosts_file=known_hosts
saves_path=../../Saves
```

The remote root must already exist and be writable. The device needs OpenSSH-compatible `sftp`, `tar` and `sha256sum`.

## Backup and restore

A backup starts automatically once when Retro Backup is launched. The Backup screen shows the worker state; pressing **A** requests a manual backup or retry if the worker is idle. Automatic and manual requests cannot overlap.

Backups include the complete `Saves` directory. Close games and avoid changing save files while backup/restore is running; the app does not lock emulator save files or create a filesystem snapshot.

Restore requires selecting a backup and pressing **A** a second time. The current directory is retained as `Saves.rollback-<timestamp>` after success; remove it manually only after confirming the restored games work. Restore is blocked while a backup is running. No remote retention or deletion is performed.

## Development

```sh
cmake -S . -B build/host -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host --parallel
```

On Debian/Ubuntu: `sudo apt-get install build-essential cmake libsdl2-dev`.

Physical Miyoo hardware and real SFTP-server validation remain required. The app does not patch `runtime.sh`, stop MainUI, or install a boot-time daemon.

## License

Application source: GPL-3.0-or-later. Third-party components retain their own licenses.
