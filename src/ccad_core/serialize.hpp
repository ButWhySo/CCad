#pragma once

#include "ccad_core/model.hpp"

#include <string>

namespace ccad {

std::string dumpProjectJson(const Project& project);
Project loadProjectJson(const std::string& json);

}  // namespace ccad

