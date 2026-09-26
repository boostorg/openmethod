// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_COMPILER_HPP
#define BOOST_OPENMETHOD_COMPILER_HPP

#include <boost/openmethod/core.hpp>
#include <boost/openmethod/detail/ostdstream.hpp>

#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/assert.hpp>
#include <boost/dynamic_bitset.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4456)
#pragma warning(disable : 4458)
#pragma warning(disable : 4702) // unreachable code
#endif

namespace boost::openmethod {

namespace detail {

template<class Reports, class Facets, typename = void>
struct aggregate_reports;

template<class... Reports, class Facet, class... MoreFacets>
struct aggregate_reports<
    mp11::mp_list<Reports...>, mp11::mp_list<Facet, MoreFacets...>,
    std::void_t<typename Facet::report>> {
    using type = typename aggregate_reports<
        mp11::mp_list<Reports..., typename Facet::report>,
        mp11::mp_list<MoreFacets...>>::type;
};

template<class... Reports, class Facet, class... MoreFacets, typename Void>
struct aggregate_reports<
    mp11::mp_list<Reports...>, mp11::mp_list<Facet, MoreFacets...>, Void> {
    using type = typename aggregate_reports<
        mp11::mp_list<Reports...>, mp11::mp_list<MoreFacets...>>::type;
};

template<class... Reports, typename Void>
struct aggregate_reports<mp11::mp_list<Reports...>, mp11::mp_list<>, Void> {
    struct type : Reports... {};
};

// Policy initialization helpers

#ifdef BOOST_MSVC
#if BOOST_MSVC > 1951
#pragma message(                                                               \
    "boost/openmethod: this MSVC version is newer than 19.51, the last "       \
    "one confirmed to need the has_initialize/has_finalize SFINAE "            \
    "workaround below (wrong instantiation of the enclosing "                  \
    "boost::openmethod::initialize/finalize function templates during "        \
    "member-call SFINAE). Check whether this compiler still has the bug; "     \
    "if not, both workarounds (and this macro) can be removed.")
#endif
// `initialize` and `finalize` are also enclosing-namespace
// boost::openmethod::initialize/finalize function templates, so probing them
// with a call expression (the generic BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN
// form) is unsafe here: while evaluating decltype(T::FN(...)) for a T that
// has no such member, MSVC (19.44 and later, still present in 19.51) wrongly
// instantiates the body of the enclosing function template (for its auto
// return type), which re-enters this probe with new types - unbounded
// recursion. 19.44 stops with C1202/C7752; 19.51 exhausts all memory
// (C1060) instead. Take the address of a specialization instead: it only
// involves T's members.
#define BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN_MSVC_SAFE(FN)                    \
    template<typename, class, class...>                                        \
    struct BOOST_PP_CAT(has_, BOOST_PP_CAT(FN, _aux)) : std::false_type {};    \
    template<class T, class... Args>                                           \
    struct BOOST_PP_CAT(has_, BOOST_PP_CAT(FN, _aux))<                         \
        std::void_t<decltype(&T::template FN<std::decay_t<Args>...>)>, T,      \
        Args...> : std::true_type {};                                          \
    template<class T, class... Args>                                           \
    constexpr bool BOOST_PP_CAT(has_, FN) =                                    \
        BOOST_PP_CAT(has_, BOOST_PP_CAT(FN, _aux))<void, T, Args...>::value
#else
#define BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN_MSVC_SAFE(FN)                    \
    BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN(FN)
#endif

BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN_MSVC_SAFE(initialize);

// Call initialize on a single policy if it has the function
template<typename Policy, typename Registry, typename Context, typename Options>
void initialize_policy(const Context& ctx, const Options& options) {
    using PolicyFn = typename Policy::template fn<Registry>;
    if constexpr (has_initialize<PolicyFn, const Context&, const Options&>) {
        PolicyFn::initialize(ctx, options);
    }
}

template<typename Registry, typename Policies = typename Registry::policy_list>
struct initialize_policies;

template<typename Registry, typename... Policies>
struct initialize_policies<Registry, mp11::mp_list<Policies...>> {
    template<typename Context, typename Options>
    static void fn(const Context& ctx, const Options& options) {
        // Policies are initialized in `policy_list` order (left to right). A
        // policy that depends on another policy having been initialized (e.g.
        // vptr_vector reading a type_hash policy's state) must therefore appear
        // *after* its dependency in the list. finalize() runs in reverse order.
        (initialize_policy<Policies, Registry>(ctx, options), ...);
    }
};

// Selects the policies whose state the transaction has to save: those that
// have a `state`, *and* whose `initialize` will actually be called for this
// Context and Options - the same test initialize_policy makes, so the two
// cannot disagree about which policies run.
template<class Context, class Options>
struct policy_state_is_volatile_q {
    template<class PolicyFn>
    using fn = mp11::mp_bool<
        has_policy_state<PolicyFn>::value &&
        has_initialize<PolicyFn, const Context&, const Options&>>;
};

// Saves the policies' states on construction, and puts them back on
// destruction unless commit() was called - so a policy's `initialize` that
// throws, after itself or another policy has already written to shared
// state, leaves the registry as it was. The registry's mutable state is one
// variable, registry_state<Registry>::st. Its other members need no saving:
// initialize() only reads the class and method lists, and write_global_data()
// replaces dispatch_data at commit time only, after which nothing can throw -
// on rollback it still holds the previous tables, which the classes'
// static_vptrs point into. (That is also why a copy could not stand in for
// it: it would be another buffer.)
//
// Only the states of the policies that are about to be initialized are saved.
// Copying the `policies` tuple whole is simpler, but it is not harmless: a
// state that no `initialize` touches is not derived data this call is about to
// replace, it is configuration the caller owns. The error handler is the case
// that bites - it is *called* from inside the window, by design
// (fast_perfect_hash reports a search failure through it), so a handler that
// disarms itself with set() before throwing would have that undone on the way
// out. Saving wide also forces every policy state in the registry to be
// copyable, including those of policies that have no `initialize` at all -
// which rules out the std::ostringstream an `output` policy written to the
// documented state pattern naturally holds - and copies each of them, vectors
// and all, on every successful initialize().
template<class Registry, class Context, class Options>
class registry_state_transaction {
    using policy_fns = mp11::mp_transform_q<
        policy_fn_q<Registry>, typename Registry::policy_list>;
    using saved_states = mp11::mp_transform<
        policy_state_t,
        mp11::mp_filter_q<
            policy_state_is_volatile_q<Context, Options>, policy_fns>>;
    using saved_type = mp11::mp_apply<detail::tuple, saved_states>;

    static_assert(
        mp11::mp_all_of<saved_states, std::is_copy_assignable>::value,
        "the `state` of a policy that defines `initialize` must be copyable: "
        "initialize() saves it, and puts it back if a policy throws");

    // The restore runs from the destructor, while an exception is in flight,
    // and a destructor is noexcept by default: a move-assignment that threw
    // there would call std::terminate, destroying the very error the
    // transaction exists to let through. It is not a theoretical shape - a
    // state holding a std::map with a stateful, non-always-equal allocator
    // degrades to an element-wise move that allocates, and `vptr_map<MapFn>`
    // lets a caller supply exactly that. Refuse it here, where the message can
    // say why, rather than terminate at run time in the one configuration no
    // test covers.
    static_assert(
        mp11::mp_all_of<saved_states, std::is_nothrow_move_assignable>::value,
        "the `state` of a policy that defines `initialize` must be nothrow "
        "move-assignable: initialize() puts it back from a destructor, while "
        "an exception is in flight, where throwing would terminate");

    // Element-wise: `saved` holds a subset of the registry's tuple.
    template<class Tuple>
    struct each;

    template<class... States>
    struct each<detail::tuple<States...>> {
        static void save(detail::tuple<States...>& to) {
            using namespace detail;

            (..., (get<States>(to) = get<States>(Registry::state().policies)));
        }

        static void restore(detail::tuple<States...>& from) {
            using namespace detail;

            (...,
                (get<States>(Registry::state().policies) =
                        std::move(get<States>(from))));
        }
    };

  public:
    registry_state_transaction() {
        each<saved_type>::save(saved);
    }

    ~registry_state_transaction() {
        if (!committed) {
            each<saved_type>::restore(saved);
        }
    }

    registry_state_transaction(const registry_state_transaction&) = delete;
    auto operator=(const registry_state_transaction&)
        -> registry_state_transaction& = delete;

    void commit() {
        committed = true;
    }

  private:
    saved_type saved;
    bool committed = false;
};

inline void merge_into(boost::dynamic_bitset<>& a, boost::dynamic_bitset<>& b) {
    if (b.size() < a.size()) {
        b.resize(a.size());
    }

    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i]) {
            b[i] = true;
        }
    }
}

inline void set_bit(boost::dynamic_bitset<>& mask, std::size_t bit) {
    if (bit >= mask.size()) {
        mask.resize(bit + 1);
    }

    mask[bit] = true;
}

inline auto is_free(
    const boost::dynamic_bitset<>& unavailable, std::size_t slot) -> bool {
    return slot >= unavailable.size() || !unavailable[slot];
}

struct generic_compiler {

    struct method;

    // A virtual parameter that takes a slot in the v-tables of this registry.
    struct parameter {
        // Where the slot goes.
        std::size_t* slot;
        // The method, or null if it is in another registry.
        struct method* method;
        std::size_t param;
    };

    static constexpr auto no_method = (std::numeric_limits<std::size_t>::max)();

    struct vtbl_entry {
        // `no_method` for an entry that this registry does not write: a hole,
        // or the slot of a parameter of another registry's method, which that
        // registry writes.
        std::size_t method_index = no_method, vp_index = 0, group_index = 0;
    };

    struct class_ {
        std::vector<detail::class_info*> ci;
        // What the records declared, before the closure is computed; emptied by
        // calculate_transitive_bases.
        std::vector<class_*> declared_bases;
        std::vector<class_*> transitive_bases;
        std::vector<class_*> direct_bases;
        std::vector<class_*> direct_derived;
        std::unordered_set<class_*> transitive_derived;
        std::vector<parameter> used_by_vp;
        // The slots occupied in this class's v-table: those of every parameter
        // allocated so far on the class or on any of its ancestors. Grows only
        // through `set_bit` and `merge_into`, so `size()` is one past the
        // highest slot.
        boost::dynamic_bitset<> used_slots;
        std::size_t first_slot = 0;
        std::size_t mark = 0; // temporary mark to detect cycles
        bool transitive_bases_done = false;
        std::vector<vtbl_entry> vtbl;
        // The v-table pointer initialize() is about to install for the class.
        // Staged here by write_global_data(), where the policies read it (see
        // class_view); written to the class_infos' static_vptr only once
        // everything that can fail has succeeded.
        vptr_type vptr = nullptr;

        auto is_base_of(class_* other) const -> bool {
            return transitive_derived.find(other) != transitive_derived.end();
        }

        // A class of another registry, in the cone of a foreign parameter,
        // has no class_info; it is known by its entry in that registry's
        // v-table (see foreign_class).
        detail::word* foreign_entry = nullptr;
        bool foreign_abstract = false;
        // The position of the class in the cone, for the trace.
        std::size_t foreign_position = 0;

        auto is_foreign() const -> bool {
            return ci.empty();
        }

        auto is_abstract() const -> bool {
            return is_foreign() ? foreign_abstract : ci[0]->is_abstract;
        }
    };

    // A parameter of a method of another registry, dispatched through the
    // v-tables of this one. Each module may register a copy of the method,
    // and with it a record of the parameter; the copies share the slot.
    struct foreign_parameter {
        struct copy {
            detail::foreign_parameter_info* info;
            // The class of each overrider of this copy, for the parameter.
            std::vector<class_*> overriders;
            // Staged by write_global_data(), published by
            // commit_global_data().
            std::vector<detail::foreign_class> cone;
            std::vector<std::size_t> overrider_positions;
        };

        class_* vp;
        std::vector<copy> copies;
        std::size_t slot = 0;
    };

    // An entry in the v-table of another registry, filled for one of this
    // registry's methods.
    struct foreign_write {
        detail::word* entry;
        // The class, in the trace.
        const class_* cls;
        struct method* method;
        std::size_t dim, group;
        detail::word value;
    };

    struct slot_choice {
        std::size_t slot, cost;
    };

    template<class Classes>
    static auto pick_slot(
        const Classes& cone, const boost::dynamic_bitset<>& unavailable)
        -> slot_choice;

    struct overrider {
        detail::overrider_info* info = nullptr;
        overrider* next = nullptr;
        std::vector<class_*> vp;
        class_* covariant_return_type = nullptr;
        void (*pf)();
        std::size_t method_index, spec_index;
    };

    using bitvec = boost::dynamic_bitset<>;

    struct group {
        std::vector<class_*> classes;
        bool has_concrete_classes{false};
    };

    using group_map = std::map<bitvec, group>;

    struct method_report {
        std::size_t cells = 0;
        std::size_t not_implemented = 0;
        std::size_t ambiguous = 0;
    };

    struct report : method_report {};

    static void accumulate(const method_report& partial, report& total);

    struct method {
        std::size_t index;
        // All method_info instances for this logical method, one per module
        // that instantiated the method's `fn`. `info` is one of these. Used to
        // propagate slots_strides to every copy after the dispatch table is
        // built.
        std::vector<detail::method_info*> infos;
        std::vector<class_*> vp;
        class_* covariant_return_type = nullptr;
        std::vector<overrider> overriders;
        std::vector<std::size_t> slots;
        std::vector<std::size_t> strides;
        std::vector<const overrider*> dispatch_table;
        // following two are dummies, when converting to a function pointer, we will
        // get the corresponding pointer from method_info
        overrider not_implemented;
        overrider ambiguous;
        vptr_type gv_dispatch_table = nullptr;
        // For each virtual parameter, the position of its proxies in
        // `foreign_classes` if it dispatches in another registry, else
        // `no_method`.
        std::vector<std::size_t> foreign_first;
        // Proxies for the classes of the foreign parameters' cones.
        std::deque<class_> foreign_classes;
        auto arity() const {
            return vp.size();
        }
        method_report report;
    };

    const method* operator[](const detail::method_info& info) const {
        auto iter = std::find_if(
            methods.begin(), methods.end(), [&info](const method& m) {
                return std::find(m.infos.begin(), m.infos.end(), &info) !=
                    m.infos.end();
            });

        if (iter != methods.end()) {
            return &*iter;
        }

        return nullptr;
    }

    std::deque<class_> classes;

    // What a policy's `initialize` sees for each class (the InitializeClass
    // blueprint): the type ids of one class_info, and the v-table pointer
    // that initialize() is about to install for the class. `vptr()` is the
    // value staged in the class_, not what the class_info's static_vptr
    // holds - that is still the previous v-table, and stays so until every
    // policy has succeeded. `static_vptr()` is the address that will receive
    // it, for the policies that store pointers to v-table pointers
    // (indirect_vptr): the one thing here that is stable across
    // re-initializations.
    struct class_view {
        const detail::class_info* ci;
        const class_* cls;

        auto type_id_begin() const {
            return ci->type_id_begin();
        }

        auto type_id_end() const {
            return ci->type_id_end();
        }

        auto vptr() const -> vptr_type {
            return cls->vptr;
        }

        auto static_vptr() const -> const vptr_type* {
            return ci->static_vptr;
        }
    };

    class const_class_iterator {
      public:
        // The view is made on the fly, so `operator->` has nothing to point
        // at; it returns one of these, which points at the view it carries.
        // The proxy lives until the end of the full expression, which is as
        // long as a policy needs it: `vptr()` and `static_vptr()` return
        // values.
        struct arrow_proxy {
            class_view view;

            auto operator->() const -> const class_view* {
                return &view;
            }
        };

        // Input, not forward: `reference` is `class_view`, a prvalue made on
        // the fly, so two iterators at the same position hand out views at
        // different addresses. That fails the forward-iterator requirement
        // that `reference` be a real reference - libstdc++ says so outright
        // under _GLIBCXX_CONCEPT_CHECKS - and the multipass guarantee with it.
        // Everything a policy needs (`++`, `!=`, `*`, `std::distance`) is an
        // input-iterator operation.
        using iterator_category = std::input_iterator_tag;
        using value_type = class_view;
        using difference_type = std::ptrdiff_t;
        using pointer = arrow_proxy;
        using reference = class_view;

        const_class_iterator() = default;

        const_class_iterator(
            std::deque<class_>::const_iterator class_iter,
            std::deque<class_>::const_iterator class_end) :
            class_iter_(class_iter), class_end_(class_end) {
            if (class_iter_ != class_end_) {
                ci_iter_ = class_iter_->ci.begin();
                advance_to_valid();
            }
        }
        auto operator*() const -> class_view {
            return {*ci_iter_, &*class_iter_};
        }

        auto operator->() const -> arrow_proxy {
            return {**this};
        }

        auto operator++() -> const_class_iterator& {
            ++ci_iter_;
            advance_to_valid();

            return *this;
        }

        auto operator++(int) -> const_class_iterator {
            const_class_iterator tmp = *this;
            ++(*this);

            return tmp;
        }

        auto operator==(const const_class_iterator& other) const -> bool {
            if (class_iter_ != other.class_iter_) {
                return false;
            }
            if (class_iter_ == class_end_) {
                return true;
            }

            return ci_iter_ == other.ci_iter_;
        }

        auto operator!=(const const_class_iterator& other) const -> bool {
            return !(*this == other);
        }

      private:
        void advance_to_valid() {
            while (
                class_iter_ != class_end_ &&
                ci_iter_ == class_iter_->ci.end()) {
                ++class_iter_;
                if (class_iter_ != class_end_) {
                    ci_iter_ = class_iter_->ci.begin();
                }
            }
        }

        std::deque<class_>::const_iterator class_iter_;
        std::deque<class_>::const_iterator class_end_;
        std::vector<detail::class_info*>::const_iterator ci_iter_;
    };

    auto classes_begin() const {
        return const_class_iterator(classes.begin(), classes.end());
    }

    auto classes_end() const {
        return const_class_iterator(classes.end(), classes.end());
    }

    // The record of a method's virtual parameter, if it dispatches in another
    // registry.
    static auto foreign_parameter_of(
        const detail::method_info& info, std::size_t param)
        -> const detail::foreign_parameter_info* {
        for (auto iter = info.foreign_begin; iter != info.foreign_end; ++iter) {
            if (iter->param == param) {
                return iter;
            }
        }

        return nullptr;
    }

    // The proxy for the class at `position` in the cone of a foreign
    // parameter.
    static auto foreign_class_of(
        method& m, std::size_t param, std::size_t position) -> class_* {
        return &m.foreign_classes[m.foreign_first[param] + position];
    }

    std::deque<method> methods;
    std::deque<foreign_parameter> foreign_parameters;
    std::vector<foreign_write> foreign_writes;
    std::size_t class_mark = 0;
    bool compilation_done = false;
};

// Choose the slot for a new parameter of a class. `cone` holds the class and
// every class deriving from it; `unavailable` is the union of their
// `used_slots`. The cost of a slot is the number of entries it adds to the
// cone's v-tables: none inside a v-table's range (a hole gets filled), one per
// slot of growth beyond either end, one for a class that has no v-table yet.
// The cheapest free slot wins, the lowest on ties.
//
// Each v-table contributes a cost that is flat inside its range and grows by
// one per slot away from it, so the sum is swept from slot 0 upwards: moving
// to the next slot adds one per range already ended and removes one per range
// not yet started.
template<class Classes>
auto generic_compiler::pick_slot(
    const Classes& cone, const boost::dynamic_bitset<>& unavailable)
    -> slot_choice {
    // Slot `slots` is beyond every v-table, hence always free.
    const auto slots = unavailable.size();
    std::vector<std::size_t> starting_at(slots), ending_at(slots);
    std::size_t ranges = 0, cost = 0;

    for (auto cls : cone) {
        const auto& used = cls->used_slots;

        if (used.empty()) {
            // A one-entry v-table, wherever the slot lands.
            ++cost;
            continue;
        }

        auto first = used.find_first();
        BOOST_ASSERT(first != boost::dynamic_bitset<>::npos);
        BOOST_ASSERT(used.size() <= slots);
        ++starting_at[first];
        ++ending_at[used.size() - 1];
        ++ranges;
        cost += first; // the distance from slot 0
    }

    slot_choice best{0, std::numeric_limits<std::size_t>::max()};
    std::size_t started = 0, ended = 0;

    for (std::size_t slot = 0;; ++slot) {
        if (is_free(unavailable, slot) && cost < best.cost) {
            best = {slot, cost};
        }

        if (slot == slots) {
            return best;
        }

        started += starting_at[slot];
        ended += ending_at[slot];
        cost = cost + ended - (ranges - started);
    }
}

template<class Compiler>
struct trace_stream {
    bool on = false;
    std::size_t indentation_level{0};

    auto operator++() -> trace_stream& {
        if constexpr (Compiler::has_trace) {
            if (on) {
                for (std::size_t i = 0; i < indentation_level; ++i) {
                    Compiler::Registry::output::stream() << "  ";
                }
            }
        }

        return *this;
    }

    struct indent {
        trace_stream& trace;
        int by;

        explicit indent(trace_stream& trace, int by = 2) :
            trace(trace), by(by) {
            trace.indentation_level += by;
        }

        ~indent() {
            trace.indentation_level -= by;
        }
    };
};

struct rflush {
    std::size_t width;
    std::size_t value;
    explicit rflush(std::size_t width, std::size_t value) :
        width(width), value(value) {
    }
};

struct type_name {
    type_name(type_id type) : type(type) {
    }
    type_id type;
};

template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const generic_compiler::class_& cls)
    -> trace_stream<Compiler>& {
    if constexpr (Compiler::has_trace) {
        if (cls.is_foreign()) {
            tr << "foreign#" << cls.foreign_position;
        } else {
            tr << type_name(cls.ci[0]->type);
        }
    }

    return tr;
}

template<class Compiler, template<typename...> class Container, typename... T>
auto operator<<(
    trace_stream<Compiler>& tr,
    [[maybe_unused]] Container<generic_compiler::class_*, T...>& classes)
    -> trace_stream<Compiler>& {
    if constexpr (Compiler::has_trace) {
        tr << "(";
        const char* sep = "";
        for (auto cls : classes) {
            tr << sep << *cls;
            sep = ", ";
        }

        tr << ")";
    }

    return tr;
}

struct spec_name {
    spec_name(
        const detail::generic_compiler::method& method,
        const detail::generic_compiler::overrider* def) :
        method(method), def(def) {
    }
    const detail::generic_compiler::method& method;
    const detail::generic_compiler::overrider* def;
};

template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const spec_name& sn)
    -> trace_stream<Compiler>& {
    if (sn.def == &sn.method.not_implemented) {
        tr << "not implemented";
    } else if (sn.def == &sn.method.ambiguous) {
        tr << "ambiguous";
    } else {
        tr << type_name(sn.def->info->type);
    }

    return tr;
}

template<typename Iterator>
struct range {
    range(Iterator first, Iterator last) : first(first), last(last) {
    }

    Iterator first, last;

    auto begin() const -> Iterator {
        return first;
    }

    auto end() const -> Iterator {
        return last;
    }
};

template<class Compiler, typename T, typename F>
auto write_range(trace_stream<Compiler>& tr, range<T> range, F fn) -> auto& {
    if constexpr (Compiler::has_trace) {
        if (tr.on) {
            tr << "(";
            const char* sep = "";
            for (auto value : range) {
                tr << sep << fn(value);
                sep = ", ";
            }

            tr << ")";
        }
    }

    return tr;
}

template<class Compiler, typename T>
auto operator<<(trace_stream<Compiler>& tr, const T& value) -> auto& {
    if constexpr (Compiler::has_trace) {
        if (tr.on) {
            Compiler::Registry::output::stream() << value;
        }
    }

    return tr;
}

template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const rflush& rf) -> auto& {
    if constexpr (Compiler::has_trace) {
        if (tr.on) {
            std::size_t digits = 1;
            auto tmp = rf.value / 10;

            while (tmp) {
                ++digits;
                tmp /= 10;
            }

            while (digits < rf.width) {
                tr << " ";
                ++digits;
            }

            tr << rf.value;
        }
    }

    return tr;
}

template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const boost::dynamic_bitset<>& bits)
    -> auto& {
    if constexpr (Compiler::has_trace) {
        if (tr.on) {
            auto i = bits.size();
            while (i != 0) {
                --i;
                Compiler::Registry::output::stream() << bits[i];
            }
        }
    }

    return tr;
}

template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const range<type_id*>& tips)
    -> auto& {
    return write_range(tr, tips, [](auto tip) { return type_name(tip); });
}

template<class Compiler, typename T>
auto operator<<(trace_stream<Compiler>& tr, const range<T>& range) -> auto& {
    return write_range(tr, range, [](auto value) { return value; });
}

struct parameter_name {
    const generic_compiler::parameter& param;
};

template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const type_name& manip) -> auto& {
    if constexpr (Compiler::has_trace) {
        Compiler::Registry::rtti::type_name(manip.type, tr);
    }

    return tr;
}

// The method of a parameter of another registry's method is not named: its
// type_id is meaningful only to that registry's rtti policy.
template<class Compiler>
auto operator<<(trace_stream<Compiler>& tr, const parameter_name& manip)
    -> auto& {
    if constexpr (Compiler::has_trace) {
        if (manip.param.method) {
            tr << type_name(manip.param.method->infos[0]->method_type_id);
        } else {
            tr << "(method of another registry)";
        }

        tr << " parameter " << manip.param.param;
    }

    return tr;
}
} // namespace detail

// Definition of the nested template struct outside the registry class
template<class... Policies>
template<class... Options>
struct registry<Policies...>::compiler : detail::generic_compiler {
    using type_index_type = decltype(rtti::type_index(0));

    typename detail::aggregate_reports<mp11::mp_list<report>, policy_list>::type
        report;

    std::unordered_map<type_index_type, class_*> class_map;

    using Registry = registry;

    compiler(Options... opts);

    auto compile();
    void initialize();
    void install_global_tables();

    void augment_classes();
    void calculate_transitive_bases(class_& cls);
    void calculate_transitive_derived(class_& cls);
    void augment_methods();
    void augment_foreign_parameters();
    void trace_foreign_parameters(const detail::method_info& info);
    void trace_unfilled_entry(
        class_& cls, std::size_t slot, const detail::word* address);
    void assign_slots();
    void assign_tree_slots(class_& cls, std::size_t base_slot);
    void assign_slots(class_& cls);
    void build_dispatch_tables();
    void build_dispatch_table(
        method& m, std::size_t dim,
        std::vector<group_map>::const_iterator group, const bitvec& candidates,
        bool concrete);
    void write_global_data();
    void commit_global_data(
        std::vector<detail::word>& new_dispatch_data) noexcept;
    void print(const method_report& report) const;
    void print_slots();
    static void select_dominant_overriders(
        std::vector<overrider*>& dominants, std::size_t& pick,
        std::size_t& remaining);
    static auto is_more_specific(const overrider* a, const overrider* b)
        -> bool;
    static auto is_base(const overrider* a, const overrider* b) -> bool;

    std::tuple<Options...> options;

    template<class Option>
    static constexpr bool has_option =
        mp11::mp_contains<mp11::mp_list<Options...>, Option>::value;

    static constexpr bool has_trace = has_option<trace>;
    static constexpr bool has_n2216 = has_option<n2216>;

    mutable detail::trace_stream<compiler> tr;
    using indent = typename detail::trace_stream<compiler>::indent;
};

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::install_global_tables() {
    if (!compilation_done) {
        abort();
    }

    // Report before installing, not after. Everything printed here is
    // compiler-local - the report gathered during compile(), the slot
    // assignment, the class lattices - so none of it needs the new tables to
    // be in place; and all of it can throw, through the user-supplied `output`
    // policy, or out of the containers print_slots() builds. Run after
    // write_global_data() it would throw past the commit, leaving the new
    // tables installed while initialize() never reaches
    // `st.initialized = true` - a state the exception-safety contract does not
    // describe, and one that makes the next call abort with `not_initialized`
    // under runtime_checks. Before it, a throw is just another failure the
    // transaction rolls back.
    print(report);
    print_slots();
    ++tr << "Finished\n";

    write_global_data();
}

template<class... Policies>
template<class... Options>
auto registry<Policies...>::compiler<Options...>::compile() {
    augment_classes();
    augment_methods();
    assign_slots();
    build_dispatch_tables();

    compilation_done = true;

    return report;
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::initialize() {
    // Clear the flag up front, and set it only once everything has succeeded.
    // A run that throws leaves the dispatch state exactly as the call found it
    // (see write_global_data()) - which is not the same as a working one: if a
    // finalize() came in between, what the call found was already torn down,
    // the classes' static_vptrs still set and the dispatch data cleared.
    // Either way it is not marked initialized:
    // those tables do not reflect the registrations that prompted the call,
    // and after a dlclose (the documented re-initialize flow) they may point
    // into unloaded code, so require_initialized() must keep refusing to
    // dispatch until an initialize() succeeds.
    registry<Policies...>::static_::st.initialized = false;
    compile();
    install_global_tables();
    registry<Policies...>::static_::st.initialized = true;
}

#ifdef _MSC_VER
namespace detail {

template<bool HasTrace, typename T>
struct msvc_tuple_get;

template<typename T>
struct msvc_tuple_get<true, T> {
    template<class Tuple>
    static decltype(auto) fn(const Tuple& t) {
        return std::get<T>(t);
    }
};

template<typename T>
struct msvc_tuple_get<false, T> {
    template<class Tuple>
    static decltype(auto) fn(const Tuple&) {
        return T();
    }
};
} // namespace detail
#endif

template<class... Policies>
template<class... Options>
registry<Policies...>::compiler<Options...>::compiler(Options... opts) :
    options(opts...) {
    if constexpr (has_trace) {
#ifdef _MSC_VER
        tr.on = detail::msvc_tuple_get<has_trace, trace>::fn(options).on;
#else
        // Even with the constexpr has_trace guard, msvc errors on this.
        tr.on = std::get<trace>(options).on;
#endif
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::calculate_transitive_bases(
    class_& cls) {
    if (cls.transitive_bases_done) {
        return;
    }

    // Set before recursing. Inheritance cannot cycle, so this only guards
    // against a malformed set of records.
    cls.transitive_bases_done = true;

    // Complete every declared base first, so that merging them below sees
    // their full closure. Recursing bumps `class_mark`, hence the fresh mark
    // afterwards.
    for (auto base : cls.declared_bases) {
        calculate_transitive_bases(*base);
    }

    auto mark = ++class_mark;

    for (auto base : cls.declared_bases) {
        if (base->mark != mark) {
            base->mark = mark;
            cls.transitive_bases.push_back(base);
        }

        for (auto base_base : base->transitive_bases) {
            if (base_base->mark != mark) {
                base_base->mark = mark;
                cls.transitive_bases.push_back(base_base);
            }
        }
    }

    cls.declared_bases.clear();
    cls.declared_bases.shrink_to_fit();
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::augment_classes() {
    using namespace detail;

    // scope
    {
        ++tr << "Static class info:\n";

        // The standard does not guarantee that there is exactly one
        // type_info object per class. However, it guarantees that the
        // type_index for a class has a unique value.
        for (auto& cr : registry::static_::st.classes) {
            if constexpr (has_deferred_static_rtti) {
                static_cast<deferred_class_info&>(cr).resolve_type_ids();
            }

            {
                indent _(tr);
                ++tr << type_name(cr.type) << ": "
                     << range{cr.first_base, cr.last_base}
                     << ", type = " << cr.type << ", &vptr = " << cr.static_vptr
                     << "\n";
            }

            auto& rtc = class_map[rtti::type_index(cr.type)];

            if (rtc == nullptr) {
                rtc = &classes.emplace_back();
            }

            // Retain one class_info per (type, static_vptr) pair, not per
            // type. Under hidden visibility a class' type_id can unify
            // across modules while Registry::static_vptr<Class> does not
            // (each module has its own COMDAT copy), so the shared class
            // list legitimately holds several class_info entries for the
            // same class. write_global_data() patches every retained
            // entry's static_vptr; dropping the extras on type alone would
            // leave other modules' static_vptr null. Two entries are true
            // duplicates only when writing through either has the same
            // effect.
            bool duplicate = false;

            for (auto ci : rtc->ci) {
                if (rtti::type_index(ci->type) == rtti::type_index(cr.type) &&
                    ci->static_vptr == cr.static_vptr) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate) {
                rtc->ci.push_back(&cr);
            }
        }
    }

    // All known classes now have exactly one associated class_* in the
    // map. Collect the bases.

    for (auto& cr : registry::static_::st.classes) {
        auto rtc = class_map[rtti::type_index(cr.type)];

        for (auto& base : range{cr.first_base, cr.last_base}) {
            auto rtb = class_map[rtti::type_index(base)];

            if (!rtb) {
                missing_class error;
                error.type = base;

                if constexpr (has_error_handler) {
                    error_handler::error(error);
                }

                abort();
            }

            if (rtc != rtb) {
                // At compile time we collected the class as its own
                // improper base, as per std::is_base_of. Eliminate that.
                rtc->declared_bases.push_back(rtb);
            }
        }
    }

    // `declared_bases` now holds whatever the records said - direct bases,
    // ancestors, or a mixture, with duplicates. Turn that into the transitive
    // closure. This is done here rather than while reading the records because
    // a record's bases are only as informative as the records already seen: a
    // class registered before its own bases would otherwise be left with a
    // short list. That matters beyond `transitive_bases` itself, because the
    // direct-base derivation below sorts on its size.

    for (auto& rtc : classes) {
        calculate_transitive_bases(rtc);
    }

    for (auto& rtc : classes) {
        // Sort base classes by number of transitive bases. This ensures that a
        // base class is never preceded by one if its own base classes.
        std::sort(
            rtc.transitive_bases.begin(), rtc.transitive_bases.end(),
            [](auto a, auto b) {
                return a->transitive_bases.size() > b->transitive_bases.size();
            });
        auto mark = ++class_mark;

        // Collect the direct base classes. The first base is certainly a
        // direct one. Remove *its* bases from the candidates, by marking
        // them. Continue with the next base that is not marked. It is the
        // next direct base. And so on...

        for (auto rtb : rtc.transitive_bases) {
            if (rtb->mark == mark) {
                continue;
            }

            rtc.direct_bases.push_back(rtb);

            for (auto rtbb : rtb->transitive_bases) {
                rtbb->mark = mark;
            }
        }
    }

    for (auto& rtc : classes) {
        for (auto rtb : rtc.direct_bases) {
            rtb->direct_derived.push_back(&rtc);
        }
    }

    for (auto& rtc : classes) {
        calculate_transitive_derived(rtc);
    }

    if constexpr (has_trace) {
        ++tr << "Inheritance lattice:\n";

        for (auto& rtc : classes) {
            indent _2(tr);
            ++tr << rtc << "\n";

            {
                indent _3(tr);
                ++tr << "bases:      " << rtc.direct_bases << "\n";
                ++tr << "derived:    " << rtc.direct_derived << "\n";
                ++tr << "covariant:  " << rtc.transitive_derived << "\n";
            }
        }
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::calculate_transitive_derived(
    class_& cls) {
    if (!cls.transitive_derived.empty()) {
        return;
    }

    cls.transitive_derived.insert(&cls);

    for (auto derived : cls.direct_derived) {
        if (derived->transitive_derived.empty()) {
            calculate_transitive_derived(*derived);
        }

        std::copy(
            derived->transitive_derived.begin(),
            derived->transitive_derived.end(),
            std::inserter(
                cls.transitive_derived, cls.transitive_derived.end()));
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::augment_methods() {
    using namespace policies;
    using namespace detail;

    // Resolve deferred type ids first so consolidation keys are correct.
    // Overriders must be resolved here too (not only in the per-overrider
    // loop below) because the cross-module overrider dedup key uses their
    // type ids.
    if constexpr (has_deferred_static_rtti) {
        for (auto& meth_info : registry::static_::st.methods) {
            static_cast<deferred_method_info&>(meth_info).resolve_type_ids();
            for (auto& spec : meth_info.overriders) {
                static_cast<deferred_overrider_info&>(spec).resolve_type_ids();
            }
        }
    }

    // Group method copies across modules (DLL boundaries) by type_index.
    // The "local" entry is the first one we encounter — the method_info that
    // lives in the module performing initialize(). It owns the compiler's
    // working state for that method; the others get their slots_strides
    // copied from it later.
    std::map<type_index_type, method*> method_map;

    for (auto& info : registry::static_::st.methods) {
        auto key = rtti::type_index(info.method_type_id);
        auto [iter, inserted] = method_map.try_emplace(key, nullptr);

        if (inserted) {
            methods.emplace_back();
            iter->second = &methods.back();
        }

        iter->second->infos.push_back(&info);
    }

    std::size_t method_index = 0;

    ++tr << "Methods:\n";
    indent _(tr);

    for (auto& method : methods) {
        method.index = method_index++;
        auto first_info = method.infos[0];
        BOOST_ASSERT(first_info);
        // Every info in method.infos was grouped here because it shares the
        // same method_type_id (see method_map above). Under a well-behaved
        // RTTI policy that key is injective per method, so all of them
        // describe the very same method and must agree on arity. A custom
        // RTTI policy that returns a shared sentinel id for unregistered
        // types (e.g. a fixed value for every non-polymorphic type) can
        // break that assumption and silently merge distinct methods; catch
        // it here instead of corrupting slots_strides downstream.
        for (auto info : method.infos) {
            (void)info; // unused when BOOST_ASSERT compiles out (NDEBUG)
            BOOST_ASSERT(info->arity() == first_info->arity());
        }
        auto arity = first_info->arity();
        method.vp.reserve(arity);
        method.slots.resize(arity);
        method.foreign_first.assign(arity, no_method);

        // A parameter that dispatches in another registry was prepared by that
        // registry's initialize(), which must have run since every module's
        // copy of the method registered it.
        for (auto info : method.infos) {
            for (auto& param : range{info->foreign_begin, info->foreign_end}) {
                if (param.generation == 0 ||
                    param.generation != *param.host_generation ||
                    param.overriders.size() != info->overriders.size()) {
                    ++tr << "registry of parameter #" << param.param
                         << " not initialized\n";

                    if constexpr (has_error_handler) {
                        parameter_registry_not_initialized error;
                        error.method = info->method_type_id;
                        error.param = param.param;
                        error_handler::error(error);
                    }

                    abort();
                }
            }
        }

        // Build the virtual-parameter classes once, from the local
        // method_info. All copies describe the same method, so any one of them
        // yields the same vp list.
        {
            std::size_t param_index = 0;

            for (auto ti : range{first_info->vp_begin, first_info->vp_end}) {
                if (auto param =
                        foreign_parameter_of(*first_info, param_index)) {
                    // The type_id means nothing here: the parameter's registry
                    // described the cone of its class, by position.
                    method.slots[param_index] = param->slot;
                    method.foreign_first[param_index] =
                        method.foreign_classes.size();

                    for (auto& cls : param->cone) {
                        auto& proxy = method.foreign_classes.emplace_back();
                        proxy.foreign_position = &cls - param->cone.data();
                        proxy.foreign_entry = cls.entry;
                        proxy.foreign_abstract = cls.is_abstract;
                    }

                    std::size_t position = 0;

                    for (auto& cls : param->cone) {
                        auto proxy =
                            foreign_class_of(method, param_index, position++);

                        for (auto derived : cls.transitive_derived) {
                            proxy->transitive_derived.insert(
                                foreign_class_of(method, param_index, derived));
                        }
                    }

                    method.vp.push_back(
                        foreign_class_of(method, param_index, 0));
                    ++param_index;

                    continue;
                }

                auto class_ = class_map[rtti::type_index(ti)];
                if (!class_) {
                    ++tr << "unknown class " << ti << "(" << type_name(ti)
                         << ") for parameter #" << (param_index + 1) << "\n";
                    missing_class error;
                    error.type = ti;

                    if constexpr (has_error_handler) {
                        error_handler::error(error);
                    }

                    abort();
                }

                method.vp.push_back(class_);
                ++param_index;
            }
        }

        ++tr << type_name(first_info->method_type_id) << " " << method.vp
             << "\n";
        indent _(tr);

        if constexpr (has_trace) {
            trace_foreign_parameters(*first_info);
        }

        if (rtti::type_index(first_info->return_type_id) !=
            rtti::type_index(rtti::template static_type<void>())) {
            auto covariant_return_iter =
                class_map.find(rtti::type_index(first_info->return_type_id));

            if (covariant_return_iter != class_map.end()) {
                method.covariant_return_type = covariant_return_iter->second;
            }
        }

        // Collect overriders from every module copy of this method, deduping
        // by *logical* identity rather than pointer identity - but only for
        // overriders declared inline_. The same overrider, defined in a
        // header and registered by two or more state-sharing modules (e.g.
        // an exe and a DLL), appears once per module as a distinct
        // overrider_info object - different address, and a different `pf`
        // (each module compiles its own copy of the function) - but they
        // share the same function type id and the same virtual-parameter
        // type ids. Keeping every copy would make each dispatch cell they
        // fill ambiguous, because is_more_specific() reports "not more
        // specific" both ways for identical vp lists.
        //
        // A NON-inline overrider with the same signature must never be
        // merged, even if it happens to match: BOOST_OPENMETHOD_OVERRIDE
        // (non-inline) keys one explicit specialization per signature, so
        // writing it twice in one translation unit is a redefinition error,
        // and defining it identically in more than one TU (as required for
        // it to appear "duplicated" in the first place) is an ODR violation
        // for a non-inline function - i.e. the situation this dedup exists
        // to handle can only arise legitimately for `inline` overriders (see
        // BOOST_OPENMETHOD_INLINE_OVERRIDE, which is the only thing that
        // sets overrider_info::inline_ = true). Two DIFFERENT overriders
        // sharing a signature (e.g. registered directly via
        // method<...>::override<Fn1> and method<...>::override<Fn2>, both
        // non-inline by default) are always genuinely distinct and must
        // remain ambiguous.
        //
        // Overriders are compared by the classes of their virtual parameters,
        // not by type_id: the type_id of a parameter that dispatches in
        // another registry means nothing to this one.
        struct candidate {
            detail::overrider_info* info;
            std::vector<class_*> vp;
        };

        std::vector<candidate> all_specs;
        std::size_t module_index = 0;

        for (auto info : method.infos) {
            indent _(tr);
            ++tr << "module " << module_index++ << "\n";
            std::size_t spec_position = 0;

            for (auto& spec : info->overriders) {
                std::vector<class_*> vp;
                vp.reserve(arity);
                std::size_t param_index = 0;

                for (auto type : range{spec.vp_begin, spec.vp_end}) {
                    if (auto param = foreign_parameter_of(*info, param_index)) {
                        vp.push_back(foreign_class_of(
                            method, param_index,
                            param->overriders[spec_position]));
                        ++param_index;

                        continue;
                    }

                    auto class_ = class_map[rtti::type_index(type)];

                    if (!class_) {
                        indent _(tr);
                        ++tr << "unknown class error for *virtual* parameter #"
                             << (param_index + 1) << "\n";
                        missing_class error;
                        error.type = type;

                        if constexpr (has_error_handler) {
                            error_handler::error(error);
                        }

                        abort();
                    }

                    vp.push_back(class_);
                    ++param_index;
                }

                ++spec_position;

                auto same = [&](const candidate& kept) {
                    return kept.info->inline_ && spec.inline_ &&
                        rtti::type_index(kept.info->type) ==
                        rtti::type_index(spec.type) &&
                        kept.vp == vp;
                };

                if (std::none_of(all_specs.begin(), all_specs.end(), same)) {
                    all_specs.push_back({&spec, std::move(vp)});
                }
            }
        }

        // initialize the function pointer in the synthetic not_implemented
        // overrider
        auto spec_size = all_specs.size();
        method.not_implemented.pf = first_info->not_implemented;
        method.not_implemented.method_index = method.index;
        method.not_implemented.spec_index = spec_size;
        method.ambiguous.pf = first_info->ambiguous;
        method.ambiguous.method_index = method.index;
        method.ambiguous.spec_index = spec_size + 1;

        method.overriders.resize(spec_size);
        auto spec_iter = method.overriders.begin();

        for (auto& spec : all_specs) {
            auto overrider_info = spec.info;
            spec_iter->method_index = method.index;
            spec_iter->spec_index = spec_iter - method.overriders.begin();

            ++tr << type_name(overrider_info->type) << " ("
                 << overrider_info->pf << ")\n";
            spec_iter->info = overrider_info;
            spec_iter->pf = overrider_info->pf;
            spec_iter->vp = std::move(spec.vp);

            if (method.covariant_return_type) {
                auto covariant_return_iter = class_map.find(
                    rtti::type_index(overrider_info->return_type));

                if (covariant_return_iter != class_map.end()) {
                    spec_iter->covariant_return_type =
                        covariant_return_iter->second;
                } else {
                    missing_class error;
                    error.type = overrider_info->return_type;

                    if constexpr (has_error_handler) {
                        error_handler::error(error);
                    }

                    abort();
                }
            }

            ++spec_iter;
        }
    }

    for (auto& method : methods) {
        std::size_t param_index = 0;

        for (auto vp : method.vp) {
            // The parameter's registry checked the classes of a foreign
            // parameter, and allocated its slot.
            if (vp->is_foreign()) {
                ++param_index;

                continue;
            }

            for (auto& overrider : method.overriders) {
                if (overrider.vp[param_index] == vp) {
                    continue;
                }

                if (!vp->is_base_of(overrider.vp[param_index])) {
                    missing_base error;
                    error.base = vp->ci[0]->type;
                    error.derived = overrider.vp[param_index]->ci[0]->type;

                    if constexpr (has_error_handler) {
                        error_handler::error(error);
                    }

                    abort();
                }
            }

            vp->used_by_vp.push_back(
                {&method.slots[param_index], &method, param_index});
            ++param_index;
        }
    }

    augment_foreign_parameters();
}

// The parameters of other registries' methods that dispatch through this
// registry's v-tables. Their type_ids were computed by this registry's rtti
// policy, so this is where they are mapped to classes and checked. Each gets a
// slot like a parameter of this registry's own methods, but the entries are
// left empty: the method's registry fills them.
template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::augment_foreign_parameters() {
    using namespace detail;

    if (registry::static_::st.foreign_parameters.empty()) {
        return;
    }

    ++tr << "Parameters of methods of other registries:\n";
    indent _(tr);

    auto find_class = [this](type_id type) {
        auto iter = class_map.find(rtti::type_index(type));

        if (iter == class_map.end()) {
            ++tr << "unknown class " << type_name(type) << "\n";
            missing_class error;
            error.type = type;

            if constexpr (has_error_handler) {
                error_handler::error(error);
            }

            abort();
        }

        return iter->second;
    };

    for (auto& info : registry::static_::st.foreign_parameters) {
        // The type ids of the parameter are this registry's to resolve: the
        // method, in another registry, resolved them at construction unless
        // this one defers them. Only this position - the others follow the
        // deferral of their own registries.
        if constexpr (has_deferred_static_rtti) {
            info.resolve_vp(info.method->vp_begin, info.param);

            for (auto& spec : info.method->overriders) {
                spec.resolve_vp(spec.vp_begin, info.param);
            }
        }

        // The same parameter of another copy of the same method.
        auto same = std::find_if(
            foreign_parameters.begin(), foreign_parameters.end(),
            [&info](const foreign_parameter& param) {
                auto other = param.copies.front().info;

                return other->method_state == info.method_state &&
                    other->param == info.param &&
                    info.same_method(other->method_type(), info.method_type());
            });

        auto& param = same != foreign_parameters.end()
            ? *same
            : foreign_parameters.emplace_back();
        auto is_new = param.copies.empty();
        auto& copy = param.copies.emplace_back();
        copy.info = &info;

        if (is_new) {
            param.vp = find_class(info.method->vp_begin[info.param]);
            param.vp->used_by_vp.push_back({&param.slot, nullptr, info.param});
        }

        ++tr << "parameter " << info.param << ": " << *param.vp
             << (is_new ? "\n" : " (another copy)\n");

        for (auto& spec : info.method->overriders) {
            auto cls = find_class(spec.vp_begin[info.param]);

            if (cls != param.vp && !param.vp->is_base_of(cls)) {
                missing_base error;
                error.base = param.vp->ci[0]->type;
                error.derived = cls->ci[0]->type;

                if constexpr (has_error_handler) {
                    error_handler::error(error);
                }

                abort();
            }

            copy.overriders.push_back(cls);
        }

        {
            indent _(tr);
            ++tr << "overriders: " << copy.overriders << "\n";
        }
    }
}

// What the registries of a method's foreign parameters published for them.
// Their classes are known here only by position in the cone: `foreign#0` is
// the parameter's class. The registry that published them names them in its
// own trace.
template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::trace_foreign_parameters(
    const detail::method_info& info) {
    using namespace detail;

    for (auto& param : range{info.foreign_begin, info.foreign_end}) {
        ++tr << "parameter " << param.param
             << " dispatches in another registry, published at generation "
             << param.generation << ": slot " << param.slot << "\n";
        indent _(tr);

        for (auto& cls : param.cone) {
            ++tr << "foreign#" << (&cls - param.cone.data()) << ": entry at "
                 << cls.entry << (cls.is_abstract ? ", abstract" : "")
                 << ", derived:";

            auto derived = cls.transitive_derived;
            std::sort(derived.begin(), derived.end());

            for (auto position : derived) {
                tr << " foreign#" << position;
            }

            tr << "\n";
        }
    }
}

// An entry of this registry's v-tables that it leaves empty: a hole, or the
// slot of a parameter of another registry's method, which that registry fills.
template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::trace_unfilled_entry(
    class_& cls, std::size_t slot, const detail::word* address) {
    for (auto& param : foreign_parameters) {
        if (param.slot == slot &&
            (param.vp == &cls || param.vp->is_base_of(&cls))) {
            ++tr << address << " for (method of another registry) parameter "
                 << param.copies.front().info->param
                 << ", filled by its registry\n";

            return;
        }
    }

    ++tr << "empty\n";
}

// Slot allocation. The scheme follows the remarks Steven Watanabe made on the
// previous allocator, recorded in issue #19: pull the slots a class may not
// take from the classes deriving from it, instead of pushing reservations up
// to the bases of its derived classes; propagate a single bit after an
// assignment, not whole bit sets; allocate bases before derived classes; and
// place a slot next to, or inside a hole of, the ranges already allocated,
// rather than at the lowest free number. SLOT_ALLOCATION.md gives the full
// account, and the measurements against the previous allocator.
template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::assign_slots() {
    ++tr << "Allocating slots...\n";

    {
        indent _(tr);

        // A root whose cone holds no class with more than one direct base
        // heads a tree, disjoint from every other cone: a class in two cones
        // would have two direct bases somewhere above it. Trees are the common
        // case, and the cheap one: a class's slots follow its parent's, and
        // every v-table is dense from slot 0, in one walk down the tree. The
        // general algorithm below would produce the same layout at a higher
        // cost; the classes of a tree are marked so that it skips them.
        ++class_mark;

        for (auto& cls : classes) {
            if (cls.direct_bases.empty() &&
                std::none_of(
                    cls.transitive_derived.begin(),
                    cls.transitive_derived.end(), [](const class_* derived) {
                        return derived->direct_bases.size() > 1;
                    })) {
                assign_tree_slots(cls, 0);
            }
        }

        // The other classes form lattices. Allocate first the classes whose
        // slot must fit into the most v-tables, while those are still alike:
        // the wider a class's cone, the more it costs to find a slot free in
        // all of it once they have grown apart. A base's cone strictly
        // includes its derived classes' cones, so this also puts bases before
        // derived classes. Between unrelated classes with cones of the same
        // size, the deeper one goes first: it has an inherited range to extend
        // and fewer options. The order of registration decides only what is
        // left.
        std::vector<class_*> order;

        for (auto& cls : classes) {
            if (cls.mark != class_mark) {
                order.push_back(&cls);
            }
        }

        std::stable_sort(
            order.begin(), order.end(), [](const class_* a, const class_* b) {
                if (a->transitive_derived.size() !=
                    b->transitive_derived.size()) {
                    return a->transitive_derived.size() >
                        b->transitive_derived.size();
                }

                return a->transitive_bases.size() > b->transitive_bases.size();
            });

        for (auto cls : order) {
            assign_slots(*cls);
        }
    }

    ++tr << "Allocating v-tables...\n";

    {
        indent _(tr);

        for (auto& cls : classes) {
            if (cls.used_slots.empty()) {
                // Either a class of a tree, whose v-table assign_tree_slots
                // sized, or a class with no slot in it or its ancestors, which
                // has no v-table.
                continue;
            }

            // Leading unused slots are not stored: the v-table starts at the
            // first slot in use, and the class's v-table pointer is offset
            // back by as much.
            cls.first_slot = cls.used_slots.find_first();
            BOOST_ASSERT(cls.first_slot != boost::dynamic_bitset<>::npos);
            cls.vtbl.resize(cls.used_slots.size() - cls.first_slot);
            ++tr << cls << " vtbl: " << cls.first_slot << "-"
                 << (cls.used_slots.size() - 1) << " slots " << cls.used_slots
                 << "\n";
        }
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::assign_tree_slots(
    class_& cls, std::size_t base_slot) {
    using namespace detail;

    cls.mark = class_mark;
    auto next_slot = base_slot;

    for (const auto& mp : cls.used_by_vp) {
        ++tr << "in " << cls << " for " << parameter_name{mp} << ": slot "
             << next_slot << "\n";
        *mp.slot = next_slot++;
    }

    // `used_slots` stays empty: the v-table is sized here, and nothing in a
    // lattice ever looks at a class of a tree.
    cls.first_slot = 0;
    cls.vtbl.resize(next_slot);

    for (auto derived : cls.direct_derived) {
        assign_tree_slots(*derived, next_slot);
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::assign_slots(class_& cls) {
    using namespace detail;

    if (cls.used_by_vp.empty()) {
        return;
    }

    // A slot is off limits if any class deriving from `cls` - itself included -
    // already has it in its v-table. That is exactly the set of slots that
    // would collide: two parameters conflict when some class inherits both,
    // and that class is in the cone of each. Nothing else needs tracking, and
    // the order in which classes are allocated cannot affect correctness.
    boost::dynamic_bitset<> unavailable;

    for (auto derived : cls.transitive_derived) {
        merge_into(derived->used_slots, unavailable);
    }

    for (const auto& mp : cls.used_by_vp) {
        auto choice = pick_slot(cls.transitive_derived, unavailable);
        ++tr << "in " << cls << " for " << parameter_name{mp} << ": slot "
             << choice.slot << ", cost " << choice.cost << ", unavailable "
             << unavailable << "\n";
        *mp.slot = choice.slot;
        set_bit(unavailable, choice.slot);

        // One bit, to the whole cone: `cls` is in it.
        for (auto derived : cls.transitive_derived) {
            set_bit(derived->used_slots, choice.slot);
        }
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::build_dispatch_tables() {
    using namespace detail;

    for (auto& m : methods) {
        auto first_info = m.infos[0];
        ++tr << "Building dispatch table for "
             << type_name(first_info->method_type_id) << "\n";
        indent _(tr);

        auto dims = m.arity();

        std::vector<group_map> groups;
        groups.resize(dims);

        {
            std::size_t dim = 0;

            for (auto vp : m.vp) {
                auto& dim_group = groups[dim];
                ++tr << "make groups for param #" << dim << ", class " << *vp
                     << "\n";
                indent _(tr);

                for (auto covariant_class : vp->transitive_derived) {
                    ++tr << "overriders applicable to " << *covariant_class
                         << "\n";
                    bitvec mask;
                    mask.resize(m.overriders.size());

                    std::size_t group_index = 0;
                    indent _2(tr);

                    for (auto& spec : m.overriders) {
                        if (spec.vp[dim]->transitive_derived.find(
                                covariant_class) !=
                            spec.vp[dim]->transitive_derived.end()) {
                            ++tr << type_name(spec.info->type) << "\n";
                            mask[group_index] = 1;
                        }
                        ++group_index;
                    }

                    auto& group = dim_group[mask];
                    group.classes.push_back(covariant_class);
                    group.has_concrete_classes = group.has_concrete_classes ||
                        !covariant_class->is_abstract();

                    ++tr << "-> mask: " << mask << "\n";
                }

                ++dim;
            }
        }

        {
            std::size_t stride = 1;
            m.strides.reserve(dims - 1);

            for (std::size_t dim = 1; dim < m.arity(); ++dim) {
                stride *= groups[dim - 1].size();
                ++tr << "    stride for dim " << dim << " = " << stride << "\n";
                m.strides.push_back(stride);
            }
        }

        for (std::size_t dim = 0; dim < m.arity(); ++dim) {
            indent _(tr);
            std::size_t group_num = 0;

            for (auto& [mask, group] : groups[dim]) {
                ++tr << "groups for dim " << dim << ":\n";
                indent _(tr);
                ++tr << group_num << " mask " << mask << ":\n";

                for (auto cls : group.classes) {
                    indent _(tr);
                    ++tr << *cls << "\n";

                    if (cls->is_foreign()) {
                        // Written at commit, see write_global_data().
                        foreign_writes.push_back(
                            {cls->foreign_entry, cls, &m, dim, group_num, {}});
                        continue;
                    }

                    auto& entry = cls->vtbl[m.slots[dim] - cls->first_slot];
                    entry.method_index = m.index;
                    entry.vp_index = dim;
                    entry.group_index = group_num;
                }

                ++group_num;
            }
        }

        {
            ++tr << "building dispatch table\n";
            bitvec all(m.overriders.size());
            all = ~all;
            build_dispatch_table(m, dims - 1, groups.end() - 1, all, true);

            if (m.arity() > 1) {
                indent _(tr);
                m.report.cells = 1;
                ++tr << "dispatch table rank: ";
                const char* prefix = "";

                for (const auto& dim_groups : groups) {
                    m.report.cells *= dim_groups.size();
                    tr << prefix << dim_groups.size();
                    prefix = " x ";
                }

                prefix = ", concrete only: ";

                for (const auto& dim_groups : groups) {
                    auto cells = std::count_if(
                        dim_groups.begin(), dim_groups.end(),
                        [](const auto& group) {
                            return group.second.has_concrete_classes;
                        });
                    tr << prefix << cells;
                    prefix = " x ";
                }

                tr << "\n";
            }

            print(m.report);
            accumulate(m.report, report);
        }
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::build_dispatch_table(
    method& m, std::size_t dim,
    std::vector<group_map>::const_iterator group_iter, const bitvec& candidates,
    bool concrete) {
    using namespace detail;

    indent _(tr);
    std::size_t group_index = 0;

    for (const auto& [group_mask, group] : *group_iter) {
        auto mask = candidates & group_mask;

        if constexpr (has_trace) {
            ++tr << "group " << dim << "/" << group_index << " mask " << mask
                 << "\n";
            indent _(tr);
            for (auto cls : range{group.classes.begin(), group.classes.end()}) {
                ++tr << *cls << "\n";
            }
        }

        if (dim == 0) {
            std::vector<overrider*> overriders;
            std::size_t i = 0;

            for (auto& spec : m.overriders) {
                if (mask[i]) {
                    overriders.push_back(&spec);
                }
                ++i;
            }

            if constexpr (has_trace) {
                ++tr << "select best of:\n";
                indent _(tr);

                for (auto& app : overriders) {
                    ++tr << "#" << app->spec_index << " "
                         << type_name(app->info->type) << "\n";
                }
            }

            std::vector<overrider*> dominants = overriders;
            std::size_t pick, remaining;

            select_dominant_overriders(dominants, pick, remaining);

            if (remaining == 0) {
                indent _(tr);
                ++tr << "not implemented\n";
                m.dispatch_table.push_back(&m.not_implemented);
                ++m.report.not_implemented;
            } else {
                if constexpr (!has_option<n2216>) {
                    if (remaining > 1) {
                        ++tr << "ambiguous\n";
                        m.dispatch_table.push_back(&m.ambiguous);
                        ++m.report.ambiguous;
                        continue;
                    }
                }

                auto overrider = dominants[pick];
                m.dispatch_table.push_back(overrider);
                ++tr;

                tr << "-> #" << overrider->spec_index << " "
                   << type_name(overrider->info->type)
                   << " pf = " << overrider->info->pf;

                if (remaining > 1) {
                    tr << " (ambiguous)";
                    ++m.report.ambiguous;
                }

                tr << "\n";

                // -------------------------------------------------------------
                // next

                // First remove the dominant overriders from the overriders.
                // Note that the dominants appear in the overriders in the same
                // relative order.
                auto candidate = overriders.begin();
                remaining = 0;

                for (auto dominant : dominants) {
                    if (*candidate == dominant) {
                        *candidate = nullptr;
                    } else {
                        ++remaining;
                    }

                    ++candidate;
                }

                if (remaining == 0) {
                    ++tr << "no 'next'\n";
                    overrider->next = &m.not_implemented;
                } else {
                    if constexpr (has_trace) {
                        ++tr << "for 'next', select best of:\n";
                        indent _(tr);

                        for (auto& app : overriders) {
                            if (app) {
                                ++tr << "#" << app->spec_index << " "
                                     << type_name(app->info->type) << "\n";
                            }
                        }
                    }

                    select_dominant_overriders(overriders, pick, remaining);

                    if constexpr (!has_option<n2216>) {
                        if (remaining > 1) {
                            ++tr << "ambiguous 'next'\n";
                            overrider->next = &m.ambiguous;
                            continue;
                        }
                    }

                    auto next_overrider = overriders[pick];
                    overrider->next = next_overrider;

                    ++tr << "-> #" << next_overrider->spec_index << " "
                         << type_name(next_overrider->info->type)
                         << " pf = " << next_overrider->info->pf;

                    if (remaining > 1) {
                        tr << " (ambiguous)";
                        // do not increment m.report.ambiguous, for same reason
                    }

                    tr << "\n";
                }
            }
        } else {
            build_dispatch_table(
                m, dim - 1, group_iter - 1, mask,
                concrete && group.has_concrete_classes);
        }

        ++group_index;
    }
}

inline void detail::generic_compiler::accumulate(
    const method_report& partial, report& total) {
    total.cells += partial.cells;
    total.not_implemented += partial.not_implemented != 0;
    total.ambiguous += partial.ambiguous != 0;
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::write_global_data() {
    using namespace policies;
    using namespace detail;

    // Three steps: stage, initialize the policies, commit. Everything that
    // can fail happens in the first two, and neither writes anything shared
    // except the policies' own states, which the transaction restores on
    // failure: the dispatch data is built in a local vector, and each class'
    // v-table pointer is staged in its class_, where the policies read it.
    // Only then are the shared locations patched - the method_infos' slots
    // and strides, the overriders' `next`, the class_infos' static_vptr - and
    // the dispatch data swapped in, by commit_global_data(), which is
    // `noexcept`. If a policy throws, the registry still holds the dispatch
    // state it had on entry - whatever that was - rather than pointers into a
    // vector that unwinding has just freed.

    auto dispatch_data_size = std::accumulate(
        methods.begin(), methods.end(), std::size_t(0),
        [](std::size_t sum, const method& m) {
            // msvc doesn't like (auto sum, auto& m) (C2187), go figure...
            return sum + m.dispatch_table.size();
        });
    dispatch_data_size = std::accumulate(
        classes.begin(), classes.end(), dispatch_data_size,
        [](auto sum, const auto& cls) { return sum + cls.vtbl.size(); });

    std::vector<detail::word> new_dispatch_data(dispatch_data_size);
    auto gv_first = new_dispatch_data.data();
    [[maybe_unused]] auto gv_last = gv_first + dispatch_data_size;
    auto gv_iter = gv_first;

    ++tr << "Initializing multi-method dispatch tables at " << gv_iter << "\n";

    for (auto& m : methods) {
        // Uni-methods just need an index in the method table, written at
        // commit time along with the multi-methods' slots and strides.
        if (m.infos[0]->arity() > 1) {
            if constexpr (has_trace) {
                ++tr << rflush(4, dispatch_data_size) << " " << " method #"
                     << m.dispatch_table[0]->method_index << " "
                     << type_name(m.infos[0]->method_type_id) << "\n";
                indent _(tr);

                for (auto& entry : m.dispatch_table) {
                    ++tr << "spec #" << entry->spec_index << " "
                         << spec_name(m, entry) << "\n";
                }
            }

            m.gv_dispatch_table = gv_iter;
            BOOST_ASSERT(gv_iter + m.dispatch_table.size() <= gv_last);
            gv_iter = std::transform(
                m.dispatch_table.begin(), m.dispatch_table.end(), gv_iter,
                [](auto spec) { return spec->pf; });
        }
    }

    ++tr << "'next' pointers\n";

    for (auto& m : methods) {
        auto first_info = m.infos[0];
        indent _(tr);
        ++tr << "method #" << " " << type_name(first_info->method_type_id)
             << "\n";

        for (auto& overrider : m.overriders) {
            if (overrider.next) {
                ++tr << "#" << overrider.spec_index << " "
                     << spec_name(m, &overrider) << " -> ";

                tr << "#" << overrider.next->spec_index << " "
                   << spec_name(m, overrider.next);
            } else {
                tr << "none";
            }

            tr << "\n";
        }
    }

    ++tr << "Initializing v-tables at " << gv_iter << "\n";

    for (auto& cls : classes) {
        cls.vptr = gv_iter - cls.first_slot;

        ++tr << rflush(4, gv_iter - gv_first) << " " << gv_iter << " vtbl for "
             << cls << " slots " << cls.first_slot << "-"
             << (cls.first_slot + cls.vtbl.size() - 1) << "\n";
        indent _(tr);

        for (auto& entry : cls.vtbl) {
            if (entry.method_index == no_method) {
                if constexpr (has_trace) {
                    trace_unfilled_entry(
                        cls, cls.first_slot + (&entry - cls.vtbl.data()),
                        gv_iter);
                }

                BOOST_ASSERT(gv_iter + 1 <= gv_last);
                *gv_iter++ = std::size_t(0);

                continue;
            }

            ++tr << "method #" << entry.method_index << " ";
            auto& method = methods[entry.method_index];

            if (method.arity() == 1) {
                auto spec = method.dispatch_table[entry.group_index];
                tr << "spec #" << spec->spec_index << "\n";
                indent _(tr);
                ++tr << type_name(method.infos[0]->method_type_id) << "\n";
                ++tr << spec_name(method, spec);
                BOOST_ASSERT(gv_iter + 1 <= gv_last);
                *gv_iter++ = spec->pf;
            } else {
                tr << "vp #" << entry.vp_index << " group #"
                   << entry.group_index << "\n";
                indent _2(tr);
                ++tr << type_name(method.infos[0]->method_type_id);
                BOOST_ASSERT(gv_iter + 1 <= gv_last);

                if (entry.vp_index == 0) {
                    *gv_iter++ = std::uintptr_t(
                        method.gv_dispatch_table + entry.group_index);
                } else {
                    *gv_iter++ = entry.group_index;
                }
            }

            tr << "\n";
        }
    }

    ++tr << rflush(4, dispatch_data_size) << " " << gv_iter << " end\n";

    // The entries of this registry's methods in other registries' v-tables.
    // They are written at commit, like everything else.
    if (!foreign_writes.empty()) {
        ++tr << "Entries in the v-tables of other registries, written at "
                "commit:\n";
    }

    for (auto& write : foreign_writes) {
        indent _(tr);
        auto& m = *write.method;
        ++tr << write.entry << " " << *write.cls << " method #" << m.index
             << " ";

        if (m.arity() == 1) {
            auto spec = m.dispatch_table[write.group];
            write.value = spec->pf;
            tr << "spec #" << spec->spec_index << "\n";
            indent _2(tr);
            ++tr << type_name(m.infos[0]->method_type_id) << "\n";
            ++tr << spec_name(m, spec) << "\n";
        } else {
            if (write.dim == 0) {
                write.value = std::uintptr_t(m.gv_dispatch_table + write.group);
            } else {
                write.value = write.group;
            }

            tr << "vp #" << write.dim << " group #" << write.group << "\n";
            indent _2(tr);
            ++tr << type_name(m.infos[0]->method_type_id) << "\n";
        }
    }

    // What this registry publishes for other registries' parameters: the
    // cone of the parameter's class - the class first, then the others in
    // `classes` order, which is the same for every module's copy of the
    // method - with each class's entry in the new v-tables. The dispatch data
    // is swapped into place at commit, which keeps the addresses.
    for (auto& param : foreign_parameters) {
        std::vector<const class_*> cone{param.vp};
        std::unordered_map<const class_*, std::size_t> position{{param.vp, 0}};

        for (auto& cls : classes) {
            if (&cls != param.vp && param.vp->is_base_of(&cls)) {
                position.emplace(&cls, cone.size());
                cone.push_back(&cls);
            }
        }

        auto& first = param.copies.front();
        first.cone.reserve(cone.size());

        for (auto cls : cone) {
            auto& foreign = first.cone.emplace_back();
            // The dispatch data is ours; `vptr` is const only for the users of
            // the v-tables.
            foreign.entry = const_cast<word*>(cls->vptr) + param.slot;
            foreign.is_abstract = cls->is_abstract();

            for (auto derived : cls->transitive_derived) {
                foreign.transitive_derived.push_back(position.at(derived));
            }
        }

        // Each copy gets its own, to swap in at commit, which cannot throw.
        for (auto& copy : param.copies) {
            if (&copy != &first) {
                copy.cone = first.cone;
            }

            for (auto cls : copy.overriders) {
                copy.overrider_positions.push_back(position.at(cls));
            }
        }

        if constexpr (has_trace) {
            if (&param == &foreign_parameters.front()) {
                ++tr << "Publishing to methods of other registries, "
                        "generation "
                     << (static_::st.generation + 1) << ":\n";
            }

            indent _(tr);
            ++tr << "parameter " << first.info->param << ", slot " << param.slot
                 << ", " << param.copies.size()
                 << (param.copies.size() == 1 ? " copy" : " copies")
                 << " of the method\n";
            indent _2(tr);

            for (auto cls : cone) {
                auto& foreign = first.cone[position.at(cls)];
                ++tr << "foreign#" << position.at(cls) << " is " << *cls
                     << ": entry at " << foreign.entry
                     << (foreign.is_abstract ? ", abstract" : "") << "\n";
            }

            for (auto& copy : param.copies) {
                ++tr << "overriders at positions "
                     << range{
                            copy.overrider_positions.begin(),
                            copy.overrider_positions.end()}
                     << "\n";
            }
        }
    }

    detail::registry_state_transaction<
        registry, compiler, std::tuple<Options...>>
        transaction;
    detail::initialize_policies<registry>::fn(*this, options);

    // Last statement that can throw: the trace goes through the `output`
    // policy, which is user-supplied. After the commit it would be the very
    // bug this arrangement exists to prevent - the policies keeping the
    // v-table pointers they just read from `new_dispatch_data`, which
    // unwinding frees.
    ++tr << "Installing\n";

    transaction.commit();
    commit_global_data(new_dispatch_data);
}

// The commit point. Called once every step that can fail has succeeded, and
// `noexcept` so that a throwing statement added here terminates loudly
// instead of leaving the policies pointing into `new_dispatch_data`, which
// the caller destroys on the way out.
template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::commit_global_data(
    std::vector<detail::word>& new_dispatch_data) noexcept {
    for (auto& m : methods) {
        auto first_info = m.infos[0];

        if (first_info->arity() == 1) {
            first_info->slots_strides_ptr[0] = m.slots[0];
        } else {
            auto strides_iter = std::copy(
                m.slots.begin(), m.slots.end(), first_info->slots_strides_ptr);
            std::copy(m.strides.begin(), m.strides.end(), strides_iter);
        }

        // Propagate slots_strides values from the local method_info to all
        // other module copies of the same method. Each module's `fn` has its
        // own slots_strides[] array; dispatch reads it directly, so every
        // copy must hold the same values.
        auto count = 2 * first_info->arity() - 1;

        for (auto copy : m.infos) {
            if (copy != first_info) {
                std::copy(
                    first_info->slots_strides_ptr,
                    first_info->slots_strides_ptr + count,
                    copy->slots_strides_ptr);
            }
        }

        for (auto& overrider : m.overriders) {
            if (overrider.next) {
                *overrider.info->next =
                    reinterpret_cast<void (*)()>(overrider.next->pf);
            }
        }
    }

    for (auto& cls : classes) {
        for (auto& ci : cls.ci) {
            *ci->static_vptr = cls.vptr;
        }
    }

    new_dispatch_data.swap(static_::st.dispatch_data);

    // After the swap: the entries below are in the new dispatch data. A
    // method of another registry dispatching through the old one learns that
    // its entries are gone from the generation.
    auto generation = ++static_::st.generation;

    for (auto& param : foreign_parameters) {
        for (auto& copy : param.copies) {
            copy.info->slot = param.slot;
            copy.info->cone.swap(copy.cone);
            copy.info->overriders.swap(copy.overrider_positions);
            copy.info->generation = generation;
        }
    }

    for (auto& write : foreign_writes) {
        *write.entry = write.value;
    }

    for (auto& m : methods) {
        for (auto info : m.infos) {
            for (
                auto& param :
                detail::range{info->foreign_begin, info->foreign_end}) {
                param.installed_generation = param.generation;
            }
        }
    }
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::select_dominant_overriders(
    std::vector<overrider*>& candidates, std::size_t& pick,
    std::size_t& remaining) {

    pick = 0;
    remaining = 0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        if (candidates[i]) {
            for (size_t j = i + 1; j < candidates.size(); ++j) {
                if (candidates[j]) {
                    if (is_more_specific(candidates[i], candidates[j])) {
                        candidates[j] = nullptr;
                    } else if (is_more_specific(candidates[j], candidates[i])) {
                        candidates[i] = nullptr;
                        break; // this one is dead
                    }
                }
            }
        }

        if (candidates[i]) {
            pick = i;
            ++remaining;
        }
    }

    if (remaining <= 1) {
        return;
    }

    if constexpr (has_option<n2216>) {
        if (!candidates[pick]->covariant_return_type) {
            return;
        }

        remaining = 0;

        for (size_t i = 0; i < candidates.size(); ++i) {
            if (candidates[i]) {
                for (size_t j = i + 1; j < candidates.size(); ++j) {
                    if (candidates[j]) {
                        BOOST_ASSERT(candidates[i] != candidates[j]);

                        if (candidates[i]->covariant_return_type->is_base_of(
                                candidates[j]->covariant_return_type)) {
                            candidates[i] = nullptr;
                        } else if (
                            candidates[j]->covariant_return_type->is_base_of(
                                candidates[i]->covariant_return_type)) {
                            candidates[j] = nullptr;
                        }
                    }
                }
            }

            if (candidates[i]) {
                pick = i;
                ++remaining;
            }
        }
    }
}

template<class... Policies>
template<class... Options>
auto registry<Policies...>::compiler<Options...>::is_more_specific(
    const overrider* a, const overrider* b) -> bool {
    bool result = false;

    auto a_iter = a->vp.begin(), a_last = a->vp.end(), b_iter = b->vp.begin();

    for (; a_iter != a_last; ++a_iter, ++b_iter) {
        if (*a_iter != *b_iter) {
            if ((*b_iter)->transitive_derived.find(*a_iter) !=
                (*b_iter)->transitive_derived.end()) {
                result = true;
            } else if (
                (*a_iter)->transitive_derived.find(*b_iter) !=
                (*a_iter)->transitive_derived.end()) {
                return false;
            }
        }
    }

    return result;
}

template<class... Policies>
template<class... Options>
auto registry<Policies...>::compiler<Options...>::is_base(
    const overrider* a, const overrider* b) -> bool {
    bool result = false;

    auto a_iter = a->vp.begin(), a_last = a->vp.end(), b_iter = b->vp.begin();

    for (; a_iter != a_last; ++a_iter, ++b_iter) {
        if (*a_iter != *b_iter) {
            if ((*a_iter)->transitive_derived.find(*b_iter) ==
                (*a_iter)->transitive_derived.end()) {
                return false;
            } else {
                result = true;
            }
        }
    }

    return result;
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::print(
    const method_report& r) const {
    ++tr;

    if (r.cells) {
        // only for multi-methods, uni-methods don't have dispatch tables
        ++tr << r.cells << " dispatch table cells, ";
    }

    tr << r.not_implemented << " not implemented, " << r.ambiguous
       << " ambiguous\n";
}

template<class... Policies>
template<class... Options>
void registry<Policies...>::compiler<Options...>::print_slots() {
    if constexpr (has_trace) {
        if (!tr.on) {
            return;
        }

        using namespace detail;

        // Slot numbering is scoped to a connected component of the class
        // graph, so partition the classes into components first. A component
        // is identified by the classes in it that have no direct bases; there
        // is more than one only in presence of multiple inheritance.
        std::unordered_map<const class_*, std::size_t> component;
        std::size_t component_count = 0;

        for (auto& seed : classes) {
            if (component.find(&seed) != component.end()) {
                continue;
            }

            auto id = component_count++;
            component[&seed] = id;
            std::vector<class_*> todo{&seed};

            while (!todo.empty()) {
                auto cls = todo.back();
                todo.pop_back();

                for (
                    auto neighbors :
                    {&cls->direct_bases, &cls->direct_derived}) {
                    for (auto next : *neighbors) {
                        if (component.emplace(next, id).second) {
                            todo.push_back(next);
                        }
                    }
                }
            }
        }

        // Collect the members in 'classes' order, for a deterministic trace.
        std::vector<std::vector<class_*>> lattices(component_count);

        for (auto& cls : classes) {
            lattices[component[&cls]].push_back(&cls);
        }

        ++tr << "Used slots:\n";
        indent _(tr);

        for (auto& lattice : lattices) {
            std::vector<class_*> roots;

            for (auto cls : lattice) {
                if (cls->direct_bases.empty()) {
                    roots.push_back(cls);
                }
            }

            ++tr << roots << "\n";
            indent _2(tr);

            struct entry {
                std::size_t slot;
                class_* cls;
                parameter mp;
            };

            std::vector<entry> slots;

            for (auto cls : lattice) {
                for (const auto& mp : cls->used_by_vp) {
                    slots.push_back({*mp.slot, cls, mp});
                }
            }

            std::stable_sort(
                slots.begin(), slots.end(),
                [](const entry& a, const entry& b) { return a.slot < b.slot; });

            ++tr << "slots:\n";

            {
                indent _3(tr);

                for (const auto& e : slots) {
                    ++tr << e.slot << ": in " << *e.cls << " for "
                         << parameter_name{e.mp} << "\n";
                }
            }

            ++tr << "v-tables:\n";

            {
                indent _3(tr);

                for (auto cls : lattice) {
                    if (cls->vtbl.empty()) {
                        continue;
                    }

                    ++tr << *cls << " slots " << cls->first_slot << "-"
                         << (cls->first_slot + cls->vtbl.size() - 1) << "\n";
                }
            }
        }
    }
}

//! Initialize a registry.
//!
//! Initialize the @ref registry passed as an explicit function template
//! argument, or @ref default_registry if the registry is not specified. The
//! default can be changed by defining @ref BOOST_OPENMETHOD_DEFAULT_REGISTRY.
//! Option objects can be passed to change the behavior of the function.
//! Currently two options exist:
//! @li @ref trace Enable tracing of the initialization process.
//! @li @ref n2216 Enable resolution of ambiguities according to the N2216
//! paper.
//!
//! `initialize` must be called, typically at the beginning of `main`, before
//! using any of the methods in a registry. It sets up the v-tables,
//! multi-method dispatch tables, and any other data required by the policies.
//!
//! The function returns an object of an unspecified type that contains a
//! `report` member, itself an object of an unspecified type, thatcontains the
//! following members:
//! @li @c std::size_t @c cells: The number of cells in all multi-method dispatch
//! tables.
//! @li @c std::size_t @c not_implemented: The number of multi-method dispatch tables that
//! contain at least one not implemented entry.
//! @li @c std::size_t @c ambiguous: The number of multi-method dispatch tables that contain at
//! least one ambiguous entry.
//!
//! @note
//! A translation unit that calls `initialize` must include the
//! `<boost/openmethod/initialize.hpp>` header.
//!
//! @note
//! Each policy's `initialize` is called in the order the policies appear in the
//! registry's `policy_list` (left to right). If a policy depends on another
//! policy having been initialized first, the dependency must appear *earlier*
//! in the list; ensuring this is the user's responsibility. @ref finalize
//! finalizes the policies in the reverse order.
//!
//! @tparam Registry The registry to initialize.
//! @tparam Options... Zero or more option types, deduced from the function
//! arguments.
//! @param options Zero or more option objects.
//! @return An object of an unspecified type.
//!
//! @par Errors
//!
//! @li @ref missing_class. A class used in a virtual parameter was not
//! registered.
//! @li The registry's policies may report additional errors.
//!
//! @par Exception safety
//!
//! If `initialize` throws - because a policy's `initialize` does, or an
//! allocation fails - the registry keeps the dispatch state it had before the
//! call: no static v-table pointer, `next` pointer, dispatch table or policy
//! state is modified. The registry is nonetheless marked as not initialized,
//! since that state does not reflect the current registrations; `initialize`
//! must be called again, successfully, before any method is called.
//!
//! Only the policies that define `initialize` have their `state` saved and
//! restored, so only those states have to be copyable. A state that no
//! `initialize` writes to is configuration, not derived data, and is left
//! alone: an @ref error_handler policy is *called* during `initialize`, and a
//! handler that changes the configuration - installing a different handler
//! with `set`, say - keeps that change whether the call succeeds or throws.
//!
//! @par Example
//!
//! Initialize the default registry with tracing enabled, and exit with an error
//! message if there were any possibility of a @reg bad_call error. User may run
//! the program again after setting environment variable
//! `BOOST_OPENMETHOD_TRACE` to `1` to troubleshoot.
//!
//! include:initialize.cpp#report
//!
//! @see [Methods and Overriders](xref:ROOT:basics.adoc)
//! @see [Shared Libraries](xref:ROOT:shared_libraries.adoc)
template<class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY, class... Options>
inline auto initialize(Options&&... options) {
    if (detail::odr_check<Registry>::count > 1) {
        // Multiple definitions of default_registry detected.
        // This indicates an ODR violation.
        // Signal a final_error using the error handler, then abort.
        if constexpr (Registry::has_error_handler) {
            Registry::error_handler::error(odr_violation());
        }

        std::abort();
    }

    typename Registry::template compiler<Options...> comp(
        std::forward<Options>(options)...);
    comp.initialize();

    return comp;
}

namespace detail {

// See the BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN_MSVC_SAFE definition near
// has_initialize, above, for why this needs the MSVC-safe form too.
BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN_MSVC_SAFE(finalize);

// Call finalize on a single policy if it has the function. Mirror of
// initialize_policy.
template<typename Policy, typename Registry, typename Options>
void finalize_policy(const Options& options) {
    using PolicyFn = typename Policy::template fn<Registry>;
    if constexpr (has_finalize<PolicyFn, const Options&>) {
        PolicyFn::finalize(options);
    }
}

// Policies are finalized in the reverse of `policy_list` order — the mirror of
// initialize_policies — so a policy is torn down before the ones it depends on.
template<
    typename Registry,
    typename Policies = mp11::mp_reverse<typename Registry::policy_list>>
struct finalize_policies;

template<typename Registry, typename... Policies>
struct finalize_policies<Registry, mp11::mp_list<Policies...>> {
    template<typename Options>
    static void fn(const Options& options) {
        (finalize_policy<Policies, Registry>(options), ...);
    }
};

} // namespace detail

template<class... Policies>
template<class... Options>
auto registry<Policies...>::finalize(Options... opts) -> void {
    std::tuple<Options...> options(opts...);
    detail::finalize_policies<registry>::fn(options);
    static_::st.dispatch_data.clear();
    ++static_::st.generation;
    static_::st.initialized = false;
}

//! Release resources held by registry.
//!
//! `finalize` may be called to release any resources allocated by
//! @ref boost::openmethod::initialize.
//!
//! @note
//! A translation unit that contains a call to `finalize` must include the
//! `<boost/openmethod/initialize.hpp>` header.
//!
//! @note
//! Each policy's `finalize` is called in the reverse of the registry's
//! `policy_list` order, the mirror of @ref initialize, so a policy is finalized
//! before the policies it depends on.
//!
//! @tparam Registry The registry to finalize.
//! @tparam Options... Zero or more option types, deduced from the function
//! arguments.
//! @param options Zero or more option objects.
//!
//! @see [Shared Libraries](xref:ROOT:shared_libraries.adoc)
template<class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY, class... Options>
inline auto finalize(Options&&... opts) -> void {
    Registry::finalize(std::forward<Options>(opts)...);
}

} // namespace boost::openmethod

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
