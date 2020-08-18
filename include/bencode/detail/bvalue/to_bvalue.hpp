#pragma once

#include "bencode/detail/bvalue/bvalue_policy.hpp"

namespace bencode::events {

/// event_consumer that generates a bvalue from parser events.
template<typename Policy = default_bvalue_policy>
class to_bvalue {
public:
    using basic_value_type = basic_bvalue<Policy>;
    using integer_type = typename detail::policy_integer_t<Policy>;
    using string_type = typename detail::policy_string_t<Policy>;
    using list_type = typename detail::policy_list_t<Policy>;
    using dict_type = typename detail::policy_dict_t<Policy>;

    explicit to_bvalue() = default;

    to_bvalue(const to_bvalue&) = delete;

    to_bvalue(to_bvalue&&) = delete;

    void integer(std::int64_t value)
    { value_.emplace_integer(value); }

    void string(std::string_view value)
    { value_.emplace_string(value); }

    void string(const std::string& value)
    { value_.emplace_string(value); }

    void string(std::string&& value)
    { value_.emplace_string(std::move(value)); }

    void begin_list([[maybe_unused]] std::optional<std::size_t> size = std::nullopt)
    {
        auto& l = stack_.emplace(bencode::btype::list);

        if constexpr (bencode::detail::has_reserve_member<list_type>) {
            if (size.has_value()) {
                get_list(l).reserve(size.value());
            }
        }
    };

    void list_item()
    {
        get_list(stack_.top()).push_back(std::move(value_));
        value_.discard();
    };

    void end_list([[maybe_unused]] std::optional<std::size_t> size = std::nullopt)
    {
        value_ = std::move(stack_.top());
        stack_.pop();
    };

    void begin_dict([[maybe_unused]] std::optional<std::size_t> size = std::nullopt)
    {
        stack_.push(basic_value_type(btype::dict));
    };

    void end_dict([[maybe_unused]] std::optional<std::size_t> size = std::nullopt)
    {
        value_ = std::move(stack_.top());
        stack_.pop();
    };

    void dict_key()
    {
        Expects(is_string(value_));
        keys_.emplace(get_string(std::move(value_)));
    };

    void dict_value()
    {
        auto& d = get_dict(stack_.top());
        d.insert_or_assign(std::move(keys_.top()), std::move(value_));
        value_.discard();
        keys_.pop();
    };

    [[nodiscard]]
    basic_value_type value()
    {
        Expects(stack_.empty());
        Expects(keys_.empty());
        return std::move(value_);
    }

    void error(bencode::parse_error& e)
    {
        throw e;
    }

private:
    basic_value_type value_;
    std::stack<basic_value_type> stack_;
    std::stack<string_type> keys_;
};

} // namespace bencode::events