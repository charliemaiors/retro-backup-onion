# Security notes

- SFTP private keys are device-local and must never be included in Git or release archives.
- Pin the server host key in `known_hosts`.
- Prefer key-only SFTP accounts with no shell, forwarding, or PTY.
- Store configuration and keys with restrictive permissions.
- Treat remote backup names and manifest contents as untrusted input.
- Do not follow symlinks while collecting the save tree.
- Restore must reject path traversal, absolute paths, duplicate paths, and symlink entries.
- No automatic deletion/pruning in the MVP.
