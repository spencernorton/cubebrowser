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
