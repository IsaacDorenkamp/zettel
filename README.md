zettel
------
`zettel` is a command-line tool designed to manage a digitial [Zettelkasten](https://zettelkasten.de). It includes functionalities for
creating, editing, searching, and viewing individual Zettels: notes expressing a singular idea.

# Global options
These options may be passed before specifying the subcommand, for example: `zettel --global-opt new ...`

### `--quiet`, `-q`
Ordinarily, `zettel` outputs some styled text informing you of the success of an operation. However, if you are using the tool with other
programs and don't want this fluff passed along, `--quiet` or `-q` will suppress these messages. However, error messages will not be
suppressed, since they are written to standard error and not standard output.  To suppress these, redirect standard error to /dev/null
(or NUL on Windows).

# Anatomy of a Zettel
A Zettel has an ID, a Title, Tags, and a Body. The ID is automatically generated and uniquely identifies a Zettel. The Title is a brief
description of what the Zettel is about. Tags are short, lowercase strings that denote what categories a Zettel belongs to. The Body of
the Zettel is its main content. The Body is primarily plain text, but it supports the usage of Markdown-style links. These links can
be regular web links, like `[google](https://google.com)`, but `zettel` also supports referencing other Zettels with simple URIs using
the `zettel` scheme with the ID of the Zettel to reference: `[link to another zettel](zettel:123)`. Zettels are stored in a text file
with the following format:
```
[ID] Title
#this #line #contains #tags
|The content of the Zettel follows as a series of lines, each beginning with a '|'.
|Each line may be of arbitrary length, but it must begin with a '|'.
```
It is not recommended to manually modify these files. Modifying the title or tags will bring the file out of sync with the database, which
may cause confusing behavior. `zettel check` can be used in case a file gets corrupted or deleted to inform you of the ways in which the
file structure is out of sync with the database. Ideally, just use `zettel` to perform operations in your digital Zettelkasten.

# Commands

### `zettel init`
To create a Zettelkasten in the current directory, simply run `zettel init`. Note that Zettels will be stored in text files within the
current directory, so ensure that you choose a directory which you can freely create (a lot of) files in without confusion.

### `zettel new`
To create a Zettel, use the `zettel new` command. A Zettel consists of a title, a list of tags, and a body. The most simple command to
create a Zettel is `zettel new [title]`. Upon executing this, you will be prompted to edit a file in Vi which will determine the body
of the Zettel. If you wish to create an empty Zettel, you may pass the `--noedit` flag to bypass this. If you wish to use other programs
with `zettel`, you may wish to create Zettels by passing content into standard input. This can be achieved by passing the `--stdin` flag.
To add tags to the Zettel, add `--tag [tag_name]` for each tag you wish to attach. You may also pass the `--report` flag to echo details
about the Zettel after creating it: one line with the ID, one line with the title, and one line with a comma-separated list of tags.

### `zettel edit`
You might want to edit a Zettel from time to time, although it is advisable to minimize this practice. However, it is fully supported by
`zettel`, and can be especially useful when initially creating and linking up related Zettels, since you need a Zettel's ID in order to
reference it. Simply enter `zettel edit [id]` in order to achieve this. Like `zettel new`, `zettel edit` supports accepting input through
standard input by passing the `--stdin` flag. In some cases, you might want to just tweak the title and the tags without modifying the
content, which you can do by passing the `--details-only` flag. To add a tag, use the `--tag [tag]` option, just like in `zettel new`.
You can also use `--notag [tag]` to remove an existing tag. By default, `zettel` ignores duplicate tags or requests to remove tags that
aren't present, but if you want to fail the operation when in either of these cases, you can set the strictness level with the `--strict [level]`
option. The valid values are `none`, `add`, `remove`, and `all`. `none` enforces no constraints, `add` causes failure when a duplicate tag is
passed, `remove` causes failure when a tag is removed that does not exist on the Zettel, and `all` enables both `add` and `remove`. By default,
the title is left untouched, but to modify it, pass the `--title [new title]` option.

### `zettel list`
Produces a simple list of all Zettels currently available. It outputs each Zettel as a single line in the format "[ID] [Title]". By default, the
ID is dimmed through ANSI magic, but you can pass `--plain` to receive pure plain text instead.

### `zettel search`
One of the core features of `zettel`, `zettel search` enables the user to search for Zettels based on title, tag, or text. Tag searches are the
default, so `zettel search some-tag` will search for Zettels with the tag "some-tag". However, you can explicitly denote a search as a tag search
by passing `--type tag`. To search by title, pass `--type title`. To do a full-text search, pass `--type text`. Full-text searches are the most
expensive searches, but as can be seen by running `stress_test_fulltext_search.py`, it still runs extremely quickly. Each search result is printed
on its own line, in the format "[Title] (ID [ID])" (NOTE: The author intends to bring this into line with `zettel list` in the future).

### `zettel delete`
It is unlikely that you will find yourself deleting many Zettels, especially since the philosophy of the Zettelkasten is to grow over into a complex
network over time, but there may be times when you don't end up needing a Zettel or create one by accident. To delete a Zettel, run
`zettel delete [ID]`. If any other Zettels have a reference to this Zettel, then the operation will fail. You can force the deletion by passing
`--force`.

### `zettel view`
This is one of the key features of `zettel`. To view a Zettel, run `zettel view [ID]`. The viewer is a TUI application which allows you not only to
view a Zettel, but also to follow its chain of references. You can cycle through the links with `k` and open a link with `<Enter>/<Return>`. Links
to other Zettels will open in the viewer, and allow you to navigate back with `b`. Other links, such as Web links, will open in other applications.
This is achieved with the `open` program (`xdg-open` on Linux).

### `zettel check`
Although this is not likely to be part of a user's arsenal, it can be useful in the event that some kind of data corruption occurs, such as the
accidental deletion or modification of Zettel files. `zettel check` will ensure that each Zettel matches its representation in the database. This
entails ensuring that its file actually exists, that all of its references match, and that its tags match. If there are any issues, these will be
logged to standard output and the program will exit with a failing status code. If there are no issues, this will be acknowledged and the program
will exit with a successful status code.

# Caveats/Known Issues
- `zettel check` does not verify Zettel titles.
- `zettel view` always returns a failing status code on exit due to being terminated by Ctrl-C.
- `zettel search` does not output results in the same format as `zettel list`.
- `zettel edit` may append a newline to a Zettel's content when using `--details-only`.
