#include "ccad_core/custom_drc_rules.hpp"
#include "ccad_core/model.hpp"

#include <iostream>

int main() {
  ccad::Board board;
  ccad::CustomDrcRules rules(&board);
  if (!rules.parseRules("power|A.id == 'P1' && B.id == 'P2'|clearance(0.50mm)\n")) return 1;
  if (rules.getRules().size() != 1) return 2;
  if (rules.evaluateClearanceOverride("P1", "P2") != 0.50) return 3;
  if (rules.evaluateClearanceOverride("P1", "P3") >= 0.0) return 4;
  if (rules.parseRules("malformed")) return 5;
  if (!rules.getRules().empty()) return 6;
  std::cout << "custom drc rules: pass\n";
  return 0;
}
