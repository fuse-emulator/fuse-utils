#!/bin/sh
# tests/tapeconv_roundtrip.sh: exercise tapeconv's TAP <-> TZX conversion
#
# Converts the committed standard-format TAP fixture to TZX and back and
# requires the result to be byte-identical, then requires tapeconv to reject
# a truncated tape without producing an output file.
#
# Environment:
#   UTILS_TEST_SRCDIR       directory holding the tests/fixtures inputs
#   UTILS_TEST_TOPBUILDDIR  directory holding the built tools
set -e

srcdir=${UTILS_TEST_SRCDIR:-.}
topbuilddir=${UTILS_TEST_TOPBUILDDIR:-.}
tapeconv="$topbuilddir/tapeconv"
fixture="$srcdir/tests/fixtures/tap-standard.tap"

tmpdir=`mktemp -d`
trap 'rm -rf "$tmpdir"' 0 INT TERM

convert() {
  errfile="$tmpdir/tapeconv.err"
  if ! "$tapeconv" "$@" 2> "$errfile"; then
    cat "$errfile" >&2
    echo "tapeconv failed" >&2
    exit 1
  fi
}

convert "$fixture" "$tmpdir/roundtrip.tzx"
convert "$tmpdir/roundtrip.tzx" "$tmpdir/roundtrip.tap"

cmp "$fixture" "$tmpdir/roundtrip.tap"

head -c 3 "$fixture" > "$tmpdir/truncated.tap"
if "$tapeconv" "$tmpdir/truncated.tap" "$tmpdir/truncated.tzx" 2> /dev/null
then
  echo "tapeconv accepted a truncated tape" >&2
  exit 1
fi
test ! -e "$tmpdir/truncated.tzx"
