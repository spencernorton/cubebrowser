#!/usr/bin/env python3
"""Guards sd/gcbrowser/cacert.pem against tampering.

The bundle must be Mozilla's trust program, distributed via
https://curl.se/ca/cacert.pem. A root that is not Mozilla's -- a TLS-intercepting
proxy's, say -- in the trust store silently defeats every HTTPS check, so the
bundle is pinned by sha256 here and only ever changed through
tools/refresh_cacert.sh, which pulls a fresh Mozilla bundle and updates the pin.

usage: check_cacert.py [path]   (default sd/gcbrowser/cacert.pem)
exit 0 = matches the pinned bundle, 1 = it does not.
"""
import hashlib
import sys

# sha256 of sd/gcbrowser/cacert.pem (Mozilla roots via curl.se).
# tools/refresh_cacert.sh updates this when pulling newer roots.
PINNED = "a41b5d356aea97a529fe27e0f7316d2f9d946d75927476cf9cf1b90637d00505"


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else "sd/gcbrowser/cacert.pem"
    try:
        data = open(path, "rb").read()
    except OSError as e:
        print(f"cannot read {path}: {e}", file=sys.stderr)
        return 1
    got = hashlib.sha256(data).hexdigest()
    if got != PINNED:
        print(f"{path} does not match the pinned CA bundle:", file=sys.stderr)
        print(f"  got:    {got}", file=sys.stderr)
        print(f"  pinned: {PINNED}", file=sys.stderr)
        print("  refresh it with tools/refresh_cacert.sh (which re-pins).", file=sys.stderr)
        return 1
    print(f"{path}: matches the pinned Mozilla bundle "
          f"({data.count(b'BEGIN CERTIFICATE')} certificates)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
