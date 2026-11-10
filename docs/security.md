# Security notes

- SFTP private keys are device-local and must never be included in Git or release archives.
- Pin the server host key in `known_hosts`; compare its fingerprint through a trusted channel before accepting it.
- The SFTP connection test uses strict host-key checking and batch-mode authentication.
- Private key files must be mode 0600. The application rejects keys accessible by group or others.
- Prefer key-only SFTP accounts with no shell, forwarding, or PTY, and restrict the account to its backup directory where possible.
- Store configuration and keys with restrictive permissions.
- Treat remote backup names and manifest contents as untrusted input.
- Do not follow symlinks while collecting the save tree.
- Restore must reject path traversal, absolute paths, duplicate paths, and symlink entries.
- No automatic deletion/pruning in the MVP.
