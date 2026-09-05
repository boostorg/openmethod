// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_DETAIL_REFLECTION_HPP
#define BOOST_OPENMETHOD_DETAIL_REFLECTION_HPP

// Detect C++26 reflection (P2996). Both the language feature (the `^^`
// operator) and the library (`std::meta`) are required.
#if !defined(BOOST_OPENMETHOD_HAS_REFLECTION)
#if defined(__cpp_impl_reflection) && __has_include(<meta>)
#include <meta>
#if defined(__cpp_lib_reflection)
#define BOOST_OPENMETHOD_HAS_REFLECTION 1
#endif
#endif
#endif

#if !defined(BOOST_OPENMETHOD_HAS_REFLECTION)
#define BOOST_OPENMETHOD_HAS_REFLECTION 0
#endif

#if BOOST_OPENMETHOD_HAS_REFLECTION

#include <cstddef>
#include <type_traits>
#include <vector>

namespace boost::openmethod::detail {

// One argument of `register_classes`: a group of reflections, written in
// braces - or, for a group of one, as the reflection itself, which the
// converting constructor turns into a group of one.
//
// The constructor is constrained, so that it cannot be picked over the copy
// constructor, and `consteval`, as `std::meta::info` is a consteval-only type.
//
// `Size` is deduced from the number of items; the array is never empty, as
// `{}` must produce a valid type too.
template<std::size_t Size>
struct reflection_group {
    std::meta::info items[Size ? Size : 1]{};
    static constexpr std::size_t size = Size;

    consteval reflection_group() = default;

    template<class... T>
        requires(... && std::is_same_v<T, std::meta::info>)
    consteval reflection_group(T... items_) : items{items_...} {
    }
};

template<class... T>
reflection_group(T...) -> reflection_group<sizeof...(T)>;
reflection_group() -> reflection_group<0>;

// =============================================================================
// vectors of reflections

consteval auto contains(
    const std::vector<std::meta::info>& types, std::meta::info type) -> bool {
    for (auto seen : types) {
        if (seen == type) {
            return true;
        }
    }

    return false;
}

consteval void push_unique(
    std::vector<std::meta::info>& types, std::meta::info type) {
    if (!contains(types, type)) {
        types.push_back(type);
    }
}

// =============================================================================
// namespaces

// True if `ns` is a namespace that a recursive scan does not enter: `std` and
// `boost`. Walking them would cost a great deal and find nothing: a method
// cannot be declared on a class the program has never heard of. The nested
// namespaces - `std::chrono`, `boost::mp11`, the inline versioning ones - are
// reached only through their parent, so they are left out with it. The
// exclusion applies only to recursion: a namespace listed explicitly is always
// scanned, which is how a class in `std` or `boost` is registered.
consteval auto is_excluded_namespace(std::meta::info ns) -> bool {
    return ns == ^^::std || ns == ^^::boost;
}

// True if the recursive scan of one of `namespaces` enters `ns` from above:
// `ns` is nested in one of them, and neither it nor a namespace in between is
// excluded. A listed namespace is entered whether it is excluded or not, so
// the question is asked of the ancestors of `ns` only, never of `ns` itself.
consteval auto scan_reaches(
    std::meta::info ns, const std::vector<std::meta::info>& namespaces)
    -> bool {
    auto scope = ns;

    while (!is_excluded_namespace(scope) && std::meta::has_parent(scope)) {
        scope = std::meta::parent_of(scope);

        if (contains(namespaces, scope)) {
            return true;
        }
    }

    return false;
}

// True if a scan of `namespaces` declares `type`: it enters the namespace
// that holds it, and finds it there, or in a class it walks into from there.
// Answered from the position of `type` alone, so that it costs the depth of
// its nesting, not a search of everything the scan found. What it says no to
// is found only by other means - through an alias that names it, in the base
// list of a class the scan does declare, or by being listed - or not at all.
consteval auto scan_declares(
    std::meta::info type, const std::vector<std::meta::info>& namespaces)
    -> bool {
    auto scope = type;

    while (true) {
        // A specialization of a class template is a member of no scope, and
        // neither is anything nested in one: `members_of` yields the template.
        if (std::meta::has_template_arguments(scope)) {
            return false;
        }

        // A local class has no parent.
        if (!std::meta::has_parent(scope)) {
            return false;
        }

        scope = std::meta::parent_of(scope);

        if (std::meta::is_namespace(scope)) {
            return contains(namespaces, scope) ||
                scan_reaches(scope, namespaces);
        }

        // A class nested in a class: the scan walks into every class it
        // declares, so keep climbing.
        if (!std::meta::is_class_type(scope)) {
            return false;
        }
    }
}

// =============================================================================
// base classes

// A class reached by `walk_bases`.
struct base_record {
    std::meta::info type;
    // Arrived at by more than one public path: a candidate for ambiguity,
    // which `is_dispatchable_base` decides.
    bool repeated = false;
    // A root, or a class with a public base that reaches one.
    bool reaches_root = false;
};

consteval auto find(const std::vector<base_record>& bases, std::meta::info type)
    -> std::size_t {
    for (std::size_t index = 0; index != bases.size(); ++index) {
        if (bases[index].type == type) {
            return index;
        }
    }

    return bases.size();
}

// Mark the class at `index`, and every class above it, as repeated: the walk
// has arrived at it again, so it and its ancestors are reached through one
// more path than before. A class already marked has had its ancestors marked
// too, which bounds the work.
consteval void mark_repeated(
    std::vector<base_record>& bases, std::size_t index) {
    if (bases[index].repeated) {
        return;
    }

    bases[index].repeated = true;

    // The range is held in a named local instead of being left to the
    // range-for to lifetime-extend, to work around GCC PR124645/PR124646.
    // r16-8235 marks a lifetime-extended temporary of consteval-only type -
    // which `vector<meta::info>` is - `DECL_EXTERNAL` unconditionally, and the
    // constant evaluator then hands every frame of a recursive call the same
    // object: the inner call destroys the vector the outer call is still
    // walking, and the loop fails with "accessing '<anonymous>' outside its
    // lifetime". A plain automatic variable is not an extended-ref temporary,
    // so each frame gets its own. Every recursive function here does the
    // same.
    //
    // Fixed upstream in r16-8430. Drop this once no supported toolchain sits
    // in between - Ubuntu 26.04, which Boost.CI uses for the C++26 leg, ships
    // 16-20260322 (r16-8246) and does.
    auto specifiers = std::meta::bases_of(
        bases[index].type, std::meta::access_context::unchecked());

    for (auto specifier : specifiers) {
        if (std::meta::is_public(specifier)) {
            // Every public base of a class the walk has been through is in
            // `bases` already.
            mark_repeated(
                bases,
                find(bases, std::meta::dealias(std::meta::type_of(specifier))));
        }
    }
}

// Append `type`, and every class reachable from it through public
// inheritance, to `bases` - each once, `type` first. Only public base
// specifiers are followed: a class reached solely through a private or
// protected base cannot take part in dispatch, because the conversion is not
// available to the library. Returns whether `type` reaches one of `roots`: it
// is one, or a public base of it does.
//
// A class the walk arrives at again is marked `repeated`, and so is everything
// above it, as `mark_repeated` explains. The mark is a suspicion, not a
// verdict: a virtual base is one subobject however many paths reach it. That
// is for `is_dispatchable_base` to decide, and it costs a template
// instantiation, so it is only asked about the classes marked here. A
// hierarchy without repeated inheritance instantiates nothing.
consteval auto walk_bases(
    std::meta::info type, const std::vector<std::meta::info>& roots,
    std::vector<base_record>& bases) -> bool {
    auto index = find(bases, type);

    if (index != bases.size()) {
        mark_repeated(bases, index);

        return bases[index].reaches_root;
    }

    bases.push_back({type, false, contains(roots, type)});

    // A named local, for the reason given in `mark_repeated`.
    auto specifiers =
        std::meta::bases_of(type, std::meta::access_context::unchecked());

    for (auto specifier : specifiers) {
        if (std::meta::is_public(specifier)) {
            auto base = std::meta::dealias(std::meta::type_of(specifier));

            // Walk every base, whatever the ones before said: the walk must
            // be complete for `repeated` to mean anything.
            if (walk_bases(base, roots, bases)) {
                bases[index].reaches_root = true;
            }
        }
    }

    return bases[index].reaches_root;
}

// True if a `derived` can be converted to a `base` - that is, if `base` is a
// public base class of `derived` and naming it is unambiguous. Exact, as it
// asks the language: a virtual base is one subobject however many paths reach
// it. It costs a template instantiation, so ask it only where the answer can
// differ from what the walk above already knows.
consteval auto is_dispatchable_base(
    std::meta::info derived, std::meta::info base) -> bool {
    return std::meta::is_convertible_type(
        std::meta::add_pointer(derived), std::meta::add_pointer(base));
}

// =============================================================================
// scan

// What a scan of a set of namespaces yields, before any registry is looked at.
struct reflected_scan {
    // The namespaces scanned - `^^::` when none was listed - and the classes
    // listed alongside them, with cv-qualification removed.
    std::vector<std::meta::info> namespaces;
    std::vector<std::meta::info> listed;
    // The specializations of the method template that the scanned scopes
    // name, in a `using` declaration or as the type of a registrar object.
    std::vector<std::meta::info> methods;
    // The complete classes the scanned scopes declare, that have at least one
    // base class. Each appears once, without a search: the scan walks a scope
    // once, and a declaration belongs to one scope. A class with no base
    // class is left out: it could only be registered as a root, and roots are
    // not found by the scan.
    std::vector<std::meta::info> declared;
    // The classes an alias names that the scan does not declare - in a
    // namespace it does not enter, or nested in a class template
    // specialization - with the same restrictions. Searched, as a class may
    // be aliased more than once.
    std::vector<std::meta::info> aliased;
};

// The specialization of `Template` that a variable `member` names, if it is a
// registrar: a variable of a type nested in the specialization, which is what
// `BOOST_OPENMETHOD_OVERRIDE` creates, and what a hand-written
// `method<...>::override<...>` is. Returns an invalid reflection otherwise.
consteval auto specialization_named_by_variable(
    std::meta::info member, std::meta::info Template) -> std::meta::info {
    auto enclosing = std::meta::type_of(member);

    if (std::meta::has_parent(enclosing)) {
        auto parent = std::meta::parent_of(enclosing);

        if (std::meta::is_type(parent) &&
            std::meta::has_template_arguments(parent) &&
            std::meta::template_of(parent) == Template) {
            return parent;
        }
    }

    return std::meta::info();
}

// True if `type`, a complete class, has at least one base class.
consteval auto has_bases(std::meta::info type) -> bool {
    return !std::meta::bases_of(type, std::meta::access_context::unchecked())
                .empty();
}

// Walk `scope` - a namespace, or a class - and the namespaces and classes
// nested in it, filling in `scan`. Nothing else is retained: the scan of a
// large namespace must not build a list of everything in it.
//
// The scan never goes through an alias. A namespace alias may name an
// enclosing namespace, which would loop forever, or a namespace the scan
// stays out of; the namespace it names is reached from its parent anyway, or
// deliberately not. A type alias registers the class it names, but the scan
// does not walk into it either: a `using` for `std::string` would drag the
// whole of `basic_string` in behind it. Nor does it ask whether an aliased
// specialization of a class template is complete: the question instantiates
// the specialization, and a `using Edge = std::pair<Node, Node>` over a `Node`
// defined elsewhere would fail to compile - in a translation unit that is
// valid C++17. A specialization is registered by being listed, by a method
// dispatching on it, or by sitting in the base list of a class the scan finds.
consteval void scan_scope(
    std::meta::info scope, std::meta::info Template, reflected_scan& scan) {
    // A named local, for the reason given in `mark_repeated`.
    auto members =
        std::meta::members_of(scope, std::meta::access_context::unchecked());

    for (auto member : members) {
        if (std::meta::is_namespace(member)) {
            if (!std::meta::is_namespace_alias(member) &&
                !is_excluded_namespace(member)) {
                scan_scope(member, Template, scan);
            }

            continue;
        }

        if (std::meta::is_type(member)) {
            if (std::meta::is_type_alias(member)) {
                // An alias may add cv-qualification - `using CDog = const
                // Dog` - which is not a distinct class to register: the
                // registry would hold `Dog` and `const Dog` as two lattice
                // nodes, each with its own hash slot and dispatch table row.
                auto type = std::meta::remove_cv(std::meta::dealias(member));

                if (std::meta::has_template_arguments(type)) {
                    // A method is found through the alias `BOOST_OPENMETHOD`
                    // declares for it, or one written by hand.
                    if (std::meta::template_of(type) == Template) {
                        push_unique(scan.methods, type);
                    }
                } else if (
                    std::meta::is_class_type(type) &&
                    std::meta::is_complete_type(type) &&
                    !scan_declares(type, scan.namespaces) && has_bases(type)) {
                    push_unique(scan.aliased, type);
                }

                continue;
            }

            // A class the scope declares. `parent_of` says so; a member class
            // whose parent is elsewhere, such as one brought in by a
            // using-declaration, is left to the scope that declares it.
            if (std::meta::is_class_type(member) &&
                std::meta::is_complete_type(member) &&
                std::meta::has_parent(member) &&
                std::meta::parent_of(member) == scope) {
                if (has_bases(member)) {
                    scan.declared.push_back(member);
                }

                scan_scope(member, Template, scan);
            }

            continue;
        }

        if (std::meta::is_variable(member)) {
            auto specialization =
                specialization_named_by_variable(member, Template);

            if (specialization != std::meta::info()) {
                push_unique(scan.methods, specialization);
            }
        }
    }
}

// Scan `namespaces` - or the global namespace, if the list is empty - for
// the specializations of `Template` and the classes described in
// `reflected_scan`. A listed namespace nested in another listed one is
// reached by the recursion, and is not scanned again.
consteval auto scan_namespaces(
    std::vector<std::meta::info> namespaces,
    std::vector<std::meta::info> listed, std::meta::info Template)
    -> reflected_scan {
    reflected_scan scan;

    if (namespaces.empty()) {
        namespaces.push_back(^^::);
    }

    scan.namespaces = namespaces;
    scan.listed = listed;

    for (auto ns : namespaces) {
        if (!scan_reaches(ns, namespaces)) {
            scan_scope(ns, Template, scan);
        }
    }

    return scan;
}

} // namespace boost::openmethod::detail

#endif

#endif
