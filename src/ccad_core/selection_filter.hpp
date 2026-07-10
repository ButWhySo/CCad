#ifndef CCAD_CORE_SELECTION_FILTER_HPP
#define CCAD_CORE_SELECTION_FILTER_HPP

namespace ccad {

// Controls which board items can be selected during interactive operations
class SelectionFilter {
public:
    SelectionFilter() = default;
    ~SelectionFilter() = default;

    enum class ItemType {
        All,
        Tracks,
        Vias,
        Pads,
        Footprints,
        Text,
        Zones
    };

    void allowType(ItemType type, bool allow);
    bool isTypeAllowed(ItemType type) const;

    void resetToDefaults();

private:
    bool allowTracks_ = true;
    bool allowVias_ = true;
    bool allowPads_ = true;
    bool allowFootprints_ = true;
    bool allowText_ = true;
    bool allowZones_ = true;
};

} // namespace ccad

#endif // CCAD_CORE_SELECTION_FILTER_HPP
