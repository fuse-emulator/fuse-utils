# Releasing fuse-utils

## Overview

This document describes the manual steps used to prepare a fuse-utils
release. The `update-release.py` helper updates release metadata in the
main documentation files and in `configure.ac`, but it does not perform
the full release process by itself.

## Release metadata helper

The repository includes `update-release.py` in the top-level directory.

### Usage

Preview changes:

```sh
./update-release.py --date "12th June, 2026" --version "1.4.6" --dry-run
```

Update the release date only:

```sh
./update-release.py --date "12th June, 2026"
```

Update both release date and version:

```sh
./update-release.py --date "12th June, 2026" --version "1.4.6"
```

### Files updated

When given `--date`, the script updates:

- the `.TH` line in `man/*.1`
- the final date line in `README`

When given `--version`, it also updates:

- the version definitions in `configure.ac`

The script leaves the existing man-page version strings unchanged unless
`--version` is given.

### Files not updated

The script does not update:

- `ChangeLog`
- generated files
- release tarballs, tags, or announcements

## Optional agent skill for ChangeLog updates

The repository also includes an optional maintainer skill at
`.agents/skills/update-changelog/SKILL.md`. If your agent environment
supports repository skills, you can use that skill while preparing a
release to draft or update the new top-of-file `ChangeLog` release
entry in the existing project format.

The skill is a convenience helper only. It is not required to build
fuse-utils or to prepare a release. If it is unavailable, update
`ChangeLog` manually in the existing style.

## Typical release workflow

1. Update `ChangeLog`. If your agent environment supports repository
   skills, you can use `.agents/skills/update-changelog/SKILL.md` to
   draft or prepare the new top-of-file release entry in the existing
   format. Otherwise, update `ChangeLog` manually.
2. Run `./update-release.py` with `--dry-run`.
3. Run it again without `--dry-run` once the changes look correct.
4. Review the resulting diff.
5. If `configure.ac` changed, regenerate the build system as needed.
6. Build the project.
7. Run a few command smoke tests.
8. Create the release tarball, tag, and publish using the normal
   maintainer process.

## Verification

After updating release metadata, verify with:

```sh
make clean && make
./listbasic --help
./tzxlist --help
```

Also inspect the diff manually to confirm the release date and version
were updated in the expected files.

## Caveats

- The date should be passed in the format used by the existing docs, for
  example `12th June, 2026`.
- The script is intended as a maintainer convenience tool.
- Review all changes before creating a release.
