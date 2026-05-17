---
name: update-changelog
description: Drafts or updates a new top-of-file fuse-utils release entry in ChangeLog using unrecorded commits and the project’s existing format. Use when preparing a fuse-utils release or updating release notes.
---

# Update ChangeLog

Update the `ChangeLog` file in the current directory with any commits
that are not yet recorded.

## Steps

1. Use `git log ChangeLog` to find the most recent modification git
   hash.
2. Run:
   ```bash
   git log --format="%H %ai %s" <last modification git hash>..HEAD
   git log <last modification git hash>..HEAD
   ```
   to find unrecorded commits and their commit bodies.
3. Filter out noise commits, including:
   - merges
   - revert accidental commit
   - add ignore
   - update for version
   - update release dates
   - first updates for
   - bring up to date
   - tidy up commit text
   - similar meta-commits that do not describe user-visible changes
4. For each remaining commit, determine which utility or component it
   affects, for example:
   - `tape2wav`
   - `snapdump`
   - `tzxlist`
   - `fmfconv`
   - `rzxcheck`
   - `audio2tape`
   - `listbasic`
   - `rzxdump`
   - `createhdf`
   - `tape2pulses`
   - `snap2tzx`
   - or `General` for build, configure, or Autotools changes
5. Add a new date entry at the top of `ChangeLog` after the first line
   of the file using the most recent unrecorded commit date, with
   Philip Kendall’s name and email as maintainer:
   `YYYY-MM-DD  Philip Kendall  <philip-fuse@shadowmagic.org.uk>`
6. Under that date, add a `* Version X.Y.Z released` entry.
7. Under that date, add entries grouped by component, matching the
   existing format exactly:
   ```text
           * component_name
             * description (Author if known).
   ```
8. Write the updated `ChangeLog`.

## Rules

- Follow the existing style in `ChangeLog` exactly.
- Preserve all existing entries unchanged.
- If the release version is not yet known, ask the user for it before
  writing the new entry.
- Do not modify any existing entries; only prepend new ones.
- Do not list every commit mechanically.
- Summarise related commits into a single readable `ChangeLog` item
  where appropriate.

## Style rules

- Keep the existing 8-space indent for entries.
- Keep the existing 10-space indent for sub-bullets.
- End descriptions with a period.
- Use author attribution in parentheses when available.
- If no author is identifiable from the commit message, omit the
  attribution.
- Group entries by component.
- Use `General` for build, configure, and Autotools changes.
- Wrap long lines to match the existing file.
- Continuation lines should align under the bullet text.
- Keep wording concise and user-facing.
- Prefer verbs like:
  - Add
  - Fix
  - Enable
  - Support
  - Remove
  - Include
  - Use
  - Document
  - Replace
  - Avoid
  - Ensure

## Author attribution rules

- Preserve `thanks, X` and patch-author attributions found in the commit
  body, matching the existing `ChangeLog` pattern.
- The name in parentheses at the end of each `ChangeLog` entry is the
  person who authored the code change.
- In commit messages, `(thanks, X)` means X reported the bug or provided
  non-code feedback; they are not the author and should appear in the
  `ChangeLog` entry before the author, for example:
  `(thanks, ICEknight) (Sergio Baldoví).`
- If the commit message names no other contributor, the committer is the
  author.
- If the commit message has a name in parentheses only, that person is
  the author, for example:
  `(Sergio Baldoví).`
