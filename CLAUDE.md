# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Boost.OpenMethod is a C++17 header-only library implementing open multi-methods (multiple dispatch). Unlike traditional virtual functions where dispatch occurs only on the first (`this`) parameter, open methods dispatch based on the runtime types of multiple arguments.

**Key Characteristics:**
- C++17 required
- Header-only library
- Part of the Boost ecosystem
- Supports both CMake and Boost.Build (b2)

## Build System

### CMake Build

**Basic build:**
```bash
mkdir build && cd build
cmake .. -DBOOST_SRC_DIR=/path/to/boost
cmake --build .
```

**Build with tests:**
```bash
cmake .. -DBOOST_OPENMETHOD_BUILD_TESTS=ON
cmake --build . --target tests
ctest
```

**Build with examples:**
```bash
cmake .. -DBOOST_OPENMETHOD_BUILD_TESTS=ON -DBOOST_OPENMETHOD_BUILD_EXAMPLES=ON
cmake --build .
```

**Important CMake options:**
- `BOOST_OPENMETHOD_BUILD_TESTS` - Enable tests (default: ON if root project)
- `BOOST_OPENMETHOD_BUILD_EXAMPLES` - Enable examples (requires tests enabled)
- `BOOST_OPENMETHOD_WARNINGS_AS_ERRORS` - Treat warnings as errors
- `BOOST_OPENMETHOD_ENABLE_REFLECTION` - Build the tests and examples with C++26 reflection; probes the compiler for the flags it needs (`-std=c++26 -freflection` on GCC 16) and fails the configure if it has none
- `BOOST_SRC_DIR` - Path to Boost source directory (default: `../..` or `$BOOST_SRC_DIR` env var)

### Boost.Build (b2)

**Build and test:**
```bash
b2 test
```

**Quick test (for CI):**
```bash
b2 test//quick
```

## Testing

### Running All Tests (CMake)
```bash
cd build
ctest
```

### Running a Single Test (CMake)
```bash
cd build
ctest -R test_dispatch  # Run specific test by name
# or directly
./boost_openmethod-test_dispatch
```

### Test Structure
- Test files: `test/test_*.cpp` - Standard unit tests using Boost.Test
- Compile-fail tests: `test/compile_fail_*.cpp` - Tests that should fail to compile
- Mixed build test: `test/mix_release_debug/` - Tests mixing debug/release builds
- Dynamic loading test: `test/dynamic_loading/` - Tests shared library support (requires Boost.DLL)
- 21+ test files covering dispatch, policies, virtual_ptr, RTTI, errors, etc.

### Compile-Fail Tests

Each `test/compile_fail_*.cpp` carries the diagnostic it expects in a marker comment right after
the license header:

```cpp
// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: repeated inheritance
```

`test/CMakeLists.txt` globs `compile_fail_*.cpp`, extracts the regex with
`MATCHES "//[ \t]*expected-error:[ \t]*([^\r\n]+)"`, and hands it to
`openmethod_compile_fail_test` as the test's `PASS_REGULAR_EXPRESSION`. Adding a test is dropping
in a file - no build-file edit. A file with no marker is a configure-time `FATAL_ERROR`, so a
silently unchecked test cannot slip through. The glob has no `CONFIGURE_DEPENDS` (matching the
`test_*.cpp` glob above it), so a new file needs a manual re-run of `cmake`.

Where the expected wording differs across compilers, match the common substring and say why in a
comment above the marker - `no matching` (clang/gcc "no matching function for call to" vs MSVC "no
matching overloaded function found"), `deleted function` (gcc "use of a", clang "call to", MSVC
"attempting to reference a").

**Do not let the compile-fail tests regenerate the build tree concurrently.** Each one runs
`cmake --build` on the shared tree as its test command, so when the tree is stale they all re-run
CMake at once and corrupt each other - on Ninja the losers die with `failed recompaction` /
`FAILED: build.ninja` before compiling anything, the expected diagnostic never appears, and the
test fails. It reproduces about two runs in three with `touch test/CMakeLists.txt; ctest -R
compile_fail -j32`, and not at all on an up-to-date tree, which is why it reads as random. The
empty `boost_openmethod-compile_fail_fixture` target plus `FIXTURES_SETUP`/`FIXTURES_REQUIRED`
does the regeneration once, before any of them.

**The Visual Studio `RESOURCE_LOCK` is still needed on top - do not remove it.** MSBuild is not
fixable by the fixture: every one of the 24 concurrent invocations walks the same project
dependency graph and stomps the same `.tlog`/`.lastbuildstate` files, not just `ZERO_CHECK`'s.
Measured with VS 18 2026 + `ctest -R compile_fail -j32`: without the lock, 7-13 of 24 fail on
*every* run; with it, 3/3 clean at 24s (serialized). The lock is scoped to the generator, so
**Windows already runs these fully parallel under `-G Ninja`** - 4/4 clean, 1.65s, same
`cl.exe`. That is the fast path on Windows; the generator cannot be defaulted from CMakeLists.txt
anyway (it is fixed before the file is read - only a preset or `CMAKE_GENERATOR` in the
environment can set it), and CI picks its own.

b2 is not affected - it compiles these sources as ordinary targets in its own dependency graph
instead of shelling out to a nested build (~40 runs at `-j32`/`-j64`/`-j128` are clean).

**b2 cannot check the message - do not try to make it.** `test/Jamfile` already loops
(`compile-fail $(src)`), but Boost.Build's `compile-fail` only inverts the exit status: the
`expect-failure-generator` sets `T_FLAG_FAIL_EXPECTED` and the engine flips OK/FAIL
(`tools/build/src/engine/make1.cpp:633`). The compiler's stderr is never captured - b2 writes a
stub `.o` containing the literal text `failed as expected` and a `.test` containing `passed`.
`capture-output` redirects output only for `run` tests, and `testing.jam` has no output-matching
rule at all. The workarounds - a compiler wrapper behind a custom toolset instance, or a
hand-rolled `make` action reimplementing the compile command per toolset - are not usable in Boost
CI. Message checking lives in CMake; the markers still document the intent under b2.

### Debug Mode Features
When building in Debug mode (`CMAKE_BUILD_TYPE=Debug`), runtime checks are automatically enabled via `BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS`.

## Architecture

### Layered Design

The library is structured in three conceptual layers:

1. **Preamble Layer** ([preamble.hpp](include/boost/openmethod/preamble.hpp))
   - Foundational types: `type_id`, `vptr_type`, `virtual_<T>`
   - Registry and policy framework
   - Error types: `not_initialized`, `bad_call`, `no_overrider`, `ambiguous_call`, etc.
   - No executable dispatch code
   - An internal foundation, *not* an entry point: every other header pulls it in, and
     nothing outside `include/` includes it directly. See *Overriding the default registry*
     below.

2. **Core API** ([core.hpp](include/boost/openmethod/core.hpp))
   - `method<Id, ReturnType(Parameters...), Registry>` - Method implementation
   - `virtual_ptr<Class, Registry>` - "Wide pointer" combining object pointer + v-table pointer
   - Dispatch algorithms: `resolve_uni()` (single dispatch), `resolve_multi_*()` (multiple dispatch)
   - Override registration via `override_impl<>`
   - Class registration via `use_classes<>`

3. **Macro Layer** ([macros.hpp](include/boost/openmethod/macros.hpp))
   - `BOOST_OPENMETHOD(name, params, return_type)` - Declare method
   - `BOOST_OPENMETHOD_OVERRIDE(name, params, return_type)` - Declare overrider
   - `BOOST_OPENMETHOD_CLASSES(classes...)` - Register class hierarchy
   - Generates static registrar objects for automatic registration

### Key Concepts

**Open Methods**: Functions where dispatch depends on runtime types of multiple parameters, not just the first.

**Virtual Parameters**: Parameters marked with `virtual_<T>` or `virtual_ptr<T>` that participate in dispatch.

**Registries**: Template-parameterized contexts holding classes, methods, and policies. Default: `boost::openmethod::default_registry`.

**Policies**: Pluggable components controlling behavior:
- `rtti` - Type identification (std_rtti, static_rtti, custom)
- `vptr` - V-table storage (vptr_vector, vptr_map)
- `type_hash` - Type ID hashing (fast_perfect_hash with hash_fn function object)
- `error_handler` - Error handling strategy (default_error_handler, throw_error_handler)
- `output` - Diagnostic output destination (stderr_output)
- `attributes` - Visibility/DLL decoration (dllexport, dllimport, local)

**Dispatch Mechanisms**:
- Single dispatch: Direct v-table lookup `vtbl[slot]`
- Multi-dispatch: Stride-based indexing through multi-dimensional dispatch tables

**virtual_ptr**: A "wide pointer" combining object pointer with v-table pointer
for efficient dispatch. Key for enabling dispatch on non-polymorphic or smart
pointer types.

### Component Interaction

```
User Code → Macros → Core API → Preamble → Policies
                                    ↓
                            Static Registration
```

Static initializers generated by macros call core API functions to register
classes, methods, and overriders. The `initialize()` function builds dispatch
tables before first use.

## Code Conventions

### Formatting
The project uses clang-format with an LLVM-based style:
- `AlignAfterOpenBracket: AlwaysBreak`
- `AllowShortFunctionsOnASingleLine: false`
- No short blocks, if statements, or loops on single lines

### Disassembly
Always show disassembly in **Intel syntax**, never AT&T (the toolchain's default
here). Add the flag to the invocation before pasting any output:
- `objdump -dC -M intel --no-show-raw-insn`
- `gcc -S -masm=intel` / `clang -S -masm=intel`

### Compiler Requirements
Tests require these C++17 features (checked by Boost.Build):
- auto nontype template params
- deduction guides
- fold expressions
- if constexpr
- inline variables
- structured bindings
- `<charconv>`, `<string_view>`, `<variant>` headers

### Documentation (AsciiDoc)

Prose lives in `doc/modules/ROOT/pages/*.adoc`; explanations belong there, not in comments inside
the example sources under `doc/modules/ROOT/examples/`, which are pulled into the rendered page
verbatim through `include::example$file.cpp[tag=content]`. Pages hard-wrap at ~79 columns and use
`cpp:name[]` for API names that have a reference page.

**Render the docs; do not just eyeball the `.adoc`.** `doc/build_antora.sh` (~2 min, writes the
gitignored `doc/html/`) is the only way to catch markup that is silently mis-parsed — asciidoctor
emits no warning for it.

**Side-by-side comparisons use a table with AsciiDoc cells.** The house shape is
`[cols="1,1"]` + `|===` with a header row (`interop_any.adoc`, `registries_and_policies.adoc`,
`shared_libraries.adoc`). A cell holding a *block* - a code listing, a nested list - must be
introduced with `a|`, not `|`. The `a` makes the cell content parsed as AsciiDoc; without it the
`[source,...]` / `----` markup renders literally, and asciidoctor says nothing.
`interop_type_erasure.adoc` compares two dispatch sequences that way:

```
[cols="1,1"]
|===
| `openmethod_vptr` | `virtual_any`

a|
[source,asm]
----
mov	rax, qword ptr [rdi + 32]
----

a|
[source,asm]
----
jmp	qword ptr [rax + 8*rcx]
----

|===
```

`|` starts a new cell, so cell content containing one must escape it as `\|`. Quick structural
check before rendering - both counts must be even, and every `[source,X]` must be followed by
`----`:

```bash
grep -c '^----$' doc/modules/ROOT/pages/<page>.adoc
grep -c '^|===$' doc/modules/ROOT/pages/<page>.adoc
```

**The backtick-apostrophe trap**: never write a possessive right after a code span. Asciidoctor
parses ``` `any`'s ``` as `` ` `` + `any` + the **`` `' `` curly-apostrophe shorthand**, which
consumes the *closing* backtick; the opening one is then left unmatched and pairs with the next
backtick in the same paragraph. Two things break at once — a literal `` ` `` appears in the output,
and the following code span loses its `<code>` formatting:

```
source:   is part of the `any`'s type - whereas the `typeid_of`-based dispatch above
rendered: is part of the any's type - whereas the `typeid_of-based dispatch above
```

Reword instead: "the reference types of the `any`", "separate from that of `default_registry`".
``{apos}`` also works and matches the house style (`shared_libraries.adoc` uses ``{empty}`` for
plurals: ``` `virtual_ptr`{empty}s ```), but rewording is safer and reads better. Before building:

```bash
grep -rn "\`'" doc/modules/ROOT/pages/*.adoc    # must return nothing
```

After building, no stray backticks should survive outside code blocks —
`grep -n '\`' doc/html/openmethod/<page>.html` should only hit backticks inside C++ comments.

### Reference (MrDocs) constraints

MrDocs turns the condition of an `enable_if_t` used as a defaulted template argument into a C++20
requires-clause by copying the **raw source text** spanning the condition expression - it does not
walk the expression tree (cppalliance/mrdocs#1016, which upstream cannot fix until MrDocs can
manipulate expression trees). `BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)`, which hides the
`detail::` qualification of the exposition-only traits, therefore disappears only when it sits
*before* the first token of the condition: it expands to nothing under `__MRDOCS__`, so it falls
outside the copied range. Anywhere inside the condition - after a `!`, after a `&&`, inside
parentheses - its name is printed verbatim, whatever shape the macro has (function-like as here,
object-like, or a bare `#ifndef __MRDOCS__` around `detail::`). That is why a constraint with two
occurrences renders the first one correctly and leaks the second.

Two rules keep the reference clean; the long-form version lives next to the macro definition in
`core.hpp`:

- **The macro must be the first thing in the condition.** Spell a negation
  `BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::) Trait<T> == false`, never `!... Trait<T>`.
- **One trait per condition.** When a constraint needs several, give each its own defaulted
  template parameter. MrDocs joins them with `&&`, in order, so the rendered clause is unchanged -
  and substitution short-circuits at the first failure, so a dependent type in a later condition
  (`typename Other::element_type`) is only formed once the earlier ones pass.

Between them these cover every constraint in the library, so **do not declare a member twice**, an
unqualified copy under `#ifdef __MRDOCS__` beside the real one. Only MrDocs ever compiles that
copy, so the two drift apart silently and the reference then documents a constraint the library
does not have. Most of the `#ifdef __MRDOCS__` blocks therefore *remove* declarations from the
reference (friends, deleted overloads) or describe something that has no real counterpart at all
(the `VirtualTraits` blueprint).

Only expressions are affected: types are printed from the AST, so the macro may appear anywhere in
one (`method::operator()` takes
`typename BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::) StripVirtualDecorator<Parameters>::type...` and
renders correctly).

**The C++26 reflection API is the one exception, and it is a forced one.** MrDocs' front-end does
not implement P2996, so the reference build runs at `-D CMAKE_CXX_STANDARD=20` and
`BOOST_OPENMETHOD_HAS_REFLECTION` is 0 there: every `#if BOOST_OPENMETHOD_HAS_REFLECTION` block is
invisible to it. `register_classes` is therefore restated as a documentation stub in an
`#elif defined(__MRDOCS__)` branch at the end of `core.hpp`. That stub names no `std::meta` type,
so nothing has to stand in for one; should a future stub need to spell one, forward-declare it
under `#ifdef __MRDOCS__` in `detail/reflection.hpp`, where `include-symbols` in `mrdocs.yml`
(`boost::openmethod::**`) keeps it out of the reference. Two rules contain the drift:

- **Each doc comment exists exactly once, on the stub.** The real declaration carries only
  `//! @see @ref <name> for documentation.`, as `inplace_vptr_derived` already does. Never copy
  a doc comment into both branches.
- **Prefer widening a guard to writing a stub** whenever the code parses as C++20: a declaration
  that is plain C++17 or C++20 can be guarded with
  `#if BOOST_OPENMETHOD_HAS_REFLECTION || defined(__MRDOCS__)`, and MrDocs then reads the real
  definition instead of a copy that can drift.

The `register_classes` stub is spelled `template<auto... Groups>` where the real one is
`template<detail::reflection_group... Groups>`. The stub drops the group type, which is an
implementation detail the reference has no reason to name - its braces are the only thing a
caller writes.

**A macro defined in both branches of an `#if` must carry its doc comment on the `#else` one.**
MrDocs compiles that branch, and a comment separated from its `#define` by preprocessor directives
is not attached to it - `BOOST_OPENMETHOD_REGISTER_CLASSES` silently produced no page at all until
its comment was moved down. Symptom to watch for: a `xref:reference:<NAME>.adoc` that renders as a
literal `href="#reference:<NAME>.adoc"`.

### Doc-comment markup traps

MrDocs parses `//!` comments as Markdown plus Doxygen commands, then emits AsciiDoc. Four shapes
mis-render silently; all four were found by rendering, none by reading the source:

- **A line starting with `- ` becomes a list item.** House style uses ` - ` as an em-dash, which is
  fine mid-line but starts a stray bullet at the head of one. Rewrap so the dash never begins a
  line, or reword to a semicolon.
- **`@ref` inside `**bold**` breaks the span**, leaving literal `**` in the output. Bold plain text
  only: `**Options**: at most one @ref ... value`, not `**One @ref ... value**`.
- **An inline `` `^^::` `` loses both carets** and renders as `::`. Escape the first one -
  `` `\^^::` `` - which comes through as `^^::`. Only this spelling is affected; `` `^^app` `` and
  `^^::` inside an `@code` block are fine.
- **`@attention` is dropped silently**, paragraph and all: no admonition, no text, no warning.
  MrDocs knows `@note` and `@warning`, which render as NOTE and WARNING blocks wherever they sit in
  the description - the first paragraph after the brief included. Use one of those.

An `xref:reference:<name>.adoc` path works only for macros, which MrDocs puts at the top level.
A namespace-scoped symbol lives under `reference/boost/openmethod/`, so link it with
`cpp:<name>[]`, which resolves wherever the page ends up.

After a doc build, `grep -rl MRDOCS doc/html/` must return nothing.

**An `@ref` whose target MrDocs cannot find is not an error.** It emits the name as plain text,
which looks exactly like a deliberate code span in the rendered page - so a wrong target survives
indefinitely, and one that names a *real but wrong* symbol renders as a confident link to the
wrong page. Nothing in the build catches either. After a doc build, cross-check every `@ref`
target in the headers against the labels MrDocs actually turned into links, in the generated
AsciiDoc under `~/.cache/antora/reference-collector/reference/openmethod/main`:

```bash
grep -rho '@ref [A-Za-z_][A-Za-z0-9_:]*' include/ | sort -u
grep -rho 'xref:[^[]*\[`\?[^]`]*' ~/.cache/antora/reference-collector/reference/openmethod/main
```

A target that is not a library symbol - `abort`, `std::variant`, a named requirement such as
`AssociativeContainer` - is not an `@ref` at all: use a code span, or a Markdown link for a named
requirement.

## Common Development Patterns

### Working with Shared Libraries / DLL Support

A registry's entire mutable state - the class/method/overrider lists plus every stateful policy's
`state` - lives in one variable, `registry_state<Registry>::st`. Sharing a registry across modules
means sharing that one symbol. Three macros do it, each taking the registry as an argument and
emitting fully qualified names, so callers never open `namespace boost::openmethod`:

```cpp
BOOST_OPENMETHOD_IMPORT_REGISTRY(R);       // header, every TU of a client module
BOOST_OPENMETHOD_EXPORT_REGISTRY(R);       // header, every TU of the owning module
BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(R);  // exactly one .cpp of the owning module
```

Methods need no decoration: method objects are *consolidated* across modules at `initialize()`
time, not shared through a symbol.

**Do not hand-write the underlying explicit instantiations.** They are not portable, and each
spelling compiles silently on one platform while failing on the other. On declspec platforms
`__declspec(dllexport)` and `extern` are incompatible on an explicit instantiation (MSVC warning
C4910), so `EXPORT` expands to nothing and `INSTANTIATE` carries the attribute; on ELF and Mach-O
the attribute must be on the *declaration*, and repeating it on the definition is an error on GCC,
so `EXPORT` carries it and `INSTANTIATE` carries none. The macros branch on `BOOST_HAS_DECLSPEC`.

**`EXPORT` is load-bearing on ELF, not documentation.** Under `-fvisibility=hidden` (e.g. the Boost
super-project's `BoostRoot.cmake`) an owner TU with neither `EXPORT` nor `INSTANTIATE` instantiates
the state implicitly as a COMDAT; ELF merges COMDATs at the *most restrictive* visibility, so the
merged symbol goes module-local, the module exports nothing, and clients fail to link.
`test/implicit_shared_libraries/custom_registry/lib2.cpp` exists solely to guard that path.

**Registries are structs deriving from `registry<Policy...>`, never aliases - do not "simplify"
this.** The short struct name keeps mangled names short for everything keyed on the registry
(methods, virtual_ptrs, `static_vptr`, registrars); an alias would expand the full policy list into
all of them. That is also why the state is keyed on `Registry::registry_type` - the `registry<...>`
base - and never on the derived struct. `registry_state` is likewise a deliberately thin,
function-free class: that is the only shape MSVC will export whole and import via `extern template`.

One self-contained example per subdirectory of `doc/modules/ROOT/examples/shared_libs/`; tests in
`test/dynamic_loading/` (whose `registry_state_id()` is compared across modules to prove the state
is a single symbol) and `test/implicit_shared_libraries/`.

### Overriding the default registry

The registry is **forward-declared** before `core.hpp` - or any header that includes it,
`<boost/openmethod.hpp>` among them - and **defined after**. Do not reintroduce the old
"include a pre-core header, define the registry, `#define`, then include" idiom:
`preamble.hpp` and `default_registry.hpp` should appear nowhere outside `include/`. Nothing
enforces that automatically; check it by hand when touching this area:

```bash
git grep -n "openmethod/preamble.hpp\|openmethod/default_registry.hpp" -- doc test
```

```cpp
struct my_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY my_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/vptr_map.hpp>   // extra policies, any order
#include <boost/openmethod/initialize.hpp>          // the TU that calls initialize()

struct my_registry
    : boost::openmethod::default_registry::with<
          boost::openmethod::policies::vptr_map<>> {};
```

A registry the library provides needs no declaration at all - `default_registry`,
`indirect_registry` and the five stock policies are complete as soon as
`<boost/openmethod.hpp>` has been included, because `core.hpp` includes
`default_registry.hpp` *before* it tests the macro:

```cpp
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY boost::openmethod::indirect_registry
#include <boost/openmethod.hpp>
```

This works because every use of the macro in the headers is a name-only context - a default
template argument, an alias, a deduction guide, or a member typedef inside a template. Three
rules:

- The registry must be **complete** before the first construct that instantiates it: a
  `BOOST_OPENMETHOD*` macro, `use_classes`, `method`, `virtual_ptr`, `inplace_vptr_base`,
  `initialize()`, or `BOOST_OPENMETHOD_{IMPORT,EXPORT,INSTANTIATE}_REGISTRY`. Violating this is
  a hard error (`incomplete type ... used in nested name specifier`, from `use_class_aux` in
  `core.hpp`) - never silent.
- It must name a **class**, declared with the same class-key (`struct`) as the definition. A
  `class`/`struct` mismatch is MSVC C4099, an error under the suite's `/W4 /WX`.
- **Qualify** the name if it could also be found in `namespace boost::openmethod`. The macro
  is expanded inside that namespace, so `#define BOOST_OPENMETHOD_DEFAULT_REGISTRY registry`
  binds to `boost::openmethod::registry` rather than the global one. That one is loud -
  `missing template arguments` - but a name that *does* resolve would bind to the wrong type
  silently. `::registry` works.

`test/CMakeLists.txt` withholds the shared PCH from any `test_*.cpp` that overrides the
registry - a force-included PCH would still precede the `#define`. It detects them by scanning
for the token `BOOST_OPENMETHOD_DEFAULT_REGISTRY` **or** for an include of a header that
carries the override on the file's behalf (`test_capture_errors.hpp`). Add another such header
and the scan has to learn about it: miss one and the file still compiles, binds to
`default_registry`, and fails at run time.

### Flattened headers for Compiler Explorer

`dev/flatten.py` rewrites every public header into a self-sufficient file under `flat/`; the
`antora` CI job regenerates them into the Pages artifact, so they are served from the root of the
site and a CE example can include them by URL. CE fetches those includes client-side, which is why
the host has to send CORS headers - GitHub Pages does, `access-control-allow-origin: *`.

**Upstream deploys from `develop` only; the fork deploys from any branch.** Any other branch would
clobber the site, and the fork is where a branch is tried out before it is merged - which is why
the `Deploy to GitHub Pages` step tests the repository *and*, for boostorg, the ref. The two sites
are `https://boostorg.github.io/openmethod` and `https://jll63.github.io/openmethod`, and CI
derives `--base-url` from `github.repository_owner` rather than hardcoding either: that URL is
baked into every generated header's banner and guard-check messages, so it has to name the site
the artifact is about to be deployed to. The script's own default is the canonical site,
boostorg.

The point of the exercise is that a CE example's include list matches a local one line for line:

```cpp
#include <https://jll63.github.io/openmethod/boost/openmethod.hpp>
#include <https://jll63.github.io/openmethod/boost/openmethod/initialize.hpp>
```

`boost/openmethod.hpp` is the root and carries its whole closure. **Every other header carries
only what the root does not provide** - its `detail/` headers, and `interop/virtual_any.hpp`,
which nothing includes directly. A dependency the root *does* provide becomes a guard check:

```cpp
#ifndef BOOST_OPENMETHOD_CORE_HPP
#error "<boost/openmethod/initialize.hpp>: #include <.../boost/openmethod.hpp> first"
#endif
```

so a missing root fails on one line instead of a wall of undeclared identifiers. Guard names are
read from the header being flattened, never hardcoded - several are legacy and do not match their
path (`initialize.hpp` is `BOOST_OPENMETHOD_COMPILER_HPP`, `preamble.hpp` is
`BOOST_OPENMETHOD_REGISTRY_HPP`, `policies/static_rtti.hpp` is
`BOOST_OPENMETHOD_POLICY_MINIMAL_RTTI_HPP`).

The rewriting is line-oriented, which holds only because no `#include <boost/openmethod/...>` in
the tree sits inside an `#if`. A `//!` doc comment containing one is left alone - the regex is
anchored at the start of the line.

`dev/check-flat.sh` (the `flat-headers` CI job, and `BOOST_SRC_DIR=... dev/check-flat.sh` locally)
compiles each generated header after the root, checks that each one *fails* on its own, and builds
and runs the `ce/*.cpp` examples against the generated tree. Those examples are also ordinary
tests in the CMake build (`ce/CMakeLists.txt`), so they cannot rot silently.

### Custom RTTI
When `<typeinfo>` is unavailable or insufficient, use static_rtti or implement custom RTTI. See `doc/modules/ROOT/examples/custom_rtti/` and policies in `include/boost/openmethod/policies/`.

### Multiple Registries
Registries are completely independent. Use separate registries to:
- Isolate method sets
- Apply different policies to different method families
- Enable coexistence of incompatible configurations

Registry type must be specified consistently across related methods and classes.

## File Organization

- `include/boost/openmethod/` - Public headers
  - `core.hpp`, `macros.hpp`, `preamble.hpp` - Main headers
  - `initialize.hpp` - Dispatch table construction
  - `default_registry.hpp` - Default policy configuration
  - `detail/` - Internal implementation details
  - `policies/` - Policy implementations
  - `interop/` - Interoperability with other systems
- `test/` - Unit tests and compile-fail tests
- `doc/modules/ROOT/examples/` - Example programs
- `doc/modules/ROOT/pages/` - AsciiDoc documentation

## Dependencies (Boost Libraries)

Required:
- Boost.Assert
- Boost.Config
- Boost.Core
- Boost.DynamicBitset
- Boost.MP11 (metaprogramming)
- Boost.Preprocessor

For testing:
- Boost.Test
- Boost.SmartPtr

For examples:
- Boost.DLL (shared library examples)

## Development Workflow

1. Make changes to headers in `include/boost/openmethod/`
2. Build tests: `cmake --build build --target tests`
3. Run tests: `cd build && ctest`
4. For changes affecting examples: enable `BOOST_OPENMETHOD_BUILD_EXAMPLES`
5. Submit PRs against the `develop` branch

### Posting in public on the maintainer's behalf

Anything published under the maintainer's account - a GitHub issue or comment, a PR body, a
mailing-list or forum post - must **identify its author in the text itself**, on the first line:

```
*(Written by Claude Code, on behalf of @jll63.)*
```

The account is a person's, and readers reasonably assume a human wrote what it says; an unlabelled
post misrepresents who is speaking, and a signature in the tool call or the commit trailer is not
visible to them. Ask before posting anyway - the attribution line does not substitute for consent.

## Important Implementation Details

### Static Registration
Classes, methods, and overriders register automatically via static constructors. This happens before `main()`. The `initialize()` function must be called before first method invocation to build dispatch tables.

### Dispatch Table Construction
The `initialize()` function:
1. Collects registered classes and overriders
2. Builds class hierarchy using provided inheritance relationships
3. Constructs dispatch tables using perfect hashing
4. Validates configuration (in debug mode or with runtime_checks policy)

### Virtual Pointer Mechanics
`virtual_ptr<T>` stores both object pointer and v-table pointer. It can be constructed from:
- Raw pointers (requires prior `use_classes` registration)
- Smart pointers (std::unique_ptr, std::shared_ptr, boost::intrusive_ptr)
- References
- Other virtual_ptr instances

The v-table pointer enables O(1) method dispatch.

### Policy State Pattern

Stateful policies keep their data in a nested `struct state` inside `fn<Registry>` and reach it
through the registry's shared state. `registry_state_type` automatically gathers every
policy's `state` into its `policies` tuple, so a policy's state is part of the single shared
`registry_state<Registry>::st` variable — no per-policy DLL decoration, `MAKE_STATICS` macro, or
`id()` function is needed (those were all removed).

To add state to a policy's `fn<Registry>`:

1. **Declare a public `struct state`** with the data members:
   ```cpp
   struct state {
       detail::hash_fn fn;
       std::vector<type_id> control;
   };
   ```
2. **Add a private accessor** returning this policy's slot in the registry's tuple:
   ```cpp
   static auto& st() {
       return Registry::template state<fast_perfect_hash>();
   }
   ```
   `Registry::state<P>()` (a templated overload of `Registry::state()`, alongside the
   non-template overload that returns the whole `registry_state_type<Registry>`) returns
   `P::fn<Registry>::state&` via `detail::get` (get-by-type) on the `policies` tuple.
3. **Use `st()`** wherever the state is read or written: `st().fn`, `st().control`, etc. (name it
   `st()` so it does not shadow the `state` type).

`registry_state_type` (in `preamble.hpp`) builds its `policies` tuple by instantiating each
policy's `fn<Registry>`, keeping those that have a nested `state` (`detail::has_policy_state`), and
storing one of each:
```cpp
mp_apply<detail::tuple,
    mp_transform<policy_state_t,
        mp_filter<has_policy_state,
            mp_transform_q<policy_fn_q<Registry>, Registry::policy_list>>>>
```
`detail::tuple` (defined in `preamble.hpp`) is a minimal tuple used instead of `std::tuple` —
which is very expensive to instantiate with MSVC — for the policy-state tuple, the `use_classes`
registrar tuple, and `method::override::impl`. It holds each element in a `tuple_element<T>` base
class (flat multiple inheritance, O(1) instantiation depth); `detail::get` retrieves an element by
type via a base-class cast. Element types must therefore be unique: lists that may contain
duplicates (`use_classes` with a class listed twice, `override<f, f>`) are deduplicated with
`mp_unique` before instantiating the tuple. The `initialize()`/`finalize()` *options* tuple
deliberately remains `std::tuple`: it is a documented policy-API signature.

Only the registry itself has an `id()` (returning `&state().classes`); the dynamic_loading test
uses it directly to compare the shared state address across modules.
