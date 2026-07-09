#ifndef CCAD_CORE_DRC_TEST_PROVIDER_HPP
#define CCAD_CORE_DRC_TEST_PROVIDER_HPP

#include "drc_item.hpp"
#include <vector>

namespace ccad {

class Board;

// Base class for Design Rule Check test algorithms.
class DrcTestProvider {
public:
    DrcTestProvider() = default;
    virtual ~DrcTestProvider() = default;

    virtual void run(const Board& board, std::vector<DrcItem>& violations) = 0;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_TEST_PROVIDER_HPP
