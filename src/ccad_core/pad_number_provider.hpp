#pragma once

#include <set>
#include <string>
#include <vector>

namespace ccad {

std::vector<std::string> provideArrayPadNumbers(const std::set<std::string>& existing_pad_numbers,
                                                const std::vector<std::string>& candidate_numbers,
                                                bool numbering_start_is_specified,
                                                std::size_t count);

}  // namespace ccad
