#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

inline void require(bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "test failure: " << message << '\n';
    std::exit(1);
  }
}

