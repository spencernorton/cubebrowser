# CubeBrowser

This fork integrates the GameCube web browser with Indigo. See README.md for
controls, card layout, builds and the console lab. Start with the newest
entries in [AGENTS/journal.md](AGENTS/journal.md).

- Work in a feature branch and submit a pull request to main.
- Run ./get-mbedtls.sh, then build with the libogc2 image used by Indigo.
- The existing parser test is tools/lab/test_ui_lab.c with source/ui_lab.c.
- Keep browser controls and card requirements documented in README.md.
- The console lab is off by default; lab=1 enables it for testing.
- Keep private infrastructure, credentials and personal paths out of this repo.
