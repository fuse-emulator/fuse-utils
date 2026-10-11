#!/bin/sh
# tests/snapconv_roundtrip.sh: exercise snapconv's SNA <-> Z80 conversion
#
# Converts the committed 48K SNA fixture to Z80 and back, both uncompressed
# and with -c forced compression, and requires both results to be
# byte-identical to the fixture.
#
# Environment:
#   UTILS_TEST_SRCDIR       directory holding the tests/fixtures inputs
#   UTILS_TEST_TOPBUILDDIR  directory holding the built tools
set -e

srcdir=${UTILS_TEST_SRCDIR:-.}
topbuilddir=${UTILS_TEST_TOPBUILDDIR:-.}
snapconv="$topbuilddir/snapconv"
fixture="$srcdir/tests/fixtures/sna-48k.sna"

tmpdir=`mktemp -d`
trap 'rm -rf "$tmpdir"' 0 INT TERM

convert() {
  errfile="$tmpdir/snapconv.err"
  if ! "$snapconv" "$@" 2> "$errfile"; then
    cat "$errfile" >&2
    echo "snapconv failed" >&2
    exit 1
  fi
}

convert "$fixture" "$tmpdir/plain.z80"
convert "$tmpdir/plain.z80" "$tmpdir/plain.sna"
cmp "$fixture" "$tmpdir/plain.sna"

convert -c "$fixture" "$tmpdir/compressed.z80"
convert "$tmpdir/compressed.z80" "$tmpdir/compressed.sna"
cmp "$fixture" "$tmpdir/compressed.sna"
