#ifndef CCAD_CORE_DRC_TEST_PROVIDER_COURTYARD_HPP
#define CCAD_CORE_DRC_TEST_PROVIDER_COURTYARD_HPP

#include "drc_test_provider.hpp"

namespace ccad {

// Test provider for checking footprint courtyard overlaps.
class DrcTestProviderCourtyard : public DrcTestProvider {
public:
    DrcTestProviderCourtyard() = default;
    ~DrcTestProviderCourtyard() override = default;

    void run(const Board& board, std::vector<DrcItem>& violations) override;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_TEST_PROVIDER_COURTYARD_HPP
