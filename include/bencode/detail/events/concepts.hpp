#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>

#include "bencode/detail/utils.hpp"
#include "bencode/detail/concepts.hpp"

namespace bencode {

/// A type satisfies the concept `event_consumer` when is has following member functions.
///
template <typename T>
concept event_consumer =
    requires(T& t) {
        t.integer(std::int64_t{});
        t.string(std::string_view{});
        t.begin_list();
        t.begin_list(std::size_t{});
        t.end_list();
        t.end_list(std::size_t{});
        t.list_item();
        t.begin_dict();
        t.begin_dict(std::size_t{});
        t.end_dict();
        t.end_dict(std::size_t{});
        t.dict_key();
        t.dict_value();
    };

namespace detail {
/// Helper type that satisfies event_consumer to.
/// Used to make concept definition for event_producer independant of the consumer type.
struct discard_consumer
{
    void integer(std::int64_t value) { }
    void string(std::string_view value) { }
    void begin_list() {}
    void begin_list(std::size_t size) {}
    void end_list() {}
    void end_list(std::size_t size) {}
    void list_item() {}
    void begin_dict() {}
    void begin_dict(std::size_t size) {}
    void end_dict() {}
    void end_dict(std::size_t size) {}
    void dict_key() {}
    void dict_value() {}
};

/// The concept `event_connecting_is_adl_overloaded` is satisfied when a user defined
/// overload of bencode_connect is found for given type.
template<typename T>
concept event_connecting_is_adl_overloaded =
    requires(discard_consumer consumer, T& producer) {
        bencode_connect(customization_for<T>, consumer, producer);
    };
} // namespace detail


// forward declaration
template <typename EC, typename U, typename T = std::remove_cvref_t<U>>
    requires serializable<T> && event_consumer<EC>
constexpr void connect(EC& consumer, U&& producer);


/// The concept `event_producer` is satisfied when a type can produce bencode events for
/// an event_consumer through the `connect` function.
template <typename T>
concept event_producer = serializable<T> &&
    requires(detail::discard_consumer consumer, T& producer) {
        connect(consumer, producer);
    };


namespace detail {

template <typename T>
consteval bool tuple_elements_event_producers_helper()
{
    using IS = std::make_index_sequence<std::tuple_size<T>::value>;
    auto r = std::apply(
            []<std::size_t... Is2>(auto&&... v) constexpr {
                return (event_producer<std::tuple_element_t<Is2, T>> && ...);
            },
            std::forward_as_tuple(IS{})
    );

    if constexpr (r) { return true; }
    static_assert(detail::always_false_v<T>, "Not all events satisfy event_producer.");
}

template <typename T>
concept tuple_elements_are_event_producers =
requires (T& t) {
    tuple_elements_event_producers_helper<T>();
};

}

} // namespace bencode