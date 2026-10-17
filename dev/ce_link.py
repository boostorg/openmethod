#!/usr/bin/env python3
# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Print a Compiler Explorer link that opens a documentation example.

The link carries the source itself, as a `/clientstate/<base64>` URL: nothing
is stored on Compiler Explorer until a reader opens it, and the link is
regenerated from the example whenever the example changes:

    dev/ce_link.py doc/modules/ROOT/examples/ast_virtual_function.cpp

The source is the tagged region the page includes - `tag=content` by default -
so the reader gets exactly the code shown above the link.
"""

import argparse
import base64
import json
import sys
import urllib.parse
import zlib
from pathlib import Path

COMPILER = "g162"  # x86-64 gcc 16.2
OPTIONS = "-std=c++17 -O2"


def tagged_region(path, tag):
    begin, end = f"// tag::{tag}[]", f"// end::{tag}[]"
    lines = path.read_text().splitlines(keepends=True)
    starts = [i for i, line in enumerate(lines) if line.strip() == begin]
    ends = [i for i, line in enumerate(lines) if line.strip() == end]

    if len(starts) != 1 or len(ends) != 1 or starts[0] > ends[0]:
        sys.exit(f"{path}: expected exactly one `{begin}` ... `{end}`")

    return "".join(lines[starts[0] + 1 : ends[0]])


def clientstate_url(source):
    state = {
        "sessions": [
            {
                "id": 1,
                "language": "c++",
                "source": source,
                "compilers": [{"id": COMPILER, "options": OPTIONS}],
                "executors": [
                    {"compiler": {"id": COMPILER, "libs": [], "options": OPTIONS}}
                ],
            }
        ]
    }
    # CE detects the deflate on its own.
    packed = zlib.compress(json.dumps(state).encode(), 9)
    encoded = base64.b64encode(packed).decode()
    # Escape `+`, `/` and `=`: a pair of `+` would be taken by AsciiDoc for a
    # passthrough before the `link:` macro sees the URL.
    encoded = urllib.parse.quote(encoded, safe="")

    return "https://godbolt.org/clientstate/" + encoded


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("example", type=Path)
    parser.add_argument("--tag", default="content")
    args = parser.parse_args()
    print(clientstate_url(tagged_region(args.example, args.tag)))


if __name__ == "__main__":
    main()
