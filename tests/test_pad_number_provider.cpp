#include "ccad_core/pad_number_provider.hpp"

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_array_pad_numbers_skip_existing_when_start_is_not_explicit() {
  const std::vector<std::string> numbers = ccad::provideArrayPadNumbers(
      {"1", "2", "5"}, {"1", "2", "3", "4", "5", "6"}, false, 3);

  require(numbers == std::vector<std::string>({"3", "4", "6"}),
          "array pad provider should skip existing pad numbers");
}

void test_array_pad_numbers_ignore_existing_when_start_is_explicit() {
  const std::vector<std::string> numbers =
      ccad::provideArrayPadNumbers({"1", "2"}, {"1", "2", "3"}, true, 2);

  require(numbers == std::vector<std::string>({"1", "2"}),
          "explicit numbering start should not reserve existing pad numbers");
}

void test_array_pad_numbers_report_exhausted_candidate_sequence() {
  bool threw = false;
  try {
    (void)ccad::provideArrayPadNumbers({"1", "2"}, {"1", "2"}, false, 1);
  } catch (const std::runtime_error& error) {
    threw = std::string(error.what()).find("candidate") != std::string::npos;
  }

  require(threw, "array pad provider should report an exhausted candidate sequence");
}

}  // namespace

int main() {
  try {
    test_array_pad_numbers_skip_existing_when_start_is_not_explicit();
    test_array_pad_numbers_ignore_existing_when_start_is_explicit();
    test_array_pad_numbers_report_exhausted_candidate_sequence();
    std::cout << "All tests passed!\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Test failed: " << error.what() << "\n";
    return 1;
  }
}
