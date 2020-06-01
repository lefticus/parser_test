#ifndef PARSER_TEST_PARSER_HPP
#define PARSER_TEST_PARSER_HPP

#include <array>
#include <iostream>
#include <optional>
#include <utility>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <ctre.hpp>

// references
// http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
// https://github.com/munificent/bantam/tree/master/src/com/stuffwithstuff/bantam
// https://github.com/MattDiesel/cpp-pratt
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
// https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
// https://stackoverflow.com/questions/380455/looking-for-a-clear-definition-of-what-a-tokenizer-parser-and-lexers-are


namespace parser_test {
// Helper function for concatenating two different ct11::fixed_strings
template <std::size_t N1, std::size_t N2>
constexpr auto operator+(const ctll::fixed_string<N1> &lhs,
                         const ctll::fixed_string<N2> &rhs) {
  char32_t result[N1 + N2 - 1]{};

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
    semicolon,
    end_of_file
  };

  struct lex_item {
    Type type{Type::unknown};
    std::string_view match;
    std::string_view remainder;
  };


  lex_item token;
  
  

  constexpr auto parse(std::string_view v) {
    token = next_token(v);
    return expression();
  }

  constexpr void match(const Type type) {
    if (token.type != type) {
      throw std::runtime_error("uhoh");
    }
    token = next();
  }

  constexpr double nud(const lex_item &item)
  {
    switch(item.type) {
      case Type::number:
        return std::stod(std::string{item.match});
      case Type::plus:
        return expression(100);
      case Type::minus:
        return -expression(100);
      case Type::left_paren: {
        const auto result = expression();
        match(Type::right_paren);
        return result;
      }
      default:
        throw std::runtime_error("Unhandled nud");
    };
  }

  constexpr double led(const lex_item &item, const double left)
  {
    switch(item.type) {
      case Type::plus:
        return left + expression(10);
      case Type::minus:
        return left - expression(10);
      case Type::asterisk:
        return left * expression(20);
      case Type::slash:
        return left / expression(20);
      case Type::caret:
        return std::pow(left, expression(30-1));
      default:
        throw std::runtime_error("Unhandled led");
    }
  }

  static constexpr auto lbp(const Type type) {
    switch(type) {
      case Type::plus: return 10;
      case Type::minus: return 10;
      case Type::asterisk: return 20;
      case Type::slash: return 20;
      case Type::caret: return 30;
      case Type::left_paren: return 0;
      case Type::right_paren: return 0;
      case Type::end_of_file: return 0;
      default:
        throw std::runtime_error(fmt::format("Unhandled value lbp {}", static_cast<int>(type)));
    }

  }

  constexpr double expression(int rbp = 0)
  {
    const auto prefix = token;
    token = next();
    auto left = nud(prefix);

    while (rbp < lbp(token.type)) {
      const auto t = token;
      token = next();
      left = led(t, left);
    }

    return left;
  }

  constexpr lex_item next() noexcept {
    return next_token(token.remainder);
  }

  static constexpr lex_item next_token(std::string_view v) noexcept {
    while (true) {
      if (const auto item = lexer(v); item) {
        if (item->type != Type::whitespace) {
          return *item;
        } else {
          v = item->remainder;
        }
      } else {
        return lex_item{Type::unknown, v, v};
      }
    }
  }

  static constexpr std::optional<lex_item> lexer(std::string_view v) noexcept {
    // prefix with ^ so that the regex search only matches the start of the
    // string Hana assures me this is efficient ;)

    if (v.empty()) {
      return lex_item{Type::end_of_file, v, v};
    }

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

    if (auto id_result = ctre::search<identifier>(v); id_result) {
      return ret(Type::identifier, id_result);
    } else if (auto string_result = ctre::search<quoted_string>(v); string_result) {
      return ret(Type::string, string_result);
    } else if (auto number_result = ctre::search<number>(v); number_result) {
      return ret(Type::number, number_result);
    }

    return std::nullopt;
  }


};
}

#endif


