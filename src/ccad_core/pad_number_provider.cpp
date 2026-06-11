#include "ccad_core/pad_number_provider.hpp"

#include <stdexcept>

namespace ccad {

std::vector<std::string> provideArrayPadNumbers(const std::set<std::string>& existing_pad_numbers,
                                                const std::vector<std::string>& candidate_numbers,
                                                bool numbering_start_is_specified,
                                                std::size_t count) {
  std::vector<std::string> result;
  result.reserve(count);

  const std::set<std::string> reserved =
      numbering_start_is_specified ? std::set<std::string>{} : existing_pad_numbers;

  std::size_t candidate_index = 0;
  while (result.size() < count) {
    if (candidate_index >= candidate_numbers.size()) {
      throw std::runtime_error("array pad number candidate sequence exhausted");
    }

    const std::string& candidate = candidate_numbers.at(candidate_index);
    ++candidate_index;
    if (reserved.count(candidate) == 0) {
      result.push_back(candidate);
    }
  }

  return result;
}

}  // namespace ccad
