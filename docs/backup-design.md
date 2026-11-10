# OnionOS save backup and restore design

## Source scope

The source is the complete SD-card `Saves` directory, defaulting to
`../../Saves` relative to the installed app. This includes all RetroArch
profiles and OnionOS metadata, rather than only the active profile.

Important subtrees include:

- `Saves/CurrentProfile/saves/[CORENAME]`: in-game save files.
- `Saves/CurrentProfile/states/[CORENAME]`: save states.

Close games and avoid changing saves during a backup or restore. The app does
not lock emulator processes or create a filesystem-level snapshot.

## Backup publication

1. Validate the source and enumerate regular files without following symlinks.
2. Create a local tar archive and SHA-256 manifest.
3. Upload both files into `.incomplete-<id>` on the SFTP server.
4. Rename the temporary directory to `YYYYMMDDTHHMMSSZ-PID` only after both
   uploads complete successfully.

The configured remote path must already exist and be writable. No retention
or remote deletion is performed.

## History and restore

History reads remote directory metadata and only displays directories matching
the timestamped backup ID format. Incomplete upload directories are excluded.

Restore downloads `backup.tar` and `manifest.sha256`, verifies the digest,
then validates archive paths and rejects absolute paths, parent traversal,
symlinks, hard links and special files. Extraction occurs in a temporary
directory beside the target `Saves` directory. Only after successful
verification and extraction is the current `Saves` renamed to
`Saves.rollback-<timestamp>` and the staged tree moved into place. If the
second rename fails, the app attempts to put the original directory back.
The rollback directory is retained after a successful restore for manual
recovery and cleanup.

## Dependencies and limitations

The device must provide OpenSSH-compatible `sftp`, `tar` and `sha256sum`.
Operations currently run synchronously and block the UI. Interval scheduling
and background execution are planned separately. Physical device and real
SFTP-server validation are still required.
