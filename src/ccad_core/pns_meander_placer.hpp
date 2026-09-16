#ifndef CCAD_CORE_PNS_MEANDER_PLACER_HPP
#define CCAD_CORE_PNS_MEANDER_PLACER_HPP

#include "pns_algo_base.hpp"
#include <utility>
#include <vector>

namespace ccad {

// Algorithm for routing length-matched tracks (meandering).
class PnsMeanderPlacer : public PnsAlgoBase {
public:
    PnsMeanderPlacer() = default;
    ~PnsMeanderPlacer() override = default;

    bool start(std::shared_ptr<PnsItem> item, int x, int y);
    bool meander(int x, int y);
    const std::vector<std::pair<int, int>>& path() const { return path_; }
    long double length() const;
    void setTargetLength(int length) { target_length_ = length < 0 ? 0 : length; }
    int targetLength() const { return target_length_; }
    void finish();

private:
    std::shared_ptr<PnsItem> start_item_;
    int target_length_ = 0;
    std::vector<std::pair<int, int>> path_;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_MEANDER_PLACER_HPP
