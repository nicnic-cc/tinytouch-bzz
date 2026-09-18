# Maintaining this fork

tinytouch-bzz is a standalone fork of
[ZimengXiong/tinyTouch](https://github.com/ZimengXiong/tinyTouch). It has its
own releases, its own version line, and its own firmware signing key, and it
still merges selected upstream changes.

## The one rule

Keep the diff against upstream small. Every line changed in `tinytouch`,
`packaging/`, or `.github/workflows/` is a future merge conflict.

- Fork-specific values live in one place. The repo slug is `RELEASE_REPO` in
  `tinytouch` and the default in `packaging/install.sh`. Nothing else names it.
- Tests derive URLs from the CLI constants and never repeat the slug.
- Leave upstream files alone unless they are broken here.
- New fork features go in self-contained blocks, such as `command_flash`.

## What differs from upstream

| area | fork |
| -- | -- |
| removed | `docs/`, `software/api/`, web flasher, docs workflow, launchd plist, `FUNDING.yml`, CI attribution policy |
| added | haptics (`haptic.c`), LED-off idle, two 9d operations per touch (`piv.c`), `tinytouch flash`, source-mode `tinytouch update`, `packaging/sync-upstream` |
| owned | `VERSION`, `README.md`, `.github/CODEOWNERS`, `channels/batch-0.json` |

## Versioning

The fork has its own version line, starting at `1.0.0-prod`. The `x.y.z-prod`
format is enforced by the CLI and by `packaging/release_integrity.py`, so keep
it. Upstream `VERSION` changes are always discarded. The first line of
`README.md` records which upstream version the fork is based on.

## The signing key decides who can update

Firmware is signed, and a device only accepts an OTA image signed with the
same key as the firmware it is already running. Hardware secure boot is off,
so a ROM flash over USB always works regardless of key.

- CI signs releases with the `TINYTOUCH_FIRMWARE_SIGNING_KEY_B64` secret in
  the `release-signing` environment.
- That secret is the same key as the maintainer's local
  `firmware/tiny_touch_unified/secure_boot_signing_key.pem`, so local builds
  and releases can OTA to each other.
- **There is no rotation path.** Changing or losing the key means every user
  must reflash with `tinytouch flash`. Keep an offline backup.
- Anyone building with their own key is off the update track until they run
  `tinytouch flash`.

## Releasing

1. Bump `VERSION`.
2. Merge to `main` and push, or run `packaging/release`.
3. `release-candidate.yml` builds signed firmware and both macOS CLIs, then
   attests the bundle. `release.yml` verifies the attestation, creates the
   `v<VERSION>` tag, and publishes the GitHub release.

A release carries `tiny_touch_factory_full.bin` for `tinytouch flash`,
`tiny_touch_unified.bin` for `tinytouch update`, the CLI tarballs,
`install.sh`, `release-manifest.json`, and `checksums.txt`.

One-time GitHub setup: enable Actions, enable immutable releases, and create
the `release-signing` environment restricted to `main` and `beta/**` with the
secret above:

```sh
base64 < firmware/tiny_touch_unified/secure_boot_signing_key.pem | tr -d '\n' | pbcopy
```

Beta branches publish prereleases, but `tinytouch update` only follows
`-prod` releases.

## Syncing upstream

```sh
packaging/sync-upstream
```

It needs a clean worktree. It fetches upstream, stages a merge without
committing, deletes everything this fork removed, keeps the fork's copy of the
owned files, lists any remaining conflicts, runs the tests, and reports
upstream URLs or contact details that came in through clean merges.

To skip an upstream change, revert it inside the staged merge before
committing. Merging everything and reverting the unwanted parts keeps the
merge base moving, so the same conflicts do not come back next time.
Cherry-picking instead makes every later sync harder.

After a sync, update the "based on upstream" line in `README.md`.
