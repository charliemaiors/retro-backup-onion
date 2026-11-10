# OnionOS save backup and restore design

The source is the complete SD-card `Saves` directory (default `../../Saves`
relative to the app). This includes all RetroArch profiles and OnionOS metadata.
Close games and avoid changing saves during a backup or restore: no emulator
process lock or filesystem snapshot is taken.

One backup is queued automatically on each application launch. There is no
timer, recurring schedule or background daemon. Manual requests use the same
worker; requests are rejected while a backup is queued or running. The worker
runs independently of the SDL event loop so the UI remains responsive.

Backups are tar snapshots with a SHA-256 manifest. The archive and manifest are
uploaded into a temporary `.incomplete-<id>` directory and published by rename
to `YYYYMMDDTHHMMSSZ-PID` only after both transfers succeed.

History lists only completed timestamped directories. Restore downloads and
verifies the archive, rejects unsafe paths and non-regular/non-directory tar
entries, extracts into a staging directory beside `Saves`, then renames the
current tree to `Saves.rollback-<timestamp>` before activating the restored
tree. The rollback directory is retained.

The device requires OpenSSH-compatible `sftp`, `tar` and `sha256sum`.
Physical Miyoo hardware and real SFTP-server validation remain required.
