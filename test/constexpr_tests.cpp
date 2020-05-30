#include <catch2/catch.hpp>
#include <parser_test/algorithm.hpp>

constexpr auto test_count_to_last()
{
  std::string_view input{ "hello\nworld" };
  return count_to_last(input.begin(), input.end(), '\n');
}


TEST_CASE("count_to_last works in constexpr context")
{
  STATIC_REQUIRE(test_count_to_last().first == 1);
  STATIC_REQUIRE(test_count_to_last().second == std::next(input.begin(), 6));
}
