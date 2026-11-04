v1.0.0
------
- Initial version.

v1.0.1
------
- Updated incorrect command descriptions, added description for "list"
- Added CHANGELOG.md

v1.1.0
------
- `zettel search` now outputs results in the same format as `zettel list`
- `zettel edit` no longer adds an excess newline when using `--details-only`
- `zettel view` no longer exits with a failure status code when exiting with Ctrl-C
- `zettel check` now also verifies Zettel titles.

v1.2.0
------
- `zettel search` now supports case-insensitive searches, still allowing for case-sensitive
  searches with the `--sensistive` or `-s` flag.
- `zettel search` now supports the use of regular expressions in full-text searches
