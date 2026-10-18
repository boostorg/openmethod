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

With `--beside`, the two examples are compiled side by side, each in its own
compiler pane, to compare the assembly:

    dev/ce_link.py doc/modules/ROOT/examples/dispatch_open_method.cpp \\
        --beside doc/modules/ROOT/examples/dispatch_virtual_function.cpp

An example that includes a Boost header gets the Boost release that Compiler
Explorer offers in its Libraries dropdown, not the headers of this tree.
"""

import argparse
import base64
import json
import re
import sys
import urllib.parse
import zlib
from pathlib import Path

COMPILER = "clang2310"  # x86-64 clang 23.1.0
OPTIONS = "-std=c++17 -O2"
BOOST = "192"  # Boost 1.92.0, as Compiler Explorer names it
FILTERS = {
    "intel": True,
    "demangle": True,
    "labels": True,
    # CE takes a function whose every instruction comes from a header for
    # library code: `call_via_virtual_ptr` is the inlined dispatch, all of it.
    "libraryCode": False,
    "directives": True,
    "commentOnly": True,
}
TAG_DIRECTIVE = re.compile(r"\s*// (tag|end)::\w+\[\]\s*$")


def tagged_region(path, tag):
    begin, end = f"// tag::{tag}[]", f"// end::{tag}[]"
    lines = path.read_text().splitlines(keepends=True)
    starts = [i for i, line in enumerate(lines) if line.strip() == begin]
    ends = [i for i, line in enumerate(lines) if line.strip() == end]

    if len(starts) != 1 or len(ends) != 1 or starts[0] > ends[0]:
        sys.exit(f"{path}: expected exactly one `{begin}` ... `{end}`")

    # Like Asciidoctor, drop the directives of the tags nested in the region.
    region = lines[starts[0] + 1 : ends[0]]

    return "".join(line for line in region if not TAG_DIRECTIVE.match(line))


def uses_boost(source):
    return "#include <boost/" in source


def session_state(source, compiler):
    libs = [{"id": "boost", "version": BOOST}] if uses_boost(source) else []
    compiler = {"id": compiler, "libs": libs, "options": OPTIONS}

    return {
        "sessions": [
            {
                "id": 1,
                "language": "c++",
                "source": source,
                "compilers": [compiler],
                "executors": [{"compiler": compiler}],
            }
        ]
    }


def component(name, **state):
    return {"type": "component", "componentName": name, "componentState": state}


def libs(source):
    return [{"name": "boost", "ver": BOOST}] if uses_boost(source) else []


def editor(id, source):
    return component("codeEditor", id=id, lang="c++", source=source)


def beside_state(base, source, compiler):
    def column(id, source):
        return {
            "type": "column",
            "content": [
                editor(id, source),
                component(
                    "compiler",
                    id=id,
                    source=id,
                    lang="c++",
                    compiler=compiler,
                    libs=libs(source),
                    options=OPTIONS,
                    filters=FILTERS,
                ),
            ],
        }

    return {"content": [{"type": "row", "content": [column(1, base), column(2, source)]}]}


def clientstate_url(state):
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
    parser.add_argument(
        "--beside", type=Path, help="compile this example beside, for the assembly"
    )
    parser.add_argument("--beside-tag", help="the tag of the other example (default: --tag)")
    parser.add_argument("--compiler", default=COMPILER, help="CE compiler id")
    args = parser.parse_args()
    source = tagged_region(args.example, args.tag)

    if args.beside:
        other = tagged_region(args.beside, args.beside_tag or args.tag)
        state = beside_state(other, source, args.compiler)
    else:
        state = session_state(source, args.compiler)

    print(clientstate_url(state))


if __name__ == "__main__":
    main()
