#ifndef CCAD_CORE_DRC_ENGINE_HPP
#define CCAD_CORE_DRC_ENGINE_HPP

#include "drc_test_provider.hpp"
#include <vector>
#include <memory>

namespace ccad {

class Board;

// Orchestrator for the Design Rule Check process.
class DrcEngine {
public:
    DrcEngine();
    ~DrcEngine() = default;

    void registerProvider(std::unique_ptr<DrcTestProvider> provider);
    void runAll(const Board& board, std::vector<DrcItem>& violations);

private:
    std::vector<std::unique_ptr<DrcTestProvider>> providers_;
};

} // namespace ccad

#endif // CCAD_CORE_DRC_ENGINE_HPP
