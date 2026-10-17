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

With `--base`, the link opens a diff instead: the base example in editor #1,
the example in editor #2, the diffs between their sources and between their
disassemblies, and the example's output.

    dev/ce_link.py doc/modules/ROOT/examples/ast.cpp \\
        --base doc/modules/ROOT/examples/ast_virtual_function_2.cpp --base-tag all

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

COMPILER = "g162"  # x86-64 gcc 16.2
OPTIONS = "-std=c++17 -O2"
BOOST = "192"  # Boost 1.92.0, as Compiler Explorer names it
FILTERS = {
    "intel": True,
    "demangle": True,
    "labels": True,
    "libraryCode": True,
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


def session_state(source):
    libs = [{"id": "boost", "version": BOOST}] if uses_boost(source) else []
    compiler = {"id": COMPILER, "libs": libs, "options": OPTIONS}

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


def diff_state(base, source):
    def libs(source):
        return [{"name": "boost", "ver": BOOST}] if uses_boost(source) else []

    def editor(id, source):
        return component("codeEditor", id=id, lang="c++", source=source)

    def compiler(id, source):
        return component(
            "compiler",
            id=id,
            source=id,
            lang="c++",
            compiler=COMPILER,
            libs=libs(source),
            options=OPTIONS,
            filters=FILTERS,
        )

    def diff(lhs, rhs, difftype):
        return component(
            "diff", lhs=lhs, rhs=rhs, lhsdifftype=difftype, rhsdifftype=difftype
        )

    return {
        "content": [
            {
                "type": "row",
                "content": [
                    {
                        "type": "column",
                        "width": 40,
                        "content": [
                            {
                                "type": "stack",
                                "activeItemIndex": 1,
                                "content": [editor(1, base), editor(2, source)],
                            },
                            {
                                "type": "stack",
                                "activeItemIndex": 2,
                                "content": [
                                    compiler(1, base),
                                    compiler(2, source),
                                    component(
                                        "executor",
                                        source=2,
                                        lang="c++",
                                        compiler=COMPILER,
                                        libs=libs(source),
                                        options=OPTIONS,
                                        compilationPanelShown=True,
                                        compilerOutShown=True,
                                    ),
                                ],
                            },
                        ],
                    },
                    {
                        "type": "column",
                        "content": [
                            diff("source_1", "source_2", 16),  # DiffType.Source
                            diff(1, 2, 0),  # DiffType.ASM
                        ],
                    },
                ],
            }
        ]
    }


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
    parser.add_argument("--base", type=Path, help="open a diff against this example")
    parser.add_argument("--base-tag", help="the tag of the base (default: --tag)")
    args = parser.parse_args()
    source = tagged_region(args.example, args.tag)

    if args.base:
        base = tagged_region(args.base, args.base_tag or args.tag)
        state = diff_state(base, source)
    else:
        state = session_state(source)

    print(clientstate_url(state))


if __name__ == "__main__":
    main()
