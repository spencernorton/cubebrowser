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
`de44626d23a85aa3cc07260f6f97e64f36600652`. The first seven patches in `dolphin/` and
`dsu_pad.py` are copied from Indigo commit
`fcdd092a68204e6f36046a6a98e648098e30e1d7`:
[emulator patches](https://github.com/spencernorton/indigo/tree/fcdd092a68204e6f36046a6a98e648098e30e1d7/buildtools/ci/runner/dolphin),
[controller](https://github.com/spencernorton/indigo/blob/fcdd092a68204e6f36046a6a98e648098e30e1d7/buildtools/ui/emulator/dsu_pad.py).
The adapter patch credits CodeFly's original work. Dolphin and these derivative
patches are GPL-2.0-or-later; their headers and attribution are preserved.
The eighth patch completes immediate EXI transfers without a selected chip
preserves outgoing SPI bytes on SD receive-mode transfers, and accepts a card
reset during an earlier read. Indigo's real reload stub uses these operations;
the earlier model waits forever on the transfer-start bit or fails to reset
and initialize the SD card.

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

ZIP persistence has a separate required `run_zip.py` route. Generate an
expiring self-signed fixture certificate with `zip-cert.cnf` (as the workflow
shows), then pass its directory with `--cert-dir`. The test starts private DNS
and TLS servers on an isolated network. Its temporary CA is copied only into
the private FAT image; the packaged CA bundle is unchanged. The actual DOL
saves the response preference, sends it on the first request after a new boot,
clears it through controller navigation and leaves it cleared after a third
boot. Read back the real FAT file after each boot. Request evidence also proves
path isolation and first-party weather-resource support. This is emulator card
qualification, not a claim about a physical card or the production form.

The required `run_indigo.py` route builds Indigo commit
`3d9512b2ec9e8a84e57a65670ebb294585b7c4d0` in the pinned SDK and requires its
DOL to match the green beta package, SHA256
`45b371fe823e315b15ba84f1f406ebc8b94d2b48ff56df0d5bdca1fcbbe5ea4c`.
Its normal Web face launches this candidate from a real FAT card image.
Named Spice/widescreen preferences must override saved Indigo/4:3 values.
Both the menu and toolbar return controls must reload Indigo Home through
the launcher's unmodified STUBHAXX code. The returned Home must respond to
Web/Library navigation, and standalone preferences must survive. Return uses
an 80 ms controller tap, released before the emulated reload reaches Home.
The second trip requires another actual HTTP/image fetch, so a
frozen browser frame cannot pass as a reload.

In this fresh test profile, Dolphin's `EnableCheats` setting skips its
[HBReload interception](https://github.com/dolphin-emu/dolphin/blob/de44626d23a85aa3cc07260f6f97e64f36600652/Source/Core/Core/HLE/HLE.cpp),
which otherwise stops emulation at `0x80001800`. There are no cheat codes in
the profile. This lets the actual guest stub read `IPL.DOL`; no fake loader,
memory patch or host-side jump substitutes for the return. See the workflow
for the pinned build and mounts, then run:

```sh
python3 tests/runtime/run_indigo.py --dol cubebrowser.dol --card-files sd/gcbrowser \
  --indigo-src /indigo --indigo-dol /indigo/cube/swiss/swiss.dol \
  --indigo-provenance runtime-evidence/indigo-return/indigo-provenance.json \
  --out runtime-evidence/indigo-return
```

This proves the emulated SD2SP2 round trip. Physical adapters, cables and the
installed console remain separate qualification.

CI repeats the round trip with `--no-motion` and checks that the saved
standalone animation preference also survives. Both profiles use complete
texture hashing: two DOLs reuse texture addresses, and Dolphin's sampled
cache can otherwise retain stale texels after a reload.
