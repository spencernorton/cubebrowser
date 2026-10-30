# Indigo launch context, version 1

The browser stays a separate DOL. Indigo supplies argv[0] through boot_dol,
then these optional `key=value` arguments. `indigo.context=1` enables the
contract; missing or unknown versions leave standalone preferences intact.
Malformed individual values are ignored. No raw launcher enums or pointers
cross the program boundary.

| Argument | Accepted value |
|---|---|
| `indigo.color` | `indigo`, `azure`, `emerald`, `gold`, `spice`, `crimson`, `rose`, `jet-black` |
| `indigo.video` | `auto`, `480i`, `480p` (progressive still requires a component-capable cable) |
| `indigo.wide` | `0` or `1`, anamorphic 4:3 or 16:9 |
| `indigo.margin` | Optional screen margin, 0–10; Indigo currently leaves the browser preference in effect |
| `indigo.motion` | `0` full, `1` reduced, `2` off; reduced and off disable browser decoration/transitions |
| `indigo.sounds`, `indigo.rumble` | `0` off or `1` on |
| `indigo.timezone` | Current local offset from UTC, including DST, in minutes, −720 to +840 |
| `indigo.device` | Config-card origin: `sda`, `sdb`, `sdc` (SD2SP2), `gcldr` |

Parse arguments before mounting storage. A supported, mountable named card
wins; otherwise retain the certificate/folder-scored fallback. The data folder
remains `sd:/gcbrowser`, matching the existing package. `sd=0` with `lab=1`
still disables all card mounting. The lab remains opt-in, independently of the
launch context, and the existing loader stub still owns return to Indigo.

Load browser settings, capture the standalone values, then apply inherited
preferences. Serialization restores only inherited fields from that captured
copy; browser home, bookmarks and browsing options retain their own saved
values. An explicit reset captures fresh defaults before reapplying context.
A saved `web.home`, including `about:home`, takes precedence over the new default
`https://norvitech.com/indigo/start/`.

The native chrome font uses the SDK's IPL font reader at runtime, with cached
ASCII glyph metrics and a baked-font fallback. Page fonts and author styles
are independent. No console ROM font bytes are distributed.

Indigo's RTC contains local wall time. A known inherited offset normalizes that
fallback clock to UTC for TLS and preference expiry; a network time response
already supplies UTC. With no known offset, the integrated display keeps the
console's local clock rather than inventing a UTC zone. Time controls stay in
Indigo. Standalone time preferences remain intact.
