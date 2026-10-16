#!/bin/sh
# Refreshes sd/gcbrowser/cacert.pem from Mozilla's trust program (via curl.se)
# and updates the sha256 pin in tools/check_cacert.py.
set -eu
cd "$(dirname "$0")/.."
echo "Downloading https://curl.se/ca/cacert.pem"
curl -fL -o sd/gcbrowser/cacert.pem https://curl.se/ca/cacert.pem
if command -v sha256sum >/dev/null 2>&1; then
    sha=$(sha256sum sd/gcbrowser/cacert.pem | awk '{print $1}')
else
    sha=$(shasum -a 256 sd/gcbrowser/cacert.pem | awk '{print $1}')
fi
sed "s/^PINNED = .*/PINNED = \"$sha\"/" tools/check_cacert.py > tools/check_cacert.py.new
mv tools/check_cacert.py.new tools/check_cacert.py
python3 tools/check_cacert.py
echo "cacert.pem refreshed and re-pinned ($sha)."
