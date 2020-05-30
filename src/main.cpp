#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>


#include <docopt/docopt.h>

#include <iostream>
#include <parser_test/parser.hpp>

static constexpr auto USAGE =
  R"(Naval Fate.

    Usage:
          naval_fate ship new <name>...
          naval_fate ship <name> move <x> <y> [--speed=<kn>]
          naval_fate ship shoot <x> <y>
          naval_fate mine (set|remove) <x> <y> [--moored | --drifting]
          naval_fate (-h | --help)
          naval_fate --version
 Options:
          -h --help     Show this screen.
          --version     Show version.
          --speed=<kn>  Speed in knots [default: 10].
          --moored      Moored (anchored) mine.
          --drifting    Drifting mine.
)";

int main(int argc, const char **argv)
{
  std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
    { std::next(argv), std::next(argv, argc) },
    true,// show help if requested
    "Naval Fate 2.0");// version string

  for (auto const &arg : args) {
    std::cout << arg.first << arg.second << std::endl;
  }


  //Use the default logger (stdout, multi-threaded, colored)
  spdlog::info("Hello, {}!", "World");

  fmt::print("Hello, from {}\n", "{fmt}");

  // constexpr std::string_view str{R"(ij int i = "bob";
  // var j = 5+2 * (5+i))"};

  constexpr std::string_view str{"3 * (2+-4)^4"};

  auto string_to_parse = str;

  parser_test::Parser parser;
  while (!string_to_parse.empty()) {
    auto str = parser.lexer(string_to_parse);
    if (!str) {
      std::cout << "Unmatched: '" << string_to_parse << "'\n";
      return EXIT_FAILURE;
    }
    std::cout << '\'' << str->match << "' '" << str->remainder << "'\n";
    string_to_parse = str->remainder;
  }
}
