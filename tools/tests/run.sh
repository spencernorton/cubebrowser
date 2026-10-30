#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
CC=${CC:-clang}
CF='-std=gnu11 -Wall -Wextra -Werror -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all'
$CC $CF -Isource tools/tests/test_viewport.c source/viewport.c -lm -o "$WORK/viewport"
"$WORK/viewport"
$CC $CF -Itools/fuzz/stubs -Isource tools/tests/test_images.c source/page_images.c source/viewport.c -lm -o "$WORK/images"
"$WORK/images"
$CC $CF -Itools/fuzz/stubs -Isource tools/tests/test_url.c source/url.c -lm -o "$WORK/url"
"$WORK/url"
$CC $CF -Isource tools/tests/test_gzip.c source/gzip.c source/inflate.c -o "$WORK/gzip"
python3 - "$WORK/page.gz" <<'PY'
import gzip,sys
from pathlib import Path
Path(sys.argv[1]).write_bytes(gzip.compress(b'browser page!\n' * 1000,mtime=0))
PY
"$WORK/gzip" "$WORK/page.gz"
$CC $CF -Isource tools/tests/test_dns.c source/dns_cache.c -o "$WORK/dns"
"$WORK/dns"
$CC $CF -Wno-unused-parameter -Itools/fuzz/stubs -Isource tools/tests/test_page.c tools/tests/page_support.c \
    source/page.c source/page_images.c source/viewport.c source/dom.c source/css.c source/url.c source/bigstack.c -lm -o "$WORK/page"
"$WORK/page"
