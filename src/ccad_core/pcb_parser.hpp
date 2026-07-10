#ifndef CCAD_CORE_PCB_PARSER_HPP
#define CCAD_CORE_PCB_PARSER_HPP

#include <string>
#include <memory>
#include "model.hpp"
#include "sexpr_parser.hpp"

namespace ccad {

// Deep parser for converting S-Expression AST into native CCad Board structures.
class PcbParser {
public:
    PcbParser() = default;

    // Load a .kicad_pcb file and return a populated Board.
    std::unique_ptr<Board> parse(const std::string& filepath);

private:
    void parseSetup(const SExpr* expr, Board* board);
    void parseNet(const SExpr* expr, Board* board);
    void parseFootprint(const SExpr* expr, Board* board);
    void parseSegment(const SExpr* expr, Board* board);
};

} // namespace ccad

#endif // CCAD_CORE_PCB_PARSER_HPP
