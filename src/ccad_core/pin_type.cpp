#include "pin_type.hpp"
#include <stdexcept>

namespace ccad {

// -- ElectricalPinType --

ElectricalPinType parse_electrical_pin_type(const std::string& name) {
    if (name == "input") return ElectricalPinType::Input;
    if (name == "output") return ElectricalPinType::Output;
    if (name == "bidirectional") return ElectricalPinType::Bidirectional;
    if (name == "tri_state") return ElectricalPinType::TriState;
    if (name == "passive") return ElectricalPinType::Passive;
    if (name == "free") return ElectricalPinType::Free;
    if (name == "unspecified") return ElectricalPinType::Unspecified;
    if (name == "power_in") return ElectricalPinType::PowerIn;
    if (name == "power_out") return ElectricalPinType::PowerOut;
    if (name == "open_collector") return ElectricalPinType::OpenCollector;
    if (name == "open_emitter") return ElectricalPinType::OpenEmitter;
    if (name == "no_connect") return ElectricalPinType::Unconnected;
    return ElectricalPinType::Unspecified; // default or fallback
}

std::string to_string(ElectricalPinType type) {
    switch (type) {
        case ElectricalPinType::Input: return "input";
        case ElectricalPinType::Output: return "output";
        case ElectricalPinType::Bidirectional: return "bidirectional";
        case ElectricalPinType::TriState: return "tri_state";
        case ElectricalPinType::Passive: return "passive";
        case ElectricalPinType::Free: return "free";
        case ElectricalPinType::Unspecified: return "unspecified";
        case ElectricalPinType::PowerIn: return "power_in";
        case ElectricalPinType::PowerOut: return "power_out";
        case ElectricalPinType::OpenCollector: return "open_collector";
        case ElectricalPinType::OpenEmitter: return "open_emitter";
        case ElectricalPinType::Unconnected: return "no_connect";
    }
    return "unspecified";
}

std::string get_electrical_pin_type_text(ElectricalPinType type) {
    switch (type) {
        case ElectricalPinType::Input: return "Input";
        case ElectricalPinType::Output: return "Output";
        case ElectricalPinType::Bidirectional: return "Bidirectional";
        case ElectricalPinType::TriState: return "Tri-state";
        case ElectricalPinType::Passive: return "Passive";
        case ElectricalPinType::Free: return "Free";
        case ElectricalPinType::Unspecified: return "Unspecified";
        case ElectricalPinType::PowerIn: return "Power input";
        case ElectricalPinType::PowerOut: return "Power output";
        case ElectricalPinType::OpenCollector: return "Open collector";
        case ElectricalPinType::OpenEmitter: return "Open emitter";
        case ElectricalPinType::Unconnected: return "Unconnected";
    }
    return "Unspecified";
}

// -- GraphicPinShape --

GraphicPinShape parse_graphic_pin_shape(const std::string& name) {
    if (name == "line") return GraphicPinShape::Line;
    if (name == "inverted") return GraphicPinShape::Inverted;
    if (name == "clock") return GraphicPinShape::Clock;
    if (name == "inverted_clock") return GraphicPinShape::InvertedClock;
    if (name == "input_low") return GraphicPinShape::InputLow;
    if (name == "clock_low") return GraphicPinShape::ClockLow;
    if (name == "output_low") return GraphicPinShape::OutputLow;
    if (name == "edge_clock_high") return GraphicPinShape::FallingEdgeClock; // KiCad historically uses edge_clock_high for falling edge clock in some files, but typically it is "falling_edge_clock".
    if (name == "falling_edge_clock") return GraphicPinShape::FallingEdgeClock;
    if (name == "nonlogic") return GraphicPinShape::NonLogic;
    return GraphicPinShape::Line; // Default
}

std::string to_string(GraphicPinShape shape) {
    switch (shape) {
        case GraphicPinShape::Line: return "line";
        case GraphicPinShape::Inverted: return "inverted";
        case GraphicPinShape::Clock: return "clock";
        case GraphicPinShape::InvertedClock: return "inverted_clock";
        case GraphicPinShape::InputLow: return "input_low";
        case GraphicPinShape::ClockLow: return "clock_low";
        case GraphicPinShape::OutputLow: return "output_low";
        case GraphicPinShape::FallingEdgeClock: return "falling_edge_clock";
        case GraphicPinShape::NonLogic: return "nonlogic";
    }
    return "line";
}

std::string get_graphic_pin_shape_text(GraphicPinShape shape) {
    switch (shape) {
        case GraphicPinShape::Line: return "Line";
        case GraphicPinShape::Inverted: return "Inverted";
        case GraphicPinShape::Clock: return "Clock";
        case GraphicPinShape::InvertedClock: return "Inverted clock";
        case GraphicPinShape::InputLow: return "Input low";
        case GraphicPinShape::ClockLow: return "Clock low";
        case GraphicPinShape::OutputLow: return "Output low";
        case GraphicPinShape::FallingEdgeClock: return "Falling edge clock";
        case GraphicPinShape::NonLogic: return "NonLogic";
    }
    return "Line";
}

// -- PinOrientation --

PinOrientation parse_pin_orientation(const std::string& name) {
    if (name == "right" || name == "R") return PinOrientation::Right;
    if (name == "left" || name == "L") return PinOrientation::Left;
    if (name == "up" || name == "U") return PinOrientation::Up;
    if (name == "down" || name == "D") return PinOrientation::Down;
    return PinOrientation::Right;
}

std::string to_string(PinOrientation orientation) {
    switch (orientation) {
        case PinOrientation::Right: return "right";
        case PinOrientation::Left: return "left";
        case PinOrientation::Up: return "up";
        case PinOrientation::Down: return "down";
    }
    return "right";
}

std::string get_pin_orientation_text(PinOrientation orientation) {
    switch (orientation) {
        case PinOrientation::Right: return "Right";
        case PinOrientation::Left: return "Left";
        case PinOrientation::Up: return "Up";
        case PinOrientation::Down: return "Down";
    }
    return "Right";
}

} // namespace ccad
