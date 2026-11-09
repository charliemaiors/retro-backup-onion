# Test checklist

## Desktop
- [ ] app opens at 640x480
- [ ] menu selection wraps/limits correctly
- [ ] Enter/A selects
- [ ] Escape/B returns
- [ ] exit works

## Miyoo Mini Plus / OnionOS 4.3
- [ ] package appears in Apps
- [ ] app launches without black screen
- [ ] screen is correctly oriented and scaled
- [ ] D-pad and A/B buttons map correctly
- [ ] exiting returns to OnionOS
- [ ] no MainUI process manipulation is required
- [ ] runtime shared libraries resolve

## Future backup tests
- [ ] failed upload never publishes `_COMPLETE`
- [ ] corrupted backup is not restorable
- [ ] interrupted restore preserves original Saves
- [ ] symlink/path traversal entries are rejected
