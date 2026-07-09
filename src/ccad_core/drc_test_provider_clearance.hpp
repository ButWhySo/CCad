#ifndef CCAD_CORE_DRC_TEST_PROVIDER_CLEARANCE_HPP
#define CCAD_CORE_DRC_TEST_PROVIDER_CLEARANCE_HPP

#include "drc_test_provider.hpp"

namespace ccad {

// Test provider for checking spatial clearances between routing and pad elements.
class DrcTestProviderClearance : public DrcTestProvider {
public:
    DrcTestProviderClearance() = default;
    ~DrcTestProviderClearance() override = default;

    void run(const Board& board, std::vector<DrcItem>& violations) override;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_TEST_PROVIDER_CLEARANCE_HPP
