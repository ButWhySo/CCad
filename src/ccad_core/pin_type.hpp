#pragma once
// Sprint 250: KiCad pin type structures
#include <string>
#include <vector>

namespace ccad {

/**
 * The symbol library pin object electrical types used in ERC tests.
 */
enum class ElectricalPinType {
    Input,         ///< usual pin input: must be connected
    Output,        ///< usual output
    Bidirectional, ///< input or output (like port for a microprocessor)
    TriState,      ///< tri state bus pin
    Passive,       ///< pin for passive symbols: must be connected, and can be connected to any pin.
    Free,          ///< not internally connected (may be connected to anything)
    Unspecified,   ///< unknown electrical properties: creates always a warning when connected
    PowerIn,       ///< power input (GND, VCC for ICs). Must be connected to a power output.
    PowerOut,      ///< output of a regulator: intended to be connected to power input pins
    OpenCollector, ///< pin type open collector
    OpenEmitter,   ///< pin type open emitter
    Unconnected    ///< not connected (must be left open)
};

/**
 * Parses canonical string identifier to ElectricalPinType.
 */
ElectricalPinType parse_electrical_pin_type(const std::string& name);

/**
 * Returns canonical string identifier for serialization.
 */
std::string to_string(ElectricalPinType type);

/**
 * Returns user-facing translated string (English in CCad core).
 */
std::string get_electrical_pin_type_text(ElectricalPinType type);


/**
 * Graphic shapes for symbol pins.
 */
enum class GraphicPinShape {
    Line,
    Inverted,
    Clock,
    InvertedClock,
    InputLow,
    ClockLow,
    OutputLow,
    FallingEdgeClock,
    NonLogic
};

GraphicPinShape parse_graphic_pin_shape(const std::string& name);
std::string to_string(GraphicPinShape shape);
std::string get_graphic_pin_shape_text(GraphicPinShape shape);


/**
 * The symbol library pin object orientations.
 */
enum class PinOrientation {
    Right,  ///< The pin extends rightwards from the connection point
    Left,   ///< The pin extends leftwards from the connection point
    Up,     ///< The pin extends upwards from the connection point
    Down    ///< The pin extends downwards from the connection point
};

PinOrientation parse_pin_orientation(const std::string& name);
std::string to_string(PinOrientation orientation);
std::string get_pin_orientation_text(PinOrientation orientation);

} // namespace ccad
