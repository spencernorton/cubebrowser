#!/bin/sh
# Downloads and verifies the libraries the build needs into third_party/:
#   mbedTLS 3.6 LTS (TLS/HTTPS) and stb_image (JPEG/PNG/GIF/BMP decoding).
# Both are pinned to an exact version and checked against a sha256, so a moved
# tag or a tampered mirror fails the build loudly instead of shipping silently.
# Use the mbedTLS release tarball, not a git clone: it includes generated files.
set -eu

MBEDTLS_VER=3.6.7
MBEDTLS_SHA=a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6
# stb has no releases; pin a reviewed commit of stb_image.h (v2.30).
STB_COMMIT=2c980bb59875b0d32144a71867fbdebb2f77cd20
STB_SHA=594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3

cd "$(dirname "$0")"
mkdir -p third_party/stb

sha() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1" | awk '{print $1}'
    else shasum -a 256 "$1" | awk '{print $1}'; fi
}
verify() {
    got=$(sha "$1")
    if [ "$got" != "$2" ]; then
        echo "CHECKSUM MISMATCH for $1" >&2
        echo "  got:  $got" >&2
        echo "  want: $2" >&2
        exit 1
    fi
}

if [ ! -f third_party/mbedtls/library/ssl_tls.c ]; then
    URL="https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-$MBEDTLS_VER/mbedtls-$MBEDTLS_VER.tar.bz2"
    echo "Downloading $URL"
    curl -fL -o third_party/mbedtls.tar.bz2 "$URL"
    verify third_party/mbedtls.tar.bz2 "$MBEDTLS_SHA"
    tar -xjf third_party/mbedtls.tar.bz2 -C third_party
    rm -rf third_party/mbedtls
    mv "third_party/mbedtls-$MBEDTLS_VER" third_party/mbedtls
    rm third_party/mbedtls.tar.bz2
fi

if [ ! -f third_party/stb/stb_image.h ]; then
    echo "Downloading stb_image.h @ $STB_COMMIT"
    curl -fL -o third_party/stb/stb_image.h "https://raw.githubusercontent.com/nothings/stb/$STB_COMMIT/stb_image.h"
    verify third_party/stb/stb_image.h "$STB_SHA"
fi

echo "Done (checksums verified). Now run: make"
