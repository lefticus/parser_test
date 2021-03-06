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
template<std::size_t N1, std::size_t N2>
constexpr auto operator+(const ctll::fixed_string<N1> &lhs, const ctll::fixed_string<N2> &rhs)
{
  char32_t result[N1 + N2 - 1]{};

  auto iter = std::begin(result);
  iter = std::copy(lhs.begin(), lhs.end(), iter);
  std::copy(rhs.begin(), rhs.end(), iter);
  return ctll::fixed_string(result);
}

struct Parser
{
  enum class Type {
    unknown = 0,
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

  struct lex_item
  {
    Type type{ Type::unknown };
    std::string_view match;
    std::string_view remainder;
  };


  lex_item token;


  constexpr auto parse(std::string_view v)
  {
    token = next_token(v);
    return expression();
  }

  constexpr void match(const Type type)
  {
    if (token.type != type) { throw std::runtime_error("uhoh"); }
    token = next();
  }

  constexpr int nud(const lex_item &item)
  {
    constexpr auto prefix_precedence = static_cast<int>(Precedence::prefix);

    switch (item.type) {
    case Type::number: return std::stoi(std::string{ item.match });
    case Type::plus: return expression(prefix_precedence);
    case Type::minus: return -expression(prefix_precedence);
    case Type::left_paren: {
      const auto result = expression();
      match(Type::right_paren);
      return result;
    }
    default: throw std::runtime_error("Unhandled nud");
    };
  }

  constexpr int factorial(int value)
  {
    int result = 1;
    while (value-- > 1)
    {
      result *= value;
    }
    return result;
  }

  constexpr int led(const lex_item &item, const int left)
  {
    switch (item.type) {
    case Type::plus: return left + expression(lbp(item.type));
    case Type::minus: return left - expression(lbp(item.type));
    case Type::asterisk: return left * expression(lbp(item.type));
    case Type::slash: return left / expression(lbp(item.type));
    case Type::bang: return factorial(left);
    case Type::caret:
      // caret, as an infix, is right-associative, so we decrease its
      // precedence slightly
      // https://github.com/munificent/bantam/blob/8b0b06a1543b7d9e84ba2bb8d916979459971b2d/src/com/stuffwithstuff/bantam/parselets/BinaryOperatorParselet.java#L20-L27
      // note that this https://eli.thegreenplace.net/2010/01/02/top-down-operator-precedence-parsing
      // example disagrees slightly, it provides a gap in precedence, where
      // the bantam example notches it down to share with other levels
      return static_cast<int>(std::pow(left, expression(lbp(item.type) - 1)));
    default: throw std::runtime_error("Unhandled led");
    }
  }

  enum struct Precedence {
    none = 0,
    assignment = 1,
    conditional = 2,
    sum = 3,
    product = 4,
    exponent = 5,
    prefix = 6,
    postfix = 7,
    call = 8
  };

  // this is for infix precedence values
  // lbp = left binding power
  static constexpr int lbp(const Type type)
  {
    const auto precedence = [type]() -> Precedence {
      switch (type) {
      case Type::plus: return Precedence::sum;
      case Type::minus: return Precedence::sum;
      case Type::asterisk: return Precedence::product;
      case Type::slash: return Precedence::product;
      case Type::caret: return Precedence::exponent;
      case Type::bang: return Precedence::postfix;
      case Type::right_paren: return Precedence::none;// right_paren will be parsed but not consumed when matching
      case Type::end_of_file: return Precedence::none;
      default: throw std::runtime_error(fmt::format("Unhandled value lbp {}", static_cast<int>(type)));
      }
    }();

    return static_cast<int>(precedence);
  }

  constexpr int expression(int rbp = 0)
  {
    // parse prefix token
    const auto prefix = token;
    token = next();
    auto left = nud(prefix);

    // parse infix / postfix tokens
    while (rbp < lbp(token.type)) {
      const auto t = token;
      token = next();
      left = led(t, left);
    }

    return left;
  }

  constexpr lex_item next() noexcept { return next_token(token.remainder); }

  static constexpr lex_item next_token(std::string_view v) noexcept
  {
    while (true) {
      if (const auto item = lexer(v); item) {
        if (item->type != Type::whitespace) {
          return *item;
        } else {
          v = item->remainder;
        }
      } else {
        return lex_item{ Type::unknown, v, v };
      }
    }
  }

  static constexpr std::optional<lex_item> lexer(std::string_view v) noexcept
  {

    if (v.empty()) { return lex_item{ Type::end_of_file, v, v }; }

    // prefix with ^ so that the regex search only matches the start of the
    // string. Hana assures me this is efficient ;) (at runtime that is)
    constexpr auto make_token = [](const auto &s) { return ctll::fixed_string{ "^" } + ctll::fixed_string{ s }; };

    const auto ret = [v](const Type type, std::string_view found) -> lex_item {
      return { type, v.substr(0, found.size()), v.substr(found.size()) };
    };

    constexpr auto identifier{ make_token("[_a-zA-Z]+[_0-9a-zA-Z]*") };
    constexpr auto quoted_string{ make_token(R"("([^"\\]|\\.)*")") };
    constexpr auto number{ make_token("[0-9]+") };
    constexpr auto whitespace{ make_token("\\s+") };

    using token = std::pair<std::string_view, Type>;
    constexpr std::array tokens{ token{ "++", Type::increment },
      token{ "--", Type::decrement },
      token{ ":", Type::colon },
      token{ ",", Type::comma },
      token{ "=", Type::assign },
      token{ "+", Type::plus },
      token{ "-", Type::minus },
      token{ "*", Type::asterisk },
      token{ "/", Type::slash },
      token{ "^", Type::caret },
      token{ "~", Type::tilde },
      token{ "!", Type::bang },
      token{ "?", Type::question },
      token{ "(", Type::left_paren },
      token{ ")", Type::right_paren },
      token{ ";", Type::semicolon } };

    if (auto result = ctre::search<whitespace>(v); result) { return ret(Type::whitespace, result); }

    for (const auto &oper : tokens) {
      if (v.starts_with(oper.first)) { return ret(oper.second, oper.first); }
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
}// namespace parser_test

#endif
