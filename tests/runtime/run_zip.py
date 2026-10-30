#!/usr/bin/env python3
"""Prove a real DOL saves, reloads and clears the scoped ZIP on an emulated card."""
from __future__ import annotations
import argparse
import hashlib
import http.server
import io
import json
import os
from pathlib import Path
import re
import socketserver
import ssl
import struct
import subprocess as sp
import tempfile
import threading
import time
from urllib.parse import urlsplit
import numpy as np
from PIL import Image
import dsu_pad

ORIGIN = "https://norvitech.com"
ROOT = "/indigo/start/"
COOKIE = "indigo_zip=80202"
FLAGS = "; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age="


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", type=Path, required=True)
    parser.add_argument("--card-files", type=Path, required=True)
    parser.add_argument("--cert-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--cpu-core", type=int, choices=(0, 1, 3, 5))
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    (out / "result.json").unlink(missing_ok=True)
    requests = []
    phase = 0
    images = {}
    for name, color in [("saved", (229, 37, 181)), ("cleared", (21, 188, 113))]:
        data = io.BytesIO()
        Image.new("RGB", (240, 120), color).save(data, format="PNG")
        images[name] = data.getvalue()

    class DNS(socketserver.BaseRequestHandler):
        def handle(self):
            data, sock = self.request
            if len(data) < 17:
                return
            end = 12
            while end < len(data) and data[end]:
                end += data[end] + 1
            if end + 5 > len(data):
                return
            question = data[12:end+5]
            answer = b"\xc0\x0c" + struct.pack("!HHIH", 1, 1, 30, 4) + bytes((192, 0, 2, 3))
            sock.sendto(data[:2] + b"\x81\x80\0\1\0\1\0\0\0\0" + question + answer, self.client_address)

    class Fixture(http.server.BaseHTTPRequestHandler):
        protocol_version = "HTTP/1.1"

        def do_GET(self):
            path = urlsplit(self.path).path
            cookie = self.headers.get("Cookie")
            requests.append({"phase": phase, "path": path, "cookie": cookie})
            data, kind, status, location, value = b"", "text/html", 200, None, None
            if path == ROOT + "save":
                value, status, location = COOKIE + FLAGS + "31536000", 303, "/outside"
            elif path == "/outside":
                # A response outside the allowed path cannot replace the saved ZIP.
                value, status, location = "indigo_zip=99999" + FLAGS + "31536000", 302, ROOT
            elif path == ROOT + "clear":
                value, status, location = "indigo_zip=" + FLAGS + "0", 303, ROOT
            elif path == ROOT:
                state = "saved" if cookie == COOKIE else "cleared"
                data = (f'<title>ZIP preference fixture</title><h1>ZIP {state}</h1>'
                        f'<a href="{ROOT}clear"><img src="{ROOT}{state}.png" width="240" height="120"></a>'
                        '<p>The candidate owns the preference and card.</p><img src="/outside.png" width="8" height="8">').encode()
            elif path == "/outside.png":
                data, kind = images["cleared"], "image/png"
            elif path in (ROOT + "saved.png", ROOT + "cleared.png"):
                data, kind = images[Path(path).stem], "image/png"
            else:
                status = 404
            self.send_response(status)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Connection", "close")
            if location:
                self.send_header("Location", location)
            if value:
                self.send_header("Set-Cookie", value)
            self.end_headers()
            self.wfile.write(data)

        def log_message(self, *_):
            pass

    dns = socketserver.ThreadingUDPServer(("0.0.0.0", 53), DNS)
    server = http.server.ThreadingHTTPServer(("0.0.0.0", 443), Fixture)
    tls = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    tls.load_cert_chain(args.cert_dir / "cert.pem", args.cert_dir / "key.pem")
    server.socket = tls.wrap_socket(server.socket, server_side=True)
    for service in (dns, server):
        threading.Thread(target=service.serve_forever, daemon=True).start()
    processes = []
    pad = dsu_pad.Pad(0)
    try:
        with tempfile.TemporaryDirectory(prefix="zip-runtime-") as directory:
            work = Path(directory)
            card = work / "card.img"
            with card.open("wb") as stream:
                stream.truncate(512 * 1024 * 1024)
            sp.run(["mkfs.fat", "-F", "32", "-s", "8", str(card)], check=True, stdout=sp.DEVNULL)
            sp.run(["mcopy", "-i", str(card), "-s", str(args.card_files.resolve()), "::/"], check=True)
            # This test CA exists only in this private fixture card, never in the package.
            ca = work / "cacert.pem"
            ca.write_bytes((args.card_files / "cacert.pem").read_bytes() + b"\n" + (args.cert_dir / "cert.pem").read_bytes())
            sp.run(["mcopy", "-o", "-i", str(card), str(ca), "::/gcbrowser/cacert.pem"], check=True)
            read, write = os.pipe()
            x = sp.Popen(["Xvfb", "-displayfd", str(write), "-screen", "0", "640x480x24", "-nolisten", "tcp"],
                         pass_fds=(write,), stdout=sp.DEVNULL, stderr=open(out / "xvfb.log", "w"))
            processes.append(x)
            os.close(write)
            with os.fdopen(read) as stream:
                display = ":" + stream.readline().strip()
            env = dict(os.environ, DISPLAY=display, LIBGL_ALWAYS_SOFTWARE="1", DOLPHIN_TICKS="1", DOLPHIN_DABR="1")
            pad.plug_in()

            def frame(name):
                if dol.poll() is not None:
                    raise RuntimeError(f"Dolphin exited ({dol.returncode})")
                raw = sp.run(["ffmpeg", "-loglevel", "error", "-f", "x11grab", "-video_size", "640x480", "-draw_mouse", "0",
                              "-i", display + "+0,0", "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb24", "-"],
                             capture_output=True, check=True, timeout=20).stdout
                rgb = np.frombuffer(raw, np.uint8).reshape(480, 640, 3)
                Image.fromarray(rgb).save(out / (name + ".png"))
                return rgb

            def rendered(name, saved):
                until = time.monotonic() + 240
                while time.monotonic() < until:
                    time.sleep(3)
                    rgb = frame(name)
                    if saved:
                        marker = (rgb[:, :, 0] > 180) & (rgb[:, :, 1] < 70) & (rgb[:, :, 2] > 140)
                    else:
                        marker = (rgb[:, :, 0] < 65) & (rgb[:, :, 1] > 140) & (rgb[:, :, 2] > 70) & (rgb[:, :, 2] < 150)
                    if int(marker.sum()) > 10000:
                        roi = rgb[12:58, 120:200]
                        verified = (roi[:, :, 1] > 140) & (roi[:, :, 0] < 145) & (roi[:, :, 2] < 160)
                        if int(verified.sum()) < 10:
                            raise RuntimeError("Page lacks its verified TLS indicator")
                        return
                raise RuntimeError(f"Page not rendered: {requests}")

            records = []
            for phase in range(3):
                settings = work / "settings.ini"
                home = ROOT + "save?zip=80202" if phase == 0 else ROOT
                settings.write_text(f"web.home={ORIGIN}{home}\nweb.imgproxy=0\nnet.ntp=\nnet.dns=192.0.2.3\ndisp.anim=0\n")
                sp.run(["mcopy", "-o", "-i", str(card), str(settings), "::/gcbrowser/settings.ini"], check=True)
                user = work / f"user-{phase}"
                config = user / "Config"
                config.mkdir(parents=True)
                (config / "Dolphin.ini").write_text(f"""[Core]
{f'CPUCore = {args.cpu_core}' if args.cpu_core is not None else ''}
DSPHLE = False
DSPThread = False
MMU = True
GFXBackend = OGL
SIDevice0 = 6
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
                (config / "Logger.ini").write_text("[Logs]\nOSREPORT = True\nPOWERPC = True\n[Options]\nVerbosity = 1\nWriteToConsole = True\n")
                (config / "GCPadNew.ini").write_text(dsu_pad.GCPAD_INI)
                (config / "DSUClient.ini").write_text(f"[Server]\nEnabled = True\nEntries = {dsu_pad.DESCRIPTION}:127.0.0.1:{pad.port};\n")
                log = out / f"dolphin-{phase}.log"
                dol = sp.Popen(["dolphin-emu-nogui", "-u", str(user), "-p", "x11", "-v", "OGL",
                                "-C", "Dolphin.Core.FallbackRegion=1", "-C", "SYSCONF.IPL.PGS=True", "-e", str(args.dol.resolve())],
                               stdout=open(log, "w"), stderr=sp.STDOUT, env=env)
                processes.append(dol)
                rendered(f"boot-{phase}", phase < 2)
                root = [r for r in requests if r["phase"] == phase and r["path"] == ROOT]
                if not root or root[0]["cookie"] != (COOKIE if phase < 2 else None):
                    raise RuntimeError(f"Wrong first request after launch {phase}: {root}")
                if phase == 1:
                    pad.press("LEFT", .12)
                    time.sleep(1)
                    pad.press("A", .2)
                    rendered("cleared", False)
                    root = [r for r in requests if r["phase"] == phase and r["path"] == ROOT]
                    if root[-1]["cookie"] is not None:
                        raise RuntimeError("Clearing did not remove the request cookie")
                if re.search(r"Invalid (?:read|write)|DSI exception|ISI exception|Unhandled exception", log.read_text(errors="replace"), re.I):
                    raise RuntimeError("Guest CPU exception")
                dol.terminate()
                try:
                    dol.wait(10)
                except sp.TimeoutExpired:
                    dol.kill()
                    dol.wait()
                processes.remove(dol)
                stored = sp.check_output(["mtype", "-i", str(card), "::/gcbrowser/start.pref"])
                if phase == 0 and b"v1\n80202\n" not in stored:
                    raise RuntimeError("ZIP was not written to the actual card image")
                if phase > 0 and stored:
                    raise RuntimeError("Cleared preference reappeared on the card")
                records.append({"boot": phase, "bytes": len(stored), "sha256": hashlib.sha256(stored).hexdigest()})
            if any(r["cookie"] is not None for r in requests if r["path"] in ("/outside", "/outside.png")):
                raise RuntimeError("ZIP escaped top-level start-page requests")
            if not any(r["path"] == ROOT+"saved.png" and r["cookie"] == COOKIE for r in requests):
                raise RuntimeError("First-party weather resource did not receive the ZIP")
            (out / "result.json").write_text(json.dumps({"passed": True,
                "dol_sha256": hashlib.file_digest(args.dol.open("rb"), "sha256").hexdigest(),
                "requests": requests, "card_records": records, "boots": 3, "verified_tls": True,
                "scope": "HTTPS start-page GETs and their first-party resources only"}, indent=2) + "\n")
            print("PASS: actual DOL saved a ZIP, sent it after restart, cleared it and kept it cleared after another restart", flush=True)
    finally:
        for process in reversed(processes):
            process.terminate()
            try:
                process.wait(5)
            except sp.TimeoutExpired:
                process.kill()
                process.wait()
        pad.close()
        server.shutdown()
        dns.shutdown()
        (out / "requests.json").write_text(json.dumps(requests, indent=2) + "\n")


if __name__ == "__main__":
    main()
