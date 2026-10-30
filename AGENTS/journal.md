## 2026-10-30 — Show native browser controls and clarify build downloads

The README explains the product with a real interaction recording or a source-grounded
architecture diagram. Recordings retain their source attribution and offer still
images for reduced-motion viewing. No runtime behaviour is changed.

## 2026-10-29 — Make packaged object order independent of locale

The exact Indigo CI package exposed a second reproducibility input: glob()
sorts source filenames using the host locale. The plain SDK uses en_US.UTF-8;
Indigo's runner uses C.UTF-8. Sort source basenames with make before linking.
The required comparison now changes locale, checkout path and parallelism to
match Indigo's package build. Requalify the resulting DOL through all normal
runtime routes and the actual Indigo package handoff; earlier binary hashes
remain historical evidence, not qualification of this new artifact.

## 2026-10-29 — Keep launcher clock and packaged bytes consistent

Use the TLS/network UTC clock for preference expiry. Indigo's RTC is local;
normalize a known inherited offset, and keep an unknown integrated display on
the console clock. Time controls remain inherited without overwriting saved
standalone values. Clock context host checks and the target build pass.

Normalize source filename prefixes so a browser built under Indigo's package
checkout has the exact DOL bytes tested by the browser producer. Two fresh
local builds at /src and /work/browser-src match byte-for-byte. Required CI
repeats this comparison before its runtime routes. No dependency, CA, loader,
lab opt-in or physical qualification rule changes.

## 2026-10-29 — Remember the start-page weather ZIP

Support only the start page's host-only five-digit indigo_zip preference on
verified HTTPS norvitech.com/indigo/start/ responses. Require Secure, HttpOnly,
SameSite=Lax, the exact path and a bounded Max-Age. Persist a small versioned
record on the selected card; retain visit-only state with an honest toast if
saving fails. Clearing expires and empties the record. Send it on safe GET
navigations and first-party start-page resources, never other pages' resources,
paths/origins, unverified TLS, explicit downloads or POSTs. Fetch start-page images
directly so the image helper does not receive the ZIP.

Nine sanitizer host suites pass. The target builds, and the local pinned
Dolphin cached interpreter proves three real DOL boots: save, first-request
persistence after restart, controller clear and another restart with no value.
Wire headers and actual FAT bytes are checked, including outside-path rejection
and first-party image support. Add this route to required CI. Its expiring CA
is copied only into its private card image, never the packaged CA bundle.
The live site form and physical-card qualification are separate integration
checks; do not substitute unit mocks for those results.

## 2026-10-29 — Native Indigo chrome and launch preferences

Replace glossy widgets and drifting cubes with dark cut-corner panels,
restrained focus rows, an IPL chrome font and a compact menu. Pages retain
independent fonts and author colours. The top-left Indigo control keeps the
existing return stub; X and Y keep their address and bookmark shortcuts.
Settings opens on browsing and inherited controls are hidden. The new default
home is the lightweight public Indigo start page; a saved home stays set.

Version 1 named launch arguments carry colour, screen shape, video, motion,
sound, rumble, active UTC offset and supported config-card origin. Capture
standalone settings before applying context and restore inherited fields only
when serializing, including after an explicit reset. Keep sd=0 and opt-in lab
semantics. Any of four neutral-armed pads can own the complete input sample;
stop rumble on handover/unplug. Steady time drives pointer, scroll and repeat.
Cache rounded geometry once and wake the network worker with a semaphore
instead of polling. Publish job state atomically and set save policy before
waking the worker.

Eight sanitizer host suites pass, including actual input timing and launch
persistence. The pinned SDK builds. The extended local Dolphin route passes
PAL widescreen from controller port 4, including Settings and address entry;
NTSC and exact-head CI qualification are completed before integration. Real
adapter/cable and return qualification remain pending while the console is
unreachable. Do not infer a hardware pass or a stable release from emulator
coverage.

## 2026-10-29 — Bound the visible page working set

Index layout spans for drawing, hit targets and nearby image scheduling while
preserving document painter order. Share repeated image textures, retain
intrinsic dimensions after eviction and reclaim distant textures within the
6 MiB working set. Batch only disjoint text spans on the same line. Move fetch
publication, relayout and scheduling out of rendering into one active update.
Add bounded, checked gzip responses, a short TTL DNS cache, known search
redirects, bounded meta refresh and controller-selectable dropdowns. Local
image URLs bypass the helper; failed helper requests try the origin once.

The sanitizer host suites and all seven saved parser inputs pass. The pinned
SDK builds the DOL. The local Dolphin cached interpreter boots that exact DOL,
fetches and renders two gzip HTML pages and their PNGs, opens/closes menus,
follows a link and restores Back. The default ARM JIT also exits on the
unchanged baseline; the optional core argument makes this local qualification
explicit without changing CI's default. Physical adapters and Indigo return
still need console qualification of the integrated candidate.

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
