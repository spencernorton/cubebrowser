#!/bin/sh
# Builds and runs the parser fuzz targets with libFuzzer + ASan/UBSan.
# --regressions runs the saved corpus with a normal sanitizer-enabled clang.
# A numeric argument also runs the two libFuzzer targets for that many seconds.
#
# seeds/ holds a small curated regression corpus (committed). The fuzzer's own
# generated corpus is kept in a scratch dir so it never lands in the repo.
set -eu
cd "$(dirname "$0")"
T=${1:-120}
case "$T" in --regressions) T=0;; ''|*[!0-9]*) echo 'usage: run.sh [seconds|--regressions]' >&2; exit 2;; esac
CC=${CC:-clang}
SRC=../../source
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cp seeds/* "$WORK"/ 2>/dev/null || true
CF="-fsanitize=address,undefined -fno-sanitize-recover=all -g -O1 -Istubs -I$SRC -w"
$CC $CF "$SRC/dom.c" fuzz_dom.c seed_driver.c -o "$WORK/seed_dom"
$CC $CF "$SRC/css.c" "$SRC/dom.c" fuzz_css.c seed_driver.c -lm -o "$WORK/seed_css"

echo "== regression seeds (must all pass) =="
for f in seeds/*; do [ -f "$f" ] || continue
  case "$f" in *.css) B="$WORK/seed_css";; *) B="$WORK/seed_dom";; esac
  printf '  %-34s ' "$(basename "$f")"; "$B" "$f" && echo ok || { echo CRASH; exit 1; }
done

if [ "$T" -eq 0 ]; then echo OK; exit 0; fi
$CC $CF -fsanitize=fuzzer "$SRC/dom.c" fuzz_dom.c -o "$WORK/fuzz_dom"
$CC $CF -fsanitize=fuzzer "$SRC/css.c" "$SRC/dom.c" fuzz_css.c -lm -o "$WORK/fuzz_css"
echo "== fuzzing dom ${T}s then css ${T}s =="
"$WORK/fuzz_dom" "$WORK" -max_total_time="$T" -timeout=10 -rss_limit_mb=2048 -artifact_prefix="$WORK/d_"
"$WORK/fuzz_css" "$WORK" -max_total_time="$T" -timeout=10 -rss_limit_mb=2048 -artifact_prefix="$WORK/c_"
echo OK
