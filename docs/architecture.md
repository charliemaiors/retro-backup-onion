# Architecture

## Current scaffold

The SDL2 UI is isolated from backup logic. `App` owns screen navigation and rendering. The current SFTP settings screen reads `config.ini` from the application directory and can run a bounded connection/authentication check.

## Components

- `ConfigStore`: reads and writes non-secret SFTP settings. Device-local SSH credentials are separate files.
- `testSftpConnection`: invokes the system OpenSSH `sftp` client without a shell, enforces batch mode and strict known-host verification, and applies a connection timeout.
- `BackupService` (planned): snapshot source files, generate manifest, upload, publish completion marker.
- `RestoreService` (planned): list versions, download to staging, verify manifest, request confirmation, preserve rollback.
- `TransferBackend` (planned): generic interface; first implementation uses SFTP.
- `JobRunner` (planned): execute long-running backup/restore work away from the SDL event/render thread.

## SFTP configuration

The application directory contains a non-secret `config.ini`. By default, `id_ed25519` and `known_hosts` are resolved relative to the same directory. Private keys must have mode 0600. Host key checking is strict; the application never silently trusts a new host key.

The connection test verifies that the SFTP client can authenticate to the configured host. It does not yet upload data or verify that the configured remote directory is writable; that belongs to the manual-backup milestone.

## Important invariants

1. Never overwrite an existing backup version.
2. Never publish `_COMPLETE` until all content and manifest transfers succeed.
3. Restore only a version with `_COMPLETE` and a valid manifest.
4. Never replace current Saves before staging has passed integrity verification.
5. Keep private keys outside the distributable package.
6. Do not patch OnionOS startup scripts in the MVP.
7. Never disable strict host-key checking to make a connection test pass.
