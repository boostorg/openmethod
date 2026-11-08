#!/usr/bin/env python3
# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Link every example of the troubleshooting page to Compiler Explorer.

Each `*Example*` label of `doc/modules/ROOT/pages/troubleshooting.adoc` gets a
link that opens the example on Compiler Explorer, with the source and the
output of the compiler - and of the program, for an error reported at run time
- and no assembly. Run it from the root of the repository whenever an example
changes; it rewrites the links in place:

    dev/ce_troubleshooting.py

The source behind each link is built from the files the page includes:

* A compilation error is `troubleshooting/preamble.hpp` - which the page shows
  once, above the examples - followed by the tagged region of the example.
* A run-time error is a Boost.Test program. The link turns it into a `main`
  that runs the tagged region, without the harness: the default error handler
  then prints the message, as it would in the reader's program.

The library headers are the flattened ones published next to the documentation
(see `ce/README.md`), so the messages are those of `develop`, not of the Boost
release that Compiler Explorer offers.

`--check` compiles and runs every example on Compiler Explorer instead, and
reports the ones that do not produce the expected message.
"""

import argparse
import json
import re
import sys
import textwrap
import urllib.request
from pathlib import Path

from ce_link import BOOST, COMPILER, clientstate_url

PAGE = Path("doc/modules/ROOT/pages/troubleshooting.adoc")
EXAMPLES = Path("doc/modules/ROOT/examples")
SITE = "https://boostorg.github.io/openmethod/"
OPTIONS = "-std=c++17"
# C++26 reflection: the version of gcc that implements P2996.
REFLECTION_COMPILER = "g161"
REFLECTION_OPTIONS = "-std=c++26 -freflection"
# Examples that cannot run on Compiler Explorer: two translation units.
NO_LINK = {"errors_odr_violation.cpp"}

LABEL = re.compile(r"^\*Example\*( - link:\S+\[Compiler Explorer\^\])?$")
INCLUDE = re.compile(r"^include::example\$([^\[]+)\[tag=(\w+)")
TAG_DIRECTIVE = re.compile(r"\s*// (tag|end)::\w+\[\]\s*$")
LIBRARY_INCLUDE = re.compile(r"^#include <(boost/openmethod[^>]*)>")
# The guards that let a compile-fail example produce its diagnostic where the
# feature it misuses is not available.
FEATURE_GUARD = re.compile(r"BOOST_OPENMETHOD_HAS_(REFLECTION|PEXT)")


def tagged_region(lines, tag):
    begin, end = f"// tag::{tag}[]", f"// end::{tag}[]"
    starts = [i for i, line in enumerate(lines) if line.strip() == begin]
    ends = [i for i, line in enumerate(lines) if line.strip() == end]
    region = lines[starts[0] + 1 : ends[0]]

    return [line for line in region if not TAG_DIRECTIVE.match(line)]


def from_site(line):
    return LIBRARY_INCLUDE.sub(rf"#include <{SITE}\1>", line)


def program(lines):
    """The lines of a source, without the comments outside its tagged regions."""
    source = []
    depth = 0

    for line in lines:
        if TAG_DIRECTIVE.match(line):
            depth += 1 if line.strip().startswith("// tag::") else -1
            continue

        if line.startswith("#include") and line in source:
            continue

        # Comments are kept in the regions the page shows, and only there.
        if depth or not line.startswith("//"):
            source.append(line)

    return source


def compile_time_source(path):
    lines = []
    guarded = False
    skipping = False

    # The whole file, not just the region the page shows: a method may have to
    # be called before the compiler instantiates the check that fails.
    for line in path.read_text().splitlines():
        if FEATURE_GUARD.search(line):
            guarded = skipping = True
        elif skipping:
            skipping = line.strip() != "#else"
        elif line == '#include "preamble.hpp"':
            preamble = (EXAMPLES / "troubleshooting/preamble.hpp").read_text()
            lines += tagged_region(preamble.splitlines(), "content")
        else:
            lines.append(line)

    if guarded:
        lines.pop(len(lines) - 1 - lines[::-1].index("#endif"))

    source = "\n".join(from_site(line) for line in program(lines))

    return re.sub(r"\n{3,}", "\n\n", source).strip() + "\n"


def run_time_source(path, tags):
    lines = path.read_text().splitlines()
    test_case = next(
        i for i, line in enumerate(lines) if line.startswith("BOOST_AUTO_TEST_CASE")
    )
    source = [
        from_site(line)
        for line in program(lines[:test_case])
        if not (
            "BOOST_TEST_MODULE" in line
            or "<boost/test/" in line
            or '"error_harness.hpp"' in line
            or FEATURE_GUARD.search(line)
        )
    ]

    while source and not source[-1].strip():
        source.pop()

    while source and not source[0].strip():
        source.pop(0)

    body = []
    test = "\n".join(lines[test_case:])

    # The test calls `initialize` itself, before the region, if the error
    # happens after it.
    if re.search(r"initialize\(\)[;.].*capture_cerr", test, re.S):
        body.append("initialize();")
        body.append("")

    # The `classes` region is at namespace scope, above.
    for tag in tags:
        if tag != "classes":
            region = "\n".join(tagged_region(lines, tag))
            body += textwrap.dedent(region).splitlines()

    source += ["", "int main() {"]
    source += [("    " + line).rstrip() for line in body]
    source += ["}"]

    return re.sub(r"\n{3,}", "\n\n", "\n".join(source)) + "\n"


def compiler_for(source, path):
    if "^^" in source:
        return REFLECTION_COMPILER, REFLECTION_OPTIONS

    if "minimal_cover_hash" in source and path.name.startswith("errors_"):
        return COMPILER, OPTIONS + " -mbmi2"

    return COMPILER, OPTIONS


def session_state(source, compiler, options):
    return {
        "sessions": [
            {
                "id": 1,
                "language": "c++",
                "source": source,
                "compilers": [],
                "executors": [
                    {
                        "compiler": {
                            "id": compiler,
                            "libs": [{"id": "boost", "version": BOOST}],
                            "options": options,
                        },
                        "compilerVisible": False,
                        "compilerOutputVisible": True,
                        "arguments": "",
                        "argumentsVisible": False,
                        "stdin": "",
                        "stdinVisible": False,
                    }
                ],
            }
        ]
    }


def examples(page_lines):
    """Yield (label line index, example path, tags) for each example."""
    for i, line in enumerate(page_lines):
        if not LABEL.match(line):
            continue

        includes = []

        for following in page_lines[i + 1 :]:
            if following.startswith("#") or following.startswith("*"):
                break

            match = INCLUDE.match(following)

            if match:
                includes.append(match.groups())

        paths = {path for path, _ in includes}

        if len(paths) != 1:
            sys.exit(f"{PAGE}:{i + 1}: expected the includes of one file")

        yield i, EXAMPLES / paths.pop(), [tag for _, tag in includes]


def source_for(path, tags):
    if path.parent.name == "troubleshooting":
        return compile_time_source(path)

    return run_time_source(path, tags)


def expected_message(path):
    text = path.read_text()
    match = re.search(r"//[ \t]*expected-error:[ \t]*([^\r\n]+)", text)

    if match:
        return match.group(1)

    match = re.search(r"// aborts with error message:\s*(?://\s*)?([^\n]+)", text)

    return re.escape(match.group(1).strip())


FETCHED = {}


def inline_site_includes(source):
    # Compiler Explorer fetches an include by URL in the browser, not on the
    # server, so a compilation requested through the API cannot.
    def fetch(match):
        url = match.group(1)

        if url not in FETCHED:
            with urllib.request.urlopen(url, timeout=60) as response:
                FETCHED[url] = response.read().decode()

        return FETCHED[url]

    return re.sub(rf"^#include <({re.escape(SITE)}[^>]+)>", fetch, source, flags=re.M)


def compile_on_ce(source, compiler, options):
    request = {
        "source": inline_site_includes(source),
        "options": {
            "userArguments": options,
            "compilerOptions": {"executorRequest": True},
            "filters": {"execute": True},
            "libraries": [{"id": "boost", "version": BOOST}],
        },
        "lang": "c++",
        "allowStoreCodeDebug": False,
    }
    http = urllib.request.Request(
        f"https://godbolt.org/api/compiler/{compiler}/compile",
        data=json.dumps(request).encode(),
        headers={"Content-Type": "application/json", "Accept": "application/json"},
    )

    with urllib.request.urlopen(http, timeout=120) as response:
        result = json.load(response)

    def text(key, within=result):
        return "\n".join(line["text"] for line in within.get(key, []))

    build = result.get("buildResult", {})

    return "\n".join(
        [text("stdout", build), text("stderr", build), text("stdout"), text("stderr")]
    )


def check(page_lines):
    failures = 0

    for _, path, tags in examples(page_lines):
        if path.name in NO_LINK:
            continue

        source = source_for(path, tags)
        compiler, options = compiler_for(source, path)
        output = compile_on_ce(source, compiler, options)
        # The regex works on bytes in CMake, where `.` would not match a
        # three-byte curly quote; here it works on characters.
        expected = expected_message(path)
        ok = re.search(expected, output) is not None
        failures += not ok
        print(f"{'ok  ' if ok else 'FAIL'} {path.name}", flush=True)

        if not ok:
            print(textwrap.indent(source, "    | "))
            print(textwrap.indent(output[-3000:], "    > "))

    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--print", type=Path, help="print the source for an example")
    args = parser.parse_args()
    page_lines = PAGE.read_text().splitlines()

    if args.check:
        sys.exit(1 if check(page_lines) else 0)

    for i, path, tags in examples(page_lines):
        if args.print and path.name == args.print.name:
            print(source_for(path, tags))
            return

        if path.name in NO_LINK:
            page_lines[i] = "*Example*"
            continue

        source = source_for(path, tags)
        compiler, options = compiler_for(source, path)
        url = clientstate_url(session_state(source, compiler, options))
        page_lines[i] = f"*Example* - link:{url}[Compiler Explorer^]"

    if not args.print:
        PAGE.write_text("\n".join(page_lines) + "\n")


if __name__ == "__main__":
    main()
