# Security notes

- SFTP private keys are device-local and must never be included in Git or release archives.
- Pin the server host key in `known_hosts`; compare its fingerprint through a trusted channel before accepting it.
- The SFTP connection test uses strict host-key checking and batch-mode authentication.
- The connection check stages the selected private key into a temporary file with mode 0600 before invoking OpenSSH. The source key remains on the device SD card and should still be protected physically.
- Prefer key-only SFTP accounts with no shell, forwarding, or PTY, and restrict the account to its backup directory where possible.
- Store configuration and keys with restrictive permissions.
- Treat remote backup names and manifest contents as untrusted input.
- Do not follow symlinks while collecting the save tree.
- Restore must reject path traversal, absolute paths, duplicate paths, and symlink entries.
- No automatic deletion/pruning in the MVP.
