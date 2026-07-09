#include "drc_engine.hpp"

namespace ccad {

DrcEngine::DrcEngine() = default;

void DrcEngine::registerProvider(std::unique_ptr<DrcTestProvider> provider) {
    if (provider) {
        providers_.push_back(std::move(provider));
    }
}

void DrcEngine::runAll(const Board& board, std::vector<DrcItem>& violations) {
    for (const auto& provider : providers_) {
        provider->run(board, violations);
    }
}

} // namespace ccad
