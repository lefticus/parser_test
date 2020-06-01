#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>


#include <docopt/docopt.h>

#include <iostream>
#include <parser_test/parser.hpp>
#include <parser_test/algorithms.hpp>

static constexpr auto USAGE =
  R"(Parser Test.

    Usage:
          parser_test [options]

 Options:
          -h --help     Show this screen.
          --version     Show version.
)";

int main(int argc, const char **argv)
{
  const auto args = docopt::docopt(USAGE,
    { std::next(argv), std::next(argv, argc) },
    true,// show help if requested
    "Parser Test 0.0");// version string

  for (auto const &arg : args) {
    std::cout << arg.first << arg.second << std::endl;
  }


  //Use the default logger (stdout, multi-threaded, colored)
  spdlog::info("Hello, {}!", "World");

  fmt::print("Hello, from {}\n", "{fmt}");

  // constexpr std::string_view str{R"(ij int i = "bob";
  // var j = 5+2 * (5+i))"};

  constexpr std::string_view str{"3 * (2+\n-+-4)^4"};

  auto string_to_parse = str;

  parser_test::Parser parser;
  while (!string_to_parse.empty()) {
    auto parsed = parser.next_token(string_to_parse);
    if (parsed.type == parser_test::Parser::Type::unknown) {
      const auto [line, location] = parser_test::count_to_last(str.begin(), parsed.remainder.begin(), '\n');
      // skip last matched newline, and find next newline after
      const auto errored_line = std::string_view{std::next(location), std::find(std::next(location), parsed.remainder.end(), '\n')};
      // count column from location to beginning of unmatched string
      const auto column = std::distance(std::next(location), parsed.remainder.begin());

      std::cout << fmt::format("Error parsing string ({},{})\n\n", line+1, column+1);
      std::cout << errored_line;
      std::cout << fmt::format("\n{:>{}}\n\n", '^', column+1);
      return EXIT_FAILURE;
    }
    std::cout << '\'' << parsed.match << "' '" << parsed.remainder << "'\n";
    string_to_parse = parsed.remainder;
  }

  fmt::print(" {} = {} ", str, parser.parse(str));
}

