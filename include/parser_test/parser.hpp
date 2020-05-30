#ifndef PARSER_TEST_PARSER_HPP
#define PARSER_TEST_PARSER_HPP

#include <array>
#include <ctre.hpp>
#include <iostream>
#include <optional>
#include <utility>


namespace parser_test {
// Helper function for concatenating two different ct11::fixed_strings
template <std::size_t N1, std::size_t N2>
constexpr auto operator+(const ctll::fixed_string<N1> &lhs,
                         const ctll::fixed_string<N2> &rhs) {
  char32_t result[N1 + N2 + 1]{};

  auto iter = std::begin(result);
  iter = std::copy(lhs.begin(), lhs.end(), iter);
  std::copy(rhs.begin(), rhs.end(), iter);
  return ctll::fixed_string(result);
}

struct Parser {
  enum class Type {
    unknown,
    identifier,
    number,
    increment,
    decrement,
    left_paren,
    right_paren,
    comma,
    assign,
    plus,
    minus,
    asterisk,
    slash,
    caret,
    tilde,
    bang,
    question,
    colon,
    string,
    whitespace,
    semicolon
  };

  struct lex_item {
    Type type;
    std::string_view match;
    std::string_view remainder;
  };

  static constexpr std::optional<lex_item> lexer(std::string_view v) noexcept {
    // prefix with ^ so that the regex search only matches the start of the
    // string Hana assures me this is effecient ;)
    constexpr auto make_token = [](const auto &s) {
      return ctll::fixed_string{"^"} + ctll::fixed_string{s};
    };

    const auto ret = [v](const Type type, std::string_view found) -> lex_item {
      return {type, v.substr(0, found.size()), v.substr(found.size())};
    };

    constexpr auto identifier{make_token("[_a-zA-Z]+[_0-9a-zA-Z]*")};
    constexpr auto quoted_string{make_token(R"("([^"\\]|\\.)*")")};
    constexpr auto number{make_token("[0-9]+")};
    constexpr auto whitespace{make_token("\\s+")};

    using token = std::pair<std::string_view, Type>;
    constexpr std::array tokens{
        token{"++", Type::increment},  token{"--", Type::decrement},
        token{":", Type::colon},       token{",", Type::comma},
        token{"=", Type::assign},      token{"+", Type::plus},
        token{"-", Type::minus},       token{"*", Type::asterisk},
        token{"/", Type::slash},       token{"^", Type::caret},
        token{"~", Type::tilde},       token{"!", Type::bang},
        token{"?", Type::question},    token{"(", Type::left_paren},
        token{")", Type::right_paren}, token{";", Type::semicolon}};

    if (auto result = ctre::search<whitespace>(v); result) {
      return ret(Type::whitespace, result);
    }

    for (const auto &oper : tokens) {
      if (v.starts_with(oper.first)) {
        return ret(oper.second, oper.first);
      }
    }

    if (auto result = ctre::search<identifier>(v); result) {
      return ret(Type::identifier, result);
    } else if (auto result = ctre::search<quoted_string>(v); result) {
      return ret(Type::string, result);
    } else if (auto result = ctre::search<number>(v); result) {
      return ret(Type::number, result);
    }

    return std::nullopt;
  }
};
}

#endif


