#pragma once

#include "ccad_core/footprint.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ccad_cli {

bool hasError(const std::vector<ccad::Diagnostic>& diagnostics);
std::string diagnosticsJson(const std::vector<ccad::Diagnostic>& diagnostics);
std::string reviewJson(const ccad::ProjectReview& review);

std::optional<double> parsePositiveDouble(const std::string& value);
std::map<std::string, std::string> parseOptions(const std::vector<std::string>& args,
                                                std::size_t start,
                                                const std::vector<std::string>& allowed);
std::string requireOption(const std::map<std::string, std::string>& options,
                          const std::string& key);
ccad::Length requirePositiveMillimeters(const std::map<std::string, std::string>& options,
                                        const std::string& key);
double requireDoubleOption(const std::map<std::string, std::string>& options,
                           const std::string& key);
double optionDoubleOrDefault(const std::map<std::string, std::string>& options,
                             const std::string& key, double default_value);

void setAuditCommand(const std::string& command);

ccad::Project loadProjectFile(const std::string& path);
bool writeProjectFile(const std::string& path, const ccad::Project& project);
ccad::Footprint loadFootprintFile(const std::string& path);

ccad::Board& requireBoard(ccad::Project& project);
void requireLayer(const ccad::Board& board, const std::string& layer_id);
void requireCopperLayer(const ccad::Board& board, const std::string& layer_id);
void requireUniqueLayerId(const ccad::Board& board, const std::string& id);
void requireInsideBoard(const ccad::Board& board, ccad::Point point, const std::string& label);
void requirePointWithMarginInsideBoard(const ccad::Board& board, ccad::Point point,
                                       ccad::Length margin, const std::string& label);
void requireCenteredRectInsideBoard(const ccad::Board& board, ccad::Point center,
                                    ccad::Size size, const std::string& label);
void requireRotatedRectInsideBoard(const ccad::Board& board, ccad::Point center,
                                   ccad::Size size, double rotation_degrees,
                                   const std::string& label);
void requireUniquePadId(const ccad::Board& board, const std::string& id);
void requireUniqueViaId(const ccad::Board& board, const std::string& id);
void requireUniqueTrackId(const ccad::Board& board, const std::string& id);
void requireUniqueKeepoutId(const ccad::Board& board, const std::string& id);
void requireUniquePlacementRegionId(const ccad::Board& board, const std::string& id);
void requireUniquePhysicalObjectId(const ccad::Board& board, const std::string& id);
void requireRectInsideBoard(const ccad::Board& board, const ccad::Rect& rect,
                            const std::string& label);
ccad::Point rotateAndTranslate(const ccad::Point& local, const ccad::Point& origin,
                               double rotation_degrees);

}  // namespace ccad_cli
