## tests/Makefile.am: automated tests for the fuse-utils tools
## Process this file with automake to produce Makefile.in
## Copyright (c) 2026
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

EXTRA_DIST += \
	tests/tapeconv_roundtrip.sh \
	tests/snapconv_roundtrip.sh \
	tests/tzxlist_listing.sh \
	tests/fixtures/tap-standard.tap \
	tests/fixtures/sna-48k.sna

check-local:
	UTILS_TEST_SRCDIR=$(srcdir) \
	UTILS_TEST_TOPBUILDDIR=$(top_builddir) \
	sh $(srcdir)/tests/tapeconv_roundtrip.sh
	UTILS_TEST_SRCDIR=$(srcdir) \
	UTILS_TEST_TOPBUILDDIR=$(top_builddir) \
	sh $(srcdir)/tests/snapconv_roundtrip.sh
	UTILS_TEST_SRCDIR=$(srcdir) \
	UTILS_TEST_TOPBUILDDIR=$(top_builddir) \
	sh $(srcdir)/tests/tzxlist_listing.sh
