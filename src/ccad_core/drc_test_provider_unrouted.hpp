#ifndef CCAD_CORE_DRC_TEST_PROVIDER_UNROUTED_HPP
#define CCAD_CORE_DRC_TEST_PROVIDER_UNROUTED_HPP

#include "drc_test_provider.hpp"

namespace ccad {

// Test provider for identifying unrouted net segments.
class DrcTestProviderUnrouted : public DrcTestProvider {
public:
    DrcTestProviderUnrouted() = default;
    ~DrcTestProviderUnrouted() override = default;

    void run(const Board& board, std::vector<DrcItem>& violations) override;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_TEST_PROVIDER_UNROUTED_HPP
