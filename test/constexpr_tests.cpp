#include <catch2/catch.hpp>
#include <parser_test/algorithms.hpp>

constexpr auto test_count_to_last(std::string_view input)
{
  return parser_test::count_to_last(input.begin(), input.end(), '\n');
}


TEST_CASE("count_to_last works in constexpr context")
{
  static constexpr std::string_view input{"Hello\nWorld"};
  
  STATIC_REQUIRE(test_count_to_last(input).first == 1);
  STATIC_REQUIRE(*test_count_to_last(input).second == '\n');
}
