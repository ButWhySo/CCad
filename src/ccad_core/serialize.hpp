#pragma once

#include "ccad_core/model.hpp"

#include <string>

namespace ccad {

std::string dumpProjectJson(const Project& project);
Project loadProjectJson(const std::string& json);

std::string formatBarcodeType(BarcodeType type);
std::string formatBarcodeEcc(BarcodeEcc ecc);
BarcodeType parseBarcodeType(const std::string& str);
BarcodeEcc parseBarcodeEcc(const std::string& str);

}  // namespace ccad

