#!/usr/bin/env python3
"""Boot the shipped DOL, fetch a local page through Dolphin's BBA, drive a pad."""
from __future__ import annotations
import argparse
import gzip
import hashlib
import http.server
import io
import json
import os
from pathlib import Path
import re
import subprocess as sp
import tempfile
import threading
import time

import numpy as np
from PIL import Image
import dsu_pad


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--dol", type=Path, required=True)
    p.add_argument("--card-files", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--cpu-core", type=int, choices=(0, 1, 3, 5), default=None,
                   help="Explicit Dolphin core override, e.g. 5 for the cached interpreter on ARM hosts")
    args = p.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / "result.json").unlink(missing_ok=True)
    requests = []
    pictures = {}
    encodings = []
    for name, color in [("/marker.png", (229, 37, 181)), ("/marker2.png", (21, 188, 113))]:
        buffer = io.BytesIO()
        Image.new("RGB", (240, 120), color).save(buffer, format="PNG")
        pictures[name] = buffer.getvalue()
    class Fixture(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            requests.append(self.path)
            if self.path in pictures:
                data, kind = pictures[self.path], "image/png"
            elif self.path in ("/first", "/second"):
                marker = "marker2.png" if self.path == "/second" else "marker.png"
                data = ("<!doctype html><html><head><title>Runtime fixture</title></head>"
                        "<body><h1>CubeBrowser runtime acceptance</h1>"
                        f'<a href="/second"><img src="/{marker}" width="240" height="120"></a>'
                        "<p>A native page, fetched and decoded by the candidate.</p></body></html>").encode()
                kind = "text/html"
            else:
                self.send_error(404)
                return
            encoding = "gzip" if self.path in ("/first", "/second") else None
            if encoding:
                if "gzip" not in self.headers.get("Accept-Encoding", ""):
                    self.send_error(400, "Candidate must advertise gzip support")
                    return
                data = gzip.compress(data, mtime=0)
                encodings.append(self.path)
            self.send_response(200)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(data)))
            if encoding:
                self.send_header("Content-Encoding", encoding)
            self.end_headers()
            self.wfile.write(data)
        def log_message(self, *_):
            pass
    server = http.server.ThreadingHTTPServer(("0.0.0.0", 18080), Fixture)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    processes = []
    pad = dsu_pad.Pad(0)
    try:
        with tempfile.TemporaryDirectory(prefix="cube-runtime-") as directory:
            work = Path(directory)
            card = work / "card.img"
            with card.open("wb") as f:
                f.truncate(512 * 1024 * 1024)
            sp.run(["mkfs.fat", "-F", "32", "-s", "8", str(card)], check=True, stdout=sp.DEVNULL)
            sp.run(["mcopy", "-i", str(card), "-s", str(args.card_files.resolve()), "::/"], check=True)
            settings = work / "settings.ini"
            settings.write_text("web.home=http://192.0.2.3:18080/first\nweb.imgproxy=0\nnet.ntp=\ndisp.anim=0\n")
            sp.run(["mcopy", "-i", str(card), str(settings), "::/gcbrowser/settings.ini"], check=True)
            user = work / "user"
            config = user / "Config"
            config.mkdir(parents=True)
            (config / "Dolphin.ini").write_text(f"""[Core]
{f"CPUCore = {args.cpu_core}" if args.cpu_core is not None else ""}
DSPHLE = False
DSPThread = False
MMU = True
GFXBackend = OGL
SIDevice0 = 6
SIDevice1 = 0
SIDevice2 = 0
SIDevice3 = 0
SlotA = 255
SlotB = 255
SerialPort2 = 15
SP2SDCardImage = {card}
SerialPort1 = 12
BBA_BUILTIN_IP = 192.0.2.3
BBA_BUILTIN_DNS = 192.0.2.3
[Analytics]
Enabled = False
PermissionAsked = True
[Interface]
ConfirmStop = False
OnScreenDisplayMessages = False
[Input]
BackgroundInput = True
[Display]
Fullscreen = True
RenderToMain = True
""")
            (config / "GFX.ini").write_text("[Settings]\nInternalResolution = 1\nSafeTextureCacheColorSamples = 0\n")
            (config / "Logger.ini").write_text("[Logs]\nOSREPORT = True\nPOWERPC = True\nSP1 = True\n[Options]\nVerbosity = 1\nWriteToConsole = True\n")
            (config / "GCPadNew.ini").write_text(dsu_pad.GCPAD_INI)
            (config / "DSUClient.ini").write_text(f"[Server]\nEnabled = True\nEntries = {dsu_pad.DESCRIPTION}:127.0.0.1:{pad.port};\n")
            read, write = os.pipe()
            x = sp.Popen(["Xvfb", "-displayfd", str(write), "-screen", "0", "640x480x24", "-nolisten", "tcp"],
                         pass_fds=(write,), stdout=sp.DEVNULL, stderr=open(out / "xvfb.log", "w"))
            processes.append(x)
            os.close(write)
            with os.fdopen(read) as f:
                display = ":" + f.readline().strip()
            env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE="1", DOLPHIN_TICKS="1", DOLPHIN_DABR="1")
            dol = sp.Popen(["dolphin-emu-nogui", "-u", str(user), "-p", "x11", "-v", "OGL",
                            "-C", "Dolphin.Core.FallbackRegion=1", "-C", "SYSCONF.IPL.PGS=True", "-e", str(args.dol.resolve())],
                           stdout=open(out / "dolphin.log", "w"), stderr=sp.STDOUT, env=env)
            processes.append(dol)
            def frame(name):
                if dol.poll() is not None:
                    raise RuntimeError(f"Dolphin exited before acceptance completed (exit {dol.returncode})")
                raw = sp.run(["ffmpeg", "-loglevel", "error", "-f", "x11grab", "-video_size", "640x480", "-draw_mouse", "0",
                              "-i", display + "+0,0", "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb24", "-"],
                             capture_output=True, check=True, timeout=20).stdout
                rgb = np.frombuffer(raw, np.uint8).reshape(480, 640, 3)
                Image.fromarray(rgb).save(out / (name + ".png"))
                return rgb
            pad.plug_in()
            until = time.monotonic() + 180
            while time.monotonic() < until:
                time.sleep(3)
                rgb = frame("boot")
                magenta = (rgb[:, :, 0] > 180) & (rgb[:, :, 1] < 70) & (rgb[:, :, 2] > 140)
                if "/first" in requests and "/marker.png" in requests and int(magenta.sum()) > 10000:
                    break
            else:
                raise RuntimeError(f"Candidate did not fetch and render the fixture: {requests}")
            first = rgb.copy()
            def changed(a, b):
                return np.count_nonzero(np.max(np.abs(a[80:435, 28:612].astype(int) - b[80:435, 28:612].astype(int)), axis=2) > 12)
            for button, name in [("Y", "bookmarks"), ("B", "bookmarks-closed"), ("START", "settings"), ("B", "settings-closed")]:
                pad.press(button, 0.2)
                time.sleep(2)
                rgb = frame(name)
                difference = changed(first, rgb)
                if name.endswith("closed"):
                    if difference > 2000:
                        raise RuntimeError(f"{button} did not return from {name}: {difference} pixels differ")
                elif difference < 10000:
                    raise RuntimeError(f"{button} did not open {name}: {difference} pixels differ")
            # D-pad focus is the browser's normal link navigation. A then
            # follows that link; B must recover the previous decoded page.
            pad.press("LEFT", 0.12)
            time.sleep(1)
            frame("link-focused")
            pad.press("A", 0.2)
            until = time.monotonic() + 60
            while time.monotonic() < until:
                time.sleep(2)
                rgb = frame("second")
                green = (rgb[:, :, 0] < 65) & (rgb[:, :, 1] > 140) & (rgb[:, :, 2] > 70) & (rgb[:, :, 2] < 150)
                if "/second" in requests and "/marker2.png" in requests and int(green.sum()) > 10000:
                    break
            else:
                raise RuntimeError(f"Controller did not follow and render the link: {requests}")
            pad.press("B", 0.2)
            time.sleep(3)
            rgb = frame("back")
            magenta = (rgb[:, :, 0] > 180) & (rgb[:, :, 1] < 70) & (rgb[:, :, 2] > 140)
            # D-pad focus adds the real link outline and moves the pointer.
            if int(magenta.sum()) < 10000 or changed(first, rgb) > 6000:
                raise RuntimeError("Back did not restore the first page")
            log = (out / "dolphin.log").read_text(errors="replace")
            if not {"/first", "/second"}.issubset(encodings):
                raise RuntimeError("Candidate did not request both gzip pages")
            if re.search(r"Invalid (?:read|write)|DSI exception|ISI exception|Unhandled exception", log, re.I):
                raise RuntimeError("Dolphin reported an invalid access or CPU exception")
            (out / "result.json").write_text(json.dumps({"dol_sha256": hashlib.file_digest(args.dol.open("rb"), "sha256").hexdigest(),
                                                         "requests": requests, "gzip_pages": encodings, "passed": True}, indent=2) + "\n")
            print("PASS: candidate booted, rendered HTML/PNG, opened and closed menus, followed a link and went back", flush=True)
    finally:
        for proc in reversed(processes):
            proc.terminate()
            try:
                proc.wait(5)
            except sp.TimeoutExpired:
                proc.kill()
                proc.wait()
        pad.close()
        server.shutdown()
        (out / "requests.json").write_text(json.dumps(requests) + "\n")


if __name__ == "__main__":
    main()
