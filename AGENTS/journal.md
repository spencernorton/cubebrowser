## 2026-10-29 — Require saved parser regressions

Repair the host SDK stand-in after gfx.h gained the video-mode type and add
a deterministic sanitizer driver for the saved HTML/CSS inputs. The required
build runs these cases before building and booting its DOL. The runner works
from any directory; only extended fuzzing needs libFuzzer. All seven saved
cases pass with host AddressSanitizer and UndefinedBehaviorSanitizer.

## 2026-10-29 — Boot and browser behavior gate

The required build now boots its own DOL with an emulated SD card, Broadband
Adapter and controller. It must fetch fixture HTML and PNG, render the image,
open and close Bookmarks and Settings, follow a link and go Back. The fixture
network has no internet access. Pin the SDK and emulator sources; preserve
screens, requests, DOL SHA-256 and emulator logs. The route passes against the
existing console-qualified DOL. Physical adapters and return to a resident
Indigo remain console qualifications.

## 2026-10-23 — Return button validated on the console

The target build passes on the pinned libogc2 SDK. CI39945054030 passes,
including the sanitizer parser test; its exact DOL was used for hardware
validation. The labelled button is visible on the start and help pages.
Bookmarks retain modal input ownership: clicking the obscured return button
keeps the dialog open. After closing it, one click returns to Indigo without
a confirmation menu; the console lab drops, then Indigo answers in about
eight seconds. The installed browser was read back byte-for-byte against
that CI artifact. Source implementation remains the tested8934fc4 tree;
this entry only records its verification.

## 2026-10-23 — Main-screen return to Indigo

Add a labelled Return to Indigo button to the browsing screen's bottom bar.
It is visible across pages and uses the existing loader return with one
press. Modal dialogs keep ownership of input. The network state and clock
retain their places; status text uses the remaining width. Document the
control and the distinction between installed and RAM-launched builds.
Build and console validation are pending on this feature branch.
