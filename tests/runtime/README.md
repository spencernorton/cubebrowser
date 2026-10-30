# Candidate runtime acceptance

The required `build` check boots its own `cubebrowser.dol` in Dolphin. A fresh
FAT32 SD card holds the packaged certificates and fixture settings. The actual
PowerPC program mounts the card, initializes the Broadband Adapter, fetches gzip HTML
and PNG from a local server, decodes the image and renders it. The test requires
both requests and a large patch of the image's pixels. Controller input must
open and close Bookmarks, Menu, Settings and the address keyboard, follow a link to a differently colored
page and return with Back. Screens, request paths, the DOL checksum and Dolphin
logs are retained on failure as well as success.

The emulator runs in an internal Docker network without internet access or
published ports. Its addresses are documentation-only test addresses. The
program under test is the normal built artifact; no test branch or simulated
UI is compiled into it. This complements physical console qualification:
emulation cannot prove real adapters, video cables or return to a resident
Indigo installation.

The Dockerfile pins Dolphin to
`de44626d23a85aa3cc07260f6f97e64f36600652`. The seven patches in `dolphin/` and
`dsu_pad.py` are copied from Indigo commit
`fcdd092a68204e6f36046a6a98e648098e30e1d7`:
[emulator patches](https://github.com/spencernorton/indigo/tree/fcdd092a68204e6f36046a6a98e648098e30e1d7/buildtools/ci/runner/dolphin),
[controller](https://github.com/spencernorton/indigo/blob/fcdd092a68204e6f36046a6a98e648098e30e1d7/buildtools/ui/emulator/dsu_pad.py).
The adapter patch credits CodeFly's original work. Dolphin and these derivative
patches are GPL-2.0-or-later; their headers and attribution are preserved.

To reproduce, build the image with `docker build -t cube-runtime tests/runtime`,
create an internal Docker network with subnet `192.0.2.0/24`, then run the image
on address `192.0.2.3` with this checkout mounted at `/work`:

```sh
python3 tests/runtime/run.py --dol cubebrowser.dol --card-files sd/gcbrowser --out runtime-evidence
```

CI runs both the normal NTSC 4:3 route and `--wide --pal --pad-port 4`.
The latter boots PAL interlaced video with an anamorphic layout and the pad
in the fourth port. The optional `--cpu-core 5` explicitly selects Dolphin's
cached interpreter for ARM hosts whose default JIT exits even on the unchanged
baseline. CI uses the default core; this option does not mask a failed route.
