import os
import pathlib
import subprocess
import sys
import tempfile


def create_zettel(zettel_exec: pathlib.Path | str, title: str, content: str) -> tuple[int, str]:
    result = subprocess.run([zettel_exec, "--quiet", "new", title, "--stdin", "--report"], input=content.encode(), capture_output=True)
    if result.returncode != 0:
        raise RuntimeError("Error while creating '%s': %s" % (title, result.stderr.decode('utf-8')))

    output = result.stdout.decode('utf-8')
    lines = output.split('\n')
    return int(lines[0]), lines[1]

def run(zettel_exec: pathlib.Path | str) -> int:
    result = subprocess.run([zettel_exec, "init"])
    if result.returncode != 0:
        raise RuntimeError("Unable to initialize Zettelkasten at %s" % os.getcwd())

    prev_id, prev_title = create_zettel(zettel_exec, "Root", "This is the root.")
    for i in range(20):
        content = f"Let's go back to [{prev_title}](zettel:{prev_id})"
        prev_id, prev_title = create_zettel(zettel_exec, f"Chain Link {i+1}", content)

    print(f"chain is in {os.getcwd()}")
    input()

    return 0


def main() -> int:
    with tempfile.TemporaryDirectory() as tdir:
        os.chdir(tdir)
        return run(pathlib.Path(__file__).parent.parent / "build/Release/zettel")


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as exc:
        print(exc)
        sys.exit(1)

