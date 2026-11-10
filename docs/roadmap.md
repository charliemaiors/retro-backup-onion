# Roadmap

## 0.1 - UI and build scaffold
- SDL2 menu and screen navigation
- Docker cross-build
- OnionOS App Folder package
- CI build artifacts

## 0.2 - SFTP configuration and connectivity
- Local config.ini stored beside the app
- SFTP host, port, username and remote path settings
- Device-local SSH private key and pinned known_hosts file
- Strict host-key checking and batch-mode authentication
- Connection test with bounded timeout
- No credentials or private keys bundled in releases

## 0.3 - Manual backup
- Discover all supported emulator save directories on OnionOS
- Stable local staging and save-file inventory
- Manifest with SHA-256 checksums
- Versioned remote upload and completion marker
- Progress, cancellation and error reporting

## 0.4 - History and restore
- Remote version listing
- Staged download and hash verification
- Safe path validation
- Rollback-safe restore

## 0.5 - Automatic backup
- Opt-in interval-based scheduler
- Quiet-period debounce and save-change detection
- Battery, storage and network safeguards
- Visible status and cancellation
- No automatic pruning in the MVP
