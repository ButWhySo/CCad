#ifndef CCAD_CORE_DRC_TEST_PROVIDER_EDGE_CLEARANCE_HPP
#define CCAD_CORE_DRC_TEST_PROVIDER_EDGE_CLEARANCE_HPP

#include "drc_test_provider.hpp"

namespace ccad {

// Test provider for checking copper clearance to board edge cuts.
class DrcTestProviderEdgeClearance : public DrcTestProvider {
public:
    DrcTestProviderEdgeClearance() = default;
    ~DrcTestProviderEdgeClearance() override = default;

    void run(const Board& board, std::vector<DrcItem>& violations) override;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_TEST_PROVIDER_EDGE_CLEARANCE_HPP
