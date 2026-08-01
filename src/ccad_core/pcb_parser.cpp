#include "pcb_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace ccad {

std::unique_ptr<Board> PcbParser::parse(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    auto root = parseSExpr(content);
    if (!root || !root->is_list || root->children.empty() || root->children[0]->value != "kicad_pcb") {
        std::cerr << "Invalid or empty kicad_pcb S-Expression" << std::endl;
        return nullptr;
    }

    auto board = std::make_unique<Board>();

    for (size_t i = 1; i < root->children.size(); ++i) {
        const auto& child = root->children[i];
        if (!child->is_list || child->children.empty()) continue;

        const std::string& token = child->children[0]->value;
        if (token == "setup") {
            parseSetup(child.get(), board.get());
        } else if (token == "net") {
            parseNet(child.get(), board.get());
        } else if (token == "footprint" || token == "module") {
            parseFootprint(child.get(), board.get());
        } else if (token == "segment") {
            parseSegment(child.get(), board.get());
        }
    }

    return board;
}

void PcbParser::parseSetup(const SExpr* expr, Board* board) {
    (void)expr;
    (void)board;
    // Stub
}

void PcbParser::parseNet(const SExpr* expr, Board* board) {
    (void)expr;
    (void)board;
    // Stub
}

void PcbParser::parseFootprint(const SExpr* expr, Board* board) {
    (void)expr;
    (void)board;
    // Stub
}

void PcbParser::parseSegment(const SExpr* expr, Board* board) {
    (void)expr;
    (void)board;
    // Stub
}

} // namespace ccad
