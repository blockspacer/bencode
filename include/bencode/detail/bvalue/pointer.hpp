#pragma once

#include <nonstd/expected.hpp>
#include "bencode/detail/bvalue/concepts.hpp"
#include "bencode/detail/bpointer.hpp"
#include "bencode/detail/bencode_type.hpp"
#include "bencode/detail/out_of_range.hpp"


namespace bencode::detail {

template <basic_bvalue_instantiation BV>
inline decltype(auto) evaluate(const bpointer& pointer, BV&& bv)
{
    auto* ptr = &bv;

    for (const auto& token : pointer) {
        switch (ptr->type()) {
        case bencode_type::dict: {
            ptr = &(get_dict(*ptr).at(token));
            break;
        }
        case bencode_type::list: {
            if (token == "-") [[unlikely]]
                throw out_of_range("unresolved token '-': list index '-' is not supported");

            auto res = detail::parse_integer<std::uint64_t>(token.begin(), token.end());
            if (!res) [[unlikely]]
                throw out_of_range(
                        fmt::format("unresolved token '{}': expected list index", token));

            ptr = &(get_list(*ptr).at(res.value()));
            break;
        }
        default:
            throw out_of_range(
                    fmt::format("unresolved token '{}': expected list or dict but got {}",
                                token, ptr->type()));
        }
    }
    return detail::forward_like<BV>(*ptr);
}


template <basic_bvalue_instantiation BV>
inline bool contains(const bpointer& pointer, const BV& bv)
{
    auto* ptr = &bv;

    for (const auto& token : pointer) {
        switch (ptr->type()) {
        case bencode_type::dict: {
            auto& d = get_dict(*ptr);
            auto it = d.find(token);
            if (it == d.end())
                return false;

            ptr = &(it->second);
            break;
        }
        case bencode_type::list: {
            if (token == "-") [[unlikely]]
                return false;

            auto res = detail::parse_integer<std::uint64_t>(token.begin(), token.end());
            if (!res) [[unlikely]]
                return false;

            auto& l = get_list(*ptr);
            if (l.size() <= res.value())
                return false;

            ptr = &(l[res.value()]);
            break;
        }
        default:
            return false;
        }
    }
    return true;
}

}