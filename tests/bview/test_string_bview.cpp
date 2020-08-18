#include <catch2/catch.hpp>

#include "bencode/bview.hpp"
#include "bencode/traits/all.hpp"
#include <type_traits>
#include <string>
#include <string_view>

using namespace std::string_literals;
namespace bc = bencode;

constexpr std::string_view data_string  = "4:spam";

constexpr std::array descriptors_string  = {
        bc::descriptor(bc::descriptor_type::string, 0, 2U, 4U),
        bc::descriptor(bc::descriptor_type::stop, 6),
};

constexpr auto s_view_const = bc::bview(begin(descriptors_string), data_string.data());
static auto s_view = bc::bview(begin(descriptors_string), data_string.data());


TEST_CASE("test string_bview") {
    bc::string_bview string(begin(descriptors_string), data_string.data());

    SECTION("construction") {
        SECTION("default construction") {
            bc::string_bview bv{};
            CHECK(is_uninitialized(bv));
        }
        SECTION("construction from pointers") {
            bc::string_bview bv(begin(descriptors_string), data_string.data());
            CHECK(is_string(bv));
        }
        SECTION("converting constructor") {
            bc::string_bview bv(s_view_const);
            CHECK(is_string(bv));
        }
    }
    SECTION("assignment") {
        bc::string_bview bv = string;
        CHECK(is_string(bv));
    }

    SECTION("equality comparison") {
        bc::string_bview bv = string;
        CHECK(bv == string);
    }
    SECTION("order comparison") {
        SECTION("with string_bview") {
            bc::string_bview bv = string;
            CHECK_FALSE(bv<string);
            CHECK_FALSE(bv>string);
            CHECK(bv>=string);
            CHECK(bv<=string);
        }
        SECTION("with std::string_view") {
            CHECK(string == "spam");
            CHECK_FALSE(string != "spam");
            CHECK(string < "zzzzzz");
            CHECK(string <= "zzzzzz");
            CHECK(string > "aaa");
            CHECK(string >= "aaa");
            CHECK(string >= "spam");
            CHECK(string <= "spam");
        }
    }
    SECTION("at()") {
        auto ref = string.at(0);
        CHECK(ref == 's');
        CHECK_THROWS_AS(string.at(6), std::out_of_range);
    }
    SECTION("front()") {
        CHECK(string.front() == 's');
    }
    SECTION("back()") {
        CHECK(string.back() == 'm');
    }
    SECTION("substr") {
        auto s1 = string.substr(1, bc::string_bview::npos);
        CHECK(s1 == "pam");

        auto s2 = string.substr(1, 2);
        CHECK(s2 == "pa");
    }

    SECTION("starts_with") {
        CHECK(string.starts_with("sp"));
        CHECK(string.starts_with('s'));
    }
    SECTION("ends_with") {
        CHECK(string.ends_with("am"));
        CHECK(string.ends_with('m'));
    }
    SECTION("empty()") {
        CHECK_FALSE(string.empty());
    }
    SECTION("size()/length()"){
        CHECK(string.size() == 4);
        CHECK(string.length() == 4);
    }
    SECTION("max_size()") {
        CHECK(string.max_size() == std::numeric_limits<std::uint32_t>::max());
    }
    SECTION("max_size()") {
        CHECK(string.max_size() == std::numeric_limits<std::uint32_t>::max());
    }

    SECTION("iterators") {
        auto first = string.begin();
        ++first;
        ++first; // points to string end
        ++first; // points to string end
        ++first; // points to string end
        CHECK(first == string.end());
        --first;
        --first; // point to string begin
        --first;
        --first;
        CHECK(first == string.begin());
        CHECK(std::distance(string.begin(), string.end()) == string.size());
        CHECK(std::distance(string.cbegin(), string.cend()) == string.size());
        CHECK(std::distance(string.rbegin(), string.rend()) == string.size());
        CHECK(std::distance(string.crbegin(), string.crend()) == string.size());
    }

    SECTION("bencoded_view") {
        CHECK(string.bencoded_view() == data_string);
    }

    SECTION("comparison with bview") {
        CHECK(s_view == string);
        CHECK_FALSE(s_view != string);
        CHECK(s_view <= string);
        CHECK(s_view >= string);
    }
}