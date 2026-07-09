#ifndef CCAD_CORE_GFX_IMPORT_UTILS_HPP
#define CCAD_CORE_GFX_IMPORT_UTILS_HPP

#include "model.hpp"
#include <string>
#include <vector>

namespace ccad {

// Represents a simple raw image format for schematic graphical imports (e.g., bitmaps).
struct ImageImportData {
    int width = 0;
    int height = 0;
    bool has_alpha = false;
    std::vector<uint8_t> pixels; // RGBA
};

// Utilities to convert images and vector graphics (SVG/DXF) into ccad core schematic primitives.
// These are intended to replace KiCad's wxWidgets/SVG plugin dependencies with headless C++ alternatives.

// Converts an image to polygon sets. (Stub)
void convertImageToPolygons(const ImageImportData& img, Point pixel_scale, std::vector<SchGraphic>& out_graphics);

// Converts SVG data to schematic library shapes. (Stub)
void convertSVGToLibShapes(const std::string& svg_data, Point pixel_scale, Point offset, std::vector<SchGraphic>& out_graphics);

} // namespace ccad

#endif // CCAD_CORE_GFX_IMPORT_UTILS_HPP
