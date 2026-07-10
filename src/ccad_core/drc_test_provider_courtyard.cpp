#include "drc_test_provider_courtyard.hpp"

namespace ccad {

void DrcTestProviderCourtyard::run(const Board& board, std::vector<DrcItem>& violations) {
    (void)board;
    (void)violations;

    // Basic structural iteration stub for courtyard overlap checking
    // Future implementations will traverse footprint bounding boxes
    
    // violations.emplace_back(DrcItem(3, "Courtyard overlap detected"));
}

} // namespace ccad
