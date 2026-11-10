# Manual backup design (OnionOS 4.3)

## Source-of-truth scope

The backup source is the complete SD-card `Saves` directory, defaulting to
`../../Saves` relative to the installed app directory. This deliberately
includes all RetroArch profiles rather than assuming that only
`CurrentProfile` is in use.

Onion's documentation identifies these important subtrees:

- `Saves/CurrentProfile/saves/[CORENAME]`: in-game save files.
- `Saves/CurrentProfile/states/[CORENAME]`: save states.

The full `Saves` directory can also contain secondary profiles, configuration
files, RetroArch lists and play-activity data. Backing up the whole directory
avoids an incomplete allow-list of extensions or emulator core names.

## Snapshot and publication

1. Validate that the configured source exists and is a directory.
2. Do not follow symbolic links while inventorying the source.
3. Create a temporary archive on the SD card; require enough free space for a
   second copy of the source data.
4. Calculate a SHA-256 digest and write a manifest.
5. Upload the archive and manifest into a uniquely named
   `.incomplete-<id>` directory below the configured remote path.
6. Rename that directory to its final timestamped name only after both uploads
   succeed. A failed upload must never look like a completed backup.

The configured remote path must already exist and be writable by the SFTP
account. No remote retention/pruning is performed.

## Dependencies and limitations

The device must provide OpenSSH-compatible `sftp`, `tar` and `sha256sum`.
The archive temporarily consumes SD-card space approximately equal to the
source directory. The first implementation runs synchronously; progress,
cancellation, remote history and restore are separate follow-up work.

## References

- Onion FAQ: save files and save states under
  `Saves/CurrentProfile/saves` and `Saves/CurrentProfile/states`.
- Onion Features wiki: recommends copying the full `Saves` directory, which
  also holds profiles, configs, RetroArch lists and play activity.
