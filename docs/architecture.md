# Architecture

## Current scaffold

The SDL2 UI is isolated from backup logic. `App` owns screen navigation and rendering. Future backup operations should be implemented behind service interfaces and executed off the SDL event/render thread.

## Planned interfaces

- `BackupService`: snapshot source files, generate manifest, upload, publish completion marker.
- `RestoreService`: list versions, download to staging, verify manifest, request confirmation, preserve rollback.
- `TransferBackend`: generic interface; first implementation invokes rclone SFTP.
- `ConfigStore`: local JSON configuration with restrictive permissions.
- `JobRunner`: background worker with progress events and cancellation.

## Important invariants

1. Never overwrite an existing backup version.
2. Never publish `_COMPLETE` until all content and manifest transfers succeed.
3. Restore only a version with `_COMPLETE` and a valid manifest.
4. Never replace current Saves before staging has passed integrity verification.
5. Keep private keys outside the distributable package.
6. Do not patch OnionOS startup scripts in the MVP.
