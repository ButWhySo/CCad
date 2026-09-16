#ifndef CCAD_CORE_MULTI_CHANNEL_PROBE_HPP
#define CCAD_CORE_MULTI_CHANNEL_PROBE_HPP

#include <string>
#include <vector>

namespace ccad {

struct Board;
class HierarchicalSheetParser;

// Bridges cross-probing selection between schematic hierarchy and replicated board rooms
class MultiChannelProbe {
public:
    MultiChannelProbe(Board* board, HierarchicalSheetParser* parser);
    ~MultiChannelProbe() = default;

    // Receives a cross-probe selection event from the schematic (e.g. user clicked "R1" on sheet UUID)
    // Returns the exact physical footprint IDs on the board corresponding to that selection
    std::vector<std::string> resolveProbedComponents(const std::string& sheetPathUuid, const std::string& componentRef) const;

    // Receives a cross-probe selection event from the board (e.g. user clicked a footprint)
    // Returns the corresponding schematic sheet path UUID and reference
    bool resolveProbedFootprint(const std::string& footprintId, std::string& outSheetPathUuid, std::string& outComponentRef) const;

private:
    Board* board_ = nullptr;
    HierarchicalSheetParser* parser_ = nullptr;
};

} // namespace ccad

#endif // CCAD_CORE_MULTI_CHANNEL_PROBE_HPP
