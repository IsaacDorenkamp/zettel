import os
import pathlib
import random
import string
import subprocess
import sys
import tempfile
import time


SEARCH_STR = "this is the search string!"


def run(zettel_exec: pathlib.Path | str) -> int:
    result = subprocess.run([zettel_exec, "init"])
    if result.returncode != 0:
        raise RuntimeError("Unable to initialize Zettelkasten at %s" % os.getcwd())

    needle = random.randint(0, 1000)
    for i in range(1000):
        sys.stdout.write(f"\rCreating note [{str(i+1).zfill(4)}/1000]")
        text = ''.join([random.choice(string.ascii_letters) for _ in range(1000)])
        if i == needle:
            split_at = len(text) // 2
            text = text[:split_at] + SEARCH_STR + text[split_at:]
        result = subprocess.run([str(zettel_exec), "new", f"Zettel {i+1}", "--stdin"], input=text.encode(), capture_output=True)
        if result.returncode != 0:
            raise RuntimeError(f"Failed to create Zettel {i+1}: {result.stderr.decode('utf-8')}")
    sys.stdout.write('\n')

    t = time.time()
    result = subprocess.run([str(zettel_exec), "--quiet", "search", "--type", "text", SEARCH_STR], capture_output=True)
    diff = time.time() - t

    output = result.stdout.decode('utf-8')
    lines = [line for line in output.split('\n') if line]
    if len(lines) == 1 and lines[0] == f"Zettel {needle+1} (ID {needle})":
        print("search successfully completed in %.3fs" % diff)
        return 0
    else:
        print("search failed in %.3fs" % diff)
        return 1


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

