# CubeBrowser

A web browser for the Nintendo GameCube. It loads real websites over a wired
network connection, including secure (HTTPS) sites, with pictures, page
styles, tables and forms, all controlled with a standard GameCube controller.
It runs as a `.dol` program launched from Swiss.

<img width="4096" height="2382" alt="IMG_20261014_224514833_HDR" src="https://github.com/user-attachments/assets/2ef54948-d5dc-43be-a0e3-c18770a2b286" />

---

## What you need

| Item | Notes |
|---|---|
| A GameCube that runs Swiss | Swiss loads the browser like any other homebrew program. |
| A network adapter | The official Nintendo Broadband Adapter, or a WIZnet-based adapter such as the ETH2GC. Plug it into your router with an Ethernet cable. |
| An SD card | In a GC Loader, an SD2SP2 or an SD Gecko. Holds the browser, its certificates, settings and bookmarks. The browser can't read a FlippyDrive's card: with only a FlippyDrive, secure (https) pages fail and nothing is saved. |
| A GameCube controller | Any of the four ports. |

Component cables are optional; they unlock 480p video.

---

## Installing

### 1. Get the files

Download the latest .zip build

- `cubebrowser.dol`, the browser itself
- a `gcbrowser` folder with `cacert.pem`, the list of trusted certificates
  used to check secure websites

(To build it yourself instead, see [Building from source](#building-from-source)
at the end.)

### 2. Copy them to your SD card

Put the `gcbrowser` folder in the **root** of the SD card, and the `.dol`
anywhere you like. The browser checks the SD2SP2, an SD Gecko in slot B or A
and the GC Loader's card, in that order, and uses the first card whose
`gcbrowser` folder holds `cacert.pem` (failing that, the first card with a
`gcbrowser` folder, then the first card it finds):

```
SD card
├── cubebrowser.dol
└── gcbrowser/
    └── cacert.pem
```

The browser adds its own files to `gcbrowser` as you use it:

| File | Contents |
|---|---|
| `settings.ini` | Your settings (plain text; editable on a PC) |
| `bookmarks.txt` | Your bookmarks |
| `seed.bin` | Saved randomness for secure connections |

### 3. Launch it

Insert the SD card and network adapter, boot Swiss, browse to
`cubebrowser.dol`, and launch it.

On the first start, the status bar at the bottom shows the browser getting an
address from your router, setting its clock, and loading certificates. This
takes a few seconds. When the dot in the bottom-right corner turns green,
you're online and the start page is ready.

---

## Using the browser

### Controls

| Button | Action |
|---|---|
| Control Stick | Move the pointer |
| Z (hold) | Slow, precise pointer movement |
| A | Click |
| B | Back |
| X | Type an address or search |
| Y | Bookmarks |
| START | Browser menu |
| C-Stick | Scroll the page |
| L / R | Page up / page down |
| D-Pad | Jump the pointer between links and buttons |

Any controller can take over once the current controller is neutral. A held or drifting secondary controller must return to neutral first. Every button except the D-Pad can be changed in Settings > Controller.

### Going to a website or searching

Press **X** (or click the address bar). Type a web address such as
`text.npr.org`, or type any words to search. Then press **START** or click
**OK**. Addresses without `https://` are opened securely automatically.

### The on-screen keyboard

| Button | Action |
|---|---|
| A | Press the key under the pointer |
| D-Pad | Jump from key to key |
| B | Delete |
| Y | Space |
| X | Shift (click the on-screen Shift key twice for caps lock) |
| L / R | Move the text cursor |
| START | Accept |

When typing an address, a row of shortcut keys (`https://`, `www.`, `.com`,
`.org`, `.net`, `/`) saves typing.

### The toolbar

From left to right: **< Indigo**, the **address bar**, **Bookmarks** (star)
and the **browser menu** (gear). Home, Forward, Reload/Stop and Browser settings
are in that menu, also opened with START. A green lock in the address bar means the site's
security certificate was checked successfully.

### Returning to Indigo

Click **< Indigo** at the top-left of the browsing screen. It is
visible on every page and returns with one press, without opening Settings.
The button is unavailable while a download, bookmarks or keyboard dialog is
open; finish or close that dialog first.

Returning restarts the Indigo installed on your card. A test build launched
from RAM does not replace that installed version.

### Bookmarks

Press **Y** to open your bookmarks. Click **Bookmark this page** to add the
current page, click a bookmark to open it, or click the **x** beside one twice
to delete it.

### Forms

Click a text box to type into it. Search boxes submit automatically when you
press OK. Checkboxes and radio buttons toggle when clicked, and submit buttons
send the form.

### Downloading files

Clicking a link to a file (such as a `.dol`, `.iso`, `.gcm` or `.zip`) asks
whether to save it to the SD card instead of opening it. Choose **Download**,
or **Download & unzip** for zip files.

The download screen shows progress, speed and time remaining. Files are saved
to `sd:/downloads` (changeable in Settings > Data), where Swiss can launch them
after you leave the browser.

- **Interrupted downloads continue where they stopped.** If the connection
  drops, choose **Retry**, or simply open the same link again later.
- **Zip files** can be unzipped automatically, or later from the "Download
  complete" screen. Files are extracted to a folder named after the zip.
- **Cancel** (press twice) stops a download and deletes the partial file.
- **Big files take time.** Speed depends on the server, but expect roughly
  0.5 to 2 MB per second. A 1.4 GB disc image takes about 15 to 45 minutes.
  Plain `http://` downloads, and files served from a computer on your own
  network, are the fastest.

Only download software you have the right to use, such as homebrew or backups
of your own games.

### Status bar

The bottom bar shows loading progress, the address of the link under the
pointer, the network status dot (green: connected, yellow: connecting, red:
no network) and the time.

---

## Features

- **Secure browsing (HTTPS)** with full certificate checking, using TLS 1.2
  and 1.3.
- **Pictures** (JPEG, PNG, GIF and BMP), loaded after the text so pages
  appear quickly.
- **Page styles:** the colours, fonts, spacing, borders and backgrounds each
  site chooses, including hiding elements sites mark as hidden (menus, cookie
  banners). The browser presents itself as a phone-sized screen, so many sites
  send their simpler mobile layouts.
- **Tables** with real side-by-side columns, so classic table-based sites look
  the way they were designed.
- **Floating pictures and boxes** with text wrapping around them.
- **Fonts:** regular, bold, italic and monospace.
- **Lists** with bullets, numbers and letters.
- **Forms:** text boxes, password fields, checkboxes, radio buttons and submit
  buttons, sent by GET or POST.
- **Framed pages** (a 1990s technique) show a link to each part of the page.
- **File downloads** straight to the SD card, with resume, progress and
  speed display, and automatic unzipping.
- **History** with Back and Forward, and **bookmarks** saved to the SD card.
- **Native Indigo chrome** with an IPL font read from the console ROM and
  the launcher's named menu colour. Standalone page-theme IDs remain compatible.
- **4:3 and 16:9** display, 480i or 480p, and adjustable screen margins for
  TVs that cut off the edges.
- **Fully remappable controls**, pointer speed and acceleration, D-Pad
  snapping, and rumble feedback.
- **Synthesized sound effects** and smooth, animated menus.
- A lightweight [Indigo Web start page](https://norvitech.com/indigo/start/)
  with search and controls help. Saved custom home pages stay set; `about:home`
  opens an offline page with shortcuts.

---

## Settings

Press **START**, then choose **Browser settings**. Browsing options open first.
When Indigo launches the browser, inherited display, motion, sound, rumble and
time preferences stay in Indigo; the corresponding browser controls are hidden
and the standalone values are preserved when saving. Changes apply immediately and are saved when
you leave the Settings screen. Hold **L + R + START** while the browser starts
to reset everything to defaults.

| Section | What you can change |
|---|---|
| **Network** | Automatic (DHCP) or manual IP address, subnet mask, gateway, DNS server, time server. Network changes apply the next time you launch the browser. |
| **Time** | Time zone and daylight saving, for the clock in the status bar. |
| **Controller** | Stick dead zone, pointer speed, acceleration, scroll speed, pointer snap, rumble, and the button for each action. To remap, click an action and press the button you want. |
| **Keyboard** | QWERTY or ABC key layout, and key repeat speed. |
| **Display** | Aspect ratio (4:3 or 16:9), video mode (Auto, 480i, 480p), screen margin, page text size, theme, animations, sound effects and volume. |
| **Browsing** | Home page, search engine (DuckDuckGo Lite, FrogFind, Wiby or your own), how the browser identifies itself to sites, page styles on/off, pictures on/off, image helper, picture labels, and the largest page size to load. |
| **Security** | Certificate checking and the certificate file location. |
| **Data** | Download folder, delete zips after unzipping, clear history, delete all bookmarks, reset all settings, save settings now. |

### Notes on a few settings

- **16:9** is for TVs set to stretch the picture to widescreen. Pages are laid
  out wider to use the extra space.
- **Page styles: Off** shows every page in a plain, readable format. Useful if
  a site's design doesn't suit the TV.
- **Identify as: Lynx (text mode)** makes some sites send very simple pages,
  which load faster.
- **Image helper** (on by default) routes pictures through the free
  images.weserv.nl service, which shrinks them for your screen and converts
  formats the GameCube can't read (such as WebP). It makes pictures much faster
  and work on more sites, but that service sees the address of each picture
  you view. Local network addresses are fetched directly, and a failed helper
  request gets one attempt at the original address. Turn it off to fetch all
  pictures directly.
- **Verify certificates** should stay on. Turning it off removes the
  protection HTTPS provides; it's only meant for troubleshooting.

---

## Sites that work well

The GameCube has a 485 MHz processor and 24 MB of memory, so lightweight sites
work best. Good places to start (all linked from the start page):

- **DuckDuckGo Lite** (search): `lite.duckduckgo.com/lite`
- **Wikipedia, mobile version**: `en.m.wikipedia.org`
- **NPR Text** (news): `text.npr.org`
- **CNN Lite** (news): `lite.cnn.com`
- **68k.news** (headlines): `68k.news`
- **FrogFind** (search that simplifies pages): `frogfind.com`
- **Wiby** (search for classic-style sites): `wiby.me`

Older and hobbyist sites from the 1990s and 2000s usually display very well.

---

## Troubleshooting

| Problem | What to try |
|---|---|
| **"No network (error -1)"** | Check the Ethernet cable and that the adapter is seated firmly. If you use manual network settings, check them under Settings > Network. Retry by opening any page. |
| **A secure site says its certificate couldn't be verified** | Usually the clock. Make sure the time server under Settings > Network is reachable (the default `pool.ntp.org` works for most people). If it persists, download a fresh certificate list from `https://curl.se/ca/cacert.pem` and replace `gcbrowser/cacert.pem` on the SD card. |
| **A page is blank or nearly empty** | The site probably builds its content with JavaScript, which isn't supported. Try the site's mobile or "lite" version, or open it through FrogFind. |
| **A page looks cluttered or oddly arranged** | Modern multi-column layouts are shown as a single column. Try Settings > Browsing > Page styles: Off for a clean, plain view. |
| **Pictures don't appear** | Check Settings > Browsing > Load images. With the image helper off, some formats (like WebP) can't be shown. Pictures load near the visible page; distant textures are released to make room as you scroll. |
| **A page is cut off at the end** | It was larger than the page size limit. Raise Settings > Browsing > Max page size. |
| **The screen edges are cut off on my TV** | Increase Settings > Display > Screen margin. |
| **The picture looks squashed or stretched** | Match Settings > Display > Aspect ratio to your TV's setting. |
| **A download stopped partway** | Choose Retry, or open the same link again; it continues from where it stopped (on servers that allow it). |
| **"Not enough space on the SD card"** | Free up space or use a larger card. FAT32 cards can't hold single files over 4 GB. |
| **Settings or bookmarks aren't saved** | The SD card wasn't found. Make sure it's in the GC Loader, SD2SP2 or SD Gecko and formatted FAT32. |
| **The pointer drifts on its own** | Raise Settings > Controller > Stick dead zone. |
| **Something's wrong and I can't get back** | Hold L + R + START while launching to reset all settings. |

---

## Limitations

- **No JavaScript.** Sites that depend on it may be blank or only partly work.
- **No modern multi-column layouts** (CSS flexbox and grid). These sections
  are stacked into a single column.
- **No video, audio or animation.** Animated GIFs show their first frame.
- **Drop-down menus** support up to 64 enabled choices per menu.
- **Frames** open one part at a time rather than side by side.
- **Unzipping** supports standard `.zip` files; 7z, RAR and RVZ aren't supported.
- **Speed:** heavy pages can take several seconds to lay out, and each new
  secure site needs a second or two to set up its connection.

## Security and privacy

- Secure sites are checked against the trusted certificate list on your SD
  card, using mbedTLS.
- The GameCube has no hardware random number generator. The browser gathers
  randomness from a seed saved between sessions, processor timing, and
  controller and network activity. That's fine for everyday browsing, but don't
  use it for banking or other high-value accounts.
- With the image helper on, images.weserv.nl sees the addresses of pictures
  you view.
- The browser keeps no cookies, so you won't stay logged in to sites.

---

## Console lab

For testing on a real console with nobody there. Started with the argument
`lab=1`, which Indigo passes when its own console lab (`LabServer=Yes`) is
on, the browser answers on TCP port 4300 the same requests as Indigo's lab,
so a test run can follow Indigo into the Web face:

| Request | What it does |
|---|---|
| `GET /xfb` | The picture on screen, raw YUYV; `X-Size` gives its size |
| `GET /log` | Pages opened, messages shown and the network's state |
| `GET /pad` | How long since a real controller's button went down |
| `POST /pad?press=A,Right&hold=150` | Presses buttons on a virtual controller |
| `POST /open?url=<address>` | Opens an address, as if typed in the address bar |
| `POST /point?x=<px>&y=<px>` | Moves the pointer there |
| `POST /reset` | Restarts the console, back to what it starts (Indigo) |

A POST needs an `X-Lab` header, which no web page can send. Without `lab=1`
the port stays closed. While the lab is on, anyone on your network can see
the screen and the addresses you open, so keep it for testing. A crash
screen then restarts the console after 10 seconds, and so do 15 minutes
with no request and no button pressed (`deadline=<seconds>`, `0` = never),
so a failed test never needs the Reset button. `sd=0` beside `lab=1` keeps
the browser off its SD card, for a run that must not write it.

---

## Building from source

The easiest way is GitHub Actions, which builds on GitHub's servers with
nothing to install:

1. Create a free GitHub account and a new **public** repository.
2. Upload everything inside this project's folder, including the hidden
   `.github` folder. If your computer hides it, create
   `.github/workflows/cubebrowser-libogc2.yml` on GitHub with **Add file >
   Create new file** and paste in the file's contents.
3. Open the **Actions** tab. Each upload starts a build; when it shows a green
   check, download the **cubebrowser** artifact.

If a build fails, click the run, then the failed step, to see the error.

**With Docker** (any operating system):

```sh
./get-mbedtls.sh
docker run --rm -v "$PWD:/src" -w /src ghcr.io/extremscorner/libogc2@sha256:f9ffb7e33c4ba4010da8e40c01366f7c24732130146abd99690403bd42d3e4c2 make
```

The build uses [libogc2](https://libogc2.org/) (for official and WIZnet
network adapters), [mbedTLS 3.6](https://github.com/Mbed-TLS/mbedtls) and
[stb_image](https://github.com/nothings/stb). `get-mbedtls.sh` downloads the
last two into `third_party/`.

The named, versioned DOL launch contract is documented in
[docs/launch-context.md](docs/launch-context.md).

### Project layout

The required build runs the saved HTML/CSS parser regressions with address
and undefined-behavior sanitizers, then boots and uses the actual DOL in the
emulator. To run the parser cases locally with Clang:

```sh
sh tools/fuzz/run.sh --regressions
```

Run the engine checks with `sh tools/tests/run.sh`. They exercise the viewport
index against an exhaustive oracle, image ownership and reclamation, text
painter order, dropdown submission, redirects, bounded gzip decoding and DNS
expiry under AddressSanitizer and UndefinedBehaviorSanitizer. The index retains
document painter order and falls back to scanning if its allocation fails.
Image textures share identical URLs and use a 6 MiB budget; this is an in-memory
working set, not a persistent browsing cache. DNS entries expire at the response
TTL, capped at five minutes. Automatic meta refresh is limited to eight hops.

Pass a number of seconds instead to run both parser fuzz targets after the
saved cases. Generated inputs stay in a temporary directory.

| Path | Contents |
|---|---|
| `source/main.c` | Start-up, main loop, screen transitions |
| `source/browser.c` | Browser screen, history, bookmarks, download scheduling |
| `source/page.c` | Layout engine (blocks, text, floats, tables), drawing, forms |
| `source/dom.c`, `source/css.c` | HTML parsing and stylesheets |
| `source/http.c`, `source/conn.c` | HTTP and secure connections |
| `source/netio.c`, `source/fetch.c` | Network setup, DNS, clock, background downloads |
| `source/image.c` | Picture decoding to GameCube textures |
| `source/download.c`, `source/unzip.c`, `source/inflate.c` | File downloads, zip extraction |
| `source/gfx.c`, `source/ui.c`, `source/osk.c` | Graphics, menus, on-screen keyboard |
| `source/settings*.c`, `source/input.c`, `source/sound.c` | Settings, controller, sound |
| `source/lab.c`, `source/ui_lab.c` | The console lab, off unless started with `lab=1` |
| `tools/make_font.py` | Regenerates the built-in fonts |
| `sd/gcbrowser/` | Files for the SD card |

The bundled fonts are DejaVu Sans (free license). For a rounder look, run
`python3 tools/make_font.py` with your own `.ttf` files (bold, regular,
italic, monospace).

---

CubeBrowser is by [Arial-Bold](https://github.com/Arial-Bold/Cubebrowser). This
is a fork maintained for integration into Indigo, used with permission; see
[NOTICE.md](NOTICE.md).
