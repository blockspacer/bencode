#pragma once

#include <concepts>
#include <ranges>

#include <gsl/gsl_assert>

#include "bview.hpp"
#include "integer_bview.hpp"
#include "bencode/detail/concepts.hpp"
#include "bencode/detail/utils.hpp"
#include "bencode/detail/events/events.hpp"
#include "bencode/detail/bencode_type.hpp"


namespace bencode::detail {

namespace rng = std::ranges;

template <event_consumer EC>
constexpr void connect_events_default_integer_impl(
        customization_point_type<integer_bview>,
        EC& consumer,
        const integer_bview& desc,
        detail::priority_tag<0>)
{
    consumer.integer(static_cast<std::int64_t>(desc));
}

template <event_consumer EC>
constexpr void connect_events_default_string_impl(
        customization_point_type<string_bview>,
        EC& consumer,
        const string_bview& desc,
        detail::priority_tag<0>)
{
    consumer.string(std::string_view(desc));
}

template <event_consumer EC>
constexpr void connect_events_default_list_impl(
        customization_point_type<list_bview>,
        EC consumer,
        const list_bview& desc,
        detail::priority_tag<0>)
{
    consumer.begin_list(rng::size(desc));
    for (const auto& e : desc) {
        connect(consumer, e);
        consumer.element();
    }
    consumer.end_list(rng::size(desc));
}

template <event_consumer EC>
constexpr void connect_events_default_dict_impl(
        customization_point_type<dict_bview>, EC consumer,
        const dict_bview& desc,
        detail::priority_tag<0>)
{
        consumer.begin_dict(rng::size(desc));
        for (auto it = rng::begin(desc); it != rng::end(desc); ++it) {
            consumer.string(it.key());
            consumer.key();
            connect(consumer, it.value());
            consumer.member();
        }
        consumer.end_dict(rng::size(desc));
}


template <event_consumer EC, typename U>
constexpr void connect_events_default_runtime_impl(
        customization_point_type<bview>,
        EC consumer,
        const bview& bref,
        detail::priority_tag<1>)
{
    using value_type = bview;
    switch (bref.type()) {
        case bencode_type::integer:  connect(consumer, get_integer(bref)); break;
        case bencode_type::string:   connect(consumer, get_string(bref));  break;
        case bencode_type::list:     connect(consumer, get_list(bref));    break;
        case bencode_type::dict:     connect(consumer, get_dict(bref));    break;
        default: Ensures(false);
    }
}

}