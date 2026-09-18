# tinytouch-bzz

Standalone fork of ZimengXiong/tinyTouch. Read [FORK.md](FORK.md) first: it
holds the rule that governs every change here (keep the diff against upstream
small) plus the release, signing-key, and upstream-sync procedures.

- Tests: `.venv/bin/python -m unittest discover -s tests` (deps in
  `software/macos-helper/requirements.txt`). Several tests assert on the text
  of workflows, packaging scripts, and firmware sources, so run them after
  editing those.
- Firmware: `./firmware/build-and-flash --build-only` (ESP-IDF 5.3.x).
- The repo slug lives only in `RELEASE_REPO` (`tinytouch`) and
  `packaging/install.sh`. Do not hardcode it anywhere else.
- Never commit `*.pem`. The signing key cannot be rotated.
