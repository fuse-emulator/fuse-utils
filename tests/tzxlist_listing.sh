#!/bin/sh
# tests/tzxlist_listing.sh: exercise tzxlist's tape listing
#
# Converts the committed TAP fixture with tapeconv, lists the resulting TZX
# with tzxlist and requires the listing to describe exactly the two standard
# speed data blocks with passing checksums; also requires tzxlist to refuse
# to list blocks from a truncated tape.
#
# Environment:
#   UTILS_TEST_SRCDIR       directory holding the tests/fixtures inputs
#   UTILS_TEST_TOPBUILDDIR  directory holding the built tools
set -e

srcdir=${UTILS_TEST_SRCDIR:-.}
topbuilddir=${UTILS_TEST_TOPBUILDDIR:-.}
tapeconv="$topbuilddir/tapeconv"
tzxlist="$topbuilddir/tzxlist"
fixture="$srcdir/tests/fixtures/tap-standard.tap"

tmpdir=`mktemp -d`
trap 'rm -rf "$tmpdir"' 0 INT TERM

errfile="$tmpdir/tapeconv.err"
if ! "$tapeconv" "$fixture" "$tmpdir/list.tzx" 2> "$errfile"; then
  cat "$errfile" >&2
  echo "tapeconv failed" >&2
  exit 1
fi

if ! "$tzxlist" "$tmpdir/list.tzx" > "$tmpdir/listing.txt" 2> "$tmpdir/tzxlist.err"
then
  cat "$tmpdir/tzxlist.err" >&2
  echo "tzxlist failed on the converted tape" >&2
  exit 1
fi

count=`grep -c "Standard Speed Data" "$tmpdir/listing.txt"`
test "$count" = 2

grep -q "Checksum: 0x1e (PASS)" "$tmpdir/listing.txt"
grep -q "Checksum: 0xff (PASS)" "$tmpdir/listing.txt"

head -c 3 "$fixture" > "$tmpdir/truncated.tap"
if "$tzxlist" "$tmpdir/truncated.tap" > "$tmpdir/truncated.txt" 2> /dev/null
then
  echo "tzxlist accepted a truncated tape" >&2
  exit 1
fi
if grep -q "Block type" "$tmpdir/truncated.txt"
then
  echo "tzxlist listed blocks from a truncated tape" >&2
  exit 1
fi
