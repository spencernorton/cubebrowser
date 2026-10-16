#!/bin/sh
# Builds and runs the parser fuzz targets with libFuzzer + ASan/UBSan.
# Meant to run inside the project's libogc2 toolchain image, which ships clang.
#   docker run --rm -v "$PWD/../..:/w" -w /w/tools/fuzz --entrypoint sh \
#     ghcr.io/extremscorner/libogc2:latest ./run.sh [seconds]
#
# seeds/ holds a small curated regression corpus (committed). The fuzzer's own
# generated corpus is kept in a scratch dir so it never lands in the repo.
set -eu
T=${1:-120}
SRC=../../source
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cp seeds/* "$WORK"/ 2>/dev/null || true
CF="-fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined -g -O1 -Istubs -I$SRC -w"
clang $CF "$SRC/dom.c" fuzz_dom.c -o "$WORK/fuzz_dom"
clang $CF "$SRC/css.c" "$SRC/dom.c" fuzz_css.c -o "$WORK/fuzz_css"

echo "== regression seeds (must all pass) =="
for f in seeds/*; do [ -f "$f" ] || continue
  case "$f" in *.css) B="$WORK/fuzz_css";; *) B="$WORK/fuzz_dom";; esac
  printf '  %-34s ' "$(basename "$f")"; "$B" "$f" >/dev/null 2>&1 && echo ok || { echo CRASH; exit 1; }
done

echo "== fuzzing dom ${T}s then css ${T}s =="
"$WORK/fuzz_dom" "$WORK" -max_total_time="$T" -timeout=10 -rss_limit_mb=2048 -artifact_prefix="$WORK/d_"
"$WORK/fuzz_css" "$WORK" -max_total_time="$T" -timeout=10 -rss_limit_mb=2048 -artifact_prefix="$WORK/c_"
echo OK
