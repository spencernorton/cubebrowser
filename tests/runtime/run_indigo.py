#!/usr/bin/env python3
"""Launch the candidate from Indigo and return through its unmodified SD stub."""
from __future__ import annotations

import argparse
import hashlib
import http.server
import importlib.util
import io
import json
from pathlib import Path
import re
import subprocess as sp
import sys
import tempfile
import threading
import time

import numpy as np
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", type=Path, required=True)
    parser.add_argument("--card-files", type=Path, required=True)
    parser.add_argument("--indigo-src", type=Path, required=True)
    parser.add_argument("--indigo-dol", type=Path, required=True)
    parser.add_argument("--indigo-provenance", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--cpu-core", type=int, choices=(0, 1, 3, 5))
    parser.add_argument("--no-motion", action="store_true")
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / "result.json").unlink(missing_ok=True)
    provenance = json.loads(args.indigo_provenance.read_text())
    indigo_hash = hashlib.sha256(args.indigo_dol.read_bytes()).hexdigest()
    if provenance["schema"] != "indigo.dol-provenance.v1" or provenance["dol"]["sha256"] != indigo_hash:
        raise RuntimeError("Indigo fixture does not match its source/build provenance")

    # Use the pinned launcher's controller and screen observations. Both DOLs
    # stay unmodified; no lab endpoint, memory edit or synthetic jump is used.
    harness_dir = args.indigo_src.resolve() / "buildtools/ui/emulator"
    sys.path.insert(0, str(harness_dir))
    spec = importlib.util.spec_from_file_location("indigo_harness", harness_dir / "run.py")
    harness = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = harness
    spec.loader.exec_module(harness)
    config = "[Core]\nEnableCheats = True\nSerialPort1 = 12\nBBA_BUILTIN_IP = 192.0.2.3\nBBA_BUILTIN_DNS = 192.0.2.3\n"
    if args.cpu_core is not None:
        config += f"CPUCore = {args.cpu_core}\n"
    # Dolphin's default HBReload hook stops the emulator at 0x80001800.
    # EnableCheats skips that hook. This fresh profile contains no cheat codes;
    # Indigo's real STUBHAXX code must reload IPL.DOL from the emulated card.
    harness.DOLPHIN_INI = harness.DOLPHIN_INI.replace("[Core]\n", config, 1)

    requests = []
    image = io.BytesIO()
    Image.new("RGB", (240, 120), (229, 37, 181)).save(image, format="PNG")
    png = image.getvalue()

    class Fixture(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            requests.append(self.path)
            if self.path == "/marker.png":
                data, kind = png, "image/png"
            else:
                data = (b"<title>Indigo launch context</title><h1>Indigo launch context</h1>"
                        b'<img src="/marker.png" width="240" height="120">' +
                        b"<p>The actual launcher supplied the browser context.</p>" * 12)
                kind = "text/html"
            self.send_response(200)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def log_message(self, *_):
            pass

    server = http.server.ThreadingHTTPServer(("0.0.0.0", 18080), Fixture)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    em = None
    try:
        with tempfile.TemporaryDirectory(prefix="indigo-browser-") as directory:
            work = Path(directory)
            card = work / "card.img"
            with card.open("wb") as stream:
                stream.truncate(512 * 1024 * 1024)
            sp.run(["mkfs.fat", "-F", "32", "-s", "8", str(card)], check=True, stdout=sp.DEVNULL)
            root = work / "root"
            (root / "swiss/settings/game").mkdir(parents=True)
            (root / "swiss/indigo").mkdir()
            (root / "gcbrowser").mkdir()
            (root / "games").mkdir()
            (root / "swiss/settings/global.ini").write_text(
                "Menu Color=Spice\nMenu Widescreen=Yes\nCube=Classic\nUp Face=None\n"
                "Left Face=None\nRight Face=None\nDown Face=Web\n"
                f"Disable UI Animations={'Yes' if args.no_motion else 'No'}\n"
                "Disable Animated Backdrop=Yes\nDisable Panel Transparency=Yes\n"
                "Disable Menu Music=Yes\nDisable Menu SFX=Yes\nDisable PAD Rumble=Yes\n"
                "Library Path=sdc:/games\nInitNetwork=No\nNTPSync=No\nTimeZone=UTC0\n")
            (root / "gcbrowser/settings.ini").write_text(
                "web.home=http://192.0.2.3:18080/first\nweb.imgproxy=0\nnet.ntp=\n"
                "disp.aspect=0\ndisp.anim=1\ndisp.theme=0\n")
            (root / "swiss/indigo/cubebrowser.dol").write_bytes(args.dol.read_bytes())
            (root / "gcbrowser/cacert.pem").write_bytes((args.card_files / "cacert.pem").read_bytes())
            (root / "ipl.dol").write_bytes(args.indigo_dol.read_bytes())
            for item in root.iterdir():
                sp.run(["mcopy", "-i", str(card), "-s", str(item), "::/"], check=True)

            em = harness.Emulator(args.indigo_dol.resolve(), None, work, out, region="ntsc",
                                  storage="sd2sp2", card=card, cable="component", empty_slots=True,
                                  exact_textures=True)
            route = harness.Route(em, out, fresh_card=False, cable="component", region="ntsc",
                                  storage="sd2sp2", menu_wide=True, sd_image=card)
            home = route.boot()
            rounds = []

            def capture(name):
                rgb = em.frame()
                Image.fromarray(rgb).save(out / (name + ".png"))
                return rgb

            for number, action in enumerate(("menu", "toolbar"), 1):
                old_requests = requests.count("/marker.png")
                route.press("DOWN")
                route.pause(0.5)
                web_rgb = capture(f"{number}-web-face")
                web = harness.text_mask(harness.detection_frame(web_rgb, True).max(axis=2))
                route.press("A")
                until = time.monotonic() + 180
                while time.monotonic() < until:
                    time.sleep(2)
                    rgb = capture(f"{number}-browser")
                    marker = (rgb[:, :, 0] > 180) & (rgb[:, :, 1] < 70) & (rgb[:, :, 2] > 140)
                    if requests.count("/marker.png") > old_requests and int(marker.sum()) > 10000:
                        break
                else:
                    raise RuntimeError(f"Indigo did not launch/render the browser: {em.where()}")
                xs = np.where(marker)[1]
                width = int(xs.max() - xs.min() + 1)
                if not 170 <= width <= 185:
                    raise RuntimeError(f"Indigo widescreen did not override saved 4:3: {width}")

                if action == "menu":
                    route.press("START")
                    route.pause(0.5)
                    rgb = capture(f"{number}-menu")
                    orange = ((rgb[:, :, 0] > 180) & (rgb[:, :, 1] > 95) &
                              (rgb[:, :, 1] < 200) & (rgb[:, :, 2] < 145))
                    if int(orange.sum()) < 20:
                        raise RuntimeError("Indigo named Spice colour was not inherited")
                    for _ in range(5):
                        route.press("DOWN")
                else:
                    route.press("UP")
                    route.press("LEFT")
                    rgb = capture(f"{number}-toolbar-focus")
                    region = rgb[50:88, 18:115]
                    focused = ((region[:, :, 0] > 180) & (region[:, :, 1] > 95) &
                               (region[:, :, 1] < 200) & (region[:, :, 2] < 145))
                    if int(focused.sum()) < 20:
                        raise RuntimeError("Controller did not focus the Indigo toolbar return")
                capture(f"{number}-return-focus")
                # Tap for several guest frames; release before the fast emulated
                # card reload reaches Home, where A would otherwise open Library.
                route.press("A", 0.08)
                returned, _ = route.settled_label(90, like=home)
                if returned is None:
                    raise RuntimeError(f"Return did not reload Indigo Home: {em.where()}")
                route.press("DOWN")
                moved, _ = route.settled_label(10, like=web)
                if moved is None:
                    raise RuntimeError("Returned Indigo Home did not respond to Web navigation")
                route.press("UP")
                returned, _ = route.settled_label(10, like=home)
                capture(f"{number}-returned-indigo")
                if returned is None:
                    raise RuntimeError("Returned Indigo did not navigate back to Library Home")
                rounds.append({"action": action, "marker_pixel_width": width, "returned_to_home": True,
                               "home_controls_responded": True})

            saved = sp.check_output(["mtype", "-i", str(card), "::/gcbrowser/settings.ini"]).decode().splitlines()
            if not {"disp.aspect=0", "disp.theme=0", "disp.anim=1"}.issubset(saved):
                raise RuntimeError("Inherited appearance overwrote standalone browser preferences")
            log = (out / "dolphin.log").read_text(errors="replace")
            if re.search(r"Invalid (?:read|write)|DSI exception|ISI exception|Unhandled exception", log, re.I):
                raise RuntimeError("Dolphin reported a guest CPU exception")
            (out / "result.json").write_text(json.dumps({
                "passed": True, "browser_sha256": hashlib.sha256(args.dol.read_bytes()).hexdigest(),
                "indigo_sha256": indigo_hash, "indigo_revision": provenance["revision"],
                "round_trips": rounds, "requests": requests, "named_spice_inherited": True,
                "standalone_appearance_preserved": True,
                "animations_disabled": args.no_motion, "complete_texture_hashing": True,
                "return_path": "Unmodified Indigo STUBHAXX reads the real FAT IPL.DOL; no HBReload HLE interception",
                "physical_hardware": "Not qualified by this emulator route",
            }, indent=2) + "\n")
            print("PASS: Indigo launches the candidate, inherits appearance and reloads through both return controls", flush=True)
    finally:
        if em:
            em.close()
        server.shutdown()
        server.server_close()
        (out / "requests.json").write_text(json.dumps(requests, indent=2) + "\n")


if __name__ == "__main__":
    main()
