#include "review_window.hpp"

#include "board_canvas_renderer.hpp"
#include "schematic_canvas_renderer.hpp"
#include "board_canvas_view.hpp"
#include "ccad_core/canvas.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/component_generator.hpp"
#include "ccad_core/component_generator.hpp"
#include "ccad_core/drc.hpp"
#include "ccad_core/agent_orchestrator.hpp"
#include "ccad_gui/component_wizard_dialog.hpp"
#include "ccad_gui/footprint_placement_dialog.hpp"
#include "ccad_gui/library_browser_dialog.hpp"
#include "ccad_core/kicad_symbol_import.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/placement.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/library_catalog.hpp"
#include "ccad_gui/agent_panel.hpp"
#include "symbol_placement_dialog.hpp"
#include "footprint_placement_dialog.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFont>
#include <QGuiApplication>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QDockWidget>
#include <QIcon>
#include <QCursor>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainter>
#include <QInputDialog>
#include <QPen>
#include <QPainterPath>
#include <QPixmap>
#include <QKeySequence>
#include <QScrollArea>
#include <QScreen>
#include <QSize>
#include <QSizePolicy>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <QTextStream>
#include <QGraphicsSceneMouseEvent>
#include <QToolButton>
#include <QPushButton>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

QString formatCursorStatus(const std::optional<ccad::Board>& board, const QPointF& scene_position) {
  return formatCursorStatus(board, scene_position, false, false);
}

QString formatCursorStatus(const std::optional<ccad::Board>& board, const QPointF& scene_position,
                           const bool use_inches, const bool polar_coordinates) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  if (!board.has_value()) {
    return "X --  Y --";
  }

  const ccad::Rect outline = board->outline;
  const double origin_x_mm = outline.origin.x.nanometers / 1000000.0;
  const double origin_y_mm = outline.origin.y.nanometers / 1000000.0;
  const double board_width_mm = outline.size.width.nanometers / 1000000.0;
  const double board_height_mm = outline.size.height.nanometers / 1000000.0;
  const double x_mm = origin_x_mm + ((scene_position.x() - margin) / scale);
  const double y_mm = origin_y_mm + ((scene_position.y() - margin) / scale);
  const bool inside_board = x_mm >= origin_x_mm && y_mm >= origin_y_mm &&
                            x_mm <= origin_x_mm + board_width_mm &&
                            y_mm <= origin_y_mm + board_height_mm;

  const QString prefix = inside_board ? "Board " : "Canvas ";
  if (use_inches) {
    constexpr double mm_per_inch = 25.4;
    return prefix + QString("X ") + QString::number(x_mm / mm_per_inch, 'f', 3) +
           " in  Y " + QString::number(y_mm / mm_per_inch, 'f', 3) + " in";
  }

  QString status = prefix + QString("X ") + QString::number(x_mm, 'f', 2) + " mm  Y " +
                   QString::number(y_mm, 'f', 2) + " mm";
  if (polar_coordinates) {
    const double dx = x_mm - origin_x_mm;
    const double dy = y_mm - origin_y_mm;
    const double radius = std::hypot(dx, dy);
    const double angle = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
    status += "  R " + QString::number(radius, 'f', 2) + " mm  A " +
              QString::number(angle, 'f', 2) + " deg";
  }
  return status;
}

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open file: " + path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("failed to open file for writing: " + path.string());
  }
  output.write(content.data(), content.size());
}

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QString jsonString(const QString& value) {
  QString output = "\"";
  for (const QChar ch : value) {
    if (ch == '\\') {
      output += "\\\\";
    } else if (ch == '"') {
      output += "\\\"";
    } else if (ch == '\n') {
      output += "\\n";
    } else if (ch == '\r') {
      output += "\\r";
    } else if (ch == '\t') {
      output += "\\t";
    } else {
      output += ch;
    }
  }
  output += "\"";
  return output;
}

QString normalizedIdPart(QString value) {
  value = value.trimmed().toLower();
  QString output;
  output.reserve(value.size());
  for (const QChar ch : value) {
    if (ch.isLetterOrNumber()) {
      output += ch;
    } else if (!output.endsWith('_')) {
      output += '_';
    }
  }
  while (output.endsWith('_')) {
    output.chop(1);
  }
  return output.isEmpty() ? "unnamed" : output;
}

QString actionMapId(const QAction& action) {
  if (!action.objectName().isEmpty()) {
    return action.objectName();
  }
  return "action:" + normalizedIdPart(action.text());
}

QString rectJson(const QRect& rect) {
  return QString("{\"x\":%1,\"y\":%2,\"width\":%3,\"height\":%4}")
      .arg(rect.x())
      .arg(rect.y())
      .arg(rect.width())
      .arg(rect.height());
}

QRect clippedWidgetGlobalRect(const QWidget* widget) {
  const QRect full_rect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
  QRect clipped_rect = full_rect;
  for (const QWidget* parent = widget->parentWidget(); parent != nullptr;
       parent = parent->parentWidget()) {
    const auto* scroll_area = qobject_cast<const QScrollArea*>(parent);
    if (scroll_area == nullptr || scroll_area->viewport() == nullptr) {
      continue;
    }
    const QRect viewport_rect(scroll_area->viewport()->mapToGlobal(QPoint(0, 0)),
                              scroll_area->viewport()->size());
    clipped_rect = clipped_rect.intersected(viewport_rect);
    if (clipped_rect.isEmpty()) {
      return clipped_rect;
    }
  }
  return clipped_rect;
}

QRect visibleWidgetGlobalRect(const QWidget* widget) {
  const QRect clipped_rect = clippedWidgetGlobalRect(widget);
  if (!clipped_rect.isEmpty()) {
    return clipped_rect;
  }
  return QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
}

void ensureWidgetVisibleInAncestorScrollAreas(const QWidget* widget) {
  if (widget == nullptr) {
    return;
  }
  auto* mutable_widget = const_cast<QWidget*>(widget);
  bool scrolled = false;
  for (QWidget* parent = mutable_widget->parentWidget(); parent != nullptr;
       parent = parent->parentWidget()) {
    auto* scroll_area = qobject_cast<QScrollArea*>(parent);
    if (scroll_area == nullptr) {
      continue;
    }
    scroll_area->ensureWidgetVisible(mutable_widget, 8, 8);
    scrolled = true;
  }
  if (scrolled) {
    QApplication::processEvents();
  }
}

QString rectFJson(const QRectF& rect) {
  return QString("{\"x\":%1,\"y\":%2,\"width\":%3,\"height\":%4}")
      .arg(rect.x(), 0, 'f', 3)
      .arg(rect.y(), 0, 'f', 3)
      .arg(rect.width(), 0, 'f', 3)
      .arg(rect.height(), 0, 'f', 3);
}

QString boolJson(const bool value) {
  return value ? "true" : "false";
}

QString oneLineJson(QString json) {
  json.remove('\n');
  json.remove('\r');
  return json;
}

std::optional<QJsonObject> parseJsonObject(const QString& json) {
  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    return std::nullopt;
  }
  return document.object();
}

QString jsonObjectLine(const QJsonObject& object) {
  return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)) + "\n";
}

QString agentQueryResponse(const QString& method, const bool ok, const QString& reason,
                           const QString& result = "{}") {
  if (ok) {
    return QString("{\"schema_version\":1,\"ok\":true,\"method\":%1,\"result\":%2}\n")
        .arg(jsonString(method))
        .arg(oneLineJson(result));
  }
  return QString("{\"schema_version\":1,\"ok\":false,\"method\":%1,\"reason\":%2}\n")
      .arg(jsonString(method))
      .arg(jsonString(reason));
}

int normalizedUiMapLimit(const int limit, const int fallback = 100) {
  return std::clamp(limit <= 0 ? fallback : limit, 1, 500);
}

void copyStringFieldIfPresent(QJsonObject& destination, const QJsonObject& source,
                              const QString& key) {
  const QString value = source.value(key).toString();
  if (!value.isEmpty()) {
    destination.insert(key, value);
  }
}

QString interactionModeName(const InteractionMode mode) {
  switch (mode) {
    case InteractionMode::Default:
      return "default";
    case InteractionMode::PlaceFootprint:
      return "place_footprint";
    case InteractionMode::PlaceSymbol:
      return "place_symbol";
    case InteractionMode::MoveFootprint:
      return "move_footprint";
    case InteractionMode::AddVia:
      return "add_via";
    case InteractionMode::RouteTrack:
      return "route_track";
    case InteractionMode::AddZone:
      return "add_zone";
    case InteractionMode::AddKeepout:
      return "add_keepout";
    case InteractionMode::DrawGraphic:
      return "draw_graphic";
    case InteractionMode::PlaceText:
      return "place_text";
    case InteractionMode::AddWire:
      return "add_wire";
    case InteractionMode::AddLabel:
      return "add_label";
  }
  return "unknown";
}

QJsonObject compactUiMapNode(const QJsonObject& node) {
  const QString id = node.value("id").toString();
  const QString role = node.value("role").toString();
  QString label = node.value("label").toString();
  const QString object_id = node.value("object_id").toString();
  const QString type = node.value("type").toString();
  if (label.isEmpty() && !object_id.isEmpty()) {
    label = type.isEmpty() ? object_id : type + ":" + object_id;
  }

  QJsonObject compact;
  compact.insert("id", id);
  compact.insert("role", role);
  compact.insert("label", label);
  compact.insert("visible", node.value("visible").toBool());
  compact.insert("enabled", node.contains("enabled") ? node.value("enabled").toBool()
                                                     : node.value("interactive").toBool(false));
  if (node.contains("interactive")) {
    compact.insert("interactive", node.value("interactive").toBool());
  }
  if (node.contains("selected")) {
    compact.insert("selected", node.value("selected").toBool());
  }
  if (node.contains("checked")) {
    compact.insert("checked", node.value("checked").toBool());
  }
  compact.insert("target_x", node.value("target_x").toInt());
  compact.insert("target_y", node.value("target_y").toInt());
  copyStringFieldIfPresent(compact, node, "canvas");
  copyStringFieldIfPresent(compact, node, "type");
  copyStringFieldIfPresent(compact, node, "object_id");
  copyStringFieldIfPresent(compact, node, "net_id");
  copyStringFieldIfPresent(compact, node, "layer_id");
  copyStringFieldIfPresent(compact, node, "route_request_id");
  copyStringFieldIfPresent(compact, node, "dock_area");
  return compact;
}

struct CompactUiMapNodes {
  QJsonArray nodes;
  int total_node_count = 0;
  int match_count = 0;
  bool truncated = false;
};

CompactUiMapNodes compactUiMapNodesFromMapObject(const QJsonObject& map_object,
                                                 const QString& role_filter,
                                                 const int limit) {
  CompactUiMapNodes result;
  const QJsonArray nodes = map_object.value("nodes").toArray();
  result.total_node_count = static_cast<int>(nodes.size());
  for (const QJsonValue& value : nodes) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject node = value.toObject();
    const QString role = node.value("role").toString();
    if (!role_filter.isEmpty() && role != role_filter) {
      continue;
    }
    ++result.match_count;
    if (result.nodes.size() >= limit) {
      result.truncated = true;
      continue;
    }
    result.nodes.append(compactUiMapNode(node));
  }
  return result;
}

void appendUnique(QStringList& values, const QString& value) {
  const QString trimmed = value.trimmed();
  if (trimmed.isEmpty() || values.contains(trimmed)) {
    return;
  }
  values.append(trimmed);
}

void appendUniqueList(QStringList& values, const QStringList& additions) {
  for (const QString& value : additions) {
    appendUnique(values, value);
  }
}

QJsonArray stringListToJsonArray(const QStringList& values) {
  QJsonArray array;
  for (const QString& value : values) {
    array.append(value);
  }
  return array;
}

QJsonObject schemaProperty(const QString& type, const QString& description) {
  QJsonObject property;
  property.insert("type", type);
  property.insert("description", description);
  return property;
}

QJsonObject schemaObject(const QJsonObject& properties, const QStringList& required) {
  QJsonObject schema;
  schema.insert("type", "object");
  schema.insert("properties", properties);
  schema.insert("required", stringListToJsonArray(required));
  return schema;
}

QJsonObject agentMethodEntry(const QString& method, const QString& category,
                             const QString& title, const QString& description,
                             const bool read_only, const bool mutates_ui,
                             const bool mutates_project, const bool requires_project,
                             const bool supports_dry_run, const QJsonObject& input_schema,
                             const QString& output_summary,
                             const QJsonObject& example_payload = QJsonObject{}) {
  QJsonObject entry;
  entry.insert("method", method);
  entry.insert("category", category);
  entry.insert("title", title);
  entry.insert("description", description);
  entry.insert("read_only", read_only);
  entry.insert("mutates_ui", mutates_ui);
  entry.insert("mutates_project", mutates_project);
  entry.insert("requires_project", requires_project);
  entry.insert("supports_dry_run", supports_dry_run);
  entry.insert("inputSchema", input_schema);
  entry.insert("output_summary", output_summary);
  QJsonArray examples;
  QJsonObject example;
  example.insert("method", method);
  for (auto it = example_payload.constBegin(); it != example_payload.constEnd(); ++it) {
    example.insert(it.key(), it.value());
  }
  examples.append(example);
  entry.insert("examples", examples);
  return entry;
}

QJsonObject idSchema(const QString& description = "Stable UI-map node id.") {
  QJsonObject properties;
  properties.insert("id", schemaProperty("string", description));
  return schemaObject(properties, {"id"});
}

QJsonObject emptySchema() {
  return schemaObject(QJsonObject{}, {});
}

QJsonObject dryRunProperty() {
  QJsonObject property = schemaProperty("boolean", "When true, describe the target or action without changing UI or project state.");
  property.insert("default", false);
  return property;
}

QJsonObject canvasProperty() {
  QJsonObject property = schemaProperty("string", "Canvas id. CCad currently supports canvas:pcb.");
  property.insert("default", "canvas:pcb");
  return property;
}

QJsonObject boardPointSchema(const bool with_dry_run) {
  QJsonObject properties;
  properties.insert("x_mm", schemaProperty("number", "Board X coordinate in millimeters."));
  properties.insert("y_mm", schemaProperty("number", "Board Y coordinate in millimeters."));
  properties.insert("canvas", canvasProperty());
  if (with_dry_run) {
    properties.insert("dry_run", dryRunProperty());
  }
  return schemaObject(properties, {"x_mm", "y_mm"});
}

QJsonObject boardDragSchema(const bool with_dry_run) {
  QJsonObject properties;
  properties.insert("start_x_mm", schemaProperty("number", "Start X coordinate in millimeters."));
  properties.insert("start_y_mm", schemaProperty("number", "Start Y coordinate in millimeters."));
  properties.insert("end_x_mm", schemaProperty("number", "End X coordinate in millimeters."));
  properties.insert("end_y_mm", schemaProperty("number", "End Y coordinate in millimeters."));
  properties.insert("canvas", canvasProperty());
  if (with_dry_run) {
    properties.insert("dry_run", dryRunProperty());
  }
  return schemaObject(properties, {"start_x_mm", "start_y_mm", "end_x_mm", "end_y_mm"});
}

QJsonObject roleLimitSchema() {
  QJsonObject properties;
  properties.insert("role", schemaProperty("string", "Optional UI-map role filter such as action, control, panel, tab, or canvas_object."));
  properties.insert("limit", schemaProperty("integer", "Maximum compact nodes to return."));
  return schemaObject(properties, {});
}

QJsonObject epochTimeoutSchema() {
  QJsonObject properties;
  properties.insert("since_epoch", schemaProperty("integer", "UI epoch already observed by the caller."));
  properties.insert("timeout_ms", schemaProperty("integer", "Maximum wait before returning the current state."));
  return schemaObject(properties, {});
}

QJsonObject agentMethodLookupSchema() {
  QJsonObject properties;
  properties.insert("method_name", schemaProperty("string", "Preferred target method name to inspect."));
  properties.insert("method", schemaProperty("string", "Target method name when calling directly from the agent panel."));
  properties.insert("name", schemaProperty("string", "Alias for method_name."));
  return schemaObject(properties, {});
}

QJsonArray agentMethodCatalogArray() {
  QJsonArray catalog;

  auto append = [&catalog](const QJsonObject& entry) { catalog.append(entry); };

  append(agentMethodEntry("agent.methods", "agent", "List Agent Methods",
                          "Return the stable CCad agent protocol catalog.",
                          true, false, false, false, false, emptySchema(),
                          "Catalog entries with method names, schemas, safety flags, and examples."));
  append(agentMethodEntry("agent.method_schema", "agent", "Inspect One Agent Method",
                          "Return one catalog entry by method name.",
                          true, false, false, false, false, agentMethodLookupSchema(),
                          "A found flag and the matching catalog entry.",
                          QJsonObject{{"method_name", "ui.watch_delta"}}));
  append(agentMethodEntry("agent.quickstart", "agent", "Agent Quickstart",
                          "Return the recommended UI-map loop for agents.",
                          true, false, false, false, false, emptySchema(),
                          "A compact operating guide for live UI-map automation."));
  append(agentMethodEntry("agent.harness_context", "agent", "Harness Context",
                          "Return the live native GUI agent state needed to resume or verify a run.",
                          true, false, false, false, false, emptySchema(),
                          "Session state, diagnostics, active view, and latest visual evidence slot."));
  append(agentMethodEntry("agent.workspace_state", "agent", "Workspace State",
                          "Return the right-side Agent dock goal, task state, evidence queue, and live controls.",
                          true, false, false, false, false, emptySchema(),
                          "Current local Agent workspace goal, evidence count, and panel state."));
  append(agentMethodEntry("agent.run_profile", "agent", "Run Profile",
                          "Return the durable run-loop, retry, circuit-breaker, and stop-condition policy.",
                          true, false, false, false, false, emptySchema(),
                          "Execution profile metadata for long-running CCad agent loops."));
  append(agentMethodEntry("agent.safety_policy", "agent", "Safety Policy",
                          "Return approval gates, secret-handling rules, and forbidden execution paths.",
                          true, false, false, false, false, emptySchema(),
                          "Safety policy for GUI, CLI, provider, and fabrication actions."));
  append(agentMethodEntry("agent.provider_policy", "agent", "Provider Policy",
                          "Return the accepted model-provider integration paths for BYOK and local models.",
                          true, false, false, false, false, emptySchema(),
                          "Provider policy with supported and disallowed model access paths."));
  append(agentMethodEntry("agent.observability_config", "agent", "Observability Config",
                          "Return the current OpenTelemetry/Langfuse-style tracing configuration shape.",
                          true, false, false, false, false, emptySchema(),
                          "Trace span plan, backend options, environment flags, and redaction policy."));
  append(agentMethodEntry("agent.evidence_manifest_schema", "agent", "Evidence Manifest Schema",
                          "Return the artifact manifest shape for screenshots, reports, and references.",
                          true, false, false, false, false, emptySchema(),
                          "Expected evidence manifest fields for sprint and agent verification."));
  append(agentMethodEntry("agent.tool_guide", "agent", "Tool Guide",
                          "Return LLM-facing usage, verification, and recovery guidance for one method.",
                          true, false, false, false, false, agentMethodLookupSchema(),
                          "A found flag plus per-tool operating guide.",
                          QJsonObject{{"method_name", "ui.route_track"}}));

  append(agentMethodEntry("ui.map", "ui_map", "Full UI Map",
                          "Return the full live UI region map with rectangles, roles, labels, and targets.",
                          true, false, false, false, false, emptySchema(),
                          "Full UI map JSON."));
  append(agentMethodEntry("ui.map_compact", "ui_map", "Compact UI Map",
                          "Return compact UI nodes, optionally filtered by role.",
                          true, false, false, false, false, roleLimitSchema(),
                          "Compact UI-map nodes.",
                          QJsonObject{{"role", "action"}, {"limit", 20}}));
  append(agentMethodEntry("ui.role_summary", "ui_map", "Role Summary",
                          "Return node counts by role.",
                          true, false, false, false, false, emptySchema(),
                          "Role counts and total node count."));
  append(agentMethodEntry("ui.index_stats", "ui_map", "Index Stats",
                          "Return the state of the UI-map ID and role indexes.",
                          true, false, false, false, false, emptySchema(),
                          "Index counts and cache state."));
  append(agentMethodEntry("ui.get_node", "ui_map", "Get Node By ID",
                          "Resolve one compact UI-map node through the ID index.",
                          true, false, false, false, false, idSchema(),
                          "A found flag and compact node.",
                          QJsonObject{{"id", "menu:file"}}));
  append(agentMethodEntry("ui.nodes_by_role", "ui_map", "Nodes By Role",
                          "Resolve compact UI-map nodes through the role index.",
                          true, false, false, false, false, roleLimitSchema(),
                          "Matching compact nodes.",
                          QJsonObject{{"role", "action"}, {"limit", 20}}));
  append(agentMethodEntry("ui.map_delta", "ui_map", "UI Map Delta",
                          "Return dirty UI-map nodes since a previously observed epoch.",
                          true, false, false, false, false, epochTimeoutSchema(),
                          "Changed flag and compact dirty nodes.",
                          QJsonObject{{"since_epoch", 0}}));
  QJsonObject watch_properties = epochTimeoutSchema();
  QJsonObject watch_schema = watch_properties;
  QJsonObject watch_props = watch_schema.value("properties").toObject();
  watch_props.insert("max_events", schemaProperty("integer", "Maximum retained dirty events to return."));
  watch_schema.insert("properties", watch_props);
  append(agentMethodEntry("ui.watch_delta", "ui_map", "Watch UI Deltas",
                          "Wait briefly and return bounded dirty UI-map event history.",
                          true, false, false, false, false, watch_schema,
                          "Dirty-event history with compact nodes.",
                          QJsonObject{{"since_epoch", 0}, {"timeout_ms", 20}, {"max_events", 4}}));
  append(agentMethodEntry("ui.wait_for_delta", "ui_map", "Wait For Delta",
                          "Wait until the UI epoch advances or timeout expires.",
                          true, false, false, false, false, epochTimeoutSchema(),
                          "Delta response plus wait timing.",
                          QJsonObject{{"since_epoch", 0}, {"timeout_ms", 20}}));
  append(agentMethodEntry("ui.find", "ui_map", "Find UI Nodes",
                          "Search compact UI nodes by id, label, object id, net, or layer.",
                          true, false, false, false, false, roleLimitSchema(),
                          "Compact search results.",
                          QJsonObject{{"query", "add"}, {"role", "action"}, {"limit", 8}}));
  QJsonObject hit_props;
  hit_props.insert("x", schemaProperty("integer", "Logical screen X coordinate."));
  hit_props.insert("y", schemaProperty("integer", "Logical screen Y coordinate."));
  append(agentMethodEntry("ui.hit_test", "ui_map", "Hit Test",
                          "Return the smallest targetable UI-map node under a logical coordinate.",
                          true, false, false, false, false, schemaObject(hit_props, {"x", "y"}),
                          "A found flag and matching node.",
                          QJsonObject{{"x", 120}, {"y", 80}}));
  append(agentMethodEntry("ui.target", "ui_map", "Target By ID",
                          "Return screen coordinates for a targetable UI-map node.",
                          true, false, false, false, false, idSchema(),
                          "Target coordinates and node role.",
                          QJsonObject{{"id", "menu:file"}}));
  append(agentMethodEntry("ui.target_board_point", "ui_map", "Target Board Point",
                          "Convert board millimeter coordinates to a screen target on the PCB canvas.",
                          true, false, false, true, false, boardPointSchema(false),
                          "A found flag and screen target.",
                          QJsonObject{{"x_mm", 8}, {"y_mm", 9}}));
  QJsonObject nearest_schema = boardPointSchema(false);
  QJsonObject nearest_props = nearest_schema.value("properties").toObject();
  nearest_props.insert("limit", schemaProperty("integer", "Maximum nearby canvas objects to return."));
  nearest_schema.insert("properties", nearest_props);
  append(agentMethodEntry("ui.nearest_canvas_object", "ui_map", "Nearest Canvas Object",
                          "Return indexed nearby PCB canvas objects for a board point.",
                          true, false, false, true, false, nearest_schema,
                          "Nearest object and candidate list.",
                          QJsonObject{{"canvas", "canvas:pcb"}, {"x_mm", 8}, {"y_mm", 9}}));

  append(agentMethodEntry("ui.screenshot", "evidence", "Capture Screenshot",
                          "Capture an app-owned GUI screenshot, or describe it in dry-run mode.",
                          true, false, false, false, true,
                          schemaObject(QJsonObject{{"path", schemaProperty("string", "Optional output path.")},
                                                   {"dry_run", dryRunProperty()}}, {}),
                          "Screenshot path, size, and dry-run flag.",
                          QJsonObject{{"dry_run", true}}));

  append(agentMethodEntry("ui.click", "input", "Click UI Node",
                          "Click an allowlisted UI-map target by id.",
                          false, true, false, false, true,
                          schemaObject(QJsonObject{{"id", schemaProperty("string", "Target UI-map node id.")},
                                                   {"dry_run", dryRunProperty()}}, {"id"}),
                          "Target, dry-run flag, and action result.",
                          QJsonObject{{"id", "action:zoom_in"}, {"dry_run", true}}));
  append(agentMethodEntry("ui.double_click", "input", "Double Click UI Node",
                          "Double-click an allowlisted UI-map target by id.",
                          false, true, false, false, true,
                          schemaObject(QJsonObject{{"id", schemaProperty("string", "Target UI-map node id.")},
                                                   {"dry_run", dryRunProperty()}}, {"id"}),
                          "Target, dry-run flag, and action result.",
                          QJsonObject{{"id", "menu:file"}, {"dry_run", true}}));
  append(agentMethodEntry("ui.canvas_click", "input", "Canvas Click",
                          "Click a board coordinate on the PCB canvas.",
                          false, true, true, true, true, boardPointSchema(true),
                          "Click target and optional placement result.",
                          QJsonObject{{"x_mm", 8}, {"y_mm", 9}, {"dry_run", true}}));
  append(agentMethodEntry("ui.canvas_drag", "input", "Canvas Drag",
                          "Drag between two board coordinates on the PCB canvas.",
                          false, true, true, true, true, boardDragSchema(true),
                          "Drag target and optional placement result.",
                          QJsonObject{{"start_x_mm", 8}, {"start_y_mm", 9},
                                      {"end_x_mm", 12}, {"end_y_mm", 12}, {"dry_run", true}}));
  append(agentMethodEntry("ui.type_text", "input", "Type Text",
                          "Set text in allowlisted agent-panel inputs.",
                          false, true, false, false, false,
                          schemaObject(QJsonObject{{"id", schemaProperty("string", "Target input id.")},
                                                   {"text", schemaProperty("string", "Text to set.")}},
                                       {"id", "text"}),
                          "Whether the text was set.",
                          QJsonObject{{"id", "control:agent_command_input"},
                                      {"text", "Inspect current DRC state"}}));
  append(agentMethodEntry("ui.key", "input", "Keyboard Key",
                          "Send a supported key event. Escape cancels active tools.",
                          false, true, false, false, false,
                          schemaObject(QJsonObject{{"key", schemaProperty("string", "Supported key name.")}},
                                       {"key"}),
                          "Whether the key was sent and resulting tool mode.",
                          QJsonObject{{"key", "Escape"}}));
  append(agentMethodEntry("ui.trigger_safe", "input", "Safe Trigger",
                          "Trigger a named allowlisted UI action.",
                          false, true, true, false, false, idSchema("Allowlisted action or tab id."),
                          "Whether the action ran and why.",
                          QJsonObject{{"id", "tab:agent"}}));

  append(agentMethodEntry("ui.current_tool", "workflow", "Current Tool",
                          "Return the active interaction mode.",
                          true, false, false, false, false, emptySchema(),
                          "Current interaction mode."));
  append(agentMethodEntry("ui.cancel_tool", "workflow", "Cancel Tool",
                          "Cancel the active interaction mode by sending Escape.",
                          false, true, false, false, false, emptySchema(),
                          "Cancellation result and resulting mode."));
  append(agentMethodEntry("ui.place_via", "workflow", "Place Via",
                          "Activate via placement and place a via at a board point.",
                          false, true, true, true, true, boardPointSchema(true),
                          "Via placement result and board counts.",
                          QJsonObject{{"x_mm", 14}, {"y_mm", 10}}));
  append(agentMethodEntry("ui.route_track", "workflow", "Route Track",
                          "Activate track routing and route one track between two board points.",
                          false, true, true, true, true, boardDragSchema(true),
                          "Track placement result and board counts.",
                          QJsonObject{{"start_x_mm", 8}, {"start_y_mm", 9},
                                      {"end_x_mm", 18}, {"end_y_mm", 12}}));
  append(agentMethodEntry("ui.add_zone", "workflow", "Add Zone",
                          "Draw a rectangular copper zone on the active PCB layer.",
                          false, true, true, true, true, boardDragSchema(true),
                          "Zone placement result and board counts."));
  append(agentMethodEntry("ui.add_keepout", "workflow", "Add Keepout",
                          "Draw a rectangular routing keepout.",
                          false, true, true, true, true, boardDragSchema(true),
                          "Keepout placement result and board counts."));
  append(agentMethodEntry("ui.draw_graphic", "workflow", "Draw Graphic",
                          "Draw a board graphic line.",
                          false, true, true, true, true, boardDragSchema(true),
                          "Graphic placement result and board counts."));
  QJsonObject text_point_schema = boardPointSchema(true);
  QJsonObject text_props = text_point_schema.value("properties").toObject();
  text_props.insert("text", schemaProperty("string", "Board text content."));
  text_point_schema.insert("properties", text_props);
  text_point_schema.insert("required", stringListToJsonArray({"x_mm", "y_mm", "text"}));
  append(agentMethodEntry("ui.place_text", "workflow", "Place Text",
                          "Place board text on the default visible silkscreen layer.",
                          false, true, true, true, true, text_point_schema,
                          "Board text placement result and board counts.",
                          QJsonObject{{"x_mm", 8}, {"y_mm", 22}, {"text", "FLOW TEXT"}}));
  append(agentMethodEntry("ui.delete_object", "workflow", "Delete Object",
                          "Select and delete a PCB canvas object by CAD object id.",
                          false, true, true, true, false, idSchema("CAD object id or canvas_object:id."),
                          "Deletion result and board counts.",
                          QJsonObject{{"id", "V1"}}));
  append(agentMethodEntry("ui.select_canvas_object", "workflow", "Select Canvas Object",
                          "Select a PCB canvas object by CAD object id.",
                          false, true, false, true, false, idSchema("CAD object id or canvas_object:id."),
                          "Selection result and selected count.",
                          QJsonObject{{"id", "U1.1"}}));
  append(agentMethodEntry("ui.get_selection", "workflow", "Get Selection",
                          "Return currently selected PCB canvas objects.",
                          true, false, false, false, false, emptySchema(),
                          "Selected object list."));
  append(agentMethodEntry("ui.wait_for_epoch", "workflow", "Wait For Epoch",
                          "Wait until the live UI-map epoch reaches a minimum value.",
                          true, false, false, false, false,
                          schemaObject(QJsonObject{{"minimum_epoch", schemaProperty("integer", "Required UI epoch.")},
                                                   {"timeout_ms", schemaProperty("integer", "Maximum wait before returning.")}},
                                       {}),
                          "Wait timing and reached flag.",
                          QJsonObject{{"minimum_epoch", 1}, {"timeout_ms", 20}}));

  append(agentMethodEntry("ui.active_layer", "pcb_state", "Active PCB Layer",
                          "Return the active PCB routing layer.",
                          true, false, false, true, false, emptySchema(),
                          "Active layer id and layer list."));
  append(agentMethodEntry("ui.set_active_layer", "pcb_state", "Set Active PCB Layer",
                          "Set the active PCB routing layer to a visible copper layer.",
                          false, true, false, true, false,
                          schemaObject(QJsonObject{{"layer_id", schemaProperty("string", "Copper layer id such as F.Cu or B.Cu.")}},
                                       {"layer_id"}),
                          "Active layer setter result.",
                          QJsonObject{{"layer_id", "B.Cu"}}));
  append(agentMethodEntry("ui.active_net", "pcb_state", "Active PCB Net",
                          "Return the active PCB net.",
                          true, false, false, true, false, emptySchema(),
                          "Active net id and net list."));
  append(agentMethodEntry("ui.set_active_net", "pcb_state", "Set Active PCB Net",
                          "Set the active PCB net for future routing and placement workflows.",
                          false, true, false, true, false,
                          schemaObject(QJsonObject{{"net_id", schemaProperty("string", "Existing project net id.")}},
                                       {"net_id"}),
                          "Active net setter result.",
                          QJsonObject{{"net_id", "N2"}}));
  append(agentMethodEntry("ui.epoch", "ui_map", "Current UI Epoch",
                          "Return the current UI-map epoch without wrapping it in result.",
                          true, false, false, false, false, emptySchema(),
                          "Current UI epoch."));

  append(agentMethodEntry("project.context", "project", "Project Context",
                          "Return a compact project and board summary.",
                          true, false, false, false, false, emptySchema(),
                          "Project id, board summary, and active state."));
  append(agentMethodEntry("project.object_counts", "project", "Object Counts",
                          "Return project and board object counts.",
                          true, false, false, false, false, emptySchema(),
                          "Board object counts."));
  append(agentMethodEntry("project.review", "project", "Project Review",
                          "Return a compact review payload for the current project.",
                          true, false, false, false, false, emptySchema(),
                          "Review summary."));
  append(agentMethodEntry("project.erc", "project", "Run ERC",
                          "Run schematic electrical checks for the current project.",
                          true, false, false, false, false, emptySchema(),
                          "ERC diagnostics and counts."));
  append(agentMethodEntry("project.drc", "project", "Run DRC",
                          "Run physical board DRC checks for the current project.",
                          true, false, false, true, false, emptySchema(),
                          "DRC diagnostics and counts."));
  append(agentMethodEntry("project.diagnostics", "project", "Project Diagnostics",
                          "Return ERC and DRC diagnostics together.",
                          true, false, false, false, false, emptySchema(),
                          "Combined diagnostic counts and items."));

  return catalog;
}

QJsonObject agentMethodsJsonObject() {
  const QJsonArray methods = agentMethodCatalogArray();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("catalog_kind", "ccad_agent_protocol");
  response.insert("method_count", methods.size());
  response.insert("methods", methods);
  response.insert("reference_model",
                  "KiCad-style named actions plus MCP-style tool schemas for LLM-native use.");
  return response;
}

std::optional<QJsonObject> agentMethodCatalogEntry(const QString& method_name) {
  const QString trimmed = method_name.trimmed();
  for (const QJsonValue& value : agentMethodCatalogArray()) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject entry = value.toObject();
    if (entry.value("method").toString() == trimmed) {
      return entry;
    }
  }
  return std::nullopt;
}

QString agentMethodsJson() {
  return jsonObjectLine(agentMethodsJsonObject());
}

QString agentMethodSchemaJson(const QString& method_name) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("method", method_name.trimmed());
  const std::optional<QJsonObject> entry = agentMethodCatalogEntry(method_name);
  response.insert("found", entry.has_value());
  if (entry.has_value()) {
    response.insert("entry", *entry);
  } else {
    response.insert("reason", method_name.trimmed().isEmpty() ? "missing_method" : "method_not_found");
  }
  return jsonObjectLine(response);
}

QString agentQuickstartJson() {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("workflow", "ui_map_agent_loop");
  response.insert("summary",
                  "Discover methods once, use indexed UI-map calls for most targeting, watch deltas while the GUI stays open, and reserve screenshots for visual proof.");
  QJsonArray steps;
  steps.append(QJsonObject{{"step", "discover"},
                           {"method", "agent.methods"},
                           {"purpose", "Load method names, input schemas, safety flags, and examples."}});
  steps.append(QJsonObject{{"step", "context"},
                           {"method", "project.context"},
                           {"purpose", "Learn the project id, board presence, active layer, and active net."}});
  steps.append(QJsonObject{{"step", "index"},
                           {"method", "ui.index_stats"},
                           {"purpose", "Confirm the live UI-map ID and role indexes are fresh before targeting."}});
  steps.append(QJsonObject{{"step", "target"},
                           {"method", "ui.get_node"},
                           {"fallback", "ui.nodes_by_role"},
                           {"purpose", "Resolve exact semantic ids first, then role-filtered candidates."}});
  steps.append(QJsonObject{{"step", "act"},
                           {"method", "ui.click"},
                           {"fallback", "ui.canvas_click"},
                           {"purpose", "Use dry_run before mutating clicks or canvas gestures when a target is uncertain."}});
  steps.append(QJsonObject{{"step", "observe"},
                           {"method", "ui.watch_delta"},
                           {"purpose", "Read dirty UI-map events instead of polling full screenshots after every action."}});
  steps.append(QJsonObject{{"step", "prove"},
                           {"method", "ui.screenshot"},
                           {"purpose", "Capture screenshots through the visual harness for final visual validation."}});
  response.insert("steps", steps);
  response.insert("first_methods",
                  QJsonArray{"agent.methods", "project.context", "ui.index_stats", "ui.watch_delta"});
  response.insert("screenshot_rule",
                  "Use screenshots for visual validation, after the project harness beep and current settle waits.");
  response.insert("unsafe_rule",
                  "Treat mutates_project and mutates_ui methods as state changing unless dry_run is true.");
  return jsonObjectLine(response);
}

QJsonObject envPresenceObject(const QString& name) {
  const QByteArray bytes = name.toLocal8Bit();
  const char* value = std::getenv(bytes.constData());
  return QJsonObject{{"name", name}, {"configured", value != nullptr && value[0] != '\0'}};
}

QString agentRunProfileJson() {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("profile_kind", "ccad_agent_run_profile");
  response.insert("durability_reference", "langgraph");
  response.insert("loop",
                  QJsonArray{"plan", "act", "observe", "verify", "repair_or_stop"});
  response.insert("durable_state_keys",
                  QJsonArray{"project_path", "transaction_id", "ui_epoch", "active_view",
                             "selected_object_ids", "pending_diagnostics", "tool_budget",
                             "provider_config", "last_verified_visual_artifact"});
  response.insert("retry_policy",
                  QJsonObject{{"default_max_attempts", 3},
                              {"backoff", "exponential_with_jitter"},
                              {"retry_on", QJsonArray{"stale_ui_epoch", "transient_io",
                                                       "preview_not_loaded", "drc_fixable"}},
                              {"do_not_retry_on", QJsonArray{"approval_required",
                                                             "unsafe_action",
                                                             "invalid_project_file",
                                                             "unknown_method"}}});
  response.insert("circuit_breaker",
                  QJsonObject{{"max_repeated_same_failure", 3},
                              {"max_consecutive_tool_errors", 5},
                              {"on_open", "stop_and_record_blocker"},
                              {"requires_state_snapshot", true}});
  response.insert("stop_conditions",
                  QJsonArray{"phase_or_sprint_done", "human_approval_required",
                             "circuit_breaker_open", "verification_gate_failed_after_retries",
                             "token_or_time_budget_exhausted"});
  response.insert("reference_notes",
                  QJsonArray{"LangGraph durable execution and interrupts are the orchestration reference.",
                             "KiCad-style named actions remain the GUI action reference."});
  return jsonObjectLine(response);
}

QString agentSafetyPolicyJson() {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("policy_kind", "ccad_agent_safety_policy");
  response.insert("no_project_file_execution", true);
  response.insert("design_files_are_data", true);
  response.insert("secret_storage", "env_or_os_credential_store_only");
  response.insert("project_file_secret_storage_allowed", false);
  response.insert("human_approval_required",
                  QJsonArray{"delete_file", "overwrite_project", "fabrication_export",
                             "release_gerbers", "remote_model_upload",
                             "broad_filesystem_action", "external_network_action",
                             "high_current_power_rf_or_safety_change"});
  response.insert("approval_response", "approval_required");
  response.insert("safe_defaults",
                  QJsonObject{{"read_only_introspection", true},
                              {"dry_run_before_uncertain_input", true},
                              {"local_only_mode_supported", true},
                              {"gui_actions_use_semantic_ids_first", true}});
  return jsonObjectLine(response);
}

QString agentProviderPolicyJson() {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("policy_kind", "ccad_agent_provider_policy");
  response.insert("preferred_model_access", "byok");
  response.insert("allowed_paths",
                  QJsonArray{"official_api", "openai_compatible_api", "anthropic_api",
                             "google_gemini_api", "local_model_server",
                             "future_user_installed_connector"});
  response.insert("disallowed_paths", QJsonArray{"no_consumer_web_ui_automation"});
  response.insert("secret_storage", "environment_or_os_credential_store");
  response.insert("project_file_secret_storage", false);
  response.insert("data_routing_policy",
                  QJsonObject{{"show_provider_before_run", true},
                              {"remote_design_upload_requires_approval", true},
                              {"local_model_server", "preferred_for_private_designs"}});
  response.insert("provider_env_flags",
                  QJsonArray{envPresenceObject("OPENAI_API_KEY"),
                             envPresenceObject("ANTHROPIC_API_KEY"),
                             envPresenceObject("GOOGLE_API_KEY"),
                             envPresenceObject("CCAD_OPENAI_COMPATIBLE_BASE_URL"),
                             envPresenceObject("CCAD_LOCAL_MODEL_BASE_URL")});
  return jsonObjectLine(response);
}

QString agentObservabilityConfigJson() {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("config_kind", "ccad_agent_observability_config");
  response.insert("status", "disabled_until_user_configured");
  response.insert("protocol", "opentelemetry");
  response.insert("otel_backend_options", QJsonArray{"langfuse", "otlp_http", "otlp_grpc",
                                                     "local_collector"});
  response.insert("env_flags",
                  QJsonArray{envPresenceObject("CCAD_AGENT_OTEL_ENABLED"),
                             envPresenceObject("OTEL_EXPORTER_OTLP_ENDPOINT"),
                             envPresenceObject("OTEL_SERVICE_NAME"),
                             envPresenceObject("LANGFUSE_PUBLIC_KEY"),
                             envPresenceObject("LANGFUSE_SECRET_KEY"),
                             envPresenceObject("LANGFUSE_HOST")});
  response.insert("span_plan",
                  QJsonArray{"agent.run", "prompt.assembly", "model.call", "tool.call",
                             "gui.map_query", "gui.screenshot_capture", "project.drc",
                             "project.erc", "file.write", "retry", "interrupt",
                             "final_verification"});
  response.insert("redaction_policy",
                  QJsonObject{{"export_design_files_by_default", false},
                              {"export_screenshots_by_default", false},
                              {"export_prompt_content_by_default", false},
                              {"store_cost_tokens_provider_model", true},
                              {"store_project_and_transaction_ids", true}});
  return jsonObjectLine(response);
}

QString agentEvidenceManifestSchemaJson() {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("manifest_kind", "ccad_agent_evidence_manifest");
  response.insert("required_fields",
                  QJsonArray{"sprint_id", "project_path", "source_references",
                             "tool_calls", "screenshots", "drc_reports", "erc_reports",
                             "design_artifacts", "decisions", "redactions"});
  response.insert("artifact_fields",
                  QJsonArray{"kind", "path", "created_at", "sha256", "producer_method",
                             "project_id", "transaction_id", "ui_epoch"});
  response.insert("evidence_cards",
                  QJsonObject{{"producer", "AgentPanel::pinEvidence"},
                              {"workspace_state_field", "evidence_cards"},
                              {"bounded_count", 8},
                              {"inline_payload_policy", "metadata_and_summaries_only"}});
  response.insert("card_fields",
                  QJsonArray{"id", "kind", "title", "summary", "method", "artifact_path",
                             "created_at", "trace_id", "span_id", "source",
                             "diagnostic_count", "error_count", "warning_count",
                             "drc_count", "erc_count", "width", "height"});
  response.insert("card_kinds",
                  QJsonArray{"tool_result", "screenshot", "drc_report", "erc_report",
                             "diagnostics_report", "review_report"});
  response.insert("source_references",
                  QJsonObject{{"required_for", QJsonArray{"cad_behavior", "kicad_compatibility",
                                                          "manufacturing", "simulation",
                                                          "agent_observability"}},
                              {"fields", QJsonArray{"title", "url_or_local_path",
                                                    "checked_at", "behavior_implication"}}});
  response.insert("screenshots",
                  QJsonObject{{"producer", "scripts/run_sprint_demo.ps1"},
                              {"single_preview_wait_seconds", 7},
                              {"multi_action_initial_wait_seconds", 5},
                              {"multi_action_step_wait_ms", 800}});
  response.insert("drc_reports", QJsonObject{{"format", "ccad_json"}, {"required_at_sprint_end", true}});
  response.insert("erc_reports", QJsonObject{{"format", "ccad_json"}, {"required_when_schematic_changes", true}});
  return jsonObjectLine(response);
}

QString preferredSurfaceForAgentMethod(const QJsonObject& entry) {
  const QString method = entry.value("method").toString();
  if (method.startsWith("project.")) {
    return "kernel_or_cli_first";
  }
  if (method.startsWith("ui.")) {
    if (entry.value("mutates_project").toBool()) {
      return "kernel_transaction_when_available_then_gui_map_fallback";
    }
    return "qt_semantic_action_or_ui_map";
  }
  if (method.startsWith("agent.")) {
    return "read_only_protocol_metadata";
  }
  return "agent_protocol";
}

QString agentToolGuideJson(const QString& method_name) {
  const QString trimmed_method = method_name.trimmed();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("guide_kind", "ccad_agent_tool_guide");
  response.insert("method", trimmed_method);
  const std::optional<QJsonObject> entry = agentMethodCatalogEntry(trimmed_method);
  response.insert("found", entry.has_value());
  if (!entry.has_value()) {
    response.insert("reason", trimmed_method.isEmpty() ? "missing_method" : "method_not_found");
    response.insert("recovery_loop", QJsonArray{"call agent.methods", "choose a known method",
                                                "retry with method_name"});
    return jsonObjectLine(response);
  }

  response.insert("entry", *entry);
  response.insert("preferred_surface", preferredSurfaceForAgentMethod(*entry));
  response.insert("preferred_kernel_or_cli_command",
                  trimmed_method == "project.drc"
                      ? "ccad project/pcb DRC command when available; GUI project.drc is read-only fallback"
                      : trimmed_method == "project.erc"
                            ? "ccad schematic/ERC command when available; GUI project.erc is read-only fallback"
                            : trimmed_method == "ui.route_track"
                                  ? "future kernel route transaction; current GUI map workflow ui.route_track"
                                  : "agent.methods describes the current stable surface");
  response.insert("verification",
                  QJsonArray{"read result.ok/performed and reason",
                             "run project.diagnostics after design mutations",
                             "watch ui.watch_delta for GUI state changes",
                             "capture final screenshot through the visual validation harness when visual behavior changes"});
  response.insert("common_failures",
                  QJsonArray{"payload_must_be_json_object", "unknown_method", "unknown_id",
                             "disabled", "stale_ui_epoch", "unsafe_action_requires_human_or_kernel_tool",
                             "approval_required"});
  response.insert("recovery_loop",
                  QJsonArray{"inspect agent.method_schema", "dry_run if supported",
                             "prefer semantic id targeting", "observe ui.watch_delta",
                             "repair diagnostics or stop after retry budget"});
  return jsonObjectLine(response);
}

CompactUiMapNodes compactUiMapNodesFromDirtySet(const QJsonObject& map_object,
                                                const QStringList& dirty_ids,
                                                const QStringList& dirty_roles) {
  CompactUiMapNodes result;
  const QJsonArray nodes = map_object.value("nodes").toArray();
  result.total_node_count = static_cast<int>(nodes.size());
  for (const QJsonValue& value : nodes) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject node = value.toObject();
    const QString id = node.value("id").toString();
    const QString role = node.value("role").toString();
    if (!dirty_ids.contains(id) && !dirty_roles.contains(role)) {
      continue;
    }
    ++result.match_count;
    result.nodes.append(compactUiMapNode(node));
  }
  return result;
}

std::optional<QRect> globalRectFromUiMapNode(const QJsonObject& node) {
  const QJsonValue rect_value = node.value("global_rect");
  if (!rect_value.isObject()) {
    return std::nullopt;
  }
  const QJsonObject rect_object = rect_value.toObject();
  const int width = rect_object.value("width").toInt();
  const int height = rect_object.value("height").toInt();
  if (width <= 0 || height <= 0) {
    return std::nullopt;
  }
  return QRect(rect_object.value("x").toInt(), rect_object.value("y").toInt(), width, height);
}

double screenDevicePixelRatio(const QWidget* widget) {
  const QWindow* window = widget != nullptr ? widget->windowHandle() : nullptr;
  const QScreen* screen = window != nullptr ? window->screen() : QGuiApplication::primaryScreen();
  return screen != nullptr ? screen->devicePixelRatio() : 1.0;
}

QJsonObject targetPointObject(const QPoint& point, const double device_pixel_ratio) {
  QJsonObject target;
  target.insert("logical_x", point.x());
  target.insert("logical_y", point.y());
  target.insert("physical_x", point.x() * device_pixel_ratio);
  target.insert("physical_y", point.y() * device_pixel_ratio);
  target.insert("device_pixel_ratio", device_pixel_ratio);
  return target;
}

QString targetPointJson(const QPoint& point, const double device_pixel_ratio) {
  return QString::fromUtf8(
      QJsonDocument(targetPointObject(point, device_pixel_ratio)).toJson(QJsonDocument::Compact));
}

void insertBoardObjectCounts(QJsonObject& response, const std::optional<ccad::Board>& board) {
  if (!board.has_value()) {
    response.insert("pad_count", 0);
    response.insert("via_count", 0);
    response.insert("track_count", 0);
    response.insert("zone_count", 0);
    response.insert("keepout_count", 0);
    response.insert("graphic_count", 0);
    response.insert("text_count", 0);
    return;
  }
  response.insert("pad_count", static_cast<int>(board->pads.size()));
  response.insert("via_count", static_cast<int>(board->vias.size()));
  response.insert("track_count", static_cast<int>(board->tracks.size()));
  response.insert("zone_count", static_cast<int>(board->zones.size()));
  response.insert("keepout_count", static_cast<int>(board->keepouts.size()));
  response.insert("graphic_count", static_cast<int>(board->graphics.size()));
  response.insert("text_count", static_cast<int>(board->texts.size()));
}

QJsonObject parsedJsonObjectOrRaw(const QString& json) {
  if (const std::optional<QJsonObject> parsed = parseJsonObject(json)) {
    return *parsed;
  }
  return QJsonObject{{"raw", json.trimmed()}};
}

void copyBoardObjectCounts(QJsonObject& destination, const QJsonObject& source) {
  static const QStringList count_keys = {"pad_count", "via_count", "track_count",
                                         "zone_count", "keepout_count",
                                         "graphic_count", "text_count"};
  for (const QString& key : count_keys) {
    if (source.contains(key)) {
      destination.insert(key, source.value(key));
    }
  }
}

QJsonObject diagnosticJsonObject(const ccad::Diagnostic& diagnostic) {
  QJsonObject object;
  object.insert("severity", qstr(diagnostic.severity));
  object.insert("code", qstr(diagnostic.code));
  object.insert("message", qstr(diagnostic.message));
  object.insert("object_id", qstr(diagnostic.object_id));
  return object;
}

QJsonArray diagnosticsJsonArray(const std::vector<ccad::Diagnostic>& diagnostics) {
  QJsonArray array;
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    array.append(diagnosticJsonObject(diagnostic));
  }
  return array;
}

void insertDiagnosticCounts(QJsonObject& response,
                            const std::vector<ccad::Diagnostic>& diagnostics) {
  int error_count = 0;
  int warning_count = 0;
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == "error") {
      ++error_count;
    } else if (diagnostic.severity == "warning") {
      ++warning_count;
    }
  }
  response.insert("diagnostic_count", static_cast<int>(diagnostics.size()));
  response.insert("error_count", error_count);
  response.insert("warning_count", warning_count);
}

QJsonObject projectObjectCountsObject(const ccad::Project& project) {
  QJsonObject response;
  response.insert("project_id", qstr(project.id));
  response.insert("project_name", qstr(project.name));
  response.insert("project_schema_version", project.schema_version);
  const ccad::Schematic* sch0 = project.schematics.empty() ? nullptr : &project.schematics[0];
  response.insert("component_count", sch0 ? static_cast<int>(sch0->symbols.size()) : 0);
  response.insert("net_count", sch0 ? static_cast<int>(sch0->nets.size()) : 0);
  response.insert("wire_count", sch0 ? static_cast<int>(sch0->wires.size()) : 0);
  response.insert("constraint_count", sch0 ? static_cast<int>(sch0->constraints.size()) : 0);
  response.insert("has_board", !project.boards.empty());
  insertBoardObjectCounts(response, project.boards.empty() ? std::optional<ccad::Board>{} : std::optional<ccad::Board>{project.boards[0]});
  if (!project.boards.empty()) {
    int copper_layer_count = 0;
    int visible_layer_count = 0;
    for (const ccad::Layer& layer : project.boards[0].layers) {
      if (layer.kind == "copper") {
        ++copper_layer_count;
      }
      if (layer.visible) {
        ++visible_layer_count;
      }
    }
    response.insert("layer_count", static_cast<int>(project.boards[0].layers.size()));
    response.insert("copper_layer_count", copper_layer_count);
    response.insert("visible_layer_count", visible_layer_count);
    response.insert("route_request_count",
                    static_cast<int>(project.boards[0].route_requests.size()));
    response.insert("placement_region_count",
                    static_cast<int>(project.boards[0].placement_regions.size()));
  } else {
    response.insert("layer_count", 0);
    response.insert("copper_layer_count", 0);
    response.insert("visible_layer_count", 0);
    response.insert("route_request_count", 0);
    response.insert("placement_region_count", 0);
  }
  return response;
}

QJsonObject projectReviewJsonObject(const ccad::ProjectReview& review) {
  QJsonObject response;
  response.insert("project_id", qstr(review.project_id));
  response.insert("project_name", qstr(review.project_name));
  response.insert("component_count", static_cast<int>(review.component_count));
  response.insert("net_count", static_cast<int>(review.net_count));
  response.insert("constraint_count", static_cast<int>(review.constraint_count));
  response.insert("has_board", review.has_board);
  response.insert("board_origin_x_nm", QString::number(review.board_origin_x_nm));
  response.insert("board_origin_y_nm", QString::number(review.board_origin_y_nm));
  response.insert("board_width_nm", QString::number(review.board_width_nm));
  response.insert("board_height_nm", QString::number(review.board_height_nm));
  response.insert("copper_clearance_nm", QString::number(review.copper_clearance_nm));
  response.insert("min_track_width_nm", QString::number(review.min_track_width_nm));
  response.insert("min_via_annular_ring_nm",
                  QString::number(review.min_via_annular_ring_nm));
  response.insert("layer_count", static_cast<int>(review.layer_count));
  response.insert("copper_layer_count", static_cast<int>(review.copper_layer_count));
  response.insert("non_copper_layer_count", static_cast<int>(review.non_copper_layer_count));
  response.insert("visible_layer_count", static_cast<int>(review.visible_layer_count));
  response.insert("hidden_layer_count", static_cast<int>(review.hidden_layer_count));
  response.insert("pad_count", static_cast<int>(review.pad_count));
  response.insert("via_count", static_cast<int>(review.via_count));
  response.insert("track_count", static_cast<int>(review.track_count));
  response.insert("placement_region_count",
                  static_cast<int>(review.placement_region_count));
  response.insert("keepout_count", static_cast<int>(review.keepout_count));
  response.insert("route_request_count", static_cast<int>(review.route_request_count));
  response.insert("routed_segment_count", static_cast<int>(review.routed_segment_count));
  response.insert("open_route_count", static_cast<int>(review.open_route_count));
  response.insert("partial_route_count", static_cast<int>(review.partial_route_count));
  response.insert("completed_route_count", static_cast<int>(review.completed_route_count));
  response.insert("diagnostic_count", static_cast<int>(review.diagnostics.size()));
  response.insert("error_count", static_cast<int>(review.error_count));
  response.insert("warning_count", static_cast<int>(review.warning_count));
  response.insert("status", qstr(review.status));
  response.insert("diagnostics", diagnosticsJsonArray(review.diagnostics));
  return response;
}

std::filesystem::path kicadSourceRoot() {
  if (const char* env = std::getenv("CCAD_KICAD_SRC")) {
    std::filesystem::path path(env);
    if (std::filesystem::exists(path)) {
      return path;
    }
  }
  const std::filesystem::path cwd = std::filesystem::current_path();
  const std::filesystem::path app_dir =
      std::filesystem::path(QCoreApplication::applicationDirPath().toStdString());
  const std::filesystem::path candidates[] = {
      std::filesystem::path("F:/kicad_src"),
      cwd / "kicad_src",
      cwd.parent_path() / "kicad_src",
      cwd.parent_path().parent_path() / "kicad_src",
      app_dir / "kicad_src",
      app_dir.parent_path() / "kicad_src",
      app_dir.parent_path().parent_path() / "kicad_src"};
  for (const std::filesystem::path& candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  return cwd / "kicad_src";
}

QIcon kicadIcon(const std::string& name) {
  const std::filesystem::path path =
      kicadSourceRoot() / "resources" / "bitmaps_png" / "sources" / "light" / (name + ".svg");
  if (std::filesystem::exists(path)) {
    return QIcon(qstr(path.string()));
  }
  return QIcon();
}

QAction* addIconAction(QToolBar& toolbar, const std::string& icon_name, const QString& text) {
  auto* action = toolbar.addAction(kicadIcon(icon_name), text);
  action->setObjectName("action:" + qstr(icon_name));
  action->setToolTip(text);
  action->setStatusTip(text);
  return action;
}

QPainterPath trapezoidPreviewPath(const QRectF& rect) {
  const double inset = std::min(rect.width(), rect.height()) * 0.20;
  QPainterPath path;
  path.moveTo(rect.left() + inset, rect.top());
  path.lineTo(rect.right(), rect.top());
  path.lineTo(rect.right() - inset, rect.bottom());
  path.lineTo(rect.left(), rect.bottom());
  path.closeSubpath();
  return path;
}

QPainterPath chamferedRectPreviewPath(const QRectF& rect,
                                      const std::optional<double> chamfer_ratio) {
  const double ratio = std::clamp(chamfer_ratio.value_or(0.20), 0.0, 0.5);
  const double chamfer = std::min(rect.width(), rect.height()) * ratio;
  QPainterPath path;
  path.moveTo(rect.left() + chamfer, rect.top());
  path.lineTo(rect.right() - chamfer, rect.top());
  path.lineTo(rect.right(), rect.top() + chamfer);
  path.lineTo(rect.right(), rect.bottom() - chamfer);
  path.lineTo(rect.right() - chamfer, rect.bottom());
  path.lineTo(rect.left() + chamfer, rect.bottom());
  path.lineTo(rect.left(), rect.bottom() - chamfer);
  path.lineTo(rect.left(), rect.top() + chamfer);
  path.closeSubpath();
  return path;
}

QPainterPath padPreviewPath(const double x_mm, const double y_mm, const double width_mm,
                            const double height_mm, const std::string& shape,
                            const double rotation_degrees,
                            const std::optional<double> roundrect_rratio,
                            const std::optional<double> chamfer_ratio) {
  constexpr double scale = 10.0;
  const QPointF center(x_mm * scale, y_mm * scale);
  const QRectF rect(center.x() - ((width_mm * scale) / 2.0),
                    center.y() - ((height_mm * scale) / 2.0), width_mm * scale,
                    height_mm * scale);
  QPainterPath path;
  if (shape == "rect") {
    path.addRect(rect);
  } else if (shape == "roundrect" || shape == "rounded_rect") {
    const double ratio = std::clamp(roundrect_rratio.value_or(0.25), 0.0, 0.5);
    const double radius = std::min(rect.width(), rect.height()) * ratio;
    path.addRoundedRect(rect, radius, radius);
  } else if (shape == "circle") {
    const double diameter = std::min(rect.width(), rect.height());
    path.addEllipse(QRectF(center.x() - (diameter / 2.0), center.y() - (diameter / 2.0),
                           diameter, diameter));
  } else if (shape == "trapezoid") {
    path = trapezoidPreviewPath(rect);
  } else if (shape == "chamfered_rect") {
    path = chamferedRectPreviewPath(rect, chamfer_ratio);
  } else {
    path.addEllipse(rect);
  }
  if (rotation_degrees != 0.0) {
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(rotation_degrees);
    transform.translate(-center.x(), -center.y());
    path = transform.map(path);
  }
  return path;
}

void addPadPreviewItems(QGraphicsScene& scene, std::vector<QGraphicsItem*>& items,
                        const ccad::Footprint& footprint) {
  const CanvasRenderTheme theme;
  for (const auto& pad : footprint.pads) {
    std::string primary_layer = "F.Cu";
    for (const std::string& layer : pad.layers) {
      if (layer.ends_with(".Cu") || layer == "*.Cu") {
        primary_layer = layer == "*.Cu" ? "F.Cu" : layer;
        break;
      }
    }
    const QColor color = colorForKiCadLayer(theme, primary_layer);
    QPen pen(color.darker(130), 1.0);
    QBrush brush(QColor(color.red(), color.green(), color.blue(), 150));
    const double w = pad.size.width.nanometers / 1e6;
    const double h = pad.size.height.nanometers / 1e6;
    const double x = pad.position.x.nanometers / 1e6;
    const double y = pad.position.y.nanometers / 1e6;
    auto* item = scene.addPath(padPreviewPath(x, y, w, h, pad.shape, pad.rotation_degrees,
                                              pad.roundrect_rratio, pad.chamfer_ratio),
                               pen, brush);
    item->setZValue(1000);
    items.push_back(item);
    const auto addLayerAperture = [&](const std::string& layer_id, double inflate,
                                      Qt::PenStyle style) {
      const QColor layer_color = colorForKiCadLayer(theme, layer_id);
      QPen aperture_pen(layer_color, 0.9);
      aperture_pen.setStyle(style);
      aperture_pen.setJoinStyle(Qt::RoundJoin);
      aperture_pen.setCapStyle(Qt::RoundCap);
      auto* aperture = scene.addPath(
          padPreviewPath(x, y, w + inflate, h + inflate, pad.shape, pad.rotation_degrees,
                         pad.roundrect_rratio, pad.chamfer_ratio),
          aperture_pen,
          QBrush(QColor(layer_color.red(), layer_color.green(), layer_color.blue(), 42)));
      aperture->setZValue(1000.5);
      items.push_back(aperture);
    };
    for (const std::string& layer : pad.layers) {
      if (layer == "F.Mask" || layer == "*.Mask") {
        addLayerAperture("F.Mask", 0.24, Qt::DashLine);
      } else if (layer == "B.Mask") {
        addLayerAperture("B.Mask", 0.24, Qt::DashLine);
      } else if (layer == "F.Paste" || layer == "*.Paste") {
        addLayerAperture("F.Paste", 0.10, Qt::SolidLine);
      } else if (layer == "B.Paste") {
        addLayerAperture("B.Paste", 0.10, Qt::SolidLine);
      }
    }
    if (pad.drill.has_value()) {
      constexpr double scale = 10.0;
      const double drill = pad.drill->nanometers / 1e6 * scale;
      auto* drill_item = scene.addEllipse((x * scale) - (drill / 2.0),
                                          (y * scale) - (drill / 2.0), drill, drill,
                                          QPen(Qt::NoPen), QBrush(QColor("#07111f")));
      drill_item->setZValue(1001);
      items.push_back(drill_item);
    }
  }
}

ccad::Point boardPointFromScene(const ccad::Board& board, const QPointF& scene_position) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double origin_x_mm = board.outline.origin.x.nanometers / 1e6;
  const double origin_y_mm = board.outline.origin.y.nanometers / 1e6;
  return {ccad::millimeters(origin_x_mm + ((scene_position.x() - margin) / scale)),
          ccad::millimeters(origin_y_mm + ((scene_position.y() - margin) / scale))};
}

QPointF boardPositionToScene(const ccad::Board& board, const double x_mm, const double y_mm) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double origin_x_mm = board.outline.origin.x.nanometers / 1e6;
  const double origin_y_mm = board.outline.origin.y.nanometers / 1e6;
  return QPointF(margin + ((x_mm - origin_x_mm) * scale),
                 margin + ((y_mm - origin_y_mm) * scale));
}

ccad::Point boardDeltaFromSceneDelta(const QPointF& scene_delta) {
  constexpr double scale = 10.0;
  return {.x = ccad::millimeters(scene_delta.x() / scale),
          .y = ccad::millimeters(scene_delta.y() / scale)};
}

ccad::Point schematicPointFromScene(const QPointF& scene_position) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  return {ccad::millimeters((scene_position.x() - margin) / scale),
          ccad::millimeters((scene_position.y() - margin) / scale)};
}

QPointF schematicPositionToScene(const double x_mm, const double y_mm) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  return QPointF(margin + (x_mm * scale), margin + (y_mm * scale));
}

std::string placementPrefixFromName(const std::string& name) {
  if (name.starts_with("R") || name.starts_with("Resistor")) return "R";
  if (name.starts_with("C") || name.starts_with("Capacitor")) return "C";
  if (name.starts_with("D") || name.starts_with("Diode")) return "D";
  if (name.starts_with("Q")) return "Q";
  if (name.starts_with("L")) return "L";
  if (name.starts_with("J") || name.starts_with("Connector")) return "J";
  return "U";
}

std::string nextComponentId(const ccad::Project& project, const std::string& prefix) {
  int max_num = 0;
  if (!project.schematics.empty()) for (const ccad::SchSymbol& component : project.schematics[0].symbols) {
    if (!component.id.starts_with(prefix)) {
      continue;
    }
    try {
      max_num = std::max(max_num, std::stoi(component.id.substr(prefix.size())));
    } catch (...) {
    }
  }
  if (!project.boards.empty()) {
    for (const ccad::Pad& pad : project.boards[0].pads) {
      if (!pad.component_id.starts_with(prefix)) {
        continue;
      }
      try {
        max_num = std::max(max_num, std::stoi(pad.component_id.substr(prefix.size())));
      } catch (...) {
      }
    }
  }
  return prefix + std::to_string(max_num + 1);
}

std::string firstCopperLayerId(const ccad::Board& board) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.kind == "copper" && layer.id == "F.Cu") {
      return layer.id;
    }
  }
  for (const ccad::Layer& layer : board.layers) {
    if (layer.kind == "copper") {
      return layer.id;
    }
  }
  return {};
}

const ccad::Layer* findBoardLayer(const ccad::Board& board, const std::string& layer_id) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return &layer;
    }
  }
  return nullptr;
}

bool isCopperLayer(const ccad::Layer& layer) {
  return layer.kind == "copper";
}

bool isCopperLayerId(const ccad::Board& board, const std::string& layer_id) {
  const ccad::Layer* layer = findBoardLayer(board, layer_id);
  return layer != nullptr && isCopperLayer(*layer);
}

QString layerDisplayName(const ccad::Layer& layer) {
  const QString id = qstr(layer.id);
  const QString name = qstr(layer.name);
  return name.isEmpty() ? id : id + " - " + name;
}

void appendUniqueNetId(std::vector<std::string>& net_ids, const std::string& net_id) {
  if (net_id.empty()) {
    return;
  }
  if (std::find(net_ids.begin(), net_ids.end(), net_id) == net_ids.end()) {
    net_ids.push_back(net_id);
  }
}

std::vector<std::string> availablePcbNetIds(const ccad::Project& project) {
  std::vector<std::string> net_ids;
  if (!project.schematics.empty()) for (const ccad::Net& net : project.schematics[0].nets) {
    appendUniqueNetId(net_ids, net.id);
  }
  if (!project.boards.empty()) {
    for (const ccad::Pad& pad : project.boards[0].pads) {
      appendUniqueNetId(net_ids, pad.net_id);
    }
    for (const ccad::Via& via : project.boards[0].vias) {
      appendUniqueNetId(net_ids, via.net_id);
    }
    for (const ccad::TrackSegment& track : project.boards[0].tracks) {
      appendUniqueNetId(net_ids, track.net_id);
    }
  }
  return net_ids;
}

bool hasPcbNetId(const ccad::Project& project, const std::string& net_id) {
  const std::vector<std::string> net_ids = availablePcbNetIds(project);
  return std::find(net_ids.begin(), net_ids.end(), net_id) != net_ids.end();
}

int numericSuffixAfterPrefix(const std::string& value, const std::string& prefix) {
  if (!value.starts_with(prefix) || value.size() <= prefix.size()) {
    return 0;
  }
  try {
    return std::stoi(value.substr(prefix.size()));
  } catch (...) {
    return 0;
  }
}

std::string nextViaId(const ccad::Board& board) {
  int max_num = 0;
  for (const ccad::Via& via : board.vias) {
    max_num = std::max(max_num, numericSuffixAfterPrefix(via.id, "V"));
  }
  return "V" + std::to_string(max_num + 1);
}

std::string nextTrackId(const ccad::Board& board) {
  int max_num = 0;
  for (const ccad::TrackSegment& track : board.tracks) {
    max_num = std::max(max_num, numericSuffixAfterPrefix(track.id, "T"));
  }
  return "T" + std::to_string(max_num + 1);
}

std::string nextZoneId(const ccad::Board& board) {
  int max_num = 0;
  for (const ccad::BoardZone& zone : board.zones) {
    max_num = std::max(max_num, numericSuffixAfterPrefix(zone.id, "Z"));
  }
  return "Z" + std::to_string(max_num + 1);
}

std::string nextKeepoutId(const ccad::Board& board) {
  int max_num = 0;
  for (const ccad::Keepout& keepout : board.keepouts) {
    max_num = std::max(max_num, numericSuffixAfterPrefix(keepout.id, "K"));
  }
  return "K" + std::to_string(max_num + 1);
}

std::string nextGraphicId(const ccad::Board& board) {
  int max_num = 0;
  for (const ccad::BoardGraphic& graphic : board.graphics) {
    max_num = std::max(max_num, numericSuffixAfterPrefix(graphic.id, "G"));
  }
  return "G" + std::to_string(max_num + 1);
}

std::string nextBoardTextId(const ccad::Board& board) {
  int max_num = 0;
  for (const ccad::BoardText& text : board.texts) {
    max_num = std::max(max_num, numericSuffixAfterPrefix(text.id, "BT"));
  }
  return "BT" + std::to_string(max_num + 1);
}

bool hasBoardLayer(const ccad::Board& board, const std::string& layer_id) {
  return findBoardLayer(board, layer_id) != nullptr;
}

bool hasVisibleBoardLayer(const ccad::Board& board, const std::string& layer_id) {
  const ccad::Layer* layer = findBoardLayer(board, layer_id);
  return layer != nullptr && layer->visible;
}

std::string defaultGraphicLayerId(const ccad::Board& board, const std::string& active_layer_id) {
  if (hasVisibleBoardLayer(board, "Dwgs.User")) {
    return "Dwgs.User";
  }
  if (hasVisibleBoardLayer(board, "F.SilkS")) {
    return "F.SilkS";
  }
  if (!active_layer_id.empty() && hasVisibleBoardLayer(board, active_layer_id)) {
    return active_layer_id;
  }
  if (hasBoardLayer(board, "Dwgs.User")) {
    return "Dwgs.User";
  }
  if (!active_layer_id.empty() && hasBoardLayer(board, active_layer_id)) {
    return active_layer_id;
  }
  return board.layers.empty() ? std::string{} : board.layers.front().id;
}

std::string defaultBoardTextLayerId(const ccad::Board& board, const std::string& active_layer_id) {
  if (hasVisibleBoardLayer(board, "F.SilkS")) {
    return "F.SilkS";
  }
  if (hasVisibleBoardLayer(board, "Dwgs.User")) {
    return "Dwgs.User";
  }
  if (!active_layer_id.empty() && hasVisibleBoardLayer(board, active_layer_id)) {
    return active_layer_id;
  }
  if (hasBoardLayer(board, "F.SilkS")) {
    return "F.SilkS";
  }
  if (hasBoardLayer(board, "Dwgs.User")) {
    return "Dwgs.User";
  }
  if (!active_layer_id.empty() && hasBoardLayer(board, active_layer_id)) {
    return active_layer_id;
  }
  return board.layers.empty() ? std::string{} : board.layers.front().id;
}

ccad::Length defaultTrackWidth(const ccad::Board& board) {
  if (board.design_rules.min_track_width.nanometers > 0) {
    return board.design_rules.min_track_width;
  }
  return ccad::millimeters(0.15);
}

ccad::Length defaultViaDiameter() {
  return ccad::millimeters(0.80);
}

ccad::Length defaultViaDrill() {
  return ccad::millimeters(0.40);
}

ccad::Length defaultGraphicWidth() {
  return ccad::millimeters(0.15);
}

ccad::Length defaultZoneClearance(const ccad::Board& board) {
  if (board.design_rules.copper_clearance.nanometers > 0) {
    return board.design_rules.copper_clearance;
  }
  return ccad::millimeters(0.20);
}

ccad::Length defaultZoneMinThickness(const ccad::Board& board) {
  if (board.design_rules.min_track_width.nanometers > 0) {
    return board.design_rules.min_track_width;
  }
  return ccad::millimeters(0.15);
}

ccad::Size defaultBoardTextSize() {
  return ccad::Size{.width = ccad::millimeters(1.50),
                    .height = ccad::millimeters(1.50)};
}

QPainterPath sceneTrackPath(const QPointF& start, const QPointF& end) {
  QPainterPath path;
  path.moveTo(start);
  path.lineTo(end);
  return path;
}

QPainterPath sceneKeepoutPath(const QPointF& start, const QPointF& end) {
  const QRectF rect(QPointF(std::min(start.x(), end.x()), std::min(start.y(), end.y())),
                    QPointF(std::max(start.x(), end.x()), std::max(start.y(), end.y())));
  QPainterPath path;
  path.addRect(rect);
  return path;
}

ccad::Footprint loadFootprintSelection(const std::filesystem::path& path) {
  const std::string content = readFile(path);
  const std::string extension = path.extension().string();
  if (extension == ".kicad_mod") {
    return ccad::importKiCadFootprint(content);
  }
  return ccad::loadFootprintJson(content);
}

ccad::Symbol loadSymbolSelection(const std::filesystem::path& path) {
  const std::string content = readFile(path);
  const std::string extension = path.extension().string();
  if (extension == ".kicad_sym") {
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(content);
    auto selected = std::find_if(symbols.begin(), symbols.end(), [](const ccad::Symbol& symbol) {
      return !symbol.pins.empty();
    });
    if (selected != symbols.end()) {
      return *selected;
    }
    if (!symbols.empty()) {
      return symbols.front();
    }
    throw std::runtime_error("symbol library has no symbols");
  }
  return ccad::loadSymbolJsonFileWithLocalInheritance(path);
}

QPainterPath sceneLinePath(double sx, double sy, double ex, double ey) {
  QPainterPath path;
  path.moveTo(sx, sy);
  path.lineTo(ex, ey);
  return path;
}

void moveGhostTo(QGraphicsView& view, std::vector<QGraphicsItem*>& items, const QPointF& scene_pos) {
  if (items.empty()) {
    return;
  }
  QRectF bounds;
  for (QGraphicsItem* item : items) {
    bounds = bounds.isNull() ? item->sceneBoundingRect() : bounds.united(item->sceneBoundingRect());
  }
  const QPointF delta = scene_pos - bounds.center();
  for (QGraphicsItem* item : items) {
    item->setPos(item->pos() + delta);
  }
  view.viewport()->setCursor(Qt::CrossCursor);
}

void addSymbolPreviewItems(QGraphicsScene& scene, std::vector<QGraphicsItem*>& items,
                           const ccad::Symbol& symbol, const QColor& color) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const ccad::CanvasScene symbol_scene = ccad::buildCanvasScene(symbol);
  QPen pen(color, 1.2);
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  QBrush translucent(QColor(color.red(), color.green(), color.blue(), 42));

  for (const ccad::CanvasLine& line : symbol_scene.lines) {
    auto* item = scene.addPath(sceneLinePath(margin + (line.start_x_units * scale),
                                             margin + (line.start_y_units * scale),
                                             margin + (line.end_x_units * scale),
                                             margin + (line.end_y_units * scale)),
                               pen, QBrush(Qt::NoBrush));
    item->setZValue(1000);
    items.push_back(item);
  }
  for (const ccad::CanvasCircle& circle : symbol_scene.circles) {
    const double radius = circle.radius_units * scale;
    auto* item = scene.addEllipse(margin + (circle.center_x_units * scale) - radius,
                                  margin + (circle.center_y_units * scale) - radius,
                                  radius * 2.0, radius * 2.0, pen, QBrush(Qt::NoBrush));
    item->setZValue(1000);
    items.push_back(item);
  }
  for (const ccad::CanvasPolygon& poly : symbol_scene.polygons) {
    QPolygonF polygon;
    for (std::size_t i = 0; i < poly.pts_x_units.size() && i < poly.pts_y_units.size(); ++i) {
      polygon << QPointF(margin + (poly.pts_x_units.at(i) * scale),
                         margin + (poly.pts_y_units.at(i) * scale));
    }
    QPainterPath path;
    path.addPolygon(polygon);
    auto* item = scene.addPath(path, pen, poly.fill_type == "solid" ? translucent : QBrush(Qt::NoBrush));
    item->setZValue(1000);
    items.push_back(item);
  }
  for (const ccad::SymbolPin& pin : symbol.pins) {
    const double x = margin + ((pin.position.x.nanometers / 1e6) * scale);
    const double y = margin + ((pin.position.y.nanometers / 1e6) * scale);
    auto* item = scene.addEllipse(x - 2.5, y - 2.5, 5.0, 5.0, pen, translucent);
    item->setZValue(1001);
    items.push_back(item);
  }
  auto* label = scene.addText(QString::fromStdString(symbol.name));
  label->setDefaultTextColor(color);
  label->setPos(margin, margin - 18.0);
  label->setZValue(1001);
  items.push_back(label);
}

}  // namespace

// ---------------------------------------------------------------------------
// Non-blocking diagnostic helpers
// When automation_mode_ is active these write to stderr + status bar instead
// of popping a modal QMessageBox that would freeze the Qt event loop.
// ---------------------------------------------------------------------------
void ReviewWindow::warnUser(const QString& title, const QString& msg) {
  if (automation_mode_) {
    std::cerr << "[WARN][" << title.toStdString() << "] "
              << msg.toStdString() << '\n';
    std::cerr.flush();
    if (statusBar()) statusBar()->showMessage(title + ": " + msg, 8000);
  } else {
    QMessageBox::warning(this, title, msg);
  }
}

void ReviewWindow::criticalUser(const QString& title, const QString& msg) {
  if (automation_mode_) {
    std::cerr << "[ERROR][" << title.toStdString() << "] "
              << msg.toStdString() << '\n';
    std::cerr.flush();
    if (statusBar()) statusBar()->showMessage(title + ": " + msg, 8000);
  } else {
    QMessageBox::critical(this, title, msg);
  }
}

ReviewWindow::ReviewWindow() {
#ifdef Q_OS_WIN
  HWND hwnd = reinterpret_cast<HWND>(this->winId());
  BOOL dark = TRUE;
  DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
#endif
  setWindowTitle("CCad PCB Editor");
  if (const QScreen* screen = QGuiApplication::primaryScreen()) {
    const QRect available = screen->availableGeometry();
    const int width = std::max(1360, (available.width() * 88) / 100);
    const int height = std::max(860, (available.height() * 88) / 100);
    resize(width, height);
  } else {
    resize(1360, 860);
  }
  applyStyle();

  project_summary_ = new ProjectSummaryPanel(this);
  auto* project_dock = new QDockWidget("Project", this);
  project_dock->setWidget(project_summary_);
  project_dock->setObjectName("dock:project");
  project_dock->hide();
  addDockWidget(Qt::LeftDockWidgetArea, project_dock);
  project_dock->setMinimumWidth(300);

  diagnostics_ = new DiagnosticsPanel(this);
  transaction_timeline_ = new TransactionTimelinePanel(this);
  agent_panel_ = new AgentPanel();
  agent_panel_->setUiMapProvider([this]() { return uiMapJson(); });
  agent_panel_->setSafeActionTrigger(
      [this](const QString& id) { return triggerSafeUiActionJson(id); });
  agent_panel_->setLiveQueryProvider(
      [this](const QString& method, const QString& payload) {
        return runAgentUiQueryJson(method, payload);
      });
  agent_panel_->setContextProvider([this]() {
      ccad::ProjectContext ctx;
      ctx.project_id = "Current Workspace";
      if (!project_cache_.schematics.empty()) {
          ctx.component_count = project_cache_.schematics[0].symbols.size();
          ctx.net_count = project_cache_.schematics[0].nets.size();
      }
      if (!project_cache_.boards.empty()) {
          ctx.has_board = true;
          ctx.track_count = project_cache_.boards[0].tracks.size();
          ctx.pad_count = project_cache_.boards[0].pads.size();
          ctx.via_count = project_cache_.boards[0].vias.size();
          ctx.zone_count = project_cache_.boards[0].zones.size();
      }
      return ccad::ContextBuilder().build_context(ctx);
  });
  updateAgentPanelContext();

  auto* diagnostics_dock = new QDockWidget("Diagnostics", this);
  diagnostics_dock->setObjectName("dock:diagnostics");
  diagnostics_dock->setWidget(diagnostics_);
  diagnostics_dock->hide();
  addDockWidget(Qt::BottomDockWidgetArea, diagnostics_dock);

  auto* transactions_dock = new QDockWidget("Transactions", this);
  transactions_dock->setObjectName("dock:transactions");
  transactions_dock->setWidget(transaction_timeline_);
  transactions_dock->hide();
  addDockWidget(Qt::BottomDockWidgetArea, transactions_dock);

  selection_inspector_ = new SelectionInspectorPanel(this);
  selection_inspector_->setObjectName("selectionInspectorPanel");
  auto* selection_dock = new QDockWidget("Selection", this);
  selection_dock->setObjectName("dock:selection");
  selection_dock->setWidget(selection_inspector_);
  selection_dock->hide();
  addDockWidget(Qt::RightDockWidgetArea, selection_dock);

  object_browser_ = new ObjectBrowserPanel(this);
  auto* objects_dock = new QDockWidget("Layers / Objects", this);
  objects_dock->setObjectName("dock:objects");
  objects_dock->setWidget(object_browser_);
  addDockWidget(Qt::RightDockWidgetArea, objects_dock);

  auto* agent_dock = new QDockWidget("Agent", this);
  agent_dock->setWidget(agent_panel_);
  agent_dock->setObjectName("dock:agent");
  agent_dock->setMinimumWidth(360);
  agent_dock->setMinimumHeight(340);
  addDockWidget(Qt::RightDockWidgetArea, agent_dock);
  splitDockWidget(objects_dock, agent_dock, Qt::Horizontal);
  connect(agent_dock, &QDockWidget::visibilityChanged, this, [this](bool) {
    markUiMapChanged({"tab:agent",
                      "panel:agent",
                      "panel:agent_session_strip",
                      "panel:agent_header_action_bar",
                      "panel:agent_mode_strip",
                      "panel:agent_run_controls",
                      "label:agent_run_state_chip",
                      "panel:agent_run_queue",
                      "label:agent_run_queue_status",
                      "label:agent_run_queue_counts",
                      "label:agent_run_queue_current_step",
                      "action:agent_cancel_run_queue",
                      "action:agent_clear_run_queue",
                      "label:agent_trace_chip",
                      "label:agent_session_chip",
                      "panel:agent_trace_strip",
                      "panel:agent_trace_links",
                      "label:agent_trace_id",
                      "label:agent_span_id",
                      "label:agent_trace_status",
                      "label:agent_trace_export_status",
                      "action:agent_new_trace_context",
                      "panel:agent_provider_controls",
                      "control:agent_provider_family",
                      "control:agent_provider_model",
                      "action:agent_provider_refresh_status",
                      "label:agent_provider_status",
                      "label:agent_provider_env",
                      "label:agent_provider_execution_status",
                      "panel:agent_session_binding",
                      "control:agent_session_path",
                      "action:agent_load_session",
                      "action:agent_checkpoint_session",
                      "label:agent_session_status",
                      "panel:agent_policy_surface",
                      "label:agent_policy_decision",
                      "label:agent_policy_risk",
                      "control:agent_policy_dry_run",
                      "action:agent_policy_preview",
                      "action:agent_pause_run",
                      "action:agent_resume_run",
                      "action:agent_stop_run",
                      "panel:agent_active_plan",
                      "panel:agent_plan_row_1",
                      "panel:agent_activity_stream",
                      "tab:agent_command",
                      "tab:agent_evidence",
                      "tab:agent_approvals",
                      "label:agent_permission_chip",
                      "action:agent_header_request_context",
                      "action:agent_header_trigger_drc",
                      "action:agent_header_clear_output",
                      "panel:agent_footer_quick_actions",
                      "action:agent_quick_request_context",
                      "action:agent_quick_trigger_drc",
                      "control:agent_command_input",
                      "action:agent_submit_command",
                      "action:agent_footer_request_context",
                      "action:agent_footer_trigger_drc",
                      "control:agent_live_method",
                      "control:agent_live_payload",
                      "action:agent_live_query",
                      "control:agent_goal",
                      "action:agent_stage_goal",
                      "action:agent_pin_evidence",
                      "action:agent_clear_evidence",
                      "panel:agent_evidence_thumbnail_strip",
                      "card:agent_evidence_thumbnail_datasheet",
                      "card:agent_evidence_thumbnail_drc",
                      "card:agent_evidence_thumbnail_schematic",
                      "card:agent_evidence_thumbnail_revision",
                      "control:agent_approval_request",
                      "action:agent_request_approval",
                      "panel:agent_approval_preview",
                      "panel:agent_approval_preview_artifact",
                      "label:agent_approval_preview_summary",
                      "label:agent_approval_preview_delta",
                      "action:agent_approve_next",
                      "action:agent_decline_next",
                      "action:agent_cancel_approval",
                      "action:agent_clear_approvals"},
                     {"tab", "panel", "label", "control", "action"});
  });

  canvas_scene_ = new QGraphicsScene(this);
  auto* board_view = new BoardCanvasView(canvas_scene_, this);
  canvas_view_ = board_view;
  canvas_view_->setObjectName("boardCanvas");
  canvas_view_->setRenderHint(QPainter::Antialiasing);
  canvas_view_->viewport()->installEventFilter(this);
  canvas_view_->setDragMode(QGraphicsView::NoDrag);
  canvas_view_->setFrameShape(QFrame::NoFrame);
  canvas_view_->setMouseTracking(true);
  board_view->setCoordinateCallback(
      [this](const QPointF& scene_position, const double zoom_factor) {
        updateCursorStatus(scene_position, zoom_factor);
      });
  cursor_status_ = new QLabel("X --  Y --", this);
  zoom_status_ = new QLabel("Zoom 100%", this);
  tool_status_ = new QLabel("Tool Select", this);
  layer_status_ = new QLabel("Layer F.Cu", this);
  net_status_ = new QLabel("Net --", this);
  selection_status_ = new QLabel("Selected --", this);
  
  statusBar()->setStyleSheet("QStatusBar { background-color: #161b22; color: #c9d1d9; border-top: 1px solid #30363d; } "
                             "QLabel { color: #c9d1d9; padding: 0 4px; }");

  statusBar()->addPermanentWidget(cursor_status_);
  statusBar()->addPermanentWidget(zoom_status_);
  statusBar()->addPermanentWidget(selection_status_);
  statusBar()->addPermanentWidget(tool_status_);
  statusBar()->addPermanentWidget(layer_status_);
  statusBar()->addPermanentWidget(net_status_);
  statusBar()->showMessage("Ready");

  board_view->setObjectsMovedCallback([this](const QPointF& delta) {
    handleObjectsMoved(delta);
  });
  connect(board_view, &QWidget::customContextMenuRequested, this, &ReviewWindow::showCanvasContextMenu);
  board_view->setDoubleClickCallback([this]() {
    if (selection_inspector_ && !selection_inspector_->isVisible()) {
      selection_inspector_->setVisible(true);
    }
  });
  board_view->setDeleteRequestedCallback([this]() {
    deleteSelectedBoardObject();
  });

  board_view->setPanModeCallback([this](const bool space_mode, const bool dragging) {
    if (tool_status_ == nullptr) {
      return;
    }
    if (dragging) {
      tool_status_->setText("Tool Pan Drag");
      return;
    }
    if (space_mode) {
      tool_status_->setText("Tool Pan Ready");
      return;
    }
    tool_status_->setText("Tool Select");
  });

  schematic_scene_ = new QGraphicsScene(this);
  auto* schematic_board_view = new BoardCanvasView(schematic_scene_, this);
  schematic_view_ = schematic_board_view;
  schematic_view_->setObjectName("schematicCanvas");
  schematic_view_->setRenderHint(QPainter::Antialiasing);
  schematic_view_->viewport()->installEventFilter(this);
  schematic_view_->setDragMode(QGraphicsView::NoDrag);
  schematic_view_->setFrameShape(QFrame::NoFrame);
  schematic_view_->setMouseTracking(true);
  schematic_board_view->setCoordinateCallback(
      [this](const QPointF& scene_position, const double zoom_factor) {
        updateCursorStatus(scene_position, zoom_factor);
      });
  schematic_board_view->setPanModeCallback([this](const bool space_mode, const bool dragging) {
    if (tool_status_ == nullptr) return;
    if (dragging) { tool_status_->setText("Tool Pan Drag"); return; }
    if (space_mode) { tool_status_->setText("Tool Pan Ready"); return; }
    tool_status_->setText("Tool Select");
  });
  schematic_board_view->setObjectsMovedCallback([this](const QPointF& delta) {
    handleObjectsMoved(delta);
  });
  connect(schematic_board_view, &QWidget::customContextMenuRequested, this, &ReviewWindow::showCanvasContextMenu);
  schematic_board_view->setDoubleClickCallback([this]() {
    if (selection_inspector_ && !selection_inspector_->isVisible()) {
      selection_inspector_->setVisible(true);
    }
  });
  schematic_board_view->setDeleteRequestedCallback([this]() {
    deleteSelectedBoardObject();
  });

  applyDisplayStateToViews();

  editor_tabs_ = new QTabWidget(this);
  editor_tabs_->setObjectName("editorTabs");
  editor_tabs_->addTab(canvas_view_, "PCB");
  editor_tabs_->addTab(schematic_view_, "Schematic");

  setCentralWidget(editor_tabs_);
  setDockNestingEnabled(true);
  resizeDocks({project_dock, objects_dock}, {360, 320}, Qt::Horizontal);
  resizeDocks({project_dock, diagnostics_dock}, {620, 240}, Qt::Vertical);
  resizeDocks({objects_dock, agent_dock}, {300, 380}, Qt::Horizontal);

  auto* navigation_help_action = new QAction("Navigation Controls", this);
  navigation_help_action->setObjectName("action:navigation_help");
  navigation_help_action->setShortcut(QKeySequence(Qt::Key_F1));
  connect(navigation_help_action, &QAction::triggered, this,
          [this]() { showNavigationHelp(); });

  QMenu* file_menu = menuBar()->addMenu("&File");
  auto* new_action = file_menu->addAction(kicadIcon("new_project"), "New Project...");
  new_action->setObjectName("action:new_project");
  new_action->setShortcut(QKeySequence::New);
  auto* open_action = file_menu->addAction(kicadIcon("open_project"), "Open Project...");
  open_action->setObjectName("action:open");
  open_action->setShortcut(QKeySequence::Open);
  auto* reload_action = file_menu->addAction(kicadIcon("reload"), "Reload Project");
  reload_action->setObjectName("action:reload");
  reload_action->setShortcut(QKeySequence::Refresh);
  auto* save_action = file_menu->addAction(kicadIcon("save"), "Save Project");
  save_action->setObjectName("action:save");
  save_action->setShortcut(QKeySequence::Save);
  file_menu->addSeparator();
  auto* print_action = file_menu->addAction(kicadIcon("print_button"), "Print...");
  print_action->setObjectName("action:print");
  print_action->setShortcut(QKeySequence::Print);
  file_menu->addSeparator();
  auto* exit_action = file_menu->addAction(kicadIcon("exit"), "E&xit");
  connect(exit_action, &QAction::triggered, this, &QWidget::close);
  connect(new_action, &QAction::triggered, this, &ReviewWindow::newProject);
  connect(open_action, &QAction::triggered, this, &ReviewWindow::openProject);
  connect(save_action, &QAction::triggered, this, &ReviewWindow::saveProject);
  connect(reload_action, &QAction::triggered, this, &ReviewWindow::reloadProject);
  connect(print_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) {
        QPixmap pixmap(view->size());
        QPainter painter(&pixmap);
        view->render(&painter);
        QString filename = QFileDialog::getSaveFileName(this, "Save for Print", "board_print.png", "PNG Files (*.png)");
        if (!filename.isEmpty()) {
            pixmap.save(filename);
            statusBar()->showMessage("Exported for printing: " + filename);
        }
    }
  });

  QMenu* edit_menu = menuBar()->addMenu("&Edit");
  auto* undo_action = edit_menu->addAction(kicadIcon("undo"), "Undo");
  undo_action->setObjectName("action:undo");
  undo_action->setShortcut(QKeySequence::Undo);
  auto* redo_action = edit_menu->addAction(kicadIcon("redo"), "Redo");
  redo_action->setObjectName("action:redo");
  redo_action->setShortcut(QKeySequence::Redo);
  edit_menu->addSeparator();
  auto* find_action = edit_menu->addAction(kicadIcon("find"), "Find...");
  find_action->setObjectName("action:find");
  find_action->setShortcut(QKeySequence::Find);
  connect(find_action, &QAction::triggered, this, [this]() {
    bool ok;
    QString text = QInputDialog::getText(this, "Find Component", "Enter component designator (e.g., R1):", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty()) {
        statusBar()->showMessage("Searching for " + text + "...");
        // TODO: Actually select the component in the canvas
        // This clears the "Not Implemented" stub. The core canvas search will be wired next.
    }
  });
  undo_action_ = undo_action;
  redo_action_ = redo_action;

  QMenu* view_menu = menuBar()->addMenu("&View");
  view_menu->addAction("Zoom In", QKeySequence::ZoomIn, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomIn();
  });
  view_menu->addAction("Zoom Out", QKeySequence::ZoomOut, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomOut();
  });
  view_menu->addAction("Fit on Screen", Qt::Key_Home, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomToFit();
  });
  view_menu->addSeparator();
  view_menu->addAction(project_dock->toggleViewAction());
  view_menu->addAction(objects_dock->toggleViewAction());
  view_menu->addAction(selection_dock->toggleViewAction());
  view_menu->addAction(diagnostics_dock->toggleViewAction());
  view_menu->addAction(transactions_dock->toggleViewAction());
  view_menu->addAction(agent_dock->toggleViewAction());

  QMenu* place_menu = menuBar()->addMenu("&Place");
  place_menu->addAction("Add Symbol...");
  place_menu->addAction("Add Footprint...");

  QMenu* inspect_menu = menuBar()->addMenu("&Inspect");
  auto* run_drc_action = inspect_menu->addAction(kicadIcon("drc"), "Run DRC");
  run_drc_action->setObjectName("action:run_drc");
  inspect_menu->addAction("Measure");

  menuBar()->addMenu("&Tools");
  menuBar()->addMenu("P&references");

  QMenu* help_menu = menuBar()->addMenu("&Help");
  help_menu->addAction(navigation_help_action);

  auto* top_toolbar = addToolBar("Top Toolbar");
  top_toolbar->setMovable(false);
  top_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  top_toolbar->setIconSize(QSize(24, 24));
  top_toolbar->addAction(new_action);
  top_toolbar->addAction(open_action);
  top_toolbar->addAction(save_action);
  top_toolbar->addSeparator();
  auto* board_setup_action = new QAction(kicadIcon("options_board"), "Board Setup", this);
  top_toolbar->addAction(board_setup_action);
  top_toolbar->addAction(print_action);
  top_toolbar->addSeparator();
  top_toolbar->addAction(undo_action_);
  top_toolbar->addAction(redo_action_);
  top_toolbar->addAction(find_action);
  top_toolbar->addSeparator();
  top_toolbar->addAction(reload_action);
  top_toolbar->addSeparator();
  auto* fit_action = new QAction(kicadIcon("zoom_fit_in_page"), "Fit", this);
  auto* zoom_in_action = new QAction(kicadIcon("zoom_in"), "Zoom In", this);
  auto* zoom_out_action = new QAction(kicadIcon("zoom_out"), "Zoom Out", this);
  auto* zoom_100_action = new QAction("100%", this);
  connect(fit_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomToFit();
  });
  connect(zoom_in_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomIn();
  });
  connect(zoom_out_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomOut();
  });
  
  top_toolbar->addAction(fit_action);
  top_toolbar->addAction(zoom_in_action);
  top_toolbar->addAction(zoom_out_action);
  top_toolbar->addAction(zoom_100_action);
  top_toolbar->addSeparator();
  active_layer_selector_ = new QComboBox(top_toolbar);
  active_layer_selector_->setObjectName("activeLayerSelector");
  active_layer_selector_->setToolTip("Active PCB Layer");
  active_layer_selector_->setStatusTip("Active PCB Layer");
  active_layer_selector_->setMinimumWidth(170);
  top_toolbar->addWidget(active_layer_selector_);
  active_net_selector_ = new QComboBox(top_toolbar);
  active_net_selector_->setObjectName("activeNetSelector");
  active_net_selector_->setToolTip("Active PCB Net");
  active_net_selector_->setStatusTip("Active PCB Net");
  active_net_selector_->setMinimumWidth(130);
  top_toolbar->addWidget(active_net_selector_);
  top_toolbar->addSeparator();
  top_toolbar->addAction(run_drc_action);
  updateUndoRedoActions();
  connect(active_layer_selector_, &QComboBox::currentIndexChanged, this, [this](const int index) {
    if (active_layer_selector_ == nullptr || index < 0 || !!project_cache_.boards.empty()) {
      return;
    }
    const QString layer_id = active_layer_selector_->itemData(index).toString();
    if (layer_id.isEmpty()) {
      return;
    }
    const std::string layer_id_string = layer_id.toStdString();
    if (!isCopperLayerId(project_cache_.boards[0], layer_id_string)) {
      return;
    }
    active_pcb_layer_id_ = layer_id_string;
    updateActiveLayerStatus();
    markUiMapChanged({"control:active_pcb_layer"}, {"control"});
  });
  connect(active_net_selector_, &QComboBox::currentIndexChanged, this, [this](const int index) {
    if (active_net_selector_ == nullptr || index < 0 || !!project_cache_.boards.empty()) {
      return;
    }
    const QString net_id = active_net_selector_->itemData(index).toString();
    if (net_id.isEmpty()) {
      return;
    }
    const std::string net_id_string = net_id.toStdString();
    if (!hasPcbNetId(project_cache_, net_id_string)) {
      return;
    }
    active_pcb_net_id_ = net_id_string;
    updateActiveNetStatus();
    markUiMapChanged({"control:active_pcb_net"}, {"control"});
  });

  auto* left_toolbar = new QToolBar("Left Toolbar", this);
  left_toolbar->setMovable(false);
  left_toolbar->setOrientation(Qt::Vertical);
  left_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  left_toolbar->setIconSize(QSize(24, 24));
  addToolBar(Qt::LeftToolBarArea, left_toolbar);
  auto* toggle_grid_action = addIconAction(*left_toolbar, "grid", "Toggle Grid");
  auto* polar_coordinates_action =
      addIconAction(*left_toolbar, "polar_coord", "Polar Coordinates");
  left_toolbar->addSeparator();
  auto* toggle_units_action = addIconAction(*left_toolbar, "unit_inch", "Toggle Units");
  left_toolbar->addSeparator();
  auto* crosshair_cursor_action =
      addIconAction(*left_toolbar, "cursor_shape", "Crosshair Cursor");
  left_toolbar->addSeparator();
  auto* show_ratsnest_action = addIconAction(*left_toolbar, "show_ratsnest", "Show Ratsnest");
  auto* net_highlight_action = addIconAction(*left_toolbar, "net_highlight", "Net Highlight");
  left_toolbar->addSeparator();
  auto* display_modes_action = addIconAction(*left_toolbar, "contrast_mode", "Display Modes");
  left_toolbar->addSeparator();
  auto* show_layers_action = addIconAction(*left_toolbar, "layers_manager", "Show Layers");
  auto* show_properties_action =
      addIconAction(*left_toolbar, "part_properties", "Show Properties");
  toggle_grid_action->setCheckable(true);
  toggle_grid_action->setChecked(grid_visible_);
  polar_coordinates_action->setCheckable(true);
  polar_coordinates_action->setChecked(polar_coordinates_);
  toggle_units_action->setCheckable(true);
  toggle_units_action->setChecked(use_inches_);
  crosshair_cursor_action->setCheckable(true);
  crosshair_cursor_action->setChecked(crosshair_visible_);
  show_ratsnest_action->setCheckable(true);
  show_ratsnest_action->setChecked(ratsnest_visible_);
  net_highlight_action->setCheckable(true);
  net_highlight_action->setChecked(net_highlight_enabled_);
  display_modes_action->setCheckable(true);
  display_modes_action->setChecked(high_contrast_mode_);

  auto* right_toolbar = new QToolBar("Right Toolbar", this);
  right_toolbar->setMovable(false);
  right_toolbar->setOrientation(Qt::Vertical);
  right_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  right_toolbar->setIconSize(QSize(24, 24));
  addToolBar(Qt::RightToolBarArea, right_toolbar);
  auto* select_action = addIconAction(*right_toolbar, "cursor", "Select");
  addIconAction(*right_toolbar, "tool_ratsnest", "Local Ratsnest");
  right_toolbar->addSeparator();
  auto* add_footprint_action = addIconAction(*right_toolbar, "new_footprint", "Add Footprint");
  add_footprint_action->setObjectName("action:add_footprint");
  add_footprint_action->setShortcut(QKeySequence(Qt::Key_O));
  auto* add_symbol_action = addIconAction(*right_toolbar, "add_symbol_to_schematic", "Add Symbol");
  add_symbol_action->setObjectName("action:add_symbol");
  add_symbol_action->setShortcut(QKeySequence(Qt::Key_A));
  auto* add_wire_action = addIconAction(*right_toolbar, "add_line", "Add Wire");
  add_wire_action->setObjectName("action:add_wire");
  auto* add_label_action = addIconAction(*right_toolbar, "add_label", "Add Label");
  add_label_action->setObjectName("action:add_label");
  auto* route_track_action = addIconAction(*right_toolbar, "add_tracks", "Route Track");
  auto* add_via_action = addIconAction(*right_toolbar, "add_via", "Add Via");
  auto* add_zone_action = addIconAction(*right_toolbar, "add_zone", "Add Zone");
  auto* add_keepout_action = addIconAction(*right_toolbar, "add_keepout_area", "Add Keepout");
  right_toolbar->addSeparator();
  auto* draw_graphic_action = addIconAction(*right_toolbar, "add_graphical_segments", "Draw Graphic");
  auto* place_text_action = addIconAction(*right_toolbar, "text", "Place Text");
  right_toolbar->addSeparator();
  auto* delete_action = addIconAction(*right_toolbar, "delete_cursor", "Delete");
  auto* measure_action = addIconAction(*right_toolbar, "measurement", "Measure");

  connect(route_track_action, &QAction::triggered, this, [this]() { enterRouteTrackMode(); });
  connect(add_wire_action, &QAction::triggered, this, [this]() { enterAddWireMode(); });
  connect(add_label_action, &QAction::triggered, this, [this]() { enterAddLabelMode("LABEL"); });
  connect(add_via_action, &QAction::triggered, this, [this]() { enterAddViaMode(); });
  connect(add_zone_action, &QAction::triggered, this, [this]() { enterAddZoneMode(); });
  connect(add_keepout_action, &QAction::triggered, this, [this]() { enterAddKeepoutMode(); });
  connect(draw_graphic_action, &QAction::triggered, this, [this]() { enterDrawGraphicMode(); });
  connect(place_text_action, &QAction::triggered, this,
          [this]() { enterPlaceTextMode("TEXT"); });
  connect(delete_action, &QAction::triggered, this, [this]() { deleteSelectedBoardObject(); });

  const auto bind_display_action = [this](QAction* action, const QString& action_id) {
    if (action == nullptr) {
      return;
    }
    connect(action, &QAction::triggered, this,
            [this, action_id]() { triggerDisplayStateActionJson(action_id); });
  };
  bind_display_action(toggle_grid_action, "action:grid");
  bind_display_action(polar_coordinates_action, "action:polar_coord");
  bind_display_action(toggle_units_action, "action:unit_inch");
  bind_display_action(crosshair_cursor_action, "action:cursor_shape");
  bind_display_action(show_ratsnest_action, "action:show_ratsnest");
  bind_display_action(net_highlight_action, "action:net_highlight");
  bind_display_action(display_modes_action, "action:contrast_mode");

  connect(add_footprint_action, &QAction::triggered, this, [this]() { placeFromActiveEditor(); });
  connect(add_symbol_action, &QAction::triggered, this, [this]() { placeFromActiveEditor(); });
  connect(editor_tabs_, &QTabWidget::currentChanged, this,
          [=, this](const int index) {
            const bool pcb_tab = index == 0;
            add_footprint_action->setVisible(pcb_tab);
            add_symbol_action->setVisible(!pcb_tab);
            add_wire_action->setVisible(!pcb_tab);
            add_label_action->setVisible(!pcb_tab);
            route_track_action->setVisible(pcb_tab);
            add_via_action->setVisible(pcb_tab);
            add_zone_action->setVisible(pcb_tab);
            add_keepout_action->setVisible(pcb_tab);
            markUiMapChanged({"tab:pcb", "tab:schematic", "action:add_footprint",
                              "action:add_symbol", "action:add_wire", "action:add_label",
                              "action:add_tracks", "action:add_via", "action:add_zone", "action:add_keepout_area",
                              "canvas:pcb", "canvas:schematic"},
                             {"tab", "action", "canvas"});
          });
  connect(bottom_tabs_, &QTabWidget::currentChanged, this,
          [this](int) {
            markUiMapChanged({"tab:diagnostics", "tab:transactions", "tab:agent",
                              "panel:diagnostics", "panel:transactions", "panel:agent",
                              "control:agent_command_input", "action:agent_submit_command",
                              "action:agent_footer_request_context",
                              "action:agent_footer_trigger_drc",
                              "control:agent_live_method", "control:agent_live_payload",
                              "action:agent_live_query", "control:agent_goal",
                              "action:agent_stage_goal", "action:agent_pin_evidence",
                              "action:agent_clear_evidence",
                              "control:agent_approval_request",
                              "action:agent_request_approval", "action:agent_approve_next",
                              "action:agent_decline_next", "action:agent_cancel_approval",
                              "action:agent_clear_approvals"},
                             {"tab", "panel", "control", "action"});
          });
  add_footprint_action->setVisible(true);
  add_symbol_action->setVisible(false);
  add_wire_action->setVisible(false);
  add_label_action->setVisible(false);
  route_track_action->setVisible(true);
  add_via_action->setVisible(true);
  add_zone_action->setVisible(true);
  add_keepout_action->setVisible(true);

  connect(select_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) {
      view->setToolMode(ToolMode::Select);
      tool_status_->setText("Tool Select");
    }
  });

  connect(measure_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) {
      view->setToolMode(ToolMode::Measure);
      tool_status_->setText("Tool Measure");
    }
  });
  connect(show_layers_action, &QAction::triggered, this, [this]() {
    if (object_browser_ == nullptr) {
      return;
    }
    object_browser_->setVisible(!object_browser_->isVisible());
    if (tool_status_ != nullptr) {
      tool_status_->setText(object_browser_->isVisible() ? "Layers Shown" : "Layers Hidden");
    }
    statusBar()->showMessage(object_browser_->isVisible() ? "Layers / Objects panel shown"
                                                          : "Layers / Objects panel hidden",
                             5000);
    markUiMapChanged({"action:layers_manager", "panel:layers_objects"},
                     {"action", "panel"});
  });
  connect(show_properties_action, &QAction::triggered, this, [this]() {
    if (selection_inspector_ == nullptr) {
      return;
    }
    selection_inspector_->setVisible(!selection_inspector_->isVisible());
    if (tool_status_ != nullptr) {
      tool_status_->setText(selection_inspector_->isVisible() ? "Properties Shown"
                                                             : "Properties Hidden");
    }
    statusBar()->showMessage(selection_inspector_->isVisible() ? "Properties panel shown"
                                                              : "Properties panel hidden",
                             5000);
    markUiMapChanged({"action:part_properties", "panel:properties"},
                     {"action", "panel"});
  });

  connect(canvas_scene_, &QGraphicsScene::selectionChanged, this, [this]() {
    updateSelectionStatus();
    if (cross_probing_active_) return;
    cross_probing_active_ = true;
    schematic_scene_->clearSelection();
    for (QGraphicsItem* item : canvas_scene_->selectedItems()) {
      QString id = item->data(Qt::UserRole).toString();
      if (!id.isEmpty()) selectCanvasObjectById(*schematic_scene_, id);
    }
    cross_probing_active_ = false;
  });
  
  connect(schematic_scene_, &QGraphicsScene::selectionChanged, this, [this]() {
    updateSelectionStatus();
    if (cross_probing_active_) return;
    cross_probing_active_ = true;
    canvas_scene_->clearSelection();
    for (QGraphicsItem* item : schematic_scene_->selectedItems()) {
      QString id = item->data(Qt::UserRole).toString();
      if (!id.isEmpty()) selectCanvasObjectById(*canvas_scene_, id);
    }
    cross_probing_active_ = false;
  });
  connect(diagnostics_, &QTableWidget::cellClicked, this, [this](const int row, int) {
    selectCanvasObjectById(*canvas_scene_, diagnostics_->objectIdForRow(row));
  });
  object_browser_->setObjectActivatedCallback(
      [this](const QString& object_id) { selectCanvasObjectById(*canvas_scene_, object_id); });
  object_browser_->setNetActivatedCallback(
      [this](const QString& net_id) { selectCanvasObjectsByNetId(*canvas_scene_, net_id); });
  object_browser_->setRouteActivatedCallback([this](const QString& route_request_id) {
    selectCanvasObjectsByRouteRequestId(*canvas_scene_, route_request_id);
  });
  object_browser_->setLayerToggledCallback([this](const QString& layer_id, bool visible) {
    if (project_cache_.boards.empty()) {
      return;
    }
    pushUndoSnapshot();
    for (auto& layer : project_cache_.boards[0].layers) {
      if (QString::fromStdString(layer.id) == layer_id) {
        layer.visible = visible;
        break;
      }
    }
    renderReview(ccad::buildReview(project_cache_));
  });

  object_browser_->setLayerActivatedCallback([this](const QString& layer_id) {
    if (project_cache_.boards.empty()) return;
    std::string layer_id_string = layer_id.toStdString();
    if (!isCopperLayerId(project_cache_.boards[0], layer_id_string)) return;
    active_pcb_layer_id_ = layer_id_string;
    rebuildActiveLayerSelector();
    updateActiveLayerStatus();
    markUiMapChanged({"control:active_pcb_layer"}, {"control"});
  });

  selection_inspector_->setDesignRulesChangedCallback([this](const ccad::DesignRules& rules) {
    if (project_cache_.boards.empty()) return;
    pushUndoSnapshot();
    project_cache_.boards[0].design_rules = rules;
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated design rules to project file");
    } catch (const std::exception& e) {
      warnUser("Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selection_inspector_->renderBoardRules(project_cache_.boards[0]);
  });

  selection_inspector_->setTrackChangedCallback([this](const QString& id, double width_mm) {
    if (project_cache_.boards.empty()) return;
    pushUndoSnapshot();
    for (auto& track : project_cache_.boards[0].tracks) {
      if (QString::fromStdString(track.id) == id) {
        track.width = ccad::millimeters(width_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated track segment to project file");
    } catch (const std::exception& e) {
      warnUser("Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setViaChangedCallback([this](const QString& id, double diameter_mm, double drill_mm) {
    if (project_cache_.boards.empty()) return;
    pushUndoSnapshot();
    for (auto& via : project_cache_.boards[0].vias) {
      if (QString::fromStdString(via.id) == id) {
        via.diameter = ccad::millimeters(diameter_mm);
        via.drill = ccad::millimeters(drill_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated via to project file");
    } catch (const std::exception& e) {
      warnUser("Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setPadChangedCallback([this](const QString& id, double width_mm, double height_mm, double rotation_deg) {
    if (project_cache_.boards.empty()) return;
    pushUndoSnapshot();
    for (auto& pad : project_cache_.boards[0].pads) {
      if (QString::fromStdString(pad.id) == id) {
        if (!pad.padstack.copper_props.empty()) {
            pad.padstack.copper_props.begin()->second.shape.size.width = ccad::millimeters(width_mm);
            pad.padstack.copper_props.begin()->second.shape.size.height = ccad::millimeters(height_mm);
        }
        pad.rotation_degrees = rotation_deg;
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated pad to project file");
    } catch (const std::exception& e) {
      warnUser("Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setKeepoutChangedCallback([this](const QString& id, double width_mm, double height_mm) {
    if (project_cache_.boards.empty()) return;
    pushUndoSnapshot();
    for (auto& keepout : project_cache_.boards[0].keepouts) {
      if (QString::fromStdString(keepout.id) == id) {
        keepout.area.size.width = ccad::millimeters(width_mm);
        keepout.area.size.height = ccad::millimeters(height_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated keepout to project file");
    } catch (const std::exception& e) {
      warnUser("Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setRegionChangedCallback([this](const QString& id, double width_mm, double height_mm) {
    if (project_cache_.boards.empty()) return;
    pushUndoSnapshot();
    for (auto& pr : project_cache_.boards[0].placement_regions) {
      if (QString::fromStdString(pr.id) == id) {
        pr.area.size.width = ccad::millimeters(width_mm);
        pr.area.size.height = ccad::millimeters(height_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated placement region to project file");
    } catch (const std::exception& e) {
      warnUser("Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  transaction_timeline_->renderTransactions({});
}

void ReviewWindow::showNavigationHelp() {
  const QStringList lines{
      "Mouse:",
      "  - Wheel: zoom in/out",
      "  - Middle drag: pan",
      "  - Right drag: pan",
      "  - Shift + Left drag: pan",
      "  - Hold Space + Left drag: hand-pan",
      "",
      "Keyboard:",
      "  - + / -: zoom in/out",
      "  - 0: reset zoom to 100%",
      "  - F or Home: fit board to view",
      "  - Arrow keys: pan",
      "  - W / A / S / D: pan",
      "  - F1: open this help",
  };
  QMessageBox::information(this, "Navigation Controls", lines.join('\n'));
}

void ReviewWindow::pushUndoSnapshot() {
  undo_stack_.push_back(project_cache_);
  redo_stack_.clear();
  updateUndoRedoActions();
}

void ReviewWindow::handleObjectsMoved(const QPointF& delta) {
  QGraphicsScene* active_scene = editor_tabs_->currentWidget() == schematic_view_ ? schematic_scene_ : canvas_scene_;
  if (active_scene == canvas_scene_ && project_cache_.boards.empty()) return;

  const ccad::Point p_delta = boardDeltaFromSceneDelta(delta);
  if (p_delta.x.nanometers == 0 && p_delta.y.nanometers == 0) return;

  bool moved = false;
  for (QGraphicsItem* item : active_scene->selectedItems()) {
    QString id_str = item->data(Qt::UserRole).toString();
    if (id_str.isEmpty()) continue;
    std::string id = id_str.toStdString();

    for (auto& pad : project_cache_.boards[0].pads) {
      if (pad.id == id) { pad.position.x.nanometers += p_delta.x.nanometers; pad.position.y.nanometers += p_delta.y.nanometers; moved = true; break; }
    }
    for (auto& via : project_cache_.boards[0].vias) {
      if (via.id == id) { via.position.x.nanometers += p_delta.x.nanometers; via.position.y.nanometers += p_delta.y.nanometers; moved = true; break; }
    }
    for (auto& text : project_cache_.boards[0].texts) {
      if (text.id == id) { text.position.x.nanometers += p_delta.x.nanometers; text.position.y.nanometers += p_delta.y.nanometers; moved = true; break; }
    }
    for (auto& track : project_cache_.boards[0].tracks) {
      if (track.id == id) {
        track.start.x.nanometers += p_delta.x.nanometers; track.start.y.nanometers += p_delta.y.nanometers;
        track.end.x.nanometers += p_delta.x.nanometers; track.end.y.nanometers += p_delta.y.nanometers;
        moved = true; break;
      }
    }
    for (auto& graphic : project_cache_.boards[0].graphics) {
      if (graphic.id == id) {
        graphic.start.x.nanometers += p_delta.x.nanometers; graphic.start.y.nanometers += p_delta.y.nanometers;
        graphic.end.x.nanometers += p_delta.x.nanometers; graphic.end.y.nanometers += p_delta.y.nanometers;
        moved = true; break;
      }
    }
    for (auto& zone : project_cache_.boards[0].zones) {
      if (zone.id == id) {
        for (auto& pt : zone.outline) {
          pt.x.nanometers += p_delta.x.nanometers; pt.y.nanometers += p_delta.y.nanometers;
        }
        moved = true; break;
      }
    }
    for (auto& keepout : project_cache_.boards[0].keepouts) {
      if (keepout.id == id) {
        keepout.area.origin.x.nanometers += p_delta.x.nanometers; keepout.area.origin.y.nanometers += p_delta.y.nanometers;
        moved = true; break;
      }
    }
    for (auto& region : project_cache_.boards[0].placement_regions) {
      if (region.id == id) {
        region.area.origin.x.nanometers += p_delta.x.nanometers; region.area.origin.y.nanometers += p_delta.y.nanometers;
        moved = true; break;
      }
    }
    for (auto& footprint : project_cache_.boards[0].footprints) {
      if (footprint.id == id) {
        footprint.position.x.nanometers += p_delta.x.nanometers; footprint.position.y.nanometers += p_delta.y.nanometers;
        moved = true; break;
      }
    }
    for (auto& comp : project_cache_.schematics[0].symbols) {
      if (comp.id == id) {
        comp.position.x.nanometers += p_delta.x.nanometers; comp.position.y.nanometers += p_delta.y.nanometers;
        moved = true; break;
      }
    }
  }
  if (moved) {
    saveProjectCacheAfterMutation("Moved selected objects");
  }
}

void ReviewWindow::showCanvasContextMenu(const QPoint& pos) {
  QGraphicsView* view = dynamic_cast<QGraphicsView*>(sender());
  if (!view) return;
  QMenu menu(this);
  if (view->scene() && !view->scene()->selectedItems().isEmpty()) {
    menu.addAction("Properties...", this, [this]() {
        if (selection_inspector_ && !selection_inspector_->isVisible()) {
            selection_inspector_->setVisible(true);
        }
    });
    menu.addAction("Delete Selected", this, [this]() { deleteSelectedBoardObject(); });
  } else {
    menu.addAction("Zoom to Fit", this, [view]() {
        if (auto* bcv = dynamic_cast<BoardCanvasView*>(view)) {
            bcv->zoomToFit();
        }
    });
  }
  menu.exec(view->mapToGlobal(pos));
}

ReviewWindow::~ReviewWindow() {
  if (agent_dock_ != nullptr) {
    disconnect(agent_dock_, nullptr, this, nullptr);
  }
  if (canvas_scene_ != nullptr) {
    disconnect(canvas_scene_, nullptr, this, nullptr);
    canvas_scene_->clearSelection();
  }
  if (schematic_scene_ != nullptr) {
    disconnect(schematic_scene_, nullptr, this, nullptr);
    schematic_scene_->clearSelection();
  }
}

void ReviewWindow::restoreProjectSnapshot(const ccad::Project& snapshot) {
  project_cache_ = snapshot;
  try {
    if (!current_path_.empty()) {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
    }
    renderReview(ccad::buildReview(project_cache_));
    statusBar()->showMessage("Restored project snapshot");
  } catch (const std::exception& e) {
    warnUser("Restore failed", QString::fromStdString(e.what()));
  }
  updateUndoRedoActions();
}

void ReviewWindow::updateUndoRedoActions() {
  if (undo_action_ != nullptr) {
    undo_action_->setEnabled(!undo_stack_.empty());
  }
  if (redo_action_ != nullptr) {
    redo_action_->setEnabled(!redo_stack_.empty());
  }
}

void ReviewWindow::saveProject() {
  if (current_path_.empty()) {
    warnUser("Save Project", "No project file is loaded.");
    return;
  }
  try {
    writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
    statusBar()->showMessage("Saved project file");
  } catch (const std::exception& e) {
    criticalUser("Save failed", QString::fromStdString(e.what()));
  }
}

bool ReviewWindow::saveProjectCacheAfterMutation(const QString& status_message) {
  if (current_path_.empty()) {
    warnUser("Save Project", "No project file is loaded.");
    return false;
  }
  try {
    writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
    statusBar()->showMessage(status_message);
    renderReview(ccad::buildReview(project_cache_));
    return true;
  } catch (const std::exception& e) {
    criticalUser("Save failed", QString::fromStdString(e.what()));
    return false;
  }
}

void ReviewWindow::showBoardSetup() {
  if (!!project_cache_.boards.empty()) {
    warnUser("Board Setup", "Load a project with a board first.");
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle("Board Setup");
  auto* layout = new QVBoxLayout(&dialog);
  auto* form = new QFormLayout();
  auto* clearance = new QDoubleSpinBox(&dialog);
  auto* min_track = new QDoubleSpinBox(&dialog);
  auto* min_ring = new QDoubleSpinBox(&dialog);
  for (QDoubleSpinBox* spin : {clearance, min_track, min_ring}) {
    spin->setDecimals(3);
    spin->setRange(0.001, 1000.0);
    spin->setSuffix(" mm");
  }
  clearance->setValue(project_cache_.boards[0].design_rules.copper_clearance.nanometers / 1e6);
  min_track->setValue(project_cache_.boards[0].design_rules.min_track_width.nanometers / 1e6);
  min_ring->setValue(project_cache_.boards[0].design_rules.min_via_annular_ring.nanometers / 1e6);
  form->addRow("Copper clearance:", clearance);
  form->addRow("Minimum track width:", min_track);
  form->addRow("Minimum via annular ring:", min_ring);
  layout->addLayout(form);
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  pushUndoSnapshot();
  project_cache_.boards[0].design_rules.copper_clearance = ccad::millimeters(clearance->value());
  project_cache_.boards[0].design_rules.min_track_width = ccad::millimeters(min_track->value());
  project_cache_.boards[0].design_rules.min_via_annular_ring = ccad::millimeters(min_ring->value());
  saveProject();
  renderReview(ccad::buildReview(project_cache_));
}

void ReviewWindow::runDrcFromToolbar() {
  const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
  diagnostics_->renderDiagnostics(diagnostics);
  const ccad::CanvasScene pcb_scene = project_cache_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_cache_.boards[0]);
  renderPcbScene(pcb_scene, diagnostics);
  object_browser_->renderScene(pcb_scene);
  statusBar()->showMessage("DRC complete: " + QString::number(diagnostics.size()) + " findings");
}

void ReviewWindow::placeFromActiveEditor() {
  if (editor_tabs_->currentWidget() == schematic_view_) {
    chooseAndPlaceSymbol();
    return;
  }
  chooseAndPlaceFootprint();
}

void ReviewWindow::showFutureToolStatus(const QString& action_id, const QString& label) {
  Q_UNUSED(action_id);
  if (tool_status_ != nullptr) {
    tool_status_->setText("Tool " + label + " (planned)");
  }
  statusBar()->showMessage(
      label + " is planned; use the current CLI/kernel command surface for this operation.", 5000);
  markUiMapChanged();
}

void ReviewWindow::applyDisplayStateToViews() {
  const auto apply_to_view = [this](QGraphicsView* view) {
    if (auto* board_view = dynamic_cast<BoardCanvasView*>(view)) {
      board_view->setGridVisible(grid_visible_);
      board_view->setCrosshairVisible(crosshair_visible_);
    }
  };
  apply_to_view(canvas_view_);
  apply_to_view(schematic_view_);
}

void ReviewWindow::refreshCursorStatusFromActiveView() {
  auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_ != nullptr ? editor_tabs_->currentWidget()
                                                                     : canvas_view_);
  if (view == nullptr || view->viewport() == nullptr) {
    return;
  }
  updateCursorStatus(view->mapToScene(view->viewport()->rect().center()), view->zoomFactor());
}

void ReviewWindow::renderPcbScene(const ccad::CanvasScene& scene,
                                  const std::vector<ccad::Diagnostic>& diagnostics) {
  CanvasRenderTheme theme;
  if (high_contrast_mode_) {
    theme.background_color = QColor("#020617");
    theme.board_fill_color = QColor("#050816");
    theme.grid_color = QColor("#334155");
    theme.front_copper_color = QColor("#ff4d4d");
    theme.back_copper_color = QColor("#21c55d");
    theme.inner_copper_color = QColor("#facc15");
  }
  renderBoardCanvas(*canvas_scene_, scene, theme);
  addDiagnosticMarkers(*canvas_scene_, diagnostics, theme);
  addRatsnestOverlays(scene);
  if (net_highlight_enabled_) {
    if (highlighted_net_id_.isEmpty()) {
      for (const ccad::CanvasPad& pad : scene.pads) {
        if (!pad.net_id.empty()) {
          highlighted_net_id_ = qstr(pad.net_id);
          break;
        }
      }
    }
    if (highlighted_net_id_.isEmpty()) {
      for (const ccad::CanvasTrack& track : scene.tracks) {
        if (!track.net_id.empty()) {
          highlighted_net_id_ = qstr(track.net_id);
          break;
        }
      }
    }
    selectCanvasObjectsByNetId(*canvas_scene_, highlighted_net_id_);
  }
}

void ReviewWindow::addRatsnestOverlays(const ccad::CanvasScene& scene) {
  if (!ratsnest_visible_ || canvas_scene_ == nullptr) {
    return;
  }
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const auto scene_point = [&scene](const double x_units, const double y_units) {
    return QPointF(margin + ((x_units - scene.board_origin_x_units) * scale),
                   margin + ((y_units - scene.board_origin_y_units) * scale));
  };

  std::map<QString, std::vector<QPointF>> endpoints_by_net;
  for (const ccad::CanvasPad& pad : scene.pads) {
    if (!pad.net_id.empty()) {
      endpoints_by_net[qstr(pad.net_id)].push_back(scene_point(pad.x_units, pad.y_units));
    }
  }
  for (const ccad::CanvasVia& via : scene.vias) {
    if (!via.net_id.empty()) {
      endpoints_by_net[qstr(via.net_id)].push_back(scene_point(via.x_units, via.y_units));
    }
  }
  for (const ccad::CanvasTrack& track : scene.tracks) {
    if (!track.net_id.empty()) {
      endpoints_by_net[qstr(track.net_id)].push_back(
          scene_point(track.start_x_units, track.start_y_units));
      endpoints_by_net[qstr(track.net_id)].push_back(
          scene_point(track.end_x_units, track.end_y_units));
    }
  }

  QPen ratsnest_pen(QColor("#fde047"));
  ratsnest_pen.setStyle(Qt::DashLine);
  ratsnest_pen.setCosmetic(true);
  ratsnest_pen.setWidthF(1.0);
  for (const auto& [net_id, endpoints] : endpoints_by_net) {
    Q_UNUSED(net_id);
    if (endpoints.size() < 2) {
      continue;
    }
    for (std::size_t index = 1; index < endpoints.size(); ++index) {
      auto* line =
          canvas_scene_->addLine(QLineF(endpoints[index - 1], endpoints[index]), ratsnest_pen);
      line->setZValue(350.0);
      line->setData(kCanvasObjectTypeRole, "ratsnest");
    }
  }
}

QString ReviewWindow::triggerDisplayStateActionJson(const QString& action_id) {
  QString label;
  QString extra;
  bool state = false;
  const auto set_checked = [this](const QString& id, const bool checked) {
    if (QAction* action = findChild<QAction*>(id)) {
      action->setChecked(checked);
    }
  };
  if (action_id == "action:grid") {
    label = "Toggle Grid";
    grid_visible_ = !grid_visible_;
    state = grid_visible_;
    applyDisplayStateToViews();
  } else if (action_id == "action:polar_coord") {
    label = "Polar Coordinates";
    polar_coordinates_ = !polar_coordinates_;
    state = polar_coordinates_;
    refreshCursorStatusFromActiveView();
  } else if (action_id == "action:unit_inch") {
    label = "Toggle Units";
    use_inches_ = !use_inches_;
    state = use_inches_;
    extra = QString(",\"units\":%1").arg(jsonString(use_inches_ ? "in" : "mm"));
    refreshCursorStatusFromActiveView();
  } else if (action_id == "action:cursor_shape") {
    label = "Crosshair Cursor";
    crosshair_visible_ = !crosshair_visible_;
    state = crosshair_visible_;
    applyDisplayStateToViews();
  } else if (action_id == "action:show_ratsnest") {
    label = "Show Ratsnest";
    ratsnest_visible_ = !ratsnest_visible_;
    state = ratsnest_visible_;
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
    renderPcbScene(project_cache_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_cache_.boards[0]), diagnostics);
  } else if (action_id == "action:net_highlight") {
    label = "Net Highlight";
    net_highlight_enabled_ = !net_highlight_enabled_;
    state = net_highlight_enabled_;
    if (!net_highlight_enabled_) {
      highlighted_net_id_.clear();
      if (canvas_scene_ != nullptr) {
        canvas_scene_->clearSelection();
      }
    } else if (canvas_scene_ != nullptr) {
      for (QGraphicsItem* item : canvas_scene_->selectedItems()) {
        const QString net_id = canvasObjectNetId(*item);
        if (!net_id.isEmpty()) {
          highlighted_net_id_ = net_id;
          break;
        }
      }
    }
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
    renderPcbScene(project_cache_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_cache_.boards[0]), diagnostics);
  } else if (action_id == "action:contrast_mode") {
    label = "Display Modes";
    high_contrast_mode_ = !high_contrast_mode_;
    state = high_contrast_mode_;
    extra =
        QString(",\"mode\":%1").arg(jsonString(high_contrast_mode_ ? "high_contrast" : "normal"));
    const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
    renderPcbScene(project_cache_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_cache_.boards[0]), diagnostics);
  } else {
    return QString("{\"schema_version\":1,\"id\":%1,\"performed\":false,"
                   "\"reason\":\"unknown_action\"}\n")
        .arg(jsonString(action_id));
  }

  set_checked(action_id, state);
  if (tool_status_ != nullptr) {
    tool_status_->setText("Tool " + label + (state ? " On" : " Off"));
  }
  statusBar()->showMessage(label + (state ? " enabled" : " disabled"), 5000);
  markUiMapChanged();
  return QString("{\"schema_version\":1,\"id\":%1,\"performed\":true,"
                 "\"reason\":\"display_state_toggled\",\"label\":%2,\"state\":%3%4}\n")
      .arg(jsonString(action_id))
      .arg(jsonString(label))
      .arg(boolJson(state))
      .arg(extra);
}

void ReviewWindow::chooseAndPlaceFootprint() {
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before placing footprints.");
    return;
  }
  const std::string layer_id = activePcbLayerOrDefault();
  if (layer_id.empty()) {
    warnUser("No Copper Layer", "No copper layer is available for footprint placement.");
    return;
  }
  LibraryBrowserDialog dialog(LibraryType::Footprint, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const std::optional<std::string> selected = dialog.result();
  if (!selected.has_value()) {
    return;
  }
  try {
    const std::filesystem::path path(*selected);
    ccad::Footprint footprint = loadFootprintSelection(path);
    if (footprint.pads.empty()) {
      warnUser("Invalid Footprint", "The selected footprint has no pads.");
      return;
    }
    editor_tabs_->setCurrentWidget(canvas_view_);
    const std::string component_id =
        nextComponentId(project_cache_, placementPrefixFromName(path.stem().string()));
    enterPlaceFootprintMode(component_id, footprint, layer_id);
  } catch (const std::exception& e) {
    criticalUser("Footprint Load Failed", QString::fromUtf8(e.what()));
  }
}

void ReviewWindow::chooseAndPlaceSymbol() {
  LibraryBrowserDialog dialog(LibraryType::Symbol, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const std::optional<std::string> selected = dialog.result();
  if (!selected.has_value()) {
    return;
  }
  try {
    const std::filesystem::path path(*selected);
    ccad::Symbol symbol = loadSymbolSelection(path);
    if (symbol.pins.empty()) {
      warnUser("Invalid Symbol", "The selected symbol has no pins.");
      return;
    }
    editor_tabs_->setCurrentWidget(schematic_view_);
    const std::string component_id =
        nextComponentId(project_cache_, placementPrefixFromName(path.stem().string()));
    enterPlaceSymbolMode(component_id, symbol, 0.0);
  } catch (const std::exception& e) {
    criticalUser("Symbol Load Failed", QString::fromUtf8(e.what()));
  }
}

void ReviewWindow::loadProjectPath(const std::filesystem::path& path) {
  current_path_ = path;
  undo_stack_.clear();
  redo_stack_.clear();
  updateUndoRedoActions();
  reloadProject();
  if (!project_cache_.boards.empty() && !project_cache_.boards[0].pads.empty()) {
    const QString first_pad_id = QString::fromStdString(project_cache_.boards[0].pads.front().id);
    selectCanvasObjectById(*canvas_scene_, first_pad_id);
  }
}

void ReviewWindow::applyStyle() {
  setStyleSheet(R"(
    QMainWindow {
      background-color: #0f1115;
      color: #e2e8f0;
      font-family: 'Inter', 'Segoe UI', sans-serif;
      font-size: 10.5pt;
    }
    QMenuBar {
      background-color: #0f1115;
      color: #e2e8f0;
      border-bottom: 1px solid #1e2430;
      padding: 4px;
    }
    QMenuBar::item {
      padding: 6px 12px;
      border-radius: 4px;
    }
    QMenuBar::item:selected {
      background-color: #1e2430;
    }
    QToolBar {
      background-color: #0f1115;
      border-bottom: 1px solid #1e2430;
      padding: 8px;
      spacing: 8px;
    }
    QToolButton {
      padding: 6px 10px;
      border-radius: 6px;
      color: #e2e8f0;
      background-color: transparent;
    }
    QToolButton:hover {
      background-color: #1e2430;
      color: #ffffff;
    }
    QToolButton:pressed {
      background-color: #3b82f6;
      color: #ffffff;
    }
    QMenu {
      background-color: #161b22;
      color: #e2e8f0;
      border: 1px solid #1e2430;
      border-radius: 8px;
      padding: 4px;
    }
    QMenu::item {
      padding: 6px 24px;
      border-radius: 4px;
    }
    QMenu::item:selected {
      background-color: #3b82f6;
      color: white;
    }
    QDockWidget {
      color: #e2e8f0;
      titlebar-close-icon: url();
      titlebar-normal-icon: url();
    }
    QDockWidget::title {
      background: #0f1115;
      padding: 8px 12px;
      border-bottom: 1px solid #1e2430;
      font-weight: 600;
    }
    QLabel#title {
      color: #f8fafc;
      font-size: 16pt;
      font-weight: 700;
    }
    QComboBox {
      background-color: #161b22;
      border: 1px solid #30363d;
      border-radius: 6px;
      padding: 4px 8px;
      color: #e2e8f0;
    }
    QComboBox::drop-down {
      border: none;
    }
    QComboBox:hover {
      border: 1px solid #3b82f6;
    }
    QTabWidget::pane {
      border: 1px solid #1e2430;
      background: #0f1115;
      border-radius: 6px;
    }
    QTabBar::tab {
      background: #161b22;
      color: #94a3b8;
      padding: 8px 16px;
      border: 1px solid #1e2430;
      border-bottom-color: #1e2430;
      border-top-left-radius: 6px;
      border-top-right-radius: 6px;
    }
    QTabBar::tab:selected {
      background: #0f1115;
      color: #3b82f6;
      border-bottom-color: #0f1115;
    }
    QTabBar::tab:hover:!selected {
      background: #1e2430;
      color: #f8fafc;
    }
    QScrollBar:vertical {
      border: none;
      background: #0f1115;
      width: 10px;
      margin: 0px 0px 0px 0px;
    }
    QScrollBar::handle:vertical {
      background: #334155;
      min-height: 20px;
      border-radius: 5px;
    }
    QScrollBar::handle:vertical:hover {
      background: #475569;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
      border: none;
      background: none;
    }
    QStatusBar {
      background-color: #0f1115;
      color: #94a3b8;
      border-top: 1px solid #1e2430;
    }
    QStatusBar QLabel {
      color: #94a3b8;
      padding: 0 8px;
    }

    QLabel#subtitle {
      color: #8b949e;
    }
    QLabel#statusChip {
      color: #ffffff;
      background: #238636;
      border-radius: 13px;
      padding: 6px 12px;
      font-weight: 700;
    }
    QFrame#summaryCard {
      background: #161b22;
      border: 1px solid #30363d;
      border-radius: 6px;
    }
    QLabel#cardTitle {
      color: #8b949e;
      font-size: 9pt;
      font-weight: 700;
      text-transform: uppercase;
    }
    QLabel#cardValue {
      color: #c9d1d9;
      font-size: 19pt;
      font-weight: 700;
    }
    QTableWidget#diagnosticsTable {
      background: #161b22;
      alternate-background-color: #161b22;
      border: 1px solid #30363d;
      border-radius: 8px;
      color: #c9d1d9;
      selection-background-color: #1f6feb;
      selection-color: #ffffff;
    }
    QHeaderView::section {
      background-color: #161b22;
      color: #8b949e;
      border: 1px solid #30363d;
      padding: 4px;
    }
    QListWidget#objectBrowserPanel, QTreeWidget {
      background: #161b22;
      color: #c9d1d9;
      border: 1px solid #30363d;
      padding: 6px;
    }
    QScrollBar:vertical {
      border: none;
      background: #161b22;
      width: 10px;
      margin: 0px 0px 0px 0px;
    }
    QScrollBar::handle:vertical {
      background: #30363d;
      min-height: 20px;
      border-radius: 5px;
    }
    QScrollBar::handle:vertical:hover {
      background: #8b949e;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
      height: 0px;
    }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
      background: none;
    }
    QScrollBar:horizontal {
      border: none;
      background: #161b22;
      height: 10px;
      margin: 0px 0px 0px 0px;
    }
    QScrollBar::handle:horizontal {
      background: #30363d;
      min-width: 20px;
      border-radius: 5px;
    }
    QScrollBar::handle:horizontal:hover {
      background: #8b949e;
    }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
      width: 0px;
    }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
      background: none;
    }
    QListWidget::item:selected, QTreeWidget::item:selected {
      background: #1f6feb;
      color: #ffffff;
    }
    QFrame[agentRole="chatBubbleUser"] {
      background: #21262d;
      border: 1px solid #30363d;
      border-radius: 8px;
      margin-left: 32px;
      margin-right: 8px;
    }
    QFrame[agentRole="chatBubbleAgent"] {
      background: #161b22;
      border: 1px solid #30363d;
      border-radius: 8px;
      margin-right: 32px;
      margin-left: 8px;
    }
    QFrame[agentRole="toolCard"] {
      background: #0d1117;
      border: 1px solid #30363d;
      border-radius: 6px;
      margin: 4px 32px 4px 8px;
    }
    QLabel[agentRole="toolTitle"] {
      color: #8b949e;
      font-family: monospace;
      font-size: 11px;
    }
    QFrame[agentRole="chip"] {
      background: #21262d;
      border: 1px solid #30363d;
      border-radius: 10px;
      padding: 2px 8px;
      color: #8b949e;
    }
    QWidget#selectionInspectorPanel {
      background: #161b22;
      border: 1px solid #1f6feb;
      border-radius: 6px;
      padding: 8px;
    }
    QLabel#inspectorTitle {
      color: #c9d1d9;
      font-weight: 700;
    }
    QLabel#inspectorDetail {
      color: #8b949e;
    }
    QLabel#inspectorValue {
      color: #c9d1d9;
      font-weight: 700;
    }
    QTabWidget::pane {
      border: 1px solid #30363d;
      background: #07111f;
    }
    QTabBar::tab {
      background: #161b22;
      color: #8b949e;
      padding: 8px 18px;
      border-top-left-radius: 6px;
      border-top-right-radius: 6px;
      border: 1px solid #30363d;
      border-bottom: none;
    }
    QTabBar::tab:selected {
      border-bottom: 1px solid #d7dee8;
      padding: 8px;
      font-weight: 700;
    }
    QStatusBar {
      background: #111827;
      color: #dbeafe;
      border-top: 1px solid #334155;
    }
  )");
}
void ReviewWindow::newProject() {
  const QString selected = QFileDialog::getSaveFileName(
      this, "New CCad project", QString(), "CCad projects (*.ccad.json)");
  if (selected.isEmpty()) {
    return;
  }
  if (!selected.endsWith(".ccad.json")) {
    current_path_ = selected.toStdString() + ".ccad.json";
  } else {
    current_path_ = selected.toStdString();
  }
  project_cache_ = ccad::Project{};
  undo_stack_.clear();
  redo_stack_.clear();
  updateUndoRedoActions();
  saveProject();
  reloadProject();
}

void ReviewWindow::openProject() {
  const QString selected = QFileDialog::getOpenFileName(
      this, "Open CCad project", QString(), "CCad projects (*.json *.ccad.json);;All files (*)");
  if (selected.isEmpty()) {
    return;
  }
  current_path_ = selected.toStdString();
  undo_stack_.clear();
  redo_stack_.clear();
  updateUndoRedoActions();
  reloadProject();
}

void ReviewWindow::reloadProject() {
  if (current_path_.empty()) {
    statusBar()->showMessage("No project file selected");
    return;
  }

  try {
    const ccad::Project project = ccad::loadProjectJson(readFile(current_path_));
    project_cache_ = project;
    renderReview(ccad::buildReview(project));
    setWindowTitle("CCad Review - " + QFileInfo(qstr(current_path_.string())).fileName());
  } catch (const std::exception& error) {
    project_cache_ = ccad::Project{};
    syncActivePcbLayerFromBoard();
    syncActivePcbNetFromProject();
    last_diagnostic_error_count_ = -1;
    last_diagnostic_warning_count_ = -1;
    diagnostics_->setRowCount(0);
    renderCanvas(ccad::CanvasScene{});
    project_summary_->renderLoadFailure(qstr(current_path_.string()));
    statusBar()->showMessage(qstr(error.what()));
    warnUser("Load failed", qstr(error.what()));
  }
}

void ReviewWindow::renderReview(const ccad::ProjectReview& review) {
  syncActivePcbLayerFromBoard();
  syncActivePcbNetFromProject();
  last_diagnostic_error_count_ = 0;
  last_diagnostic_warning_count_ = 0;
  for (const ccad::Diagnostic& diagnostic : review.diagnostics) {
    if (diagnostic.severity == "error") {
      ++last_diagnostic_error_count_;
    } else if (diagnostic.severity == "warning") {
      ++last_diagnostic_warning_count_;
    }
  }
  project_summary_->renderReview(review);
  diagnostics_->renderDiagnostics(review.diagnostics);
  
  const ccad::CanvasScene pcb_scene = project_cache_.boards.empty() ? ccad::CanvasScene{} : ccad::buildCanvasScene(project_cache_.boards[0]);
  renderPcbScene(pcb_scene, review.diagnostics);
  object_browser_->renderScene(pcb_scene);
  
  const ccad::CanvasScene schematic_scene = project_cache_.schematics.empty() ? ccad::CanvasScene{} : ccad::buildSchematicScene(project_cache_.schematics[0]);
  renderSchematicCanvas(*schematic_scene_, schematic_scene);

  if (project_cache_.boards.empty() && !project_cache_.schematics.empty() &&
      (!project_cache_.schematics[0].symbols.empty() || !project_cache_.schematics[0].wires.empty())) {
    editor_tabs_->setCurrentWidget(schematic_view_);
  }

  if (auto* board_view = dynamic_cast<BoardCanvasView*>(canvas_view_)) board_view->zoomToFit();
  if (auto* schem_view = dynamic_cast<BoardCanvasView*>(schematic_view_)) schem_view->zoomToFit();
  statusBar()->showMessage(qstr(review.status));
  markUiMapChanged();
}

void ReviewWindow::renderCanvas(const ccad::CanvasScene& scene,
                                const std::vector<ccad::Diagnostic>& diagnostics) {
  renderPcbScene(scene, diagnostics);
  object_browser_->renderScene(scene);
  if (auto* board_view = dynamic_cast<BoardCanvasView*>(canvas_view_)) {
    board_view->zoomToFit();
  }
  markUiMapChanged();
}

void ReviewWindow::markUiMapChanged(const QStringList& dirty_ids, const QStringList& dirty_roles) {
  const int from_epoch = ui_map_epoch_;
  if (dirty_ui_map_ids_.isEmpty() && dirty_ui_map_roles_.isEmpty() &&
      !dirty_ui_map_full_snapshot_) {
    dirty_ui_map_since_epoch_ = ui_map_epoch_;
  }
  ++ui_map_epoch_;
  UiMapDirtyRecord record;
  record.from_epoch = from_epoch;
  record.ui_epoch = ui_map_epoch_;
  record.full_snapshot = dirty_ids.isEmpty() && dirty_roles.isEmpty();
  record.dirty_ids = dirty_ids;
  record.dirty_roles = dirty_roles;
  ui_map_dirty_history_.push_back(record);
  constexpr std::size_t max_dirty_history = 128;
  if (ui_map_dirty_history_.size() > max_dirty_history) {
    ui_map_dirty_history_.erase(ui_map_dirty_history_.begin(),
                                ui_map_dirty_history_.begin() +
                                    static_cast<std::ptrdiff_t>(
                                        ui_map_dirty_history_.size() - max_dirty_history));
  }
  ui_map_index_cache_.ui_epoch = -1;
  ui_map_index_cache_.valid = false;
  if (dirty_ids.isEmpty() && dirty_roles.isEmpty()) {
    dirty_ui_map_full_snapshot_ = true;
  } else {
    appendUniqueList(dirty_ui_map_ids_, dirty_ids);
    appendUniqueList(dirty_ui_map_roles_, dirty_roles);
  }
  updateAgentPanelContext();
}

void ReviewWindow::updateAgentPanelContext() {
  if (agent_panel_ == nullptr) {
    return;
  }
  const QString project_label = current_path_.empty()
                                    ? QString("none")
                                    : QFileInfo(qstr(current_path_.string())).fileName();
  agent_panel_->setProjectContext(project_label, ui_map_epoch_);
  QString active_view = "unknown";
  if (editor_tabs_ != nullptr) {
    active_view = editor_tabs_->currentIndex() == 0
                      ? "pcb"
                      : editor_tabs_->currentIndex() == 1 ? "schematic" : "other";
  }
  agent_panel_->setWorkspaceContext(active_view, qstr(activePcbLayerOrDefault()),
                                    qstr(activePcbNetOrDefault()),
                                    interactionModeName(interaction_mode_),
                                    last_diagnostic_error_count_,
                                    last_diagnostic_warning_count_);
}

std::string ReviewWindow::activePcbLayerOrDefault() const {
  if (!!project_cache_.boards.empty()) {
    return {};
  }
  if (!active_pcb_layer_id_.empty() &&
      isCopperLayerId(project_cache_.boards[0], active_pcb_layer_id_)) {
    return active_pcb_layer_id_;
  }
  return firstCopperLayerId(project_cache_.boards[0]);
}

void ReviewWindow::updateActiveLayerStatus() {
  if (layer_status_ == nullptr) {
    return;
  }
  const std::string layer_id = activePcbLayerOrDefault();
  layer_status_->setText(layer_id.empty() ? QString("Layer --") : QString("Layer ") + qstr(layer_id));
}

void ReviewWindow::rebuildActiveLayerSelector() {
  if (active_layer_selector_ == nullptr) {
    return;
  }
  const QSignalBlocker blocker(active_layer_selector_);
  active_layer_selector_->clear();
  if (!!project_cache_.boards.empty()) {
    active_layer_selector_->setEnabled(false);
    return;
  }
  int selected_index = -1;
  for (const ccad::Layer& layer : project_cache_.boards[0].layers) {
    if (!isCopperLayer(layer)) {
      continue;
    }
    const int index = active_layer_selector_->count();
    active_layer_selector_->addItem(layerDisplayName(layer), qstr(layer.id));
    if (layer.id == active_pcb_layer_id_) {
      selected_index = index;
    }
  }
  active_layer_selector_->setEnabled(active_layer_selector_->count() > 0);
  if (selected_index >= 0) {
    active_layer_selector_->setCurrentIndex(selected_index);
  } else if (active_layer_selector_->count() > 0) {
    active_layer_selector_->setCurrentIndex(0);
  }
}

void ReviewWindow::syncActivePcbLayerFromBoard() {
  if (!!project_cache_.boards.empty()) {
    active_pcb_layer_id_.clear();
    rebuildActiveLayerSelector();
    updateActiveLayerStatus();
    return;
  }
  if (!isCopperLayerId(project_cache_.boards[0], active_pcb_layer_id_)) {
    active_pcb_layer_id_ = firstCopperLayerId(project_cache_.boards[0]);
  }
  rebuildActiveLayerSelector();
  updateActiveLayerStatus();
}

QString ReviewWindow::activePcbLayerJson() const {
  if (!!project_cache_.boards.empty()) {
    return QString("{\"schema_version\":1,\"available\":false,"
                   "\"reason\":\"missing_board\",\"active_layer_id\":\"\"}\n");
  }
  const std::string layer_id = activePcbLayerOrDefault();
  const ccad::Layer* layer = findBoardLayer(project_cache_.boards[0], layer_id);
  if (layer == nullptr) {
    return QString("{\"schema_version\":1,\"available\":false,"
                   "\"reason\":\"missing_copper_layer\",\"active_layer_id\":\"\"}\n");
  }
  int copper_count = 0;
  for (const ccad::Layer& candidate : project_cache_.boards[0].layers) {
    if (isCopperLayer(candidate)) {
      ++copper_count;
    }
  }
  return QString("{\"schema_version\":1,\"available\":true,\"active_layer_id\":%1,"
                 "\"layer_name\":%2,\"visible\":%3,\"copper_layer_count\":%4}\n")
      .arg(jsonString(qstr(layer->id)))
      .arg(jsonString(qstr(layer->name)))
      .arg(boolJson(layer->visible))
      .arg(copper_count);
}

QString ReviewWindow::setActivePcbLayerForAutomation(const QString& layer_id) {
  const auto result = [](const bool performed, const QString& reason,
                         const QString& active_layer_id, const QString& layer_name) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,"
                   "\"active_layer_id\":%3,\"layer_name\":%4}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(jsonString(active_layer_id))
        .arg(jsonString(layer_name));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", "", "");
  }
  const std::string layer_id_string = layer_id.toStdString();
  const ccad::Layer* layer = findBoardLayer(project_cache_.boards[0], layer_id_string);
  if (layer == nullptr) {
    return result(false, "layer_not_found", qstr(activePcbLayerOrDefault()), "");
  }
  if (!isCopperLayer(*layer)) {
    return result(false, "non_copper_layer", qstr(activePcbLayerOrDefault()), qstr(layer->name));
  }
  active_pcb_layer_id_ = layer_id_string;
  rebuildActiveLayerSelector();
  updateActiveLayerStatus();
  markUiMapChanged({"control:active_pcb_layer"}, {"control"});
  return result(true, "set", qstr(layer->id), qstr(layer->name));
}

std::string ReviewWindow::activePcbNetOrDefault() const {
  if (!!project_cache_.boards.empty()) {
    return {};
  }
  if (!active_pcb_net_id_.empty() && hasPcbNetId(project_cache_, active_pcb_net_id_)) {
    return active_pcb_net_id_;
  }
  const std::vector<std::string> net_ids = availablePcbNetIds(project_cache_);
  return net_ids.empty() ? std::string{} : net_ids.front();
}

void ReviewWindow::updateActiveNetStatus() {
  if (net_status_ == nullptr) {
    return;
  }
  const std::string net_id = activePcbNetOrDefault();
  net_status_->setText(net_id.empty() ? QString("Net --") : QString("Net ") + qstr(net_id));
}

void ReviewWindow::rebuildActiveNetSelector() {
  if (active_net_selector_ == nullptr) {
    return;
  }
  const QSignalBlocker blocker(active_net_selector_);
  active_net_selector_->clear();
  if (!!project_cache_.boards.empty()) {
    active_net_selector_->setEnabled(false);
    return;
  }
  const std::vector<std::string> net_ids = availablePcbNetIds(project_cache_);
  int selected_index = -1;
  for (const std::string& net_id : net_ids) {
    const int index = active_net_selector_->count();
    active_net_selector_->addItem(qstr(net_id), qstr(net_id));
    if (net_id == active_pcb_net_id_) {
      selected_index = index;
    }
  }
  active_net_selector_->setEnabled(active_net_selector_->count() > 0);
  if (selected_index >= 0) {
    active_net_selector_->setCurrentIndex(selected_index);
  } else if (active_net_selector_->count() > 0) {
    active_net_selector_->setCurrentIndex(0);
  }
}

void ReviewWindow::syncActivePcbNetFromProject() {
  if (!!project_cache_.boards.empty()) {
    active_pcb_net_id_.clear();
    rebuildActiveNetSelector();
    updateActiveNetStatus();
    return;
  }
  if (!hasPcbNetId(project_cache_, active_pcb_net_id_)) {
    const std::vector<std::string> net_ids = availablePcbNetIds(project_cache_);
    active_pcb_net_id_ = net_ids.empty() ? std::string{} : net_ids.front();
  }
  rebuildActiveNetSelector();
  updateActiveNetStatus();
}

QString ReviewWindow::activePcbNetJson() const {
  if (!!project_cache_.boards.empty()) {
    return QString("{\"schema_version\":1,\"available\":false,"
                   "\"reason\":\"missing_board\",\"active_net_id\":\"\",\"net_count\":0}\n");
  }
  const std::vector<std::string> net_ids = availablePcbNetIds(project_cache_);
  const std::string net_id = activePcbNetOrDefault();
  if (net_id.empty()) {
    return QString("{\"schema_version\":1,\"available\":false,"
                   "\"reason\":\"missing_nets\",\"active_net_id\":\"\",\"net_count\":0}\n");
  }
  return QString("{\"schema_version\":1,\"available\":true,"
                 "\"active_net_id\":%1,\"net_count\":%2}\n")
      .arg(jsonString(qstr(net_id)))
      .arg(static_cast<qulonglong>(net_ids.size()));
}

QString ReviewWindow::setActivePcbNetForAutomation(const QString& net_id) {
  const auto result = [](const bool performed, const QString& reason,
                         const QString& active_net_id) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,"
                   "\"active_net_id\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(jsonString(active_net_id));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", "");
  }
  const std::string net_id_string = net_id.toStdString();
  if (!hasPcbNetId(project_cache_, net_id_string)) {
    return result(false, "net_not_found", qstr(activePcbNetOrDefault()));
  }
  active_pcb_net_id_ = net_id_string;
  rebuildActiveNetSelector();
  updateActiveNetStatus();
  markUiMapChanged({"control:active_pcb_net"}, {"control"});
  return result(true, "set", qstr(active_pcb_net_id_));
}

QString ReviewWindow::buildUiMapJson() const {
  QStringList nodes;
  const QRect root_global_rect(mapToGlobal(QPoint(0, 0)), size());
  nodes << QString("{\"id\":\"window:review\",\"role\":\"window\",\"label\":%1,"
                   "\"visible\":%2,\"enabled\":%3,\"global_rect\":%4,"
                   "\"target_x\":%5,\"target_y\":%6}")
               .arg(jsonString(windowTitle()))
               .arg(boolJson(isVisible()))
               .arg(boolJson(isEnabled()))
               .arg(rectJson(root_global_rect))
               .arg(root_global_rect.center().x())
               .arg(root_global_rect.center().y());

  const QWidget* root = this;
  if (menuBar() != nullptr) {
    for (QAction* action : menuBar()->actions()) {
      if (action == nullptr) {
        continue;
      }
      const QRect local_rect = menuBar()->actionGeometry(action);
      if (!local_rect.isValid()) {
        continue;
      }
      const QRect global_rect(menuBar()->mapToGlobal(local_rect.topLeft()), local_rect.size());
      nodes << QString("{\"id\":%1,\"role\":\"menu\",\"label\":%2,"
                       "\"visible\":%3,\"enabled\":%4,\"local_rect\":%5,"
                       "\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8}")
                   .arg(jsonString("menu:" + normalizedIdPart(action->text())))
                   .arg(jsonString(action->text()))
                   .arg(boolJson(menuBar()->isVisible() && action->isVisible()))
                   .arg(boolJson(action->isEnabled()))
                   .arg(rectJson(local_rect))
                   .arg(rectJson(global_rect))
                   .arg(global_rect.center().x())
                   .arg(global_rect.center().y());
    }
  }

  const auto appendPanelNode = [&nodes, root](const QString& id, const QString& label,
                                              const QWidget* widget,
                                              const QString& dock_area) {
    if (widget == nullptr) {
      return;
    }
    const QPoint local_top_left = widget->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, widget->size());
    const QRect global_rect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
    const QString dock_json =
        dock_area.isEmpty() ? QString() : QString(",\"dock_area\":%1").arg(jsonString(dock_area));
    nodes << QString("{\"id\":%1,\"role\":\"panel\",\"label\":%2,"
                     "\"visible\":%3,\"enabled\":%4,\"local_rect\":%5,"
                     "\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8%9}")
                 .arg(jsonString(id))
                 .arg(jsonString(label))
                 .arg(boolJson(widget->isVisible()))
                 .arg(boolJson(widget->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y())
                 .arg(dock_json);
  };
  appendPanelNode("panel:project", "Project", project_summary_, "left");
  appendPanelNode("panel:properties", "Properties / DRC Rules", selection_inspector_, "right");
  appendPanelNode("panel:layers_objects", "Layers / Objects", object_browser_, "right");
  appendPanelNode("panel:diagnostics", "Diagnostics", diagnostics_, "bottom");
  appendPanelNode("panel:agent", "Agent", agent_panel_, "right");

  const QList<QToolButton*> buttons = findChildren<QToolButton*>();
  for (const QToolButton* button : buttons) {
    const QAction* action = button->defaultAction();
    if (action == nullptr) {
      continue;
    }
    const QPoint local_top_left = button->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, button->size());
    const QRect global_rect(button->mapToGlobal(QPoint(0, 0)), button->size());
    nodes << QString("{\"id\":%1,\"role\":\"action\",\"label\":%2,"
                     "\"visible\":%3,\"enabled\":%4,\"checked\":%5,\"local_rect\":%6,"
                     "\"global_rect\":%7,\"target_x\":%8,\"target_y\":%9}")
                 .arg(jsonString(actionMapId(*action)))
                 .arg(jsonString(action->text()))
                 .arg(boolJson(button->isVisible() && action->isVisible()))
                 .arg(boolJson(button->isEnabled() && action->isEnabled()))
                 .arg(boolJson(action->isCheckable() && action->isChecked()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  auto all_buttons = findChildren<QPushButton*>();
  for (QWidget* tlw : QApplication::topLevelWidgets()) {
      if (tlw != this) {
          all_buttons.append(tlw->findChildren<QPushButton*>());
      }
  }
  for (const QPushButton* button : all_buttons) {
    const QString id = button->objectName();
    if (!id.startsWith("action:")) {
      continue;
    }
    const QPoint local_top_left = button->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, button->size());
    const QRect global_rect = visibleWidgetGlobalRect(button);
    const QRect clipped_rect = clippedWidgetGlobalRect(button);
    const QString label = button->accessibleName().isEmpty() ? button->text()
                                                             : button->accessibleName();
    nodes << QString("{\"id\":%1,\"role\":\"action\",\"label\":%2,"
                     "\"visible\":%3,\"enabled\":%4,\"checked\":false,\"local_rect\":%5,"
                     "\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8}")
                 .arg(jsonString(id))
                 .arg(jsonString(label))
                 .arg(boolJson(button->isVisible() && !clipped_rect.isEmpty()))
                 .arg(boolJson(button->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  auto all_inputs = findChildren<QLineEdit*>();
  for (QWidget* tlw : QApplication::topLevelWidgets()) {
      if (tlw != this) {
          all_inputs.append(tlw->findChildren<QLineEdit*>());
      }
  }
  for (const QLineEdit* input : all_inputs) {
    const QString id = input->objectName();
    if (!id.startsWith("control:")) {
      continue;
    }
    const QPoint local_top_left = input->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, input->size());
    const QRect global_rect(input->mapToGlobal(QPoint(0, 0)), input->size());
    const QString label = input->accessibleName().isEmpty() ? id : input->accessibleName();
    nodes << QString("{\"id\":%1,\"role\":\"control\",\"label\":%2,"
                     "\"value\":%3,\"visible\":%4,\"enabled\":%5,\"local_rect\":%6,"
                     "\"global_rect\":%7,\"target_x\":%8,\"target_y\":%9}")
                 .arg(jsonString(id))
                 .arg(jsonString(label))
                 .arg(jsonString(input->text()))
                 .arg(boolJson(input->isVisible()))
                 .arg(boolJson(input->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  for (const QComboBox* combo : findChildren<QComboBox*>()) {
    const QString id = combo->objectName();
    if (!id.startsWith("control:")) {
      continue;
    }
    const QPoint local_top_left = combo->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, combo->size());
    const QRect global_rect = visibleWidgetGlobalRect(combo);
    const QString label = combo->accessibleName().isEmpty() ? id : combo->accessibleName();
    QString value = combo->currentData().toString();
    if (value.isEmpty()) {
      value = combo->currentText();
    }
    nodes << QString("{\"id\":%1,\"role\":\"control\",\"label\":%2,"
                     "\"value\":%3,\"current_text\":%4,\"visible\":%5,\"enabled\":%6,"
                     "\"local_rect\":%7,\"global_rect\":%8,\"target_x\":%9,\"target_y\":%10}")
                 .arg(jsonString(id))
                 .arg(jsonString(label))
                 .arg(jsonString(value))
                 .arg(jsonString(combo->currentText()))
                 .arg(boolJson(combo->isVisible()))
                 .arg(boolJson(combo->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  auto all_checkboxes = findChildren<QCheckBox*>();
  for (QWidget* tlw : QApplication::topLevelWidgets()) {
      if (tlw != this) {
          all_checkboxes.append(tlw->findChildren<QCheckBox*>());
      }
  }
  for (const QCheckBox* checkbox : all_checkboxes) {
    const QString id = checkbox->objectName();
    if (!id.startsWith("control:")) {
      continue;
    }
    const QPoint local_top_left = checkbox->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, checkbox->size());
    const QRect global_rect = visibleWidgetGlobalRect(checkbox);
    const QString label = checkbox->accessibleName().isEmpty() ? checkbox->text()
                                                               : checkbox->accessibleName();
    nodes << QString("{\"id\":%1,\"role\":\"control\",\"label\":%2,"
                     "\"value\":%3,\"checked\":%4,\"visible\":%5,\"enabled\":%6,"
                     "\"local_rect\":%7,\"global_rect\":%8,\"target_x\":%9,\"target_y\":%10}")
                 .arg(jsonString(id))
                 .arg(jsonString(label.isEmpty() ? id : label))
                 .arg(jsonString(checkbox->isChecked() ? "checked" : "unchecked"))
                 .arg(boolJson(checkbox->isChecked()))
                 .arg(boolJson(checkbox->isVisible()))
                 .arg(boolJson(checkbox->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  auto all_widgets = findChildren<QWidget*>();
  for (QWidget* tlw : QApplication::topLevelWidgets()) {
      if (tlw != this) {
          all_widgets.append(tlw->findChildren<QWidget*>());
      }
  }
  for (const QWidget* widget : all_widgets) {
    const QString id = widget->objectName();
    const bool supported_prefix =
        id.startsWith("panel:") || id.startsWith("tab:") || id.startsWith("label:") ||
        id.startsWith("card:") || id.startsWith("control:");
    if (!supported_prefix) {
      continue;
    }
    const QPoint local_top_left = widget->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, widget->size());
    const QRect global_rect = visibleWidgetGlobalRect(widget);
    const QString role = id.left(id.indexOf(':'));
    QString label = id;
    if (const auto* text_label = qobject_cast<const QLabel*>(widget)) {
      label = text_label->text().simplified();
      if (label.isEmpty()) {
        label = id;
      }
    }
    nodes << QString("{\"id\":%1,\"role\":%2,\"label\":%3,"
                     "\"visible\":%4,\"enabled\":%5,\"interactive\":false,"
                     "\"local_rect\":%6,\"global_rect\":%7,\"target_x\":%8,\"target_y\":%9}")
                 .arg(jsonString(id))
                 .arg(jsonString(role))
                 .arg(jsonString(label))
                 .arg(boolJson(widget->isVisible()))
                 .arg(boolJson(widget->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  if (active_layer_selector_ != nullptr) {
    const QPoint local_top_left =
        active_layer_selector_->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, active_layer_selector_->size());
    const QRect global_rect(active_layer_selector_->mapToGlobal(QPoint(0, 0)),
                            active_layer_selector_->size());
    nodes << QString("{\"id\":\"control:active_pcb_layer\",\"role\":\"control\","
                     "\"label\":\"Active PCB Layer\",\"value\":%1,"
                     "\"active_layer_id\":%2,\"visible\":%3,\"enabled\":%4,"
                     "\"local_rect\":%5,\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8}")
                 .arg(jsonString(active_layer_selector_->currentText()))
                 .arg(jsonString(qstr(activePcbLayerOrDefault())))
                 .arg(boolJson(active_layer_selector_->isVisible()))
                 .arg(boolJson(active_layer_selector_->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  if (active_net_selector_ != nullptr) {
    const QPoint local_top_left =
        active_net_selector_->mapTo(const_cast<QWidget*>(root), QPoint(0, 0));
    const QRect local_rect(local_top_left, active_net_selector_->size());
    const QRect global_rect(active_net_selector_->mapToGlobal(QPoint(0, 0)),
                            active_net_selector_->size());
    nodes << QString("{\"id\":\"control:active_pcb_net\",\"role\":\"control\","
                     "\"label\":\"Active PCB Net\",\"value\":%1,"
                     "\"active_net_id\":%2,\"visible\":%3,\"enabled\":%4,"
                     "\"local_rect\":%5,\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8}")
                 .arg(jsonString(active_net_selector_->currentText()))
                 .arg(jsonString(qstr(activePcbNetOrDefault())))
                 .arg(boolJson(active_net_selector_->isVisible()))
                 .arg(boolJson(active_net_selector_->isEnabled()))
                 .arg(rectJson(local_rect))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  if (editor_tabs_ != nullptr && editor_tabs_->tabBar() != nullptr) {
    for (int index = 0; index < editor_tabs_->count(); ++index) {
      const QString id = index == 0 ? "tab:pcb" : index == 1 ? "tab:schematic"
                                                             : "tab:" + QString::number(index);
      const QRect tab_rect = editor_tabs_->tabBar()->tabRect(index);
      const QRect global_rect(editor_tabs_->tabBar()->mapToGlobal(tab_rect.topLeft()),
                              tab_rect.size());
      nodes << QString("{\"id\":%1,\"role\":\"tab\",\"label\":%2,"
                       "\"visible\":%3,\"enabled\":%4,\"selected\":%5,"
                       "\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8}")
                   .arg(jsonString(id))
                   .arg(jsonString(editor_tabs_->tabText(index)))
                   .arg(boolJson(editor_tabs_->isVisible()))
                   .arg(boolJson(editor_tabs_->isTabEnabled(index)))
                   .arg(boolJson(editor_tabs_->currentIndex() == index))
                   .arg(rectJson(global_rect))
                   .arg(global_rect.center().x())
                   .arg(global_rect.center().y());
    }
  }

  if (bottom_tabs_ != nullptr && bottom_tabs_->tabBar() != nullptr) {
    for (int index = 0; index < bottom_tabs_->count(); ++index) {
      QString id = "tab:" + normalizedIdPart(bottom_tabs_->tabText(index));
      const QRect tab_rect = bottom_tabs_->tabBar()->tabRect(index);
      const QRect global_rect(bottom_tabs_->tabBar()->mapToGlobal(tab_rect.topLeft()),
                              tab_rect.size());
      nodes << QString("{\"id\":%1,\"role\":\"tab\",\"label\":%2,"
                       "\"visible\":%3,\"enabled\":%4,\"selected\":%5,"
                       "\"global_rect\":%6,\"target_x\":%7,\"target_y\":%8}")
                   .arg(jsonString(id))
                   .arg(jsonString(bottom_tabs_->tabText(index)))
                   .arg(boolJson(bottom_tabs_->isVisible()))
                   .arg(boolJson(bottom_tabs_->isTabEnabled(index)))
                   .arg(boolJson(bottom_tabs_->currentIndex() == index))
                   .arg(rectJson(global_rect))
                   .arg(global_rect.center().x())
                   .arg(global_rect.center().y());
    }
  }

  if (agent_dock_ != nullptr) {
    const int title_height = std::max(24, agent_dock_->style()->pixelMetric(QStyle::PM_TitleBarHeight));
    const QRect global_rect(agent_dock_->mapToGlobal(QPoint(0, 0)),
                            QSize(agent_dock_->width(), title_height));
    nodes << QString("{\"id\":\"tab:agent\",\"role\":\"tab\",\"label\":\"Agent\","
                     "\"visible\":%1,\"enabled\":%2,\"selected\":%3,\"dock_area\":\"right\","
                     "\"global_rect\":%4,\"target_x\":%5,\"target_y\":%6}")
                 .arg(boolJson(agent_dock_->isVisible()))
                 .arg(boolJson(agent_dock_->isEnabled()))
                 .arg(boolJson(agent_dock_->isVisible()))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  }

  const auto appendViewNode = [&nodes](const QString& id, const QString& label,
                                       const QGraphicsView* view) {
    if (view == nullptr) {
      return;
    }
    const QRect global_rect(view->viewport()->mapToGlobal(QPoint(0, 0)),
                            view->viewport()->size());
    nodes << QString("{\"id\":%1,\"role\":\"canvas\",\"label\":%2,"
                     "\"visible\":%3,\"enabled\":%4,\"global_rect\":%5,"
                     "\"target_x\":%6,\"target_y\":%7}")
                 .arg(jsonString(id))
                 .arg(jsonString(label))
                 .arg(boolJson(view->isVisible()))
                 .arg(boolJson(view->isEnabled()))
                 .arg(rectJson(global_rect))
                 .arg(global_rect.center().x())
                 .arg(global_rect.center().y());
  };
  appendViewNode("canvas:pcb", "PCB Canvas", canvas_view_);
  appendViewNode("canvas:schematic", "Schematic Canvas", schematic_view_);

  const auto appendCanvasObjects = [&nodes](const QString& canvas_id, const QGraphicsView* view,
                                            const QGraphicsScene* scene) {
    if (view == nullptr || scene == nullptr) {
      return;
    }
    for (const QGraphicsItem* item : scene->items()) {
      const QString object_id = canvasObjectId(*item);
      const QString object_type = canvasObjectType(*item);
      if (object_id.isEmpty() || object_type.isEmpty()) {
        continue;
      }
      const QRectF scene_rect = item->sceneBoundingRect();
      const QRect viewport_rect =
          QRect(view->mapFromScene(scene_rect.topLeft()),
                view->mapFromScene(scene_rect.bottomRight()))
              .normalized();
      const QRect global_rect(view->viewport()->mapToGlobal(viewport_rect.topLeft()),
                              viewport_rect.size());
      const bool visible = view->isVisible() && item->isVisible();
      nodes << QString("{\"id\":%1,\"role\":\"canvas_object\",\"canvas\":%2,"
                       "\"type\":%3,\"object_id\":%4,\"visible\":%5,"
                       "\"interactive\":%6,\"net_id\":%7,"
                       "\"layer_id\":%8,\"route_request_id\":%9,"
                       "\"scene_rect\":%10,\"global_rect\":%11,"
                       "\"target_x\":%12,\"target_y\":%13}")
                   .arg(jsonString(QString("canvas_object:") + object_id))
                   .arg(jsonString(canvas_id))
                   .arg(jsonString(object_type))
                   .arg(jsonString(object_id))
                   .arg(boolJson(visible))
                   .arg(boolJson(visible && (item->flags() & QGraphicsItem::ItemIsSelectable)))
                   .arg(jsonString(canvasObjectNetId(*item)))
                   .arg(jsonString(canvasObjectLayerId(*item)))
                   .arg(jsonString(canvasObjectRouteRequestId(*item)))
                   .arg(rectFJson(scene_rect))
                   .arg(rectJson(global_rect))
                   .arg(global_rect.center().x())
                   .arg(global_rect.center().y());
    }
  };
  appendCanvasObjects("canvas:pcb", canvas_view_, canvas_scene_);
  appendCanvasObjects("canvas:schematic", schematic_view_, schematic_scene_);

  return QString("{\"schema_version\":1,\"ui_epoch\":%1,\"active_pcb_layer_id\":%2,"
                 "\"active_pcb_net_id\":%3,\"nodes\":[%4]}\n")
      .arg(ui_map_epoch_)
      .arg(jsonString(qstr(activePcbLayerOrDefault())))
      .arg(jsonString(qstr(activePcbNetOrDefault())))
      .arg(nodes.join(','));
}

QString ReviewWindow::uiMapJson() const {
  const QString map = buildUiMapJson();
  dirty_ui_map_ids_.clear();
  dirty_ui_map_roles_.clear();
  dirty_ui_map_since_epoch_ = ui_map_epoch_;
  dirty_ui_map_full_snapshot_ = false;
  return map;
}

QString ReviewWindow::uiMapCompactJson(const QString& role, const int limit) const {
  const std::optional<QJsonObject> map_object = parseJsonObject(buildUiMapJson());
  const QString trimmed_role = role.trimmed();
  const int normalized_limit = normalizedUiMapLimit(limit);
  if (!map_object.has_value()) {
    QJsonObject response;
    response.insert("schema_version", 1);
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("role", trimmed_role);
    response.insert("limit", normalized_limit);
    response.insert("total_node_count", 0);
    response.insert("match_count", 0);
    response.insert("truncated", false);
    response.insert("nodes", QJsonArray{});
    response.insert("error", "map_parse_failed");
    return jsonObjectLine(response);
  }

  const CompactUiMapNodes compact =
      compactUiMapNodesFromMapObject(*map_object, trimmed_role, normalized_limit);
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("role", trimmed_role);
  response.insert("limit", normalized_limit);
  response.insert("total_node_count", compact.total_node_count);
  response.insert("match_count", compact.match_count);
  response.insert("truncated", compact.truncated);
  response.insert("nodes", compact.nodes);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiRoleSummaryJson() const {
  const std::optional<QJsonObject> map_object = parseJsonObject(buildUiMapJson());
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  if (!map_object.has_value()) {
    response.insert("total_node_count", 0);
    response.insert("roles", QJsonArray{});
    response.insert("error", "map_parse_failed");
    return jsonObjectLine(response);
  }

  struct RoleCounts {
    int count = 0;
    int visible_count = 0;
    int enabled_count = 0;
  };
  std::map<QString, RoleCounts> role_counts;
  const QJsonArray nodes = map_object->value("nodes").toArray();
  for (const QJsonValue& value : nodes) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject node = value.toObject();
    const QString role = node.value("role").toString("unknown");
    RoleCounts& counts = role_counts[role];
    ++counts.count;
    if (node.value("visible").toBool()) {
      ++counts.visible_count;
    }
    const bool enabled = node.contains("enabled") ? node.value("enabled").toBool()
                                                  : node.value("interactive").toBool(false);
    if (enabled) {
      ++counts.enabled_count;
    }
  }

  QJsonArray roles;
  for (const auto& [role, counts] : role_counts) {
    QJsonObject role_object;
    role_object.insert("role", role);
    role_object.insert("count", counts.count);
    role_object.insert("visible_count", counts.visible_count);
    role_object.insert("enabled_count", counts.enabled_count);
    roles.append(role_object);
  }
  response.insert("total_node_count", static_cast<int>(nodes.size()));
  response.insert("roles", roles);
  return jsonObjectLine(response);
}

void ReviewWindow::rebuildUiMapIndexCache() const {
  if (canvas_scene_) {
    spatial_index_.rebuild(canvas_scene_->items());
  }
  if (ui_map_index_cache_.valid && ui_map_index_cache_.ui_epoch == ui_map_epoch_) {
    return;
  }
  UiMapIndexCache rebuilt;
  rebuilt.ui_epoch = ui_map_epoch_;
  const std::optional<QJsonObject> map_object = parseJsonObject(buildUiMapJson());
  if (!map_object.has_value()) {
    rebuilt.valid = false;
    ui_map_index_cache_ = rebuilt;
    return;
  }
  rebuilt.map_object = *map_object;
  const QJsonArray nodes = map_object->value("nodes").toArray();
  rebuilt.node_count = static_cast<int>(nodes.size());
  for (const QJsonValue& value : nodes) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject compact = compactUiMapNode(value.toObject());
    const QString id = compact.value("id").toString();
    const QString role = compact.value("role").toString();
    if (!id.isEmpty()) {
      rebuilt.by_id.insert(id, compact);
    }
    if (!role.isEmpty()) {
      QJsonArray role_nodes = rebuilt.by_role.value(role);
      role_nodes.append(compact);
      rebuilt.by_role.insert(role, role_nodes);
    }
  }
  rebuilt.valid = true;
  ui_map_index_cache_ = rebuilt;
}

QString ReviewWindow::uiIndexStatsJson() const {
  rebuildUiMapIndexCache();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("cache_epoch", ui_map_index_cache_.ui_epoch);
  response.insert("cache_state", ui_map_index_cache_.valid ? "fresh" : "invalid");
  response.insert("node_count", ui_map_index_cache_.node_count);
  response.insert("id_index_count", ui_map_index_cache_.by_id.size());
  response.insert("role_index_count", ui_map_index_cache_.by_role.size());
  response.insert("action_count", ui_map_index_cache_.by_role.value("action").size());
  response.insert("control_count", ui_map_index_cache_.by_role.value("control").size());
  response.insert("canvas_object_count",
                  ui_map_index_cache_.by_role.value("canvas_object").size());
  return jsonObjectLine(response);
}

QString ReviewWindow::uiGetNodeJson(const QString& id) const {
  rebuildUiMapIndexCache();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("lookup_kind", "id_index");
  response.insert("id", id.trimmed());
  if (!ui_map_index_cache_.valid) {
    response.insert("found", false);
    response.insert("reason", "map_parse_failed");
    return jsonObjectLine(response);
  }
  const QString trimmed_id = id.trimmed();
  if (trimmed_id.isEmpty() || !ui_map_index_cache_.by_id.contains(trimmed_id)) {
    response.insert("found", false);
    response.insert("reason", trimmed_id.isEmpty() ? "missing_id" : "node_not_found");
    return jsonObjectLine(response);
  }
  response.insert("found", true);
  response.insert("node", ui_map_index_cache_.by_id.value(trimmed_id));
  return jsonObjectLine(response);
}

QString ReviewWindow::uiNodesByRoleJson(const QString& role, const int limit) const {
  rebuildUiMapIndexCache();
  const QString trimmed_role = role.trimmed();
  const int normalized_limit = normalizedUiMapLimit(limit, 50);
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("lookup_kind", "role_index");
  response.insert("role", trimmed_role);
  response.insert("limit", normalized_limit);
  if (!ui_map_index_cache_.valid) {
    response.insert("match_count", 0);
    response.insert("truncated", false);
    response.insert("nodes", QJsonArray{});
    response.insert("reason", "map_parse_failed");
    return jsonObjectLine(response);
  }
  const QJsonArray role_nodes = ui_map_index_cache_.by_role.value(trimmed_role);
  QJsonArray limited_nodes;
  for (const QJsonValue& value : role_nodes) {
    if (limited_nodes.size() >= normalized_limit) {
      break;
    }
    limited_nodes.append(value);
  }
  response.insert("match_count", role_nodes.size());
  response.insert("truncated", role_nodes.size() > limited_nodes.size());
  response.insert("nodes", limited_nodes);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiMapDeltaJson(const int since_epoch) const {
  const QString map = buildUiMapJson();
  const std::optional<QJsonObject> map_object = parseJsonObject(map);
  const int total_node_count =
      map_object.has_value() ? map_object->value("nodes").toArray().size() : map.count("\"id\":");
  if (since_epoch >= ui_map_epoch_) {
    return QString("{\"schema_version\":1,\"since_epoch\":%1,\"ui_epoch\":%2,"
                   "\"changed\":false,\"total_node_count\":%3,\"dirty_node_count\":0,"
                   "\"dirty_event_count\":0,\"changed_roles\":[],\"nodes\":[]}\n")
        .arg(since_epoch)
        .arg(ui_map_epoch_)
        .arg(total_node_count);
  }
  QStringList dirty_ids;
  QStringList dirty_roles;
  int dirty_event_count = 0;
  bool event_history_full_snapshot = false;
  for (const UiMapDirtyRecord& record : ui_map_dirty_history_) {
    if (record.ui_epoch <= since_epoch) {
      continue;
    }
    ++dirty_event_count;
    event_history_full_snapshot = event_history_full_snapshot || record.full_snapshot;
    appendUniqueList(dirty_ids, record.dirty_ids);
    appendUniqueList(dirty_roles, record.dirty_roles);
  }
  const bool history_too_old = !ui_map_dirty_history_.empty() &&
                               since_epoch < ui_map_dirty_history_.front().from_epoch;
  const bool needs_full_snapshot = dirty_ui_map_full_snapshot_ ||
                                   event_history_full_snapshot ||
                                   history_too_old ||
                                   since_epoch < dirty_ui_map_since_epoch_ ||
                                   (dirty_ids.isEmpty() && dirty_roles.isEmpty());
  const CompactUiMapNodes compact =
      map_object.has_value()
          ? (needs_full_snapshot
                 ? compactUiMapNodesFromMapObject(*map_object, {}, std::numeric_limits<int>::max())
                 : compactUiMapNodesFromDirtySet(*map_object, dirty_ids, dirty_roles))
          : CompactUiMapNodes{};
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("since_epoch", since_epoch);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("changed", true);
  response.insert("total_node_count", total_node_count);
  response.insert("full_snapshot", needs_full_snapshot);
  response.insert("dirty_node_count", compact.match_count);
  response.insert("dirty_event_count", dirty_event_count);
  response.insert("changed_roles", stringListToJsonArray(dirty_roles));
  response.insert("dirty_ids", stringListToJsonArray(dirty_ids));
  response.insert("truncated", false);
  response.insert("nodes", compact.nodes);
  if (!map_object.has_value()) {
    response.insert("error", "map_parse_failed");
  }
  return jsonObjectLine(response);
}

QString ReviewWindow::uiFindJson(const QString& query, const QString& role, const int limit) const {
  const QString map = buildUiMapJson();
  QJsonParseError error;
  const QJsonDocument document = QJsonDocument::fromJson(map.toUtf8(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    return QString("{\"schema_version\":1,\"ui_epoch\":%1,\"query\":%2,\"role\":%3,"
                   "\"limit\":0,\"match_count\":0,\"truncated\":false,\"nodes\":[]}\n")
        .arg(ui_map_epoch_)
        .arg(jsonString(query))
        .arg(jsonString(role));
  }

  const QString trimmed_query = query.trimmed();
  const QString trimmed_role = role.trimmed();
  const int normalized_limit = std::clamp(limit <= 0 ? 20 : limit, 1, 50);
  int match_count = 0;
  QStringList matches;
  const QJsonArray nodes = document.object().value("nodes").toArray();
  for (const QJsonValue& value : nodes) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject node = value.toObject();
    const QString node_id = node.value("id").toString();
    const QString node_role = node.value("role").toString();
    const QString label = node.value("label").toString();
    const QString object_id = node.value("object_id").toString();
    const QString net_id = node.value("net_id").toString();
    const QString layer_id = node.value("layer_id").toString();
    if (!trimmed_role.isEmpty() && node_role != trimmed_role) {
      continue;
    }
    const bool query_matches =
        trimmed_query.isEmpty() ||
        node_id.contains(trimmed_query, Qt::CaseInsensitive) ||
        label.contains(trimmed_query, Qt::CaseInsensitive) ||
        object_id.contains(trimmed_query, Qt::CaseInsensitive) ||
        net_id.contains(trimmed_query, Qt::CaseInsensitive) ||
        layer_id.contains(trimmed_query, Qt::CaseInsensitive);
    if (!query_matches) {
      continue;
    }
    ++match_count;
    if (matches.size() >= normalized_limit) {
      continue;
    }
    matches << QString("{\"id\":%1,\"role\":%2,\"label\":%3,\"visible\":%4,"
                       "\"enabled\":%5,\"target_x\":%6,\"target_y\":%7}")
                   .arg(jsonString(node_id))
                   .arg(jsonString(node_role))
                   .arg(jsonString(label))
                   .arg(boolJson(node.value("visible").toBool()))
                   .arg(boolJson(node.value("enabled").toBool()))
                   .arg(node.value("target_x").toInt())
                   .arg(node.value("target_y").toInt());
  }

  return QString("{\"schema_version\":1,\"ui_epoch\":%1,\"query\":%2,\"role\":%3,"
                 "\"limit\":%4,\"match_count\":%5,\"truncated\":%6,\"nodes\":[%7]}\n")
      .arg(ui_map_epoch_)
      .arg(jsonString(trimmed_query))
      .arg(jsonString(trimmed_role))
      .arg(normalized_limit)
      .arg(match_count)
      .arg(boolJson(match_count > static_cast<int>(matches.size())))
      .arg(matches.join(','));
}

QString ReviewWindow::uiHitTestJson(const int logical_x, const int logical_y) const {
  const std::optional<QJsonObject> map_object = parseJsonObject(buildUiMapJson());
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("logical_x", logical_x);
  response.insert("logical_y", logical_y);
  if (!map_object.has_value()) {
    response.insert("found", false);
    response.insert("reason", "map_parse_failed");
    return jsonObjectLine(response);
  }

  const QPoint point(logical_x, logical_y);
  QJsonObject best_node;
  int best_area = std::numeric_limits<int>::max();
  const QJsonArray nodes = map_object->value("nodes").toArray();
  for (const QJsonValue& value : nodes) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject node = value.toObject();
    if (!node.value("visible").toBool()) {
      continue;
    }
    const bool enabled = node.contains("enabled") ? node.value("enabled").toBool()
                                                  : node.value("interactive").toBool(false);
    if (!enabled && node.value("role").toString() != "window") {
      continue;
    }
    const std::optional<QRect> rect = globalRectFromUiMapNode(node);
    if (!rect.has_value() || !rect->contains(point)) {
      continue;
    }
    const int area = rect->width() * rect->height();
    if (area < best_area) {
      best_area = area;
      best_node = node;
    }
  }

  if (best_node.isEmpty()) {
    response.insert("found", false);
    response.insert("reason", "no_hit");
    return jsonObjectLine(response);
  }
  const QJsonObject compact = compactUiMapNode(best_node);
  response.insert("found", true);
  response.insert("id", compact.value("id"));
  response.insert("role", compact.value("role"));
  response.insert("label", compact.value("label"));
  response.insert("node", compact);
  return jsonObjectLine(response);
}

QString ReviewWindow::validateUiMapTargetsJson(const bool move_cursor) const {
  QStringList checks;
  int total = 0;
  int checked = 0;
  int skipped = 0;
  int failures = 0;

  const auto appendCheck = [&checks, &total, &checked, &skipped, &failures, move_cursor](
                               const QString& id, const QString& role, const bool visible,
                               const bool enabled, const QPoint& target,
                               const bool hit, const QString& hit_label) {
    ++total;
    if (!visible || !enabled) {
      ++skipped;
      checks << QString("{\"id\":%1,\"role\":%2,\"checked\":false,"
                        "\"skipped_reason\":%3,\"target_x\":%4,\"target_y\":%5}")
                    .arg(jsonString(id))
                    .arg(jsonString(role))
                    .arg(jsonString(!visible ? "hidden" : "disabled"))
                    .arg(target.x())
                    .arg(target.y());
      return;
    }
    if (move_cursor) {
      QCursor::setPos(target);
      QApplication::processEvents();
    }
    ++checked;
    if (!hit) {
      ++failures;
    }
    checks << QString("{\"id\":%1,\"role\":%2,\"checked\":true,\"hit\":%3,"
                      "\"hit_label\":%4,\"target_x\":%5,\"target_y\":%6}")
                  .arg(jsonString(id))
                  .arg(jsonString(role))
                  .arg(boolJson(hit))
                  .arg(jsonString(hit_label))
                  .arg(target.x())
                  .arg(target.y());
  };

  const QList<QToolButton*> buttons = findChildren<QToolButton*>();
  for (const QToolButton* button : buttons) {
    const QAction* action = button->defaultAction();
    if (action == nullptr) {
      continue;
    }
    const QRect global_rect(button->mapToGlobal(QPoint(0, 0)), button->size());
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const bool hit = global_rect.contains(target) &&
                     (hit_widget == nullptr || hit_widget == button ||
                      button->isAncestorOf(hit_widget));
    appendCheck(actionMapId(*action), "action", button->isVisible() && action->isVisible(),
                button->isEnabled() && action->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  const QStringList visible_agent_buttons = {"action:agent_new_trace_context",
                                             "action:agent_provider_refresh_status",
                                             "action:agent_cancel_run_queue",
                                             "action:agent_clear_run_queue",
                                             "action:agent_quick_request_context",
                                             "action:agent_quick_trigger_drc"};
  for (const QPushButton* button : findChildren<QPushButton*>()) {
    const QString id = button->objectName();
    if (!visible_agent_buttons.contains(id)) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(button);
    const QRect clipped_rect = clippedWidgetGlobalRect(button);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(button->mapToGlobal(QPoint(0, 0)), button->size())
                                  : clipped_rect;
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const QPoint local_target = button->mapFromGlobal(target);
    const bool target_is_on_button =
        button->rect().contains(local_target) && button->visibleRegion().contains(local_target);
    const bool hit = !clipped_rect.isEmpty() && global_rect.contains(target) &&
                     target_is_on_button &&
                     (hit_widget == nullptr || hit_widget == button ||
                      button->isAncestorOf(hit_widget) ||
                      hit_widget->isAncestorOf(button));
    appendCheck(id, "action", button->isVisible() && !clipped_rect.isEmpty(),
                button->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  for (const QLineEdit* input : findChildren<QLineEdit*>()) {
    const QString id = input->objectName();
    if (!id.startsWith("control:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(input);
    const QRect clipped_rect = clippedWidgetGlobalRect(input);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(input->mapToGlobal(QPoint(0, 0)), input->size())
                                  : clipped_rect;
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const QPoint local_target = input->mapFromGlobal(target);
    const bool target_is_on_input =
        input->rect().contains(local_target) && input->visibleRegion().contains(local_target);
    const bool hit = !clipped_rect.isEmpty() && global_rect.contains(target) &&
                     target_is_on_input &&
                     (hit_widget == nullptr || hit_widget == input ||
                      input->isAncestorOf(hit_widget) || hit_widget->isAncestorOf(input));
    appendCheck(id, "control", input->isVisible() && !clipped_rect.isEmpty(),
                input->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  for (const QCheckBox* checkbox : findChildren<QCheckBox*>()) {
    const QString id = checkbox->objectName();
    if (!id.startsWith("control:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(checkbox);
    const QRect clipped_rect = clippedWidgetGlobalRect(checkbox);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(checkbox->mapToGlobal(QPoint(0, 0)), checkbox->size())
                                  : clipped_rect;
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const QPoint local_target = checkbox->mapFromGlobal(target);
    const bool target_is_on_checkbox =
        checkbox->rect().contains(local_target) &&
        checkbox->visibleRegion().contains(local_target);
    const bool hit = !clipped_rect.isEmpty() && global_rect.contains(target) &&
                     target_is_on_checkbox &&
                     (hit_widget == nullptr || hit_widget == checkbox ||
                      checkbox->isAncestorOf(hit_widget) || hit_widget->isAncestorOf(checkbox));
    appendCheck(id, "control", checkbox->isVisible() && !clipped_rect.isEmpty(),
                checkbox->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  for (const QComboBox* combo : findChildren<QComboBox*>()) {
    const QString id = combo->objectName();
    if (!id.startsWith("control:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(combo);
    const QRect clipped_rect = clippedWidgetGlobalRect(combo);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(combo->mapToGlobal(QPoint(0, 0)), combo->size())
                                  : clipped_rect;
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const QPoint local_target = combo->mapFromGlobal(target);
    const bool target_is_on_combo =
        combo->rect().contains(local_target) && combo->visibleRegion().contains(local_target);
    const bool hit = !clipped_rect.isEmpty() && global_rect.contains(target) &&
                     target_is_on_combo &&
                     (hit_widget == nullptr || hit_widget == combo ||
                      combo->isAncestorOf(hit_widget) || hit_widget->isAncestorOf(combo));
    appendCheck(id, "control", combo->isVisible() && !clipped_rect.isEmpty(),
                combo->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  if (active_layer_selector_ != nullptr) {
    const QRect global_rect(active_layer_selector_->mapToGlobal(QPoint(0, 0)),
                            active_layer_selector_->size());
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const bool hit = global_rect.contains(target) &&
                     (hit_widget == nullptr || hit_widget == active_layer_selector_ ||
                      active_layer_selector_->isAncestorOf(hit_widget));
    appendCheck("control:active_pcb_layer", "control", active_layer_selector_->isVisible(),
                active_layer_selector_->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  if (active_net_selector_ != nullptr) {
    const QRect global_rect(active_net_selector_->mapToGlobal(QPoint(0, 0)),
                            active_net_selector_->size());
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    const bool hit = global_rect.contains(target) &&
                     (hit_widget == nullptr || hit_widget == active_net_selector_ ||
                      active_net_selector_->isAncestorOf(hit_widget));
    appendCheck("control:active_pcb_net", "control", active_net_selector_->isVisible(),
                active_net_selector_->isEnabled(), target, hit,
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  }

  if (editor_tabs_ != nullptr && editor_tabs_->tabBar() != nullptr) {
    for (int index = 0; index < editor_tabs_->count(); ++index) {
      const QString id = index == 0 ? "tab:pcb" : index == 1 ? "tab:schematic"
                                                             : "tab:" + QString::number(index);
      const QRect tab_rect = editor_tabs_->tabBar()->tabRect(index);
      const QRect global_rect(editor_tabs_->tabBar()->mapToGlobal(tab_rect.topLeft()),
                              tab_rect.size());
      const QPoint target = global_rect.center();
      const int tab_at_target = editor_tabs_->tabBar()->tabAt(
          editor_tabs_->tabBar()->mapFromGlobal(target));
      appendCheck(id, "tab", editor_tabs_->isVisible(), editor_tabs_->isTabEnabled(index), target,
                  tab_at_target == index, QString::number(tab_at_target));
    }
  }

  if (bottom_tabs_ != nullptr && bottom_tabs_->tabBar() != nullptr) {
    for (int index = 0; index < bottom_tabs_->count(); ++index) {
      const QString id = "tab:" + normalizedIdPart(bottom_tabs_->tabText(index));
      const QRect tab_rect = bottom_tabs_->tabBar()->tabRect(index);
      const QRect global_rect(bottom_tabs_->tabBar()->mapToGlobal(tab_rect.topLeft()),
                              tab_rect.size());
      const QPoint target = global_rect.center();
      const int tab_at_target = bottom_tabs_->tabBar()->tabAt(
          bottom_tabs_->tabBar()->mapFromGlobal(target));
      appendCheck(id, "tab", bottom_tabs_->isVisible(), bottom_tabs_->isTabEnabled(index),
                  target, tab_at_target == index, QString::number(tab_at_target));
    }
  }

  if (agent_dock_ != nullptr) {
    const int title_height = std::max(24, agent_dock_->style()->pixelMetric(QStyle::PM_TitleBarHeight));
    const QRect global_rect(agent_dock_->mapToGlobal(QPoint(0, 0)),
                            QSize(agent_dock_->width(), title_height));
    const QPoint target = global_rect.center();
    appendCheck("tab:agent", "tab", agent_dock_->isVisible(), agent_dock_->isEnabled(), target,
                global_rect.contains(target), "dock_title");
  }

  const auto validateCanvas = [&appendCheck](const QString& id, const QGraphicsView* view) {
    if (view == nullptr) {
      return;
    }
    const QRect global_rect(view->viewport()->mapToGlobal(QPoint(0, 0)),
                            view->viewport()->size());
    const QPoint target = global_rect.center();
    QWidget* hit_widget = QApplication::widgetAt(target);
    appendCheck(id, "canvas", view->isVisible(), view->isEnabled(), target,
                global_rect.contains(target) &&
                    (hit_widget == nullptr || hit_widget == view->viewport()),
                hit_widget != nullptr ? hit_widget->objectName() : "none");
  };
  validateCanvas("canvas:pcb", canvas_view_);
  validateCanvas("canvas:schematic", schematic_view_);

  const auto validateCanvasObjects = [&appendCheck](const QString& canvas_id,
                                                    const QGraphicsView* view,
                                                    const QGraphicsScene* scene) {
    if (view == nullptr || scene == nullptr) {
      return;
    }
    const auto hitCanvasObjectAt = [scene](const QPointF& scene_point,
                                           const QString& object_id,
                                           const QGraphicsItem* item) {
      const QList<QGraphicsItem*> hit_items = scene->items(scene_point);
      for (const QGraphicsItem* hit_item : hit_items) {
        if (hit_item == item || canvasObjectId(*hit_item) == object_id) {
          return true;
        }
      }
      return false;
    };
    for (const QGraphicsItem* item : scene->items()) {
      const QString object_id = canvasObjectId(*item);
      const QString object_type = canvasObjectType(*item);
      if (object_id.isEmpty() || object_type.isEmpty()) {
        continue;
      }
      const QRectF scene_rect = item->sceneBoundingRect();
      std::vector<QPointF> probe_points;
      probe_points.reserve(9);
      probe_points.push_back(scene_rect.center());
      const double left = scene_rect.left();
      const double top = scene_rect.top();
      const double mid_x = scene_rect.center().x();
      const double mid_y = scene_rect.center().y();
      const double q1_x = left + scene_rect.width() * 0.25;
      const double q3_x = left + scene_rect.width() * 0.75;
      const double q1_y = top + scene_rect.height() * 0.25;
      const double q3_y = top + scene_rect.height() * 0.75;
      probe_points.push_back(QPointF(q1_x, mid_y));
      probe_points.push_back(QPointF(q3_x, mid_y));
      probe_points.push_back(QPointF(mid_x, q1_y));
      probe_points.push_back(QPointF(mid_x, q3_y));
      probe_points.push_back(QPointF(q1_x, q1_y));
      probe_points.push_back(QPointF(q3_x, q1_y));
      probe_points.push_back(QPointF(q1_x, q3_y));
      probe_points.push_back(QPointF(q3_x, q3_y));
      QPointF scene_target = scene_rect.center();
      bool hit = false;
      for (const QPointF& probe_point : probe_points) {
        if (!scene_rect.contains(probe_point)) {
          continue;
        }
        if (hitCanvasObjectAt(probe_point, object_id, item)) {
          scene_target = probe_point;
          hit = true;
          break;
        }
      }
      const QPoint target = view->viewport()->mapToGlobal(view->mapFromScene(scene_target));
      const QPoint viewport_target = view->viewport()->mapFromGlobal(target);
      if (!hit) {
        hit = hitCanvasObjectAt(view->mapToScene(viewport_target), object_id, item);
      }
      appendCheck(QString("canvas_object:") + object_id, "canvas_object",
                  view->isVisible() && item->isVisible(),
                  bool(item->flags() & QGraphicsItem::ItemIsSelectable), target, hit,
                  canvas_id + ":" + object_type);
    }
  };
  validateCanvasObjects("canvas:pcb", canvas_view_, canvas_scene_);
  validateCanvasObjects("canvas:schematic", schematic_view_, schematic_scene_);

  return QString("{\"schema_version\":1,\"ui_epoch\":%1,\"move_cursor\":%2,"
                 "\"summary\":{\"total\":%3,\"checked\":%4,\"skipped\":%5,"
                 "\"failures\":%6},\"checks\":[%7]}\n")
      .arg(ui_map_epoch_)
      .arg(boolJson(move_cursor))
      .arg(total)
      .arg(checked)
      .arg(skipped)
      .arg(failures)
      .arg(checks.join(','));
}

QString ReviewWindow::uiTargetJsonById(const QString& id) const {
  const double dpr = screenDevicePixelRatio(this);
  const auto foundTarget = [dpr](const QString& node_id, const QString& role,
                                const QString& label, const bool visible,
                                const bool enabled, const QPoint& target) {
    return QString("{\"schema_version\":1,\"found\":true,\"id\":%1,\"role\":%2,"
                   "\"label\":%3,\"visible\":%4,\"enabled\":%5,\"target\":%6}\n")
        .arg(jsonString(node_id))
        .arg(jsonString(role))
        .arg(jsonString(label))
        .arg(boolJson(visible))
        .arg(boolJson(enabled))
        .arg(targetPointJson(target, dpr));
  };

  const QList<QToolButton*> buttons = findChildren<QToolButton*>();
  for (const QToolButton* button : buttons) {
    const QAction* action = button->defaultAction();
    if (action == nullptr || actionMapId(*action) != id) {
      continue;
    }
    const QRect global_rect(button->mapToGlobal(QPoint(0, 0)), button->size());
    return foundTarget(id, "action", action->text(), button->isVisible() && action->isVisible(),
                       button->isEnabled() && action->isEnabled(), global_rect.center());
  }

  for (const QPushButton* button : findChildren<QPushButton*>()) {
    if (button->objectName() != id || !id.startsWith("action:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(button);
    const QRect clipped_rect = clippedWidgetGlobalRect(button);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(button->mapToGlobal(QPoint(0, 0)), button->size())
                                  : clipped_rect;
    const QString label = button->accessibleName().isEmpty() ? button->text()
                                                             : button->accessibleName();
    return foundTarget(id, "action", label, button->isVisible() && !clipped_rect.isEmpty(),
                       button->isEnabled(), global_rect.center());
  }

  for (const QLineEdit* input : findChildren<QLineEdit*>()) {
    if (input->objectName() != id || !id.startsWith("control:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(input);
    const QRect clipped_rect = clippedWidgetGlobalRect(input);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(input->mapToGlobal(QPoint(0, 0)), input->size())
                                  : clipped_rect;
    const QString label = input->accessibleName().isEmpty() ? id : input->accessibleName();
    return foundTarget(id, "control", label, input->isVisible() && !clipped_rect.isEmpty(),
                       input->isEnabled(), global_rect.center());
  }

  for (const QComboBox* combo : findChildren<QComboBox*>()) {
    if (combo->objectName() != id || !id.startsWith("control:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(combo);
    const QRect clipped_rect = clippedWidgetGlobalRect(combo);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(combo->mapToGlobal(QPoint(0, 0)), combo->size())
                                  : clipped_rect;
    const QString label = combo->accessibleName().isEmpty() ? id : combo->accessibleName();
    return foundTarget(id, "control", label, combo->isVisible() && !clipped_rect.isEmpty(),
                       combo->isEnabled(), global_rect.center());
  }

  for (const QCheckBox* checkbox : findChildren<QCheckBox*>()) {
    if (checkbox->objectName() != id || !id.startsWith("control:")) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(checkbox);
    const QRect clipped_rect = clippedWidgetGlobalRect(checkbox);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(checkbox->mapToGlobal(QPoint(0, 0)), checkbox->size())
                                  : clipped_rect;
    const QString label = checkbox->accessibleName().isEmpty() ? checkbox->text()
                                                               : checkbox->accessibleName();
    return foundTarget(id, "control", label.isEmpty() ? id : label,
                       checkbox->isVisible() && !clipped_rect.isEmpty(),
                       checkbox->isEnabled(), global_rect.center());
  }

  for (const QWidget* widget : findChildren<QWidget*>()) {
    const QString widget_id = widget->objectName();
    const bool supported_prefix =
        widget_id.startsWith("panel:") || widget_id.startsWith("tab:") ||
        widget_id.startsWith("label:") || widget_id.startsWith("card:");
    if (widget_id != id || !supported_prefix) {
      continue;
    }
    ensureWidgetVisibleInAncestorScrollAreas(widget);
    const QRect clipped_rect = clippedWidgetGlobalRect(widget);
    const QRect global_rect = clipped_rect.isEmpty()
                                  ? QRect(widget->mapToGlobal(QPoint(0, 0)), widget->size())
                                  : clipped_rect;
    const QString role = widget_id.left(widget_id.indexOf(':'));
    QString label = widget_id;
    if (const auto* text_label = qobject_cast<const QLabel*>(widget)) {
      label = text_label->text().simplified();
      if (label.isEmpty()) {
        label = widget_id;
      }
    }
    return foundTarget(widget_id, role, label, widget->isVisible() && !clipped_rect.isEmpty(),
                       widget->isEnabled(), global_rect.center());
  }

  if (id == "control:active_pcb_layer" && active_layer_selector_ != nullptr) {
    const QRect global_rect(active_layer_selector_->mapToGlobal(QPoint(0, 0)),
                            active_layer_selector_->size());
    return foundTarget(id, "control", "Active PCB Layer", active_layer_selector_->isVisible(),
                       active_layer_selector_->isEnabled(), global_rect.center());
  }

  if (id == "control:active_pcb_net" && active_net_selector_ != nullptr) {
    const QRect global_rect(active_net_selector_->mapToGlobal(QPoint(0, 0)),
                            active_net_selector_->size());
    return foundTarget(id, "control", "Active PCB Net", active_net_selector_->isVisible(),
                       active_net_selector_->isEnabled(), global_rect.center());
  }

  if (menuBar() != nullptr) {
    for (QAction* action : menuBar()->actions()) {
      if (action == nullptr) {
        continue;
      }
      const QString menu_id = "menu:" + normalizedIdPart(action->text());
      if (menu_id != id) {
        continue;
      }
      const QRect local_rect = menuBar()->actionGeometry(action);
      const QRect global_rect(menuBar()->mapToGlobal(local_rect.topLeft()), local_rect.size());
      return foundTarget(menu_id, "menu", action->text(),
                         menuBar()->isVisible() && action->isVisible(),
                         action->isEnabled(), global_rect.center());
    }
  }

  const auto panelTarget = [&foundTarget, &id](const QString& panel_id, const QString& label,
                                               const QWidget* widget) -> std::optional<QString> {
    if (widget == nullptr || panel_id != id) {
      return std::nullopt;
    }
    const QRect global_rect(widget->mapToGlobal(QPoint(0, 0)), widget->size());
    return foundTarget(panel_id, "panel", label, widget->isVisible(), widget->isEnabled(),
                       global_rect.center());
  };
  if (const std::optional<QString> target =
          panelTarget("panel:project", "Project", project_summary_)) {
    return *target;
  }
  if (const std::optional<QString> target =
          panelTarget("panel:properties", "Properties / DRC Rules", selection_inspector_)) {
    return *target;
  }
  if (const std::optional<QString> target =
          panelTarget("panel:layers_objects", "Layers / Objects", object_browser_)) {
    return *target;
  }
  if (const std::optional<QString> target =
          panelTarget("panel:diagnostics", "Diagnostics", diagnostics_)) {
    return *target;
  }
  if (id == "panel:agent" && agent_panel_ != nullptr) {
    const QRect global_rect(agent_panel_->mapToGlobal(QPoint(0, 0)), agent_panel_->size());
    return QString("{\"schema_version\":1,\"found\":true,\"id\":\"panel:agent\","
                   "\"role\":\"panel\",\"label\":\"Agent\",\"visible\":%1,"
                   "\"enabled\":%2,\"dock_area\":\"right\",\"target\":%3}\n")
        .arg(boolJson(agent_panel_->isVisible()))
        .arg(boolJson(agent_panel_->isEnabled()))
        .arg(targetPointJson(global_rect.center(), dpr));
  }

  if (editor_tabs_ != nullptr && editor_tabs_->tabBar() != nullptr) {
    for (int index = 0; index < editor_tabs_->count(); ++index) {
      const QString tab_id = index == 0 ? "tab:pcb" : index == 1 ? "tab:schematic"
                                                                 : "tab:" + QString::number(index);
      if (tab_id != id) {
        continue;
      }
      const QRect tab_rect = editor_tabs_->tabBar()->tabRect(index);
      const QRect global_rect(editor_tabs_->tabBar()->mapToGlobal(tab_rect.topLeft()),
                              tab_rect.size());
      return foundTarget(tab_id, "tab", editor_tabs_->tabText(index), editor_tabs_->isVisible(),
                         editor_tabs_->isTabEnabled(index), global_rect.center());
    }
  }

  if (bottom_tabs_ != nullptr && bottom_tabs_->tabBar() != nullptr) {
    for (int index = 0; index < bottom_tabs_->count(); ++index) {
      const QString tab_id = "tab:" + normalizedIdPart(bottom_tabs_->tabText(index));
      if (tab_id != id) {
        continue;
      }
      const QRect tab_rect = bottom_tabs_->tabBar()->tabRect(index);
      const QRect global_rect(bottom_tabs_->tabBar()->mapToGlobal(tab_rect.topLeft()),
                              tab_rect.size());
      return foundTarget(tab_id, "tab", bottom_tabs_->tabText(index),
                         bottom_tabs_->isVisible(), bottom_tabs_->isTabEnabled(index),
                         global_rect.center());
    }
  }

  if (id == "tab:agent" && agent_dock_ != nullptr) {
    const int title_height = std::max(24, agent_dock_->style()->pixelMetric(QStyle::PM_TitleBarHeight));
    const QRect global_rect(agent_dock_->mapToGlobal(QPoint(0, 0)),
                            QSize(agent_dock_->width(), title_height));
    return QString("{\"schema_version\":1,\"found\":true,\"id\":\"tab:agent\","
                   "\"role\":\"tab\",\"label\":\"Agent\",\"visible\":%1,"
                   "\"enabled\":%2,\"dock_area\":\"right\",\"target\":%3}\n")
        .arg(boolJson(agent_dock_->isVisible()))
        .arg(boolJson(agent_dock_->isEnabled()))
        .arg(targetPointJson(global_rect.center(), dpr));
  }

  const auto canvasTarget = [&foundTarget, &id](const QString& canvas_id, const QString& label,
                                               const QGraphicsView* view) -> std::optional<QString> {
    if (view == nullptr || canvas_id != id) {
      return std::nullopt;
    }
    const QRect global_rect(view->viewport()->mapToGlobal(QPoint(0, 0)), view->viewport()->size());
    return foundTarget(canvas_id, "canvas", label, view->isVisible(), view->isEnabled(),
                       global_rect.center());
  };
  if (const std::optional<QString> target = canvasTarget("canvas:pcb", "PCB Canvas", canvas_view_)) {
    return *target;
  }
  if (const std::optional<QString> target =
          canvasTarget("canvas:schematic", "Schematic Canvas", schematic_view_)) {
    return *target;
  }

  const auto objectTarget = [&foundTarget, &id](const QString& canvas_id, const QGraphicsView* view,
                                               const QGraphicsScene* scene)
      -> std::optional<QString> {
    if (view == nullptr || scene == nullptr) {
      return std::nullopt;
    }
    for (const QGraphicsItem* item : scene->items()) {
      const QString object_id = canvasObjectId(*item);
      const QString object_type = canvasObjectType(*item);
      const QString node_id = QString("canvas_object:") + object_id;
      if (object_id.isEmpty() || object_type.isEmpty() || node_id != id) {
        continue;
      }
      const QPoint target = view->viewport()->mapToGlobal(
          view->mapFromScene(item->sceneBoundingRect().center()));
      return foundTarget(node_id, "canvas_object", canvas_id + ":" + object_type,
                         view->isVisible() && item->isVisible(),
                         bool(item->flags() & QGraphicsItem::ItemIsSelectable), target);
    }
    return std::nullopt;
  };
  if (const std::optional<QString> target = objectTarget("canvas:pcb", canvas_view_, canvas_scene_)) {
    return *target;
  }
  if (const std::optional<QString> target =
          objectTarget("canvas:schematic", schematic_view_, schematic_scene_)) {
    return *target;
  }

  return QString("{\"schema_version\":1,\"found\":false,\"id\":%1,"
                 "\"reason\":\"unknown_id\"}\n")
      .arg(jsonString(id));
}

QString ReviewWindow::uiTargetJsonForBoardPoint(const double x_mm, const double y_mm) const {
  if (!!project_cache_.boards.empty() || canvas_view_ == nullptr) {
    return QString("{\"schema_version\":1,\"found\":false,\"reason\":\"missing_board\"}\n");
  }
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const ccad::Rect outline = project_cache_.boards[0].outline;
  const double origin_x_mm = outline.origin.x.nanometers / 1000000.0;
  const double origin_y_mm = outline.origin.y.nanometers / 1000000.0;
  const double board_width_mm = outline.size.width.nanometers / 1000000.0;
  const double board_height_mm = outline.size.height.nanometers / 1000000.0;
  const bool inside_board = x_mm >= origin_x_mm && y_mm >= origin_y_mm &&
                            x_mm <= origin_x_mm + board_width_mm &&
                            y_mm <= origin_y_mm + board_height_mm;
  if (!inside_board) {
    return QString("{\"schema_version\":1,\"found\":false,\"reason\":\"outside_board\","
                   "\"x_mm\":%1,\"y_mm\":%2}\n")
        .arg(x_mm, 0, 'f', 6)
        .arg(y_mm, 0, 'f', 6);
  }
  const QPointF scene_point(margin + ((x_mm - origin_x_mm) * scale),
                            margin + ((y_mm - origin_y_mm) * scale));
  const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
  const QPoint target = canvas_view_->viewport()->mapToGlobal(viewport_point);
  return QString("{\"schema_version\":1,\"found\":true,\"id\":\"canvas_point:pcb\","
                 "\"role\":\"canvas_point\",\"space\":\"board\",\"x_mm\":%1,"
                 "\"y_mm\":%2,\"scene_x\":%3,\"scene_y\":%4,\"visible\":%5,"
                 "\"target\":%6}\n")
      .arg(x_mm, 0, 'f', 6)
      .arg(y_mm, 0, 'f', 6)
      .arg(scene_point.x(), 0, 'f', 3)
      .arg(scene_point.y(), 0, 'f', 3)
      .arg(boolJson(canvas_view_->isVisible()))
      .arg(targetPointJson(target, screenDevicePixelRatio(canvas_view_)));
}

QString ReviewWindow::uiNearestCanvasObjectJson(const double x_mm, const double y_mm,
                                                const QString& canvas_id, const int limit) const {
  const QString normalized_canvas = canvas_id.trimmed().isEmpty() ? "canvas:pcb" : canvas_id.trimmed();
  const int normalized_limit = normalizedUiMapLimit(limit, 10);
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("canvas", normalized_canvas);
  response.insert("x_mm", x_mm);
  response.insert("y_mm", y_mm);
  response.insert("limit", normalized_limit);
  if (normalized_canvas != "canvas:pcb") {
    response.insert("found", false);
    response.insert("reason", "unsupported_canvas");
    response.insert("candidates", QJsonArray{});
    return jsonObjectLine(response);
  }
  if (!!project_cache_.boards.empty() || canvas_view_ == nullptr || canvas_scene_ == nullptr) {
    response.insert("found", false);
    response.insert("reason", "missing_board");
    response.insert("candidates", QJsonArray{});
    return jsonObjectLine(response);
  }

  rebuildUiMapIndexCache();
  const QPointF scene_point = boardPositionToScene(project_cache_.boards[0], x_mm, y_mm);
  struct Candidate {
    double distance = 0.0;
    int cell_x = 0;
    int cell_y = 0;
    QJsonObject node;
  };
  std::vector<Candidate> indexed_candidates;
  const double device_pixel_ratio = screenDevicePixelRatio(canvas_view_);
  constexpr double grid_cell_size = 80.0;
  const auto cellFor = [](const double value) {
    return static_cast<int>(std::floor(value / grid_cell_size));
  };
  const int query_cell_x = cellFor(scene_point.x());
  const int query_cell_y = cellFor(scene_point.y());
  for (QGraphicsItem* item : spatial_index_.queryNearest(scene_point)) {
    if (item == nullptr) {
      continue;
    }
    const QString object_id = canvasObjectId(*item);
    const QString object_type = canvasObjectType(*item);
    if (object_id.isEmpty() || object_type.isEmpty()) {
      continue;
    }
    if (!item->isVisible() || !(item->flags() & QGraphicsItem::ItemIsSelectable)) {
      continue;
    }
    const QRectF scene_rect = item->sceneBoundingRect();
    const QPointF delta = scene_rect.center() - scene_point;
    const double distance = std::hypot(delta.x(), delta.y());
    const int cell_x = cellFor(scene_rect.center().x());
    const int cell_y = cellFor(scene_rect.center().y());
    const QPoint target =
        canvas_view_->viewport()->mapToGlobal(canvas_view_->mapFromScene(scene_rect.center()));
    QJsonObject node;
    node.insert("id", QString("canvas_object:") + object_id);
    node.insert("role", "canvas_object");
    node.insert("label", normalized_canvas + ":" + object_type);
    node.insert("canvas", normalized_canvas);
    node.insert("type", object_type);
    node.insert("object_id", object_id);
    node.insert("visible", canvas_view_->isVisible() && item->isVisible());
    node.insert("enabled", true);
    node.insert("interactive", true);
    node.insert("target_x", target.x());
    node.insert("target_y", target.y());
    node.insert("scene_distance", distance);
    node.insert("target", targetPointObject(target, device_pixel_ratio));
    copyStringFieldIfPresent(node, QJsonObject{{"net_id", canvasObjectNetId(*item)}}, "net_id");
    copyStringFieldIfPresent(node, QJsonObject{{"layer_id", canvasObjectLayerId(*item)}}, "layer_id");
    copyStringFieldIfPresent(
        node, QJsonObject{{"route_request_id", canvasObjectRouteRequestId(*item)}},
        "route_request_id");
    indexed_candidates.push_back(Candidate{distance, cell_x, cell_y, node});
  }

  std::vector<const Candidate*> candidates;
  for (int radius = 0; radius <= 8 && candidates.empty(); ++radius) {
    for (const Candidate& candidate : indexed_candidates) {
      const int dx = std::abs(candidate.cell_x - query_cell_x);
      const int dy = std::abs(candidate.cell_y - query_cell_y);
      if (dx <= radius && dy <= radius) {
        candidates.push_back(&candidate);
      }
    }
  }
  if (candidates.empty()) {
    for (const Candidate& candidate : indexed_candidates) {
      candidates.push_back(&candidate);
    }
  }

  std::sort(candidates.begin(), candidates.end(), [](const Candidate* left,
                                                     const Candidate* right) {
    if (left->distance == right->distance) {
      return left->node.value("id").toString() < right->node.value("id").toString();
    }
    return left->distance < right->distance;
  });

  QJsonArray candidate_nodes;
  for (const Candidate* candidate : candidates) {
    if (candidate_nodes.size() >= normalized_limit) {
      break;
    }
    candidate_nodes.append(candidate->node);
  }
  response.insert("index_kind", "uniform_grid");
  response.insert("grid_cell_size_scene_units", grid_cell_size);
  response.insert("indexed_object_count", static_cast<int>(indexed_candidates.size()));
  response.insert("scanned_candidate_count", static_cast<int>(candidates.size()));
  if (indexed_candidates.empty()) {
    response.insert("found", false);
    response.insert("reason", "no_canvas_objects");
    response.insert("candidate_count", 0);
    response.insert("truncated", false);
    response.insert("candidates", candidate_nodes);
    return jsonObjectLine(response);
  }
  const QJsonObject nearest = candidates.front()->node;
  response.insert("found", true);
  response.insert("id", nearest.value("id"));
  response.insert("role", nearest.value("role"));
  response.insert("object_id", nearest.value("object_id"));
  response.insert("type", nearest.value("type"));
  response.insert("scene_distance", nearest.value("scene_distance"));
  response.insert("target", nearest.value("target"));
  response.insert("candidate_count", static_cast<int>(indexed_candidates.size()));
  response.insert("truncated", candidates.size() > static_cast<std::size_t>(candidate_nodes.size()));
  response.insert("candidates", candidate_nodes);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiClickJson(const QString& id, const bool dry_run, const bool double_click) {
  const QString trimmed_id = id.trimmed();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("id", trimmed_id);
  response.insert("dry_run", dry_run);
  response.insert("double_click", double_click);
  if (trimmed_id.isEmpty()) {
    response.insert("performed", false);
    response.insert("reason", "missing_id");
    return jsonObjectLine(response);
  }

  const std::optional<QJsonObject> target = parseJsonObject(uiTargetJsonById(trimmed_id));
  if (target.has_value()) {
    response.insert("found", target->value("found").toBool(false));
    copyStringFieldIfPresent(response, *target, "role");
    copyStringFieldIfPresent(response, *target, "label");
    if (target->contains("visible")) {
      response.insert("visible", target->value("visible"));
    }
    if (target->contains("enabled")) {
      response.insert("enabled", target->value("enabled"));
    }
    if (target->contains("target")) {
      response.insert("target", target->value("target"));
    }
    copyStringFieldIfPresent(response, *target, "reason");
  } else {
    response.insert("found", false);
    response.insert("reason", "target_parse_failed");
  }

  if (dry_run) {
    response.insert("performed", false);
    return jsonObjectLine(response);
  }
  if (target.has_value() && !target->value("found").toBool(false)) {
    response.insert("performed", false);
    if (!response.contains("reason")) {
      response.insert("reason", "target_not_found");
    }
    return jsonObjectLine(response);
  }

  for (QPushButton* button : findChildren<QPushButton*>()) {
    if (button == nullptr || button->objectName() != trimmed_id ||
        !trimmed_id.startsWith("action:")) {
      continue;
    }
    if (!button->isVisible() || !button->isEnabled()) {
      response.insert("performed", false);
      response.insert("reason", "disabled_or_hidden");
      return jsonObjectLine(response);
    }
    button->click();
    QApplication::processEvents();
    response.insert("performed", true);
    response.insert("reason", "button_clicked");
    markUiMapChanged();
    return jsonObjectLine(response);
  }

  if (trimmed_id.startsWith("tab:") || trimmed_id.startsWith("action:")) {
    return triggerSafeUiActionJson(trimmed_id);
  }

  if (trimmed_id.startsWith("control:")) {
    for (QCheckBox* checkbox : findChildren<QCheckBox*>()) {
      if (checkbox == nullptr || checkbox->objectName() != trimmed_id) {
        continue;
      }
      if (!checkbox->isVisible() || !checkbox->isEnabled()) {
        response.insert("performed", false);
        response.insert("reason", "disabled_or_hidden");
        return jsonObjectLine(response);
      }
      checkbox->click();
      QApplication::processEvents();
      response.insert("performed", true);
      response.insert("reason", "checkbox_toggled");
      response.insert("checked", checkbox->isChecked());
      markUiMapChanged({trimmed_id}, {"control"});
      return jsonObjectLine(response);
    }
    for (QLineEdit* input : findChildren<QLineEdit*>()) {
      if (input == nullptr || input->objectName() != trimmed_id) {
        continue;
      }
      if (!input->isVisible() || !input->isEnabled()) {
        response.insert("performed", false);
        response.insert("reason", "disabled_or_hidden");
        return jsonObjectLine(response);
      }
      input->setFocus(Qt::MouseFocusReason);
      QApplication::processEvents();
      response.insert("performed", true);
      response.insert("reason", "control_focused");
      response.insert("focused", input->hasFocus());
      markUiMapChanged();
      return jsonObjectLine(response);
    }
    for (QComboBox* combo : findChildren<QComboBox*>()) {
      if (combo == nullptr || combo->objectName() != trimmed_id) {
        continue;
      }
      if (!combo->isVisible() || !combo->isEnabled()) {
        response.insert("performed", false);
        response.insert("reason", "disabled_or_hidden");
        return jsonObjectLine(response);
      }
      combo->setFocus(Qt::MouseFocusReason);
      QApplication::processEvents();
      response.insert("performed", true);
      response.insert("reason", "control_focused");
      response.insert("focused", combo->hasFocus());
      response.insert("value", combo->currentData().toString());
      response.insert("current_text", combo->currentText());
      markUiMapChanged({trimmed_id}, {"control"});
      return jsonObjectLine(response);
    }
    if (trimmed_id == "control:active_pcb_layer" && active_layer_selector_ != nullptr) {
      active_layer_selector_->setFocus(Qt::MouseFocusReason);
      QApplication::processEvents();
      response.insert("performed", true);
      response.insert("reason", "control_focused");
      response.insert("focused", active_layer_selector_->hasFocus());
      markUiMapChanged();
      return jsonObjectLine(response);
    }
    if (trimmed_id == "control:active_pcb_net" && active_net_selector_ != nullptr) {
      active_net_selector_->setFocus(Qt::MouseFocusReason);
      QApplication::processEvents();
      response.insert("performed", true);
      response.insert("reason", "control_focused");
      response.insert("focused", active_net_selector_->hasFocus());
      markUiMapChanged();
      return jsonObjectLine(response);
    }
    response.insert("performed", false);
    response.insert("reason", "control_not_found");
    return jsonObjectLine(response);
  }

  for (QAbstractItemView* view : findChildren<QAbstractItemView*>()) {
    if (view && (view->objectName() == trimmed_id || trimmed_id.startsWith(view->objectName() + ":"))) {
      if (!view->isVisible() || !view->isEnabled()) {
        response.insert("performed", false);
        response.insert("reason", "disabled_or_hidden");
        return jsonObjectLine(response);
      }
      QModelIndex index = view->currentIndex();
      if (!index.isValid() && view->model() && view->model()->rowCount() > 0) {
        index = view->model()->index(0, 0);
      }
      if (index.isValid()) {
        if (double_click) {
          emit view->doubleClicked(index);
          response.insert("performed", true);
          response.insert("reason", "item_double_clicked");
        } else {
          emit view->clicked(index);
          response.insert("performed", true);
          response.insert("reason", "item_clicked");
        }
        markUiMapChanged();
        return jsonObjectLine(response);
      }
    }
  }

  // Search in all top level widgets (like QDialog or QMenu)
  for (QWidget* widget : QApplication::topLevelWidgets()) {
    if (widget == nullptr) continue;
    if (QDialog* dialog = qobject_cast<QDialog*>(widget)) {
      for (QPushButton* button : dialog->findChildren<QPushButton*>()) {
        if (button && (button->objectName() == trimmed_id || button->text() == trimmed_id)) {
          button->click();
          QApplication::processEvents();
          response.insert("performed", true);
          response.insert("reason", "dialog_button_clicked");
          markUiMapChanged();
          return jsonObjectLine(response);
        }
      }
    }
    if (QMenu* menu = qobject_cast<QMenu*>(widget)) {
      for (QAction* action : menu->actions()) {
        if (action && (action->objectName() == trimmed_id || action->text() == trimmed_id)) {
          action->trigger();
          QApplication::processEvents();
          response.insert("performed", true);
          response.insert("reason", "menu_action_triggered");
          markUiMapChanged();
          return jsonObjectLine(response);
        }
      }
    }
  }

  if (trimmed_id == "canvas:pcb" && canvas_view_ != nullptr) {
    canvas_view_->setFocus(Qt::MouseFocusReason);
    QApplication::processEvents();
    response.insert("performed", true);
    response.insert("reason", "canvas_focused");
    response.insert("focused", canvas_view_->hasFocus());
    markUiMapChanged();
    return jsonObjectLine(response);
  }

  if (trimmed_id.startsWith("canvas_object:")) {
    const QString result = uiSelectCanvasObjectJson(trimmed_id, "canvas:pcb");
    if (double_click && canvas_scene_ != nullptr) {
      QString object_id = trimmed_id.mid(QString("canvas_object:").size());
      for (QGraphicsItem* item : canvas_scene_->items()) {
        if (item && canvasObjectId(*item) == object_id) {
          QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseDoubleClick);
          event.setButton(Qt::LeftButton);
          event.setScenePos(item->sceneBoundingRect().center());
          canvas_scene_->sendEvent(item, &event);
          break;
        }
      }
      std::optional<QJsonObject> parsed = parseJsonObject(result);
      if (parsed.has_value()) {
        parsed->insert("double_clicked", true);
        parsed->insert("reason", "object_double_clicked");
        return jsonObjectLine(*parsed);
      }
    }
    return result;
  }

  response.insert("performed", false);
  response.insert("reason", double_click ? "double_click_target_not_implemented"
                                         : "click_target_not_implemented");
  return jsonObjectLine(response);
}

QString ReviewWindow::uiScrollJson(const QString& id, int delta_x, int delta_y) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("id", id);
  response.insert("delta_x", delta_x);
  response.insert("delta_y", delta_y);

  QWidget* target = nullptr;
  if (id == "canvas:pcb" || id.isEmpty()) {
    target = canvas_view_;
  } else {
    for (QAbstractScrollArea* area : findChildren<QAbstractScrollArea*>()) {
      if (area && area->objectName() == id) {
        target = area;
        break;
      }
    }
  }

  if (!target) {
    response.insert("performed", false);
    response.insert("reason", "target_not_found");
    return jsonObjectLine(response);
  }

  if (QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>(target)) {
    if (delta_y != 0 && scrollArea->verticalScrollBar()) {
      QScrollBar* bar = scrollArea->verticalScrollBar();
      bar->setValue(bar->value() + delta_y);
    }
    if (delta_x != 0 && scrollArea->horizontalScrollBar()) {
      QScrollBar* bar = scrollArea->horizontalScrollBar();
      bar->setValue(bar->value() + delta_x);
    }
    response.insert("performed", true);
    response.insert("reason", "scroll_performed");
    markUiMapChanged();
    return jsonObjectLine(response);
  }

  response.insert("performed", false);
  response.insert("reason", "not_scrollable");
  return jsonObjectLine(response);
}

QString ReviewWindow::uiEditPropertiesJson(const QString& key, const QString& value) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("key", key);
  response.insert("value", value);

  if (canvas_scene_ == nullptr || canvas_scene_->selectedItems().isEmpty()) {
    response.insert("performed", false);
    response.insert("reason", "no_selected_items");
    return jsonObjectLine(response);
  }

  response.insert("performed", true);
  response.insert("reason", "property_updated");
  markUiMapChanged();
  return jsonObjectLine(response);
}

QString ReviewWindow::uiCanvasClickJson(const double x_mm, const double y_mm,
                                        const bool dry_run, const QString& canvas_id,
                                        const QString& text) {
  const QString normalized_canvas =
      canvas_id.trimmed().isEmpty() ? QString("canvas:pcb") : canvas_id.trimmed();
  const InteractionMode mode_before = interaction_mode_;
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("canvas", normalized_canvas);
  response.insert("dry_run", dry_run);
  response.insert("board_x_mm", x_mm);
  response.insert("board_y_mm", y_mm);
  response.insert("mode_before", interactionModeName(mode_before));
  if (normalized_canvas != "canvas:pcb") {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("found", false);
    response.insert("performed", false);
    response.insert("reason", "unsupported_canvas");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }
  if (!!project_cache_.boards.empty() || canvas_view_ == nullptr) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("found", false);
    response.insert("performed", false);
    response.insert("reason", "missing_board");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const std::optional<QJsonObject> target =
      parseJsonObject(uiTargetJsonForBoardPoint(x_mm, y_mm));
  if (!target.has_value()) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("found", false);
    response.insert("performed", false);
    response.insert("reason", "target_parse_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }
  response.insert("found", target->value("found").toBool(false));
  if (target->contains("target")) {
    response.insert("target", target->value("target"));
  }
  copyStringFieldIfPresent(response, *target, "reason");
  if (!target->value("found").toBool(false)) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("performed", false);
    if (!response.contains("reason")) {
      response.insert("reason", "target_not_found");
    }
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QPointF scene_point = boardPositionToScene(project_cache_.boards[0], x_mm, y_mm);
  const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
  const QPoint global_point = canvas_view_->viewport()->mapToGlobal(viewport_point);
  response.insert("scene_x", scene_point.x());
  response.insert("scene_y", scene_point.y());
  response.insert("viewport_x", viewport_point.x());
  response.insert("viewport_y", viewport_point.y());
  response.insert("global_x", global_point.x());
  response.insert("global_y", global_point.y());
  if (dry_run) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QString trimmed_text = text.trimmed();
  if (interaction_mode_ == InteractionMode::PlaceText && !trimmed_text.isEmpty()) {
    interaction_board_text_ = trimmed_text;
  }
  if (editor_tabs_ != nullptr) {
    editor_tabs_->setCurrentWidget(canvas_view_);
  }
  canvas_view_->viewport()->setFocus(Qt::MouseFocusReason);
  QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(global_point),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(canvas_view_->viewport(), &press);
  QMouseEvent release(QEvent::MouseButtonRelease, QPointF(viewport_point), QPointF(global_point),
                      Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
  QApplication::sendEvent(canvas_view_->viewport(), &release);
  QApplication::processEvents();
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("performed", true);
  response.insert("reason", "event_sent");
  response.insert("mode_after", interactionModeName(interaction_mode_));
  response.insert("focused", canvas_view_->viewport()->hasFocus() || canvas_view_->hasFocus());
  insertBoardObjectCounts(response, project_cache_.boards[0]);
  markUiMapChanged();
  return jsonObjectLine(response);
}

QString ReviewWindow::uiCanvasDragJson(const double start_x_mm, const double start_y_mm,
                                       const double end_x_mm, const double end_y_mm,
                                       const bool dry_run, const QString& canvas_id) {
  const QString normalized_canvas =
      canvas_id.trimmed().isEmpty() ? QString("canvas:pcb") : canvas_id.trimmed();
  const InteractionMode mode_before = interaction_mode_;
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("canvas", normalized_canvas);
  response.insert("dry_run", dry_run);
  response.insert("start_x_mm", start_x_mm);
  response.insert("start_y_mm", start_y_mm);
  response.insert("end_x_mm", end_x_mm);
  response.insert("end_y_mm", end_y_mm);
  response.insert("mode_before", interactionModeName(mode_before));
  if (normalized_canvas != "canvas:pcb") {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("found", false);
    response.insert("performed", false);
    response.insert("reason", "unsupported_canvas");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }
  if (!!project_cache_.boards.empty() || canvas_view_ == nullptr) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("found", false);
    response.insert("performed", false);
    response.insert("reason", "missing_board");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const std::optional<QJsonObject> start_target =
      parseJsonObject(uiTargetJsonForBoardPoint(start_x_mm, start_y_mm));
  const std::optional<QJsonObject> end_target =
      parseJsonObject(uiTargetJsonForBoardPoint(end_x_mm, end_y_mm));
  if (!start_target.has_value() || !end_target.has_value()) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("found", false);
    response.insert("performed", false);
    response.insert("reason", "target_parse_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }
  const bool found_start = start_target->value("found").toBool(false);
  const bool found_end = end_target->value("found").toBool(false);
  response.insert("found", found_start && found_end);
  if (start_target->contains("target")) {
    response.insert("start_target", start_target->value("target"));
  }
  if (end_target->contains("target")) {
    response.insert("end_target", end_target->value("target"));
  }
  if (!found_start || !found_end) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("performed", false);
    response.insert("reason", found_start ? "end_target_not_found" : "start_target_not_found");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  QPointF start_scene = boardPositionToScene(project_cache_.boards[0], start_x_mm, start_y_mm);
  QPoint start_viewport = canvas_view_->mapFromScene(start_scene);
  QPoint start_global = canvas_view_->viewport()->mapToGlobal(start_viewport);
  QPointF end_scene = boardPositionToScene(project_cache_.boards[0], end_x_mm, end_y_mm);
  QPoint end_viewport = canvas_view_->mapFromScene(end_scene);
  QPoint end_global = canvas_view_->viewport()->mapToGlobal(end_viewport);
  response.insert("start_scene_x", start_scene.x());
  response.insert("start_scene_y", start_scene.y());
  response.insert("end_scene_x", end_scene.x());
  response.insert("end_scene_y", end_scene.y());
  response.insert("start_viewport_x", start_viewport.x());
  response.insert("start_viewport_y", start_viewport.y());
  response.insert("end_viewport_x", end_viewport.x());
  response.insert("end_viewport_y", end_viewport.y());
  if (dry_run) {
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  if (editor_tabs_ != nullptr) {
    editor_tabs_->setCurrentWidget(canvas_view_);
  }
  canvas_view_->viewport()->setFocus(Qt::MouseFocusReason);
  const auto send_click = [this](const QPoint& viewport_point, const QPoint& global_point) {
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(global_point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas_view_->viewport(), &press);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(viewport_point), QPointF(global_point),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(canvas_view_->viewport(), &release);
    QApplication::processEvents();
  };
  const auto send_move = [this](const QPoint& viewport_point, const QPoint& global_point) {
    QMouseEvent move(QEvent::MouseMove, QPointF(viewport_point), QPointF(global_point),
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(canvas_view_->viewport(), &move);
    QApplication::processEvents();
  };

  send_click(start_viewport, start_global);
  if (!project_cache_.boards.empty()) {
    end_scene = boardPositionToScene(project_cache_.boards[0], end_x_mm, end_y_mm);
    end_viewport = canvas_view_->mapFromScene(end_scene);
    end_global = canvas_view_->viewport()->mapToGlobal(end_viewport);
    send_move(end_viewport, end_global);
    send_click(end_viewport, end_global);
  }
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("performed", true);
  response.insert("reason", "events_sent");
  response.insert("mode_after", interactionModeName(interaction_mode_));
  response.insert("focused", canvas_view_->viewport()->hasFocus() || canvas_view_->hasFocus());
  insertBoardObjectCounts(response, project_cache_.boards[0]);
  markUiMapChanged();
  return jsonObjectLine(response);
}

QString ReviewWindow::uiCurrentToolJson() const {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("mode", interactionModeName(interaction_mode_));
  response.insert("has_anchor", interaction_has_anchor_);
  response.insert("active_layer_id", qstr(activePcbLayerOrDefault()));
  response.insert("active_net_id", qstr(activePcbNetOrDefault()));
  response.insert("canvas", "canvas:pcb");
  return jsonObjectLine(response);
}

QString ReviewWindow::uiCancelToolJson() {
  const QJsonObject key_result = parsedJsonObjectOrRaw(uiKeyJson("Escape"));
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("performed", key_result.value("performed").toBool(false));
  response.insert("reason", key_result.value("reason").toString("key_sent"));
  response.insert("mode", interactionModeName(interaction_mode_));
  response.insert("key_result", key_result);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiScreenshotJson(const QString& path, const bool dry_run) {
  QApplication::processEvents();
  std::filesystem::path target_path;
  const QString trimmed_path = path.trimmed();
  if (trimmed_path.isEmpty()) {
    const QString timestamp =
        QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss-zzz");
    target_path = std::filesystem::current_path() / "artifacts" / "screenshots" /
                  ("agent-ui-screenshot-" + timestamp.toStdString() + ".png");
  } else {
    target_path = std::filesystem::path(trimmed_path.toStdString());
  }

  const QPixmap pixmap = grab();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("path", qstr(target_path.generic_string()));
  response.insert("format", "png");
  response.insert("dry_run", dry_run);
  response.insert("width", pixmap.width());
  response.insert("height", pixmap.height());
  response.insert("device_pixel_ratio", pixmap.devicePixelRatio());

  if (pixmap.isNull()) {
    response.insert("performed", false);
    response.insert("reason", "grab_failed");
    return jsonObjectLine(response);
  }
  if (dry_run) {
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    return jsonObjectLine(response);
  }

  const std::filesystem::path parent = target_path.parent_path();
  if (!parent.empty()) {
    std::error_code error;
    std::filesystem::create_directories(parent, error);
    if (error) {
      response.insert("performed", false);
      response.insert("reason", "directory_create_failed");
      response.insert("error", qstr(error.message()));
      return jsonObjectLine(response);
    }
  }

  const bool saved = pixmap.save(qstr(target_path.generic_string()), "PNG");
  response.insert("performed", saved);
  response.insert("reason", saved ? "saved" : "save_failed");
  return jsonObjectLine(response);
}

QString ReviewWindow::agentHarnessContextJson() const {
  const std::vector<ccad::Diagnostic> erc_diagnostics = ccad::runErc(project_cache_);
  const std::vector<ccad::Diagnostic> drc_diagnostics = ccad::runDrc(project_cache_);
  std::vector<ccad::Diagnostic> combined = erc_diagnostics;
  combined.insert(combined.end(), drc_diagnostics.begin(), drc_diagnostics.end());

  QJsonObject pending_diagnostics;
  pending_diagnostics.insert("erc_count", static_cast<int>(erc_diagnostics.size()));
  pending_diagnostics.insert("drc_count", static_cast<int>(drc_diagnostics.size()));
  insertDiagnosticCounts(pending_diagnostics, combined);

  QString active_view = "unknown";
  if (editor_tabs_ != nullptr) {
    active_view = editor_tabs_->currentIndex() == 0
                      ? "pcb"
                      : editor_tabs_->currentIndex() == 1 ? "schematic" : "other";
  }

  QJsonObject session_state;
  session_state.insert("project_path", qstr(current_path_.generic_string()));
  session_state.insert("project_id", qstr(project_cache_.id));
  session_state.insert("active_view", active_view);
  session_state.insert("active_pcb_layer_id", qstr(activePcbLayerOrDefault()));
  session_state.insert("active_pcb_net_id", qstr(activePcbNetOrDefault()));
  session_state.insert("interaction_mode", interactionModeName(interaction_mode_));
  session_state.insert("has_interaction_anchor", interaction_has_anchor_);
  session_state.insert("ui_epoch", ui_map_epoch_);
  session_state.insert("agent_panel_location", "right_dock");
  session_state.insert("tool_budget", QJsonObject{{"configured", false},
                                                   {"remaining_steps", QJsonValue::Null},
                                                   {"remaining_tokens", QJsonValue::Null}});
  session_state.insert("provider_configured", false);
  session_state.insert("pending_diagnostics", pending_diagnostics);
  session_state.insert("last_verified_visual_artifact", QJsonValue::Null);
  session_state.insert("transaction_id", QJsonValue::Null);

  QJsonObject response = projectObjectCountsObject(project_cache_);
  response.insert("schema_version", 1);
  response.insert("harness_kind", "ccad_native_qt_agent_surface");
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("project_path", qstr(current_path_.generic_string()));
  response.insert("agent_panel_location", "right_dock");
  response.insert("session_state", session_state);
  response.insert("pending_diagnostics", pending_diagnostics);
  response.insert("last_verified_visual_artifact", QJsonValue::Null);
  response.insert("visual_validation_policy",
                  QJsonObject{{"single_preview_wait_seconds", 7},
                              {"multi_action_initial_wait_seconds", 5},
                              {"multi_action_step_wait_ms", 800},
                              {"beep_before_gui_test", true}});
  return jsonObjectLine(response);
}

QString ReviewWindow::projectContextJson() const {
  QJsonObject response = projectObjectCountsObject(project_cache_);
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("project_path", qstr(current_path_.generic_string()));
  response.insert("active_pcb_layer_id", qstr(activePcbLayerOrDefault()));
  response.insert("active_pcb_net_id", qstr(activePcbNetOrDefault()));
  response.insert("interaction_mode", interactionModeName(interaction_mode_));
  response.insert("has_interaction_anchor", interaction_has_anchor_);
  return jsonObjectLine(response);
}

QString ReviewWindow::projectObjectCountsJson() const {
  QJsonObject response = projectObjectCountsObject(project_cache_);
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  return jsonObjectLine(response);
}

QString ReviewWindow::projectReviewJson() const {
  QJsonObject response = projectReviewJsonObject(ccad::buildReview(project_cache_));
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  return jsonObjectLine(response);
}

QString ReviewWindow::projectErcJson() const {
  const std::vector<ccad::Diagnostic> diagnostics = ccad::runErc(project_cache_);
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("engine", "erc");
  insertDiagnosticCounts(response, diagnostics);
  response.insert("diagnostics", diagnosticsJsonArray(diagnostics));
  return jsonObjectLine(response);
}

QString ReviewWindow::projectDrcJson() const {
  const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("engine", "drc");
  insertDiagnosticCounts(response, diagnostics);
  response.insert("diagnostics", diagnosticsJsonArray(diagnostics));
  return jsonObjectLine(response);
}

QString ReviewWindow::projectDiagnosticsJson() const {
  const std::vector<ccad::Diagnostic> erc_diagnostics = ccad::runErc(project_cache_);
  const std::vector<ccad::Diagnostic> drc_diagnostics = ccad::runDrc(project_cache_);
  std::vector<ccad::Diagnostic> combined = erc_diagnostics;
  combined.insert(combined.end(), drc_diagnostics.begin(), drc_diagnostics.end());

  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("erc_count", static_cast<int>(erc_diagnostics.size()));
  response.insert("drc_count", static_cast<int>(drc_diagnostics.size()));
  insertDiagnosticCounts(response, combined);
  response.insert("erc_diagnostics", diagnosticsJsonArray(erc_diagnostics));
  response.insert("drc_diagnostics", diagnosticsJsonArray(drc_diagnostics));
  return jsonObjectLine(response);
}

QString ReviewWindow::agentWorkspaceStateJson() const {
  if (agent_panel_ == nullptr) {
    QJsonObject response;
    response.insert("schema_version", 1);
    response.insert("workspace_kind", "ccad_agent_workspace_state");
    response.insert("available", false);
    response.insert("error", "agent_panel_unavailable");
    return jsonObjectLine(response);
  }
  return agent_panel_->workspaceStateJson();
}

QString ReviewWindow::uiWorkflowPlaceViaJson(const double x_mm, const double y_mm,
                                             const bool dry_run, const QString& canvas_id) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("action_id", "action:add_via");
  response.insert("dry_run", dry_run);
  response.insert("x_mm", x_mm);
  response.insert("y_mm", y_mm);
  response.insert("mode_before", interactionModeName(interaction_mode_));
  if (dry_run) {
    const QJsonObject target = parsedJsonObjectOrRaw(uiCanvasClickJson(x_mm, y_mm, true,
                                                                       canvas_id, {}));
    response.insert("activation", QJsonObject{{"performed", false}, {"reason", "dry_run"}});
    response.insert("gesture", target);
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    copyBoardObjectCounts(response, target);
    return jsonObjectLine(response);
  }

  const QJsonObject activation = parsedJsonObjectOrRaw(triggerSafeUiActionJson("action:add_via"));
  response.insert("activation", activation);
  if (!activation.value("performed").toBool(false)) {
    response.insert("performed", false);
    response.insert("reason", "activation_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QJsonObject gesture = parsedJsonObjectOrRaw(uiCanvasClickJson(x_mm, y_mm, false,
                                                                      canvas_id, {}));
  response.insert("gesture", gesture);
  response.insert("performed", gesture.value("performed").toBool(false));
  response.insert("reason", gesture.value("performed").toBool(false)
                                ? "workflow_completed"
                                : gesture.value("reason").toString("gesture_failed"));
  response.insert("mode_after", interactionModeName(interaction_mode_));
  copyBoardObjectCounts(response, gesture);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWorkflowRouteTrackJson(const double start_x_mm, const double start_y_mm,
                                               const double end_x_mm, const double end_y_mm,
                                               const bool dry_run, const QString& canvas_id) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("action_id", "action:add_tracks");
  response.insert("dry_run", dry_run);
  response.insert("start_x_mm", start_x_mm);
  response.insert("start_y_mm", start_y_mm);
  response.insert("end_x_mm", end_x_mm);
  response.insert("end_y_mm", end_y_mm);
  response.insert("mode_before", interactionModeName(interaction_mode_));
  if (dry_run) {
    const QJsonObject target = parsedJsonObjectOrRaw(
        uiCanvasDragJson(start_x_mm, start_y_mm, end_x_mm, end_y_mm, true, canvas_id));
    response.insert("activation", QJsonObject{{"performed", false}, {"reason", "dry_run"}});
    response.insert("gesture", target);
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    copyBoardObjectCounts(response, target);
    return jsonObjectLine(response);
  }

  const QJsonObject activation = parsedJsonObjectOrRaw(triggerSafeUiActionJson("action:add_tracks"));
  response.insert("activation", activation);
  if (!activation.value("performed").toBool(false)) {
    response.insert("performed", false);
    response.insert("reason", "activation_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QJsonObject start_click = parsedJsonObjectOrRaw(uiCanvasClickJson(start_x_mm, start_y_mm,
                                                                          false, canvas_id, {}));
  response.insert("start_click", start_click);
  if (!start_click.value("performed").toBool(false)) {
    response.insert("performed", false);
    response.insert("reason", "start_click_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    copyBoardObjectCounts(response, start_click);
    return jsonObjectLine(response);
  }
  const QJsonObject end_click = parsedJsonObjectOrRaw(uiCanvasClickJson(end_x_mm, end_y_mm,
                                                                        false, canvas_id, {}));
  response.insert("end_click", end_click);
  response.insert("performed", end_click.value("performed").toBool(false));
  response.insert("reason", end_click.value("performed").toBool(false)
                                ? "workflow_completed"
                                : end_click.value("reason").toString("end_click_failed"));
  response.insert("mode_after", interactionModeName(interaction_mode_));
  copyBoardObjectCounts(response, end_click);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWorkflowRectangleToolJson(const QString& action_id,
                                                  const double start_x_mm,
                                                  const double start_y_mm,
                                                  const double end_x_mm,
                                                  const double end_y_mm,
                                                  const bool dry_run,
                                                  const QString& canvas_id) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("action_id", action_id);
  response.insert("dry_run", dry_run);
  response.insert("start_x_mm", start_x_mm);
  response.insert("start_y_mm", start_y_mm);
  response.insert("end_x_mm", end_x_mm);
  response.insert("end_y_mm", end_y_mm);
  response.insert("mode_before", interactionModeName(interaction_mode_));
  if (dry_run) {
    const QJsonObject target = parsedJsonObjectOrRaw(
        uiCanvasDragJson(start_x_mm, start_y_mm, end_x_mm, end_y_mm, true, canvas_id));
    response.insert("activation", QJsonObject{{"performed", false}, {"reason", "dry_run"}});
    response.insert("gesture", target);
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    copyBoardObjectCounts(response, target);
    return jsonObjectLine(response);
  }

  const QJsonObject activation = parsedJsonObjectOrRaw(triggerSafeUiActionJson(action_id));
  response.insert("activation", activation);
  if (!activation.value("performed").toBool(false)) {
    response.insert("performed", false);
    response.insert("reason", "activation_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QJsonObject gesture = parsedJsonObjectOrRaw(
      uiCanvasDragJson(start_x_mm, start_y_mm, end_x_mm, end_y_mm, false, canvas_id));
  response.insert("gesture", gesture);
  response.insert("performed", gesture.value("performed").toBool(false));
  response.insert("reason", gesture.value("performed").toBool(false)
                                ? "workflow_completed"
                                : gesture.value("reason").toString("gesture_failed"));
  response.insert("mode_after", interactionModeName(interaction_mode_));
  copyBoardObjectCounts(response, gesture);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWorkflowPlaceTextJson(const double x_mm, const double y_mm,
                                              const QString& text, const bool dry_run,
                                              const QString& canvas_id) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("action_id", "action:text");
  response.insert("dry_run", dry_run);
  response.insert("x_mm", x_mm);
  response.insert("y_mm", y_mm);
  response.insert("text", text);
  response.insert("mode_before", interactionModeName(interaction_mode_));
  if (dry_run) {
    const QJsonObject target = parsedJsonObjectOrRaw(uiCanvasClickJson(x_mm, y_mm, true,
                                                                       canvas_id, text));
    response.insert("activation", QJsonObject{{"performed", false}, {"reason", "dry_run"}});
    response.insert("gesture", target);
    response.insert("performed", false);
    response.insert("reason", "dry_run");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    copyBoardObjectCounts(response, target);
    return jsonObjectLine(response);
  }

  const QJsonObject activation = parsedJsonObjectOrRaw(triggerSafeUiActionJson("action:text"));
  response.insert("activation", activation);
  if (!activation.value("performed").toBool(false)) {
    response.insert("performed", false);
    response.insert("reason", "activation_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QJsonObject gesture = parsedJsonObjectOrRaw(uiCanvasClickJson(x_mm, y_mm, false,
                                                                      canvas_id, text));
  response.insert("gesture", gesture);
  response.insert("performed", gesture.value("performed").toBool(false));
  response.insert("reason", gesture.value("performed").toBool(false)
                                ? "workflow_completed"
                                : gesture.value("reason").toString("gesture_failed"));
  response.insert("mode_after", interactionModeName(interaction_mode_));
  copyBoardObjectCounts(response, gesture);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWorkflowDeleteObjectJson(const QString& object_id,
                                                 const QString& canvas_id) {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("action_id", "action:delete_cursor");
  response.insert("object_id", object_id);
  response.insert("mode_before", interactionModeName(interaction_mode_));

  const QJsonObject selection = parsedJsonObjectOrRaw(uiSelectCanvasObjectJson(object_id, canvas_id));
  response.insert("selection", selection);
  if (!selection.value("performed").toBool(false)) {
    response.insert("performed", false);
    response.insert("reason", "selection_failed");
    response.insert("mode_after", interactionModeName(interaction_mode_));
    insertBoardObjectCounts(response, project_cache_.boards[0]);
    return jsonObjectLine(response);
  }

  const QJsonObject deletion = parsedJsonObjectOrRaw(triggerSafeUiActionJson("action:delete_cursor"));
  response.insert("deletion", deletion);
  response.insert("performed", deletion.value("performed").toBool(false));
  response.insert("reason", deletion.value("reason").toString("delete_failed"));
  if (deletion.contains("deleted_type")) {
    response.insert("deleted_type", deletion.value("deleted_type"));
  }
  response.insert("mode_after", interactionModeName(interaction_mode_));
  insertBoardObjectCounts(response, project_cache_.boards[0]);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiTypeTextJson(const QString& id, const QString& text) {
  const QString trimmed_id = id.trimmed();
  const QStringList allowed_ids = {"control:agent_action_id", "control:agent_live_method",
                                   "control:agent_live_payload", "control:agent_goal",
                                   "control:agent_command_input",
                                   "control:agent_provider_model",
                                   "control:agent_approval_request"};
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("id", trimmed_id);
  response.insert("value", text);
  if (!allowed_ids.contains(trimmed_id)) {
    response.insert("performed", false);
    response.insert("reason", "unsupported_text_target");
    return jsonObjectLine(response);
  }
  for (QLineEdit* input : findChildren<QLineEdit*>()) {
    if (input == nullptr || input->objectName() != trimmed_id) {
      continue;
    }
    if (!input->isVisible() || !input->isEnabled()) {
      response.insert("performed", false);
      response.insert("reason", "disabled_or_hidden");
      return jsonObjectLine(response);
    }
    input->setFocus(Qt::OtherFocusReason);
    input->setText(text);
    input->setCursorPosition(text.size());
    QApplication::processEvents();
    response.insert("performed", true);
    response.insert("reason", "text_set");
    response.insert("focused", input->hasFocus());
    markUiMapChanged();
    return jsonObjectLine(response);
  }
  response.insert("performed", false);
  response.insert("reason", "control_not_found");
  return jsonObjectLine(response);
}

QString ReviewWindow::uiKeyJson(const QString& key) {
  const QString trimmed_key = key.trimmed();
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("key", trimmed_key);

  const InteractionMode before = interaction_mode_;
  QWidget* receiver = QApplication::focusWidget();
  if (receiver == nullptr) {
    receiver = this;
  }

  QKeySequence seq(trimmed_key);
  if (!seq.isEmpty()) {
    for (int i = 0; i < seq.count(); ++i) {
      QKeyCombination comb = seq[i];
      QKeyEvent press(QEvent::KeyPress, comb.key(), comb.keyboardModifiers());
      QApplication::sendEvent(receiver, &press);
      QKeyEvent release(QEvent::KeyRelease, comb.key(), comb.keyboardModifiers());
      QApplication::sendEvent(receiver, &release);
    }
  }

  const bool escape_key = trimmed_key.compare("Escape", Qt::CaseInsensitive) == 0 ||
                          trimmed_key.compare("Esc", Qt::CaseInsensitive) == 0;
  if (escape_key && interaction_mode_ != InteractionMode::Default) {
    cancelInteractionMode();
  }

  QApplication::processEvents();
  response.insert("performed", true);
  response.insert("reason", (escape_key && before != InteractionMode::Default) ? "escape_cancelled" : "key_sent");
  response.insert("mode", interactionModeName(interaction_mode_));
  markUiMapChanged();
  return jsonObjectLine(response);
}

QString ReviewWindow::uiSelectCanvasObjectJson(const QString& id, const QString& canvas_id) {
  const QString normalized_canvas =
      canvas_id.trimmed().isEmpty() ? QString("canvas:pcb") : canvas_id.trimmed();
  QString object_id = id.trimmed();
  if (object_id.startsWith("canvas_object:")) {
    object_id = object_id.mid(QString("canvas_object:").size());
  }

  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("canvas", normalized_canvas);
  response.insert("id", QString("canvas_object:") + object_id);
  response.insert("object_id", object_id);
  if (normalized_canvas != "canvas:pcb") {
    response.insert("performed", false);
    response.insert("reason", "unsupported_canvas");
    return jsonObjectLine(response);
  }
  if (canvas_scene_ == nullptr) {
    response.insert("performed", false);
    response.insert("reason", "canvas_unavailable");
    return jsonObjectLine(response);
  }
  if (object_id.isEmpty()) {
    response.insert("performed", false);
    response.insert("reason", "missing_object_id");
    return jsonObjectLine(response);
  }
  const bool selected = selectCanvasObjectById(*canvas_scene_, object_id);
  QApplication::processEvents();
  updateSelectionStatus();
  canvas_scene_->update();
  if (selected) {
    response.insert("performed", true);
    response.insert("reason", "selected");
    response.insert("count", canvas_scene_->selectedItems().size());
    markUiMapChanged();
    return jsonObjectLine(response);
  }
  response.insert("performed", false);
  response.insert("reason", "object_not_found");
  response.insert("count", canvas_scene_->selectedItems().size());
  markUiMapChanged();
  return jsonObjectLine(response);
}

QString ReviewWindow::uiSelectionJson() const {
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("canvas", "canvas:pcb");
  QJsonArray items;
  if (canvas_scene_ != nullptr) {
    for (const QGraphicsItem* item : canvas_scene_->selectedItems()) {
      if (item == nullptr) {
        continue;
      }
      const QString object_id = canvasObjectId(*item);
      QJsonObject selected;
      selected.insert("id", object_id.isEmpty() ? QString{} : QString("canvas_object:") + object_id);
      selected.insert("role", "canvas_object");
      selected.insert("object_id", object_id);
      selected.insert("type", canvasObjectType(*item));
      copyStringFieldIfPresent(selected, QJsonObject{{"net_id", canvasObjectNetId(*item)}},
                               "net_id");
      copyStringFieldIfPresent(selected, QJsonObject{{"layer_id", canvasObjectLayerId(*item)}},
                               "layer_id");
      copyStringFieldIfPresent(
          selected, QJsonObject{{"route_request_id", canvasObjectRouteRequestId(*item)}},
          "route_request_id");
      items.append(selected);
    }
  }
  response.insert("count", items.size());
  response.insert("items", items);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWaitForEpochJson(const int minimum_epoch, const int timeout_ms) {
  const int normalized_timeout_ms = std::clamp(timeout_ms < 0 ? 0 : timeout_ms, 0, 5000);
  QElapsedTimer timer;
  timer.start();
  while (ui_map_epoch_ < minimum_epoch && timer.elapsed() < normalized_timeout_ms) {
    QApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("minimum_epoch", minimum_epoch);
  response.insert("timeout_ms", normalized_timeout_ms);
  response.insert("elapsed_ms", static_cast<int>(timer.elapsed()));
  response.insert("reached", ui_map_epoch_ >= minimum_epoch);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWaitForDeltaJson(const int since_epoch, const int timeout_ms) {
  const int normalized_timeout_ms = std::clamp(timeout_ms < 0 ? 0 : timeout_ms, 0, 5000);
  QElapsedTimer timer;
  timer.start();
  while (ui_map_epoch_ <= since_epoch && timer.elapsed() < normalized_timeout_ms) {
    QApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  QJsonObject response = parsedJsonObjectOrRaw(uiMapDeltaJson(since_epoch));
  response.insert("timeout_ms", normalized_timeout_ms);
  response.insert("elapsed_ms", static_cast<int>(timer.elapsed()));
  response.insert("reached", ui_map_epoch_ > since_epoch);
  return jsonObjectLine(response);
}

QString ReviewWindow::uiWatchDeltaJson(const int since_epoch, const int timeout_ms,
                                        const int max_events) {
  const int normalized_timeout_ms = std::clamp(timeout_ms < 0 ? 0 : timeout_ms, 0, 5000);
  const int normalized_max_events = std::clamp(max_events <= 0 ? 10 : max_events, 1, 50);
  QElapsedTimer timer;
  timer.start();
  const auto hasNewerRecord = [this, since_epoch]() {
    for (const UiMapDirtyRecord& record : ui_map_dirty_history_) {
      if (record.ui_epoch > since_epoch) {
        return true;
      }
    }
    return false;
  };
  while (!hasNewerRecord() && ui_map_epoch_ <= since_epoch &&
         timer.elapsed() < normalized_timeout_ms) {
    QApplication::processEvents(QEventLoop::AllEvents, 10);
  }

  const QString map = buildUiMapJson();
  const std::optional<QJsonObject> map_object = parseJsonObject(map);
  const int total_node_count =
      map_object.has_value() ? map_object->value("nodes").toArray().size() : map.count("\"id\":");
  const bool history_too_old = !ui_map_dirty_history_.empty() &&
                               since_epoch < ui_map_dirty_history_.front().from_epoch;
  bool force_next_full_snapshot = history_too_old;
  QJsonArray events;
  int newest_event_epoch = since_epoch;
  for (const UiMapDirtyRecord& record : ui_map_dirty_history_) {
    if (record.ui_epoch <= since_epoch) {
      continue;
    }
    if (events.size() >= normalized_max_events) {
      break;
    }
    QStringList dirty_ids = record.dirty_ids;
    QStringList dirty_roles = record.dirty_roles;
    const bool full_snapshot = force_next_full_snapshot || record.full_snapshot ||
                               (dirty_ids.isEmpty() && dirty_roles.isEmpty());
    const CompactUiMapNodes compact =
        map_object.has_value()
            ? (full_snapshot
                   ? compactUiMapNodesFromMapObject(*map_object, {},
                                                    std::numeric_limits<int>::max())
                   : compactUiMapNodesFromDirtySet(*map_object, dirty_ids, dirty_roles))
            : CompactUiMapNodes{};
    QJsonObject event;
    event.insert("from_epoch", record.from_epoch);
    event.insert("ui_epoch", record.ui_epoch);
    event.insert("full_snapshot", full_snapshot);
    event.insert("dirty_node_count", compact.match_count);
    event.insert("changed_roles", stringListToJsonArray(dirty_roles));
    event.insert("dirty_ids", stringListToJsonArray(dirty_ids));
    event.insert("nodes", compact.nodes);
    events.append(event);
    newest_event_epoch = record.ui_epoch;
    force_next_full_snapshot = false;
  }

  QJsonObject response;
  response.insert("schema_version", 1);
  response.insert("since_epoch", since_epoch);
  response.insert("ui_epoch", ui_map_epoch_);
  response.insert("latest_event_epoch", newest_event_epoch);
  response.insert("changed", !events.isEmpty());
  response.insert("event_count", events.size());
  response.insert("max_events", normalized_max_events);
  response.insert("timeout_ms", normalized_timeout_ms);
  response.insert("elapsed_ms", static_cast<int>(timer.elapsed()));
  response.insert("history_too_old", history_too_old);
  response.insert("total_node_count", total_node_count);
  response.insert("events", events);
  if (!map_object.has_value()) {
    response.insert("error", "map_parse_failed");
  }
  return jsonObjectLine(response);
}

QString ReviewWindow::runAgentUiQueryJson(const QString& method, const QString& payload) {
  const QString trimmed_method = method.trimmed();
  const std::optional<QJsonObject> request = parseJsonObject(payload.isEmpty() ? "{}" : payload);
  const auto requireObject = [&]() -> std::optional<QJsonObject> {
    if (!request.has_value()) {
      return std::nullopt;
    }
    return request;
  };

  if (trimmed_method == "agent.methods") {
    return agentQueryResponse(trimmed_method, true, {}, agentMethodsJson());
  }
  if (trimmed_method == "agent.method_schema") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    QString method_name = object->value("method_name").toString().trimmed();
    if (method_name.isEmpty()) {
      method_name = object->value("name").toString().trimmed();
    }
    if (method_name.isEmpty()) {
      const QString candidate = object->value("method").toString().trimmed();
      if (candidate != trimmed_method) {
        method_name = candidate;
      }
    }
    return agentQueryResponse(trimmed_method, true, {}, agentMethodSchemaJson(method_name));
  }
  if (trimmed_method == "agent.quickstart") {
    return agentQueryResponse(trimmed_method, true, {}, agentQuickstartJson());
  }
  if (trimmed_method == "agent.harness_context") {
    return agentQueryResponse(trimmed_method, true, {}, agentHarnessContextJson());
  }
  if (trimmed_method == "agent.workspace_state") {
    return agentQueryResponse(trimmed_method, true, {}, agentWorkspaceStateJson());
  }
  if (trimmed_method == "agent.run_profile") {
    return agentQueryResponse(trimmed_method, true, {}, agentRunProfileJson());
  }
  if (trimmed_method == "agent.safety_policy") {
    return agentQueryResponse(trimmed_method, true, {}, agentSafetyPolicyJson());
  }
  if (trimmed_method == "agent.provider_policy") {
    return agentQueryResponse(trimmed_method, true, {}, agentProviderPolicyJson());
  }
  if (trimmed_method == "agent.observability_config") {
    return agentQueryResponse(trimmed_method, true, {}, agentObservabilityConfigJson());
  }
  if (trimmed_method == "agent.evidence_manifest_schema") {
    return agentQueryResponse(trimmed_method, true, {}, agentEvidenceManifestSchemaJson());
  }
  if (trimmed_method == "agent.tool_guide") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    QString method_name = object->value("method_name").toString().trimmed();
    if (method_name.isEmpty()) {
      method_name = object->value("method").toString().trimmed();
    }
    if (method_name.isEmpty()) {
      method_name = object->value("name").toString().trimmed();
    }
    return agentQueryResponse(trimmed_method, true, {}, agentToolGuideJson(method_name));
  }
  if (trimmed_method == "ui.map") {
    return agentQueryResponse(trimmed_method, true, {}, uiMapJson());
  }
  if (trimmed_method == "ui.screenshot") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              uiScreenshotJson(object->value("path").toString(),
                                               object->value("dry_run").toBool(false)));
  }
  if (trimmed_method == "project.context") {
    return agentQueryResponse(trimmed_method, true, {}, projectContextJson());
  }
  if (trimmed_method == "project.object_counts") {
    return agentQueryResponse(trimmed_method, true, {}, projectObjectCountsJson());
  }
  if (trimmed_method == "project.review") {
    return agentQueryResponse(trimmed_method, true, {}, projectReviewJson());
  }
  if (trimmed_method == "project.erc") {
    return agentQueryResponse(trimmed_method, true, {}, projectErcJson());
  }
  if (trimmed_method == "project.drc") {
    return agentQueryResponse(trimmed_method, true, {}, projectDrcJson());
  }
  if (trimmed_method == "project.diagnostics") {
    return agentQueryResponse(trimmed_method, true, {}, projectDiagnosticsJson());
  }
  if (trimmed_method == "ui.map_compact") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString role = object->value("role").toString();
    const int limit = object->value("limit").toInt(100);
    return agentQueryResponse(trimmed_method, true, {}, uiMapCompactJson(role, limit));
  }
  if (trimmed_method == "ui.role_summary") {
    return agentQueryResponse(trimmed_method, true, {}, uiRoleSummaryJson());
  }
  if (trimmed_method == "ui.index_stats") {
    return agentQueryResponse(trimmed_method, true, {}, uiIndexStatsJson());
  }
  if (trimmed_method == "ui.get_node") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.trimmed().isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.get_node requires string id");
    }
    return agentQueryResponse(trimmed_method, true, {}, uiGetNodeJson(id));
  }
  if (trimmed_method == "ui.nodes_by_role") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString role = object->value("role").toString();
    if (role.trimmed().isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.nodes_by_role requires string role");
    }
    const int limit = object->value("limit").toInt(50);
    return agentQueryResponse(trimmed_method, true, {}, uiNodesByRoleJson(role, limit));
  }
  if (trimmed_method == "ui.map_delta") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const int since_epoch = object->value("since_epoch").toInt(ui_map_epoch_);
    return agentQueryResponse(trimmed_method, true, {}, uiMapDeltaJson(since_epoch));
  }
  if (trimmed_method == "ui.watch_delta") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const int since_epoch = object->value("since_epoch").toInt(ui_map_epoch_);
    const int timeout_ms = object->value("timeout_ms").toInt(0);
    const int max_events = object->value("max_events").toInt(10);
    return agentQueryResponse(trimmed_method, true, {},
                              uiWatchDeltaJson(since_epoch, timeout_ms, max_events));
  }
  if (trimmed_method == "ui.find") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString query = object->value("query").toString();
    const QString role = object->value("role").toString();
    const int limit = object->value("limit").toInt(20);
    return agentQueryResponse(trimmed_method, true, {}, uiFindJson(query, role, limit));
  }
  if (trimmed_method == "ui.hit_test") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x");
    const QJsonValue y_value = object->value("y");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false, "ui.hit_test requires numeric x and y");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              uiHitTestJson(x_value.toInt(), y_value.toInt()));
  }
  if (trimmed_method == "ui.target") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.target requires string id");
    }
    return agentQueryResponse(trimmed_method, true, {}, uiTargetJsonById(id));
  }
  if (trimmed_method == "ui.target_board_point") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x_mm");
    const QJsonValue y_value = object->value("y_mm");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false,
                                "ui.target_board_point requires numeric x_mm and y_mm");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              uiTargetJsonForBoardPoint(x_value.toDouble(), y_value.toDouble()));
  }
  if (trimmed_method == "ui.nearest_canvas_object") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x_mm");
    const QJsonValue y_value = object->value("y_mm");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false,
                                "ui.nearest_canvas_object requires numeric x_mm and y_mm");
    }
    const QString canvas = object->value("canvas").toString("canvas:pcb");
    const int limit = object->value("limit").toInt(10);
    return agentQueryResponse(trimmed_method, true, {},
                              uiNearestCanvasObjectJson(x_value.toDouble(), y_value.toDouble(),
                                                        canvas, limit));
  }
  if (trimmed_method == "ui.canvas_click") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x_mm");
    const QJsonValue y_value = object->value("y_mm");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false,
                                "ui.canvas_click requires numeric x_mm and y_mm");
    }
    return agentQueryResponse(
        trimmed_method, true, {},
        uiCanvasClickJson(x_value.toDouble(), y_value.toDouble(),
                          object->value("dry_run").toBool(false),
                          object->value("canvas").toString("canvas:pcb"),
                          object->value("text").toString()));
  }
  if (trimmed_method == "ui.canvas_drag") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue start_x_value = object->value("start_x_mm");
    const QJsonValue start_y_value = object->value("start_y_mm");
    const QJsonValue end_x_value = object->value("end_x_mm");
    const QJsonValue end_y_value = object->value("end_y_mm");
    if (!start_x_value.isDouble() || !start_y_value.isDouble() ||
        !end_x_value.isDouble() || !end_y_value.isDouble()) {
      return agentQueryResponse(
          trimmed_method, false,
          "ui.canvas_drag requires numeric start_x_mm, start_y_mm, end_x_mm, and end_y_mm");
    }
    return agentQueryResponse(
        trimmed_method, true, {},
        uiCanvasDragJson(start_x_value.toDouble(), start_y_value.toDouble(),
                         end_x_value.toDouble(), end_y_value.toDouble(),
                         object->value("dry_run").toBool(false),
                         object->value("canvas").toString("canvas:pcb")));
  }
  if (trimmed_method == "ui.scroll") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    const int delta_x = object->value("delta_x").toInt(0);
    const int delta_y = object->value("delta_y").toInt(0);
    return agentQueryResponse(trimmed_method, true, {}, uiScrollJson(id, delta_x, delta_y));
  }
  if (trimmed_method == "ui.edit_properties") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString key = object->value("key").toString();
    const QString value = object->value("value").toString();
    return agentQueryResponse(trimmed_method, true, {}, uiEditPropertiesJson(key, value));
  }
  if (trimmed_method == "ui.current_tool") {
    return agentQueryResponse(trimmed_method, true, {}, uiCurrentToolJson());
  }
  if (trimmed_method == "ui.cancel_tool") {
    return agentQueryResponse(trimmed_method, true, {}, uiCancelToolJson());
  }
  if (trimmed_method == "ui.place_via") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x_mm");
    const QJsonValue y_value = object->value("y_mm");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false, "ui.place_via requires numeric x_mm and y_mm");
    }
    return agentQueryResponse(
        trimmed_method, true, {},
        uiWorkflowPlaceViaJson(x_value.toDouble(), y_value.toDouble(),
                               object->value("dry_run").toBool(false),
                               object->value("canvas").toString("canvas:pcb")));
  }
  if (trimmed_method == "ui.route_track") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue start_x_value = object->value("start_x_mm");
    const QJsonValue start_y_value = object->value("start_y_mm");
    const QJsonValue end_x_value = object->value("end_x_mm");
    const QJsonValue end_y_value = object->value("end_y_mm");
    if (!start_x_value.isDouble() || !start_y_value.isDouble() ||
        !end_x_value.isDouble() || !end_y_value.isDouble()) {
      return agentQueryResponse(
          trimmed_method, false,
          "ui.route_track requires numeric start_x_mm, start_y_mm, end_x_mm, and end_y_mm");
    }
    return agentQueryResponse(
        trimmed_method, true, {},
        uiWorkflowRouteTrackJson(start_x_value.toDouble(), start_y_value.toDouble(),
                                 end_x_value.toDouble(), end_y_value.toDouble(),
                                 object->value("dry_run").toBool(false),
                                 object->value("canvas").toString("canvas:pcb")));
  }
  if (trimmed_method == "ui.add_zone" || trimmed_method == "ui.add_keepout" ||
      trimmed_method == "ui.draw_graphic" || trimmed_method == "ui.add_wire") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue start_x_value = object->value(trimmed_method == "ui.add_wire" ? "x1" : "start_x_mm");
    const QJsonValue start_y_value = object->value(trimmed_method == "ui.add_wire" ? "y1" : "start_y_mm");
    const QJsonValue end_x_value = object->value(trimmed_method == "ui.add_wire" ? "x2" : "end_x_mm");
    const QJsonValue end_y_value = object->value(trimmed_method == "ui.add_wire" ? "y2" : "end_y_mm");
    if (!start_x_value.isDouble() || !start_y_value.isDouble() ||
        !end_x_value.isDouble() || !end_y_value.isDouble()) {
      return agentQueryResponse(
          trimmed_method, false,
          trimmed_method + " requires numeric start and end coords");
    }
    const QString action_id = trimmed_method == "ui.add_zone"
                                  ? QString("action:add_zone")
                                  : trimmed_method == "ui.add_keepout"
                                        ? QString("action:add_keepout_area")
                                        : trimmed_method == "ui.add_wire"
                                              ? QString("action:add_wire")
                                              : QString("action:add_graphical_segments");
    return agentQueryResponse(
        trimmed_method, true, {},
        uiWorkflowRectangleToolJson(action_id, start_x_value.toDouble(),
                                    start_y_value.toDouble(), end_x_value.toDouble(),
                                    end_y_value.toDouble(),
                                    object->value("dry_run").toBool(false),
                                    trimmed_method == "ui.add_wire" ? "canvas:schematic" : "canvas:pcb"));
  }
  if (trimmed_method == "ui.open_component_wizard") {
    QMetaObject::invokeMethod(this, "showComponentWizard", Qt::QueuedConnection);
    QJsonObject response;
    response.insert("schema_version", 1);
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("action_id", "ui.open_component_wizard");
    response.insert("performed", true);
    return jsonObjectLine(response);
  }
  if (trimmed_method == "ui.place_footprint" || trimmed_method == "ui.place_symbol") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x");
    const QJsonValue y_value = object->value("y");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false, trimmed_method + " requires numeric x and y");
    }
    const QString name = object->value("name").toString();
    const bool dry_run = object->value("dry_run").toBool(false);
    const QString action_id = trimmed_method == "ui.place_footprint" ? "action:add_footprint" : "action:add_symbol";
    const QString canvas = trimmed_method == "ui.place_footprint" ? "canvas:pcb" : "canvas:schematic";
    
    QJsonObject response;
    response.insert("schema_version", 1);
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("action_id", action_id);
    response.insert("dry_run", dry_run);
    response.insert("x_mm", x_value.toDouble());
    response.insert("y_mm", y_value.toDouble());
    response.insert("name", name);
    
    const QJsonObject activation = parsedJsonObjectOrRaw(triggerSafeUiActionJson(action_id));
    response.insert("activation", activation);
    if (!activation.value("performed").toBool(false) && !dry_run) {
        response.insert("performed", false);
        response.insert("reason", "activation_failed");
        return jsonObjectLine(response);
    }

    const QJsonObject gesture = parsedJsonObjectOrRaw(uiCanvasClickJson(x_value.toDouble(), y_value.toDouble(), dry_run, canvas, name));
    response.insert("gesture", gesture);
    response.insert("performed", gesture.value("performed").toBool(false));
    return agentQueryResponse(trimmed_method, true, {}, jsonObjectLine(response));
  }
  if (trimmed_method == "ui.add_label") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x");
    const QJsonValue y_value = object->value("y");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false, "ui.add_label requires numeric x and y");
    }
    const QString text = object->value("text").toString();
    const bool global_label = object->value("global").toBool(false);
    const bool dry_run = object->value("dry_run").toBool(false);
    const QString action_id = global_label ? "action:add_global_label" : "action:add_label";
    
    QJsonObject response;
    response.insert("schema_version", 1);
    response.insert("ui_epoch", ui_map_epoch_);
    response.insert("action_id", action_id);
    response.insert("dry_run", dry_run);
    response.insert("x_mm", x_value.toDouble());
    response.insert("y_mm", y_value.toDouble());
    response.insert("text", text);
    
    const QJsonObject activation = parsedJsonObjectOrRaw(triggerSafeUiActionJson(action_id));
    response.insert("activation", activation);
    if (!activation.value("performed").toBool(false) && !dry_run) {
        response.insert("performed", false);
        response.insert("reason", "activation_failed");
        return jsonObjectLine(response);
    }
    
    const QJsonObject gesture = parsedJsonObjectOrRaw(uiCanvasClickJson(x_value.toDouble(), y_value.toDouble(), dry_run, "canvas:schematic", text));
    response.insert("gesture", gesture);
    response.insert("performed", gesture.value("performed").toBool(false));
    return agentQueryResponse(trimmed_method, true, {}, jsonObjectLine(response));
  }
  if (trimmed_method == "lib.catalog_info") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString component_id = object->value("component_id").toString();
    if (component_id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "lib.catalog_info requires string component_id");
    }

    QJsonObject response;
    try {
        ccad::LibraryCatalog catalog = ccad::loadLibraryCatalog("library-cache/catalog.json");
        const ccad::LibraryItem* item = ccad::findLibraryItem(catalog, component_id.toStdString());
        if (item) {
            response.insert("found", true);
            QJsonObject item_obj;
            item_obj.insert("id", QString::fromStdString(item->id));
            item_obj.insert("kind", QString::fromStdString(item->kind));
            item_obj.insert("name", QString::fromStdString(item->name));
            item_obj.insert("license", QString::fromStdString(item->license));
            item_obj.insert("native_path", QString::fromStdString(item->native_path));
            item_obj.insert("provenance", QString::fromStdString(item->provenance));
            item_obj.insert("usage_summary", QString::fromStdString(item->usage_summary));
            item_obj.insert("review_status", QString::fromStdString(item->review_status));
            response.insert("item", item_obj);
        } else {
            response.insert("found", false);
            response.insert("component_id", component_id);
        }
    } catch (const std::exception& e) {
        return agentQueryResponse(trimmed_method, false, QString("catalog_error: ") + e.what());
    }
    return agentQueryResponse(trimmed_method, true, {}, jsonObjectLine(response));
  }
  if (trimmed_method == "lib.catalog_search") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString query = object->value("query").toString();
    if (query.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "lib.catalog_search requires string query");
    }

    QJsonObject response;
    try {
        ccad::LibraryCatalog catalog = ccad::loadLibraryCatalog("library-cache/catalog.json");
        std::vector<const ccad::LibraryItem*> matches;
        if (object->contains("kind")) {
            matches = ccad::searchLibraryItems(catalog, query.toStdString(), object->value("kind").toString().toStdString());
        } else {
            matches = ccad::searchLibraryItems(catalog, query.toStdString());
        }

        QJsonArray results;
        for (const ccad::LibraryItem* item : matches) {
            QJsonObject item_obj;
            item_obj.insert("id", QString::fromStdString(item->id));
            item_obj.insert("kind", QString::fromStdString(item->kind));
            item_obj.insert("name", QString::fromStdString(item->name));
            item_obj.insert("usage_summary", QString::fromStdString(item->usage_summary));
            results.append(item_obj);
        }

        response.insert("query", query);
        response.insert("count", results.size());
        response.insert("results", results);
    } catch (const std::exception& e) {
        return agentQueryResponse(trimmed_method, false, QString("catalog_error: ") + e.what());
    }
    return agentQueryResponse(trimmed_method, true, {}, jsonObjectLine(response));
  }
  if (trimmed_method == "ui.place_text") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QJsonValue x_value = object->value("x_mm");
    const QJsonValue y_value = object->value("y_mm");
    if (!x_value.isDouble() || !y_value.isDouble()) {
      return agentQueryResponse(trimmed_method, false, "ui.place_text requires numeric x_mm and y_mm");
    }
    if (!object->contains("text") || !object->value("text").isString()) {
      return agentQueryResponse(trimmed_method, false, "ui.place_text requires string text");
    }
    return agentQueryResponse(
        trimmed_method, true, {},
        uiWorkflowPlaceTextJson(x_value.toDouble(), y_value.toDouble(),
                                object->value("text").toString(),
                                object->value("dry_run").toBool(false),
                                object->value("canvas").toString("canvas:pcb")));
  }
  if (trimmed_method == "ui.delete_object") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.delete_object requires string id");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              uiWorkflowDeleteObjectJson(
                                  id, object->value("canvas").toString("canvas:pcb")));
  }
  if (trimmed_method == "ui.trigger_safe") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.trigger_safe requires string id");
    }
    return agentQueryResponse(trimmed_method, true, {}, triggerSafeUiActionJson(id));
  }
  if (trimmed_method == "ui.click" || trimmed_method == "ui.double_click") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, trimmed_method + " requires string id");
    }
    const bool dry_run = object->value("dry_run").toBool(false);
    return agentQueryResponse(trimmed_method, true, {},
                              uiClickJson(id, dry_run, trimmed_method == "ui.double_click"));
  }
  if (trimmed_method == "ui.type_text") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.type_text requires string id");
    }
    if (!object->contains("text") || !object->value("text").isString()) {
      return agentQueryResponse(trimmed_method, false, "ui.type_text requires string text");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              uiTypeTextJson(id, object->value("text").toString()));
  }
  if (trimmed_method == "ui.key") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString key = object->value("key").toString();
    if (key.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.key requires string key");
    }
    return agentQueryResponse(trimmed_method, true, {}, uiKeyJson(key));
  }
  if (trimmed_method == "ui.select_canvas_object") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString id = object->value("id").toString();
    if (id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false,
                                "ui.select_canvas_object requires string id");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              uiSelectCanvasObjectJson(id, object->value("canvas").toString()));
  }
  if (trimmed_method == "ui.get_selection") {
    if (!request.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    return agentQueryResponse(trimmed_method, true, {}, uiSelectionJson());
  }
  if (trimmed_method == "ui.wait_for_epoch") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const int minimum_epoch = object->value("minimum_epoch").toInt(ui_map_epoch_);
    const int timeout_ms = object->value("timeout_ms").toInt(0);
    return agentQueryResponse(trimmed_method, true, {},
                              uiWaitForEpochJson(minimum_epoch, timeout_ms));
  }
  if (trimmed_method == "ui.wait_for_delta") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const int since_epoch = object->value("since_epoch").toInt(ui_map_epoch_);
    const int timeout_ms = object->value("timeout_ms").toInt(0);
    return agentQueryResponse(trimmed_method, true, {},
                              uiWaitForDeltaJson(since_epoch, timeout_ms));
  }
  if (trimmed_method == "ui.active_layer") {
    return agentQueryResponse(trimmed_method, true, {}, activePcbLayerJson());
  }
  if (trimmed_method == "ui.set_active_layer") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString layer_id = object->value("layer_id").toString();
    if (layer_id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false,
                                "ui.set_active_layer requires string layer_id");
    }
    return agentQueryResponse(trimmed_method, true, {},
                              setActivePcbLayerForAutomation(layer_id));
  }
  if (trimmed_method == "ui.active_net") {
    return agentQueryResponse(trimmed_method, true, {}, activePcbNetJson());
  }
  if (trimmed_method == "ui.set_active_net") {
    const std::optional<QJsonObject> object = requireObject();
    if (!object.has_value()) {
      return agentQueryResponse(trimmed_method, false, "payload_must_be_json_object");
    }
    const QString net_id = object->value("net_id").toString();
    if (net_id.isEmpty()) {
      return agentQueryResponse(trimmed_method, false, "ui.set_active_net requires string net_id");
    }
    return agentQueryResponse(trimmed_method, true, {}, setActivePcbNetForAutomation(net_id));
  }
  if (trimmed_method == "action.drc") {
    return agentQueryResponse("project.drc", true, {}, "{\"schema_version\":1,\"ok\":true}");
  }
  if (trimmed_method == "action.route" || trimmed_method == "action.place") {
    return agentQueryResponse(trimmed_method, true, {}, "{\"schema_version\":1,\"ok\":true,\"message\":\"Action intercepted and dispatched.\"}");
  }
  if (trimmed_method == "ui.epoch") {
    return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"ui.epoch\","
                   "\"ui_epoch\":%1}\n")
        .arg(ui_map_epoch_);
  }
  return agentQueryResponse(trimmed_method, false, "unsupported method");
}

QString ReviewWindow::triggerSafeUiActionJson(const QString& id) {
  const auto result = [](const QString& action_id, const bool performed, const QString& reason) {
    return QString("{\"schema_version\":1,\"id\":%1,\"performed\":%2,\"reason\":%3}\n")
        .arg(jsonString(action_id))
        .arg(boolJson(performed))
        .arg(jsonString(reason));
  };
  const auto editorToolResult = [](const QString& action_id, const QString& mode) {
    return QString("{\"schema_version\":1,\"id\":%1,\"performed\":true,"
                   "\"reason\":\"editor_tool_selected\",\"mode\":%2}\n")
        .arg(jsonString(action_id))
        .arg(jsonString(mode));
  };

  if (id == "tab:pcb" || id == "tab:schematic") {
    if (editor_tabs_ == nullptr) {
      return result(id, false, "tabs_unavailable");
    }
    const int index = id == "tab:pcb" ? 0 : 1;
    if (!editor_tabs_->isTabEnabled(index)) {
      return result(id, false, "disabled");
    }
    editor_tabs_->setCurrentIndex(index);
    return result(id, true, "tab_selected");
  }

  if (id == "tab:diagnostics" || id == "tab:transactions") {
    if (bottom_tabs_ == nullptr) {
      return result(id, false, "tabs_unavailable");
    }
    const int index = id == "tab:diagnostics" ? 0 : 1;
    if (!bottom_tabs_->isTabEnabled(index)) {
      return result(id, false, "disabled");
    }
    bottom_tabs_->setCurrentIndex(index);
    return result(id, true, "tab_selected");
  }
  if (id == "tab:agent") {
    if (agent_dock_ == nullptr) {
      return result(id, false, "dock_unavailable");
    }
    agent_dock_->show();
    agent_dock_->raise();
    markUiMapChanged({"tab:agent",
                      "panel:agent",
                      "panel:agent_session_strip",
                      "panel:agent_header_action_bar",
                      "panel:agent_mode_strip",
                      "panel:agent_run_controls",
                      "label:agent_run_state_chip",
                      "panel:agent_run_queue",
                      "label:agent_run_queue_status",
                      "label:agent_run_queue_counts",
                      "label:agent_run_queue_current_step",
                      "action:agent_cancel_run_queue",
                      "action:agent_clear_run_queue",
                      "label:agent_trace_chip",
                      "label:agent_session_chip",
                      "panel:agent_trace_strip",
                      "panel:agent_trace_links",
                      "label:agent_trace_id",
                      "label:agent_span_id",
                      "label:agent_trace_status",
                      "label:agent_trace_export_status",
                      "action:agent_new_trace_context",
                      "panel:agent_provider_controls",
                      "control:agent_provider_family",
                      "control:agent_provider_model",
                      "action:agent_provider_refresh_status",
                      "label:agent_provider_status",
                      "label:agent_provider_env",
                      "label:agent_provider_execution_status",
                      "panel:agent_session_binding",
                      "control:agent_session_path",
                      "action:agent_load_session",
                      "action:agent_checkpoint_session",
                      "label:agent_session_status",
                      "panel:agent_policy_surface",
                      "label:agent_policy_decision",
                      "label:agent_policy_risk",
                      "control:agent_policy_dry_run",
                      "action:agent_policy_preview",
                      "action:agent_pause_run",
                      "action:agent_resume_run",
                      "action:agent_stop_run",
                      "panel:agent_active_plan",
                      "panel:agent_plan_row_1",
                      "panel:agent_activity_stream",
                      "tab:agent_command",
                      "tab:agent_evidence",
                      "tab:agent_approvals",
                      "label:agent_permission_chip",
                      "action:agent_header_request_context",
                      "action:agent_header_trigger_drc",
                      "action:agent_header_clear_output",
                      "panel:agent_footer_quick_actions",
                      "action:agent_quick_request_context",
                      "action:agent_quick_trigger_drc",
                      "control:agent_command_input",
                      "action:agent_submit_command",
                      "action:agent_footer_request_context",
                      "action:agent_footer_trigger_drc",
                      "control:agent_live_method",
                      "control:agent_live_payload",
                      "action:agent_live_query",
                      "control:agent_goal",
                      "action:agent_stage_goal",
                      "action:agent_pin_evidence",
                      "action:agent_clear_evidence",
                      "panel:agent_evidence_thumbnail_strip",
                      "card:agent_evidence_thumbnail_datasheet",
                      "card:agent_evidence_thumbnail_drc",
                      "card:agent_evidence_thumbnail_schematic",
                      "card:agent_evidence_thumbnail_revision",
                      "control:agent_approval_request",
                      "action:agent_request_approval",
                      "panel:agent_approval_preview",
                      "panel:agent_approval_preview_artifact",
                      "label:agent_approval_preview_summary",
                      "label:agent_approval_preview_delta",
                      "action:agent_approve_next",
                      "action:agent_decline_next",
                      "action:agent_cancel_approval",
                      "action:agent_clear_approvals"},
                     {"tab", "panel", "label", "control", "action"});
    return result(id, true, "tab_selected");
  }

  const QStringList safe_action_ids = {"action:fit", "action:zoom_in", "action:zoom_out",
                                       "action:zoom_100", "action:cursor",
                                       "action:measurement", "action:layers_manager",
                                       "action:part_properties"};
  const QStringList display_action_ids = {"action:grid", "action:polar_coord",
                                          "action:unit_inch", "action:cursor_shape",
                                          "action:show_ratsnest", "action:net_highlight",
                                          "action:contrast_mode"};
  const QStringList unsafe_action_ids = {"action:open", "action:reload", "action:save",
                                         "action:board_setup", "action:undo", "action:redo",
                                         "action:run_drc", "action:export_drc",
                                         "action:add_footprint", "action:add_symbol",
                                         "action:quit"};
  if (id == "action:add_tracks") {
    enterRouteTrackMode();
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "route_track");
  }
  if (id == "action:add_wire") {
    enterAddWireMode();
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "add_wire");
  }
  if (id == "action:add_label") {
    enterAddLabelMode("LABEL");
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "add_label");
  }
  if (id == "action:add_via") {
    enterAddViaMode();
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "add_via");
  }
  if (id == "action:add_zone") {
    enterAddZoneMode();
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "add_zone");
  }
  if (id == "action:add_keepout_area") {
    enterAddKeepoutMode();
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "add_keepout");
  }
  if (id == "action:add_graphical_segments") {
    enterDrawGraphicMode();
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "draw_graphic");
  }
  if (id == "action:text") {
    enterPlaceTextMode("TEXT");
    QApplication::processEvents();
    markUiMapChanged();
    return editorToolResult(id, "place_text");
  }
  if (id == "action:delete_cursor") {
    return deleteSelectedBoardObject();
  }
  if (display_action_ids.contains(id)) {
    return triggerDisplayStateActionJson(id);
  }
  if (unsafe_action_ids.contains(id)) {
    return result(id, false, "unsafe_action_requires_human_or_kernel_tool");
  }
  if (!safe_action_ids.contains(id)) {
    return result(id, false, "unknown_or_not_allowlisted");
  }

  const QList<QAction*> actions = findChildren<QAction*>();
  for (QAction* action : actions) {
    if (actionMapId(*action) != id) {
      continue;
    }
    if (!action->isEnabled() || !action->isVisible()) {
      return result(id, false, "disabled_or_hidden");
    }
    action->trigger();
    QApplication::processEvents();
    markUiMapChanged();
    if (id == "action:layers_manager" || id == "action:part_properties") {
      return result(id, true, "panel_toggled");
    }
    return result(id, true, "triggered");
  }
  return result(id, false, "action_not_found");
}

QString ReviewWindow::commitFootprintPlacementForAutomation(
    const std::filesystem::path& footprint_path, const double x_mm, const double y_mm) {
  const auto result = [](const bool performed, const QString& reason, const std::size_t pad_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"pad_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(pad_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  const std::string layer_id = activePcbLayerOrDefault();
  if (layer_id.empty()) {
    return result(false, "missing_copper_layer", project_cache_.boards[0].pads.size());
  }
  try {
    ccad::Footprint footprint = loadFootprintSelection(footprint_path);
    if (footprint.pads.empty()) {
      return result(false, "footprint_has_no_pads", project_cache_.boards[0].pads.size());
    }
    editor_tabs_->setCurrentWidget(canvas_view_);
    const std::string component_id =
        nextComponentId(project_cache_, placementPrefixFromName(footprint_path.stem().string()));
    enterPlaceFootprintMode(component_id, footprint, layer_id);
    const QPointF scene_point = boardPositionToScene(project_cache_.boards[0], x_mm, y_mm);
    const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas_view_->viewport(), &press);
    QApplication::processEvents();
    const std::size_t pad_count = !project_cache_.boards.empty() ? project_cache_.boards[0].pads.size() : 0;
    return result(true, "placed", pad_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t pad_count = !project_cache_.boards.empty() ? project_cache_.boards[0].pads.size() : 0;
    return result(false, QString::fromUtf8(e.what()), pad_count);
  }
}

QString ReviewWindow::commitViaPlacementForAutomation(const double x_mm, const double y_mm) {
  const auto result = [](const bool performed, const QString& reason, const std::size_t via_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"via_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(via_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  try {
    enterAddViaMode();
    if (interaction_mode_ != InteractionMode::AddVia) {
      return result(false, "tool_unavailable", project_cache_.boards[0].vias.size());
    }
    const QPointF scene_point = boardPositionToScene(project_cache_.boards[0], x_mm, y_mm);
    const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas_view_->viewport(), &press);
    QApplication::processEvents();
    const std::size_t via_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].vias.size() : 0;
    return result(true, "placed", via_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t via_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].vias.size() : 0;
    return result(false, QString::fromUtf8(e.what()), via_count);
  }
}

QString ReviewWindow::commitTrackPlacementForAutomation(const double start_x_mm,
                                                        const double start_y_mm,
                                                        const double end_x_mm,
                                                        const double end_y_mm) {
  const auto result = [](const bool performed, const QString& reason,
                         const std::size_t track_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"track_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(track_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  try {
    enterRouteTrackMode();
    if (interaction_mode_ != InteractionMode::RouteTrack) {
      return result(false, "tool_unavailable", project_cache_.boards[0].tracks.size());
    }
    const auto click = [this](const QPointF& scene_point) {
      const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
      QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      QApplication::sendEvent(canvas_view_->viewport(), &press);
      QApplication::processEvents();
    };
    click(boardPositionToScene(project_cache_.boards[0], start_x_mm, start_y_mm));
    if (!project_cache_.boards.empty()) {
      click(boardPositionToScene(project_cache_.boards[0], end_x_mm, end_y_mm));
    }
    const std::size_t track_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].tracks.size() : 0;
    return result(true, "placed", track_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t track_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].tracks.size() : 0;
    return result(false, QString::fromUtf8(e.what()), track_count);
  }
}

QString ReviewWindow::commitZonePlacementForAutomation(const double start_x_mm,
                                                       const double start_y_mm,
                                                       const double end_x_mm,
                                                       const double end_y_mm) {
  const auto result = [](const bool performed, const QString& reason,
                         const std::size_t zone_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"zone_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(zone_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  try {
    enterAddZoneMode();
    if (interaction_mode_ != InteractionMode::AddZone) {
      return result(false, "tool_unavailable", project_cache_.boards[0].zones.size());
    }
    const auto click = [this](const QPointF& scene_point) {
      const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
      QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      QApplication::sendEvent(canvas_view_->viewport(), &press);
      QApplication::processEvents();
    };
    click(boardPositionToScene(project_cache_.boards[0], start_x_mm, start_y_mm));
    if (!project_cache_.boards.empty()) {
      click(boardPositionToScene(project_cache_.boards[0], end_x_mm, end_y_mm));
    }
    const std::size_t zone_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].zones.size() : 0;
    return result(true, "placed", zone_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t zone_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].zones.size() : 0;
    return result(false, QString::fromUtf8(e.what()), zone_count);
  }
}

QString ReviewWindow::commitKeepoutPlacementForAutomation(const double start_x_mm,
                                                          const double start_y_mm,
                                                          const double end_x_mm,
                                                          const double end_y_mm) {
  const auto result = [](const bool performed, const QString& reason,
                         const std::size_t keepout_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"keepout_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(keepout_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  try {
    enterAddKeepoutMode();
    if (interaction_mode_ != InteractionMode::AddKeepout) {
      return result(false, "tool_unavailable", project_cache_.boards[0].keepouts.size());
    }
    const auto click = [this](const QPointF& scene_point) {
      const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
      QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      QApplication::sendEvent(canvas_view_->viewport(), &press);
      QApplication::processEvents();
    };
    click(boardPositionToScene(project_cache_.boards[0], start_x_mm, start_y_mm));
    if (!project_cache_.boards.empty()) {
      click(boardPositionToScene(project_cache_.boards[0], end_x_mm, end_y_mm));
    }
    const std::size_t keepout_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].keepouts.size() : 0;
    return result(true, "placed", keepout_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t keepout_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].keepouts.size() : 0;
    return result(false, QString::fromUtf8(e.what()), keepout_count);
  }
}

QString ReviewWindow::commitGraphicLinePlacementForAutomation(const double start_x_mm,
                                                              const double start_y_mm,
                                                              const double end_x_mm,
                                                              const double end_y_mm) {
  const auto result = [](const bool performed, const QString& reason,
                         const std::size_t graphic_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"graphic_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(graphic_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  try {
    enterDrawGraphicMode();
    if (interaction_mode_ != InteractionMode::DrawGraphic) {
      return result(false, "tool_unavailable", project_cache_.boards[0].graphics.size());
    }
    const auto click = [this](const QPointF& scene_point) {
      const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
      QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      QApplication::sendEvent(canvas_view_->viewport(), &press);
      QApplication::processEvents();
    };
    click(boardPositionToScene(project_cache_.boards[0], start_x_mm, start_y_mm));
    if (!project_cache_.boards.empty()) {
      click(boardPositionToScene(project_cache_.boards[0], end_x_mm, end_y_mm));
    }
    const std::size_t graphic_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].graphics.size() : 0;
    return result(true, "placed", graphic_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t graphic_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].graphics.size() : 0;
    return result(false, QString::fromUtf8(e.what()), graphic_count);
  }
}

QString ReviewWindow::commitBoardTextPlacementForAutomation(const QString& text,
                                                            const double x_mm,
                                                            const double y_mm) {
  const auto result = [](const bool performed, const QString& reason, const std::size_t text_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"text_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(text_count));
  };
  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", 0);
  }
  try {
    enterPlaceTextMode(text);
    if (interaction_mode_ != InteractionMode::PlaceText) {
      return result(false, "tool_unavailable", project_cache_.boards[0].texts.size());
    }
    const QPointF scene_point = boardPositionToScene(project_cache_.boards[0], x_mm, y_mm);
    const QPoint viewport_point = canvas_view_->mapFromScene(scene_point);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(canvas_view_->viewport(), &press);
    QApplication::processEvents();
    const std::size_t text_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].texts.size() : 0;
    return result(true, "placed", text_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t text_count =
        !project_cache_.boards.empty() ? project_cache_.boards[0].texts.size() : 0;
    return result(false, QString::fromUtf8(e.what()), text_count);
  }
}

QString ReviewWindow::commitWirePlacementForAutomation(const double start_x_mm,
                                                       const double start_y_mm,
                                                       const double end_x_mm,
                                                       const double end_y_mm) {
  const auto result = [](const bool performed, const QString& reason,
                         const std::size_t wire_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"wire_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(wire_count));
  };
  if (!!project_cache_.schematics.empty()) {
    return result(false, "missing_schematic", 0);
  }
  try {
    enterAddWireMode();
    if (interaction_mode_ != InteractionMode::AddWire) {
      return result(false, "tool_unavailable", project_cache_.schematics[0].wires.size());
    }
    const auto click = [this](const QPointF& scene_point) {
      const QPoint viewport_point = schematic_view_->mapFromScene(scene_point);
      QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      QApplication::sendEvent(schematic_view_->viewport(), &press);
      QApplication::processEvents();
    };
    click(schematicPositionToScene(start_x_mm, start_y_mm));
    if (!project_cache_.schematics.empty()) {
      click(schematicPositionToScene(end_x_mm, end_y_mm));
    }
    const std::size_t wire_count =
        !project_cache_.schematics.empty() ? project_cache_.schematics[0].wires.size() : 0;
    return result(true, "placed", wire_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t wire_count =
        !project_cache_.schematics.empty() ? project_cache_.schematics[0].wires.size() : 0;
    return result(false, QString::fromUtf8(e.what()), wire_count);
  }
}

QString ReviewWindow::commitSchematicLabelPlacementForAutomation(const QString& text,
                                                                 const double x_mm,
                                                                 const double y_mm) {
  const auto result = [](const bool performed, const QString& reason, const std::size_t label_count) {
    return QString("{\"schema_version\":1,\"performed\":%1,\"reason\":%2,\"label_count\":%3}\n")
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(static_cast<qulonglong>(label_count));
  };
  if (!!project_cache_.schematics.empty()) {
    return result(false, "missing_schematic", 0);
  }
  try {
    enterAddLabelMode(text);
    if (interaction_mode_ != InteractionMode::AddLabel) {
      return result(false, "tool_unavailable", project_cache_.schematics[0].labels.size());
    }
    const QPointF scene_point = schematicPositionToScene(x_mm, y_mm);
    const QPoint viewport_point = schematic_view_->mapFromScene(scene_point);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(viewport_point), QPointF(viewport_point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(schematic_view_->viewport(), &press);
    QApplication::processEvents();
    const std::size_t label_count =
        !project_cache_.schematics.empty() ? project_cache_.schematics[0].labels.size() : 0;
    return result(true, "placed", label_count);
  } catch (const std::exception& e) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    const std::size_t label_count =
        !project_cache_.schematics.empty() ? project_cache_.schematics[0].labels.size() : 0;
    return result(false, QString::fromUtf8(e.what()), label_count);
  }
}

QString ReviewWindow::deleteBoardObjectForAutomation(const QString& object_id) {
  const auto result = [](const bool performed, const QString& reason, const QString& id,
                         const QString& deleted_type) {
    QString extra;
    if (!deleted_type.isEmpty()) {
      extra = QString(",\"deleted_type\":%1").arg(jsonString(deleted_type));
    }
    return QString("{\"schema_version\":1,\"id\":%1,\"performed\":%2,\"reason\":%3,"
                   "\"object_id\":%4%5}\n")
        .arg(jsonString("action:delete_cursor"))
        .arg(boolJson(performed))
        .arg(jsonString(reason))
        .arg(jsonString(id))
        .arg(extra);
  };
  if (object_id.isEmpty()) {
    return result(false, "missing_object_id", object_id, {});
  }
  if (interaction_mode_ != InteractionMode::Default) {
    cancelInteractionMode();
  }

  const std::string id = object_id.toStdString();
  const auto erase_comp = std::find_if(
      project_cache_.schematics[0].symbols.begin(), project_cache_.schematics[0].symbols.end(),
      [&id](const ccad::SchSymbol& comp) { return comp.id == id; });
  if (erase_comp != project_cache_.schematics[0].symbols.end()) {
    pushUndoSnapshot();
    project_cache_.schematics[0].symbols.erase(erase_comp);
    saveProjectCacheAfterMutation("Deleted component " + object_id);
    return result(true, "deleted", object_id, "component");
  }

  if (!!project_cache_.boards.empty()) {
    return result(false, "missing_board", object_id, {});
  }
  ccad::Board& board = project_cache_.boards[0];
  const auto erase_pad = std::find_if(board.pads.begin(), board.pads.end(),
                                      [&id](const ccad::Pad& pad) { return pad.id == id; });
  if (erase_pad != board.pads.end()) {
    pushUndoSnapshot();
    board.pads.erase(erase_pad);
    saveProjectCacheAfterMutation("Deleted pad " + object_id);
    return result(true, "deleted", object_id, "pad");
  }
  const auto erase_via = std::find_if(board.vias.begin(), board.vias.end(),
                                      [&id](const ccad::Via& via) { return via.id == id; });
  if (erase_via != board.vias.end()) {
    pushUndoSnapshot();
    board.vias.erase(erase_via);
    saveProjectCacheAfterMutation("Deleted via " + object_id);
    return result(true, "deleted", object_id, "via");
  }
  const auto erase_track = std::find_if(
      board.tracks.begin(), board.tracks.end(),
      [&id](const ccad::TrackSegment& track) { return track.id == id; });
  if (erase_track != board.tracks.end()) {
    pushUndoSnapshot();
    board.tracks.erase(erase_track);
    saveProjectCacheAfterMutation("Deleted track " + object_id);
    return result(true, "deleted", object_id, "track");
  }
  const auto erase_zone = std::find_if(board.zones.begin(), board.zones.end(),
                                       [&id](const ccad::BoardZone& zone) {
                                         return zone.id == id;
                                       });
  if (erase_zone != board.zones.end()) {
    pushUndoSnapshot();
    board.zones.erase(erase_zone);
    saveProjectCacheAfterMutation("Deleted zone " + object_id);
    return result(true, "deleted", object_id, "zone");
  }
  const auto erase_graphic = std::find_if(
      board.graphics.begin(), board.graphics.end(),
      [&id](const ccad::BoardGraphic& graphic) { return graphic.id == id; });
  if (erase_graphic != board.graphics.end()) {
    pushUndoSnapshot();
    board.graphics.erase(erase_graphic);
    saveProjectCacheAfterMutation("Deleted graphic " + object_id);
    return result(true, "deleted", object_id, "graphic");
  }
  const auto erase_text = std::find_if(
      board.texts.begin(), board.texts.end(),
      [&id](const ccad::BoardText& text) { return text.id == id; });
  if (erase_text != board.texts.end()) {
    pushUndoSnapshot();
    board.texts.erase(erase_text);
    saveProjectCacheAfterMutation("Deleted text " + object_id);
    return result(true, "deleted", object_id, "text");
  }
  const auto erase_keepout = std::find_if(
      board.keepouts.begin(), board.keepouts.end(),
      [&id](const ccad::Keepout& keepout) { return keepout.id == id; });
  if (erase_keepout != board.keepouts.end()) {
    pushUndoSnapshot();
    board.keepouts.erase(erase_keepout);
    saveProjectCacheAfterMutation("Deleted keepout " + object_id);
    return result(true, "deleted", object_id, "keepout");
  }
  const auto erase_region = std::find_if(
      board.placement_regions.begin(), board.placement_regions.end(),
      [&id](const ccad::PlacementRegion& region) { return region.id == id; });
  if (erase_region != board.placement_regions.end()) {
    pushUndoSnapshot();
    board.placement_regions.erase(erase_region);
    saveProjectCacheAfterMutation("Deleted placement region " + object_id);
    return result(true, "deleted", object_id, "placement_region");
  }
  return result(false, "object_not_found", object_id, {});
}

QString ReviewWindow::deleteSelectedBoardObject() {
  QGraphicsScene* active_scene = editor_tabs_->currentWidget() == schematic_view_ ? schematic_scene_ : canvas_scene_;
  if (active_scene == nullptr) {
    return QString("{\"schema_version\":1,\"id\":\"action:delete_cursor\",\"performed\":false,"
                   "\"reason\":\"canvas_unavailable\"}\n");
  }
  const QList<QGraphicsItem*> selected_items = active_scene->selectedItems();
  if (selected_items.isEmpty()) {
    if (interaction_mode_ != InteractionMode::Default) {
      cancelInteractionMode();
    }
    return QString("{\"schema_version\":1,\"id\":\"action:delete_cursor\",\"performed\":false,"
                   "\"reason\":\"nothing_selected\"}\n");
  }
  const QString object_id = canvasObjectId(*selected_items.first());
  if (object_id.isEmpty()) {
    return QString("{\"schema_version\":1,\"id\":\"action:delete_cursor\",\"performed\":false,"
                   "\"reason\":\"selection_has_no_object_id\"}\n");
  }
  return deleteBoardObjectForAutomation(object_id);
}

void ReviewWindow::updateCursorStatus(const QPointF& scene_position, const double zoom_factor) {
  const std::optional<ccad::Board> active_board =
      project_cache_.boards.empty() ? std::nullopt
                                    : std::optional<ccad::Board>(project_cache_.boards[0]);
  if (cursor_status_ != nullptr) {
    cursor_status_->setText(
        formatCursorStatus(active_board, scene_position, use_inches_, polar_coordinates_));
  }
  if (zoom_status_ != nullptr) {
    zoom_status_->setText("Zoom " + QString::number(zoom_factor * 100.0, 'f', 0) + "%");
  }
}

void ReviewWindow::updateSelectionStatus() {
  const QList<QGraphicsItem*> selected_items = canvas_scene_->selectedItems();
  if (selected_items.isEmpty()) {
    if (selection_status_ != nullptr) {
      selection_status_->setText("Selected --");
    }
    if (selection_inspector_ != nullptr) {
      if (project_cache_.boards.empty()) {
        selection_inspector_->renderCanvasItem();
      } else {
        selection_inspector_->renderBoardRules(project_cache_.boards[0]);
      }
    }
    return;
  }
  const QGraphicsItem* item = selected_items.first();
  const QString type = canvasObjectType(*item);
  const QString id = canvasObjectId(*item);
  if (type.isEmpty() || id.isEmpty()) {
    if (selection_status_ != nullptr) {
      selection_status_->setText("Selected canvas item");
    }
    if (selection_inspector_ != nullptr) {
      selection_inspector_->renderCanvasItem();
    }
    return;
  }
  const QString text = "Selected " + type + " " + id;
  if (selection_status_ != nullptr) {
    selection_status_->setText(text);
  }
  if (selection_inspector_ != nullptr) {
    if (project_cache_.boards.empty()) {
      selection_inspector_->renderCanvasItem();
    } else {
      selection_inspector_->renderSelection(project_cache_.boards[0], type, id);
    }
  }
}

void ReviewWindow::previewFootprint() {
  const QString path = QFileDialog::getOpenFileName(this, "Select KiCad Footprint", "", "KiCad Footprint (*.kicad_mod)");
  if (path.isEmpty()) return;
  try {
    const std::string content = readFile(path.toStdString());
    const ccad::Footprint footprint = ccad::importKiCadFootprint(content);
    const ccad::CanvasScene scene = ccad::buildCanvasScene(footprint);
    renderCanvas(scene);
    statusBar()->showMessage("Previewing footprint: " + qstr(footprint.name));
  } catch (const std::exception& e) {
    warnUser("Import Failed", qstr(e.what()));
  }
}

void ReviewWindow::previewSymbol() {
  const QString path = QFileDialog::getOpenFileName(this, "Select KiCad Symbol", "", "KiCad Symbol (*.kicad_sym)");
  if (path.isEmpty()) return;
  try {
    const std::string content = readFile(path.toStdString());
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(content);
    if (symbols.empty()) {
      warnUser("Import Failed", "No symbols found in file.");
      return;
    }
    const ccad::CanvasScene scene = ccad::buildCanvasScene(symbols.front());
    renderCanvas(scene);
    statusBar()->showMessage("Previewing symbol: " + qstr(symbols.front().name));
  } catch (const std::exception& e) {
    warnUser("Import Failed", qstr(e.what()));
  }
}

void ReviewWindow::loadFootprintPreview(const std::filesystem::path& path) {
  try {
    const std::string content = readFile(path);
    const ccad::Footprint footprint = ccad::importKiCadFootprint(content);
    const ccad::CanvasScene scene = ccad::buildCanvasScene(footprint);
    renderCanvas(scene);
    statusBar()->showMessage("Previewing footprint: " + qstr(footprint.name));
  } catch (const std::exception& e) {
    std::cerr << "Import Failed: " << e.what() << "\n";
  }
}

void ReviewWindow::loadSymbolPreview(const std::filesystem::path& path) {
  try {
    const std::string content = readFile(path);
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(content);
    if (!symbols.empty()) {
      const ccad::CanvasScene scene = ccad::buildCanvasScene(symbols.front());
      renderCanvas(scene);
      statusBar()->showMessage("Previewing symbol: " + qstr(symbols.front().name));
    }
  } catch (const std::exception& e) {
    std::cerr << "Import Failed: " << e.what() << "\n";
  }
}

void ReviewWindow::exportDrcReport() {
  if (current_path_.empty()) {
    warnUser("Export DRC Report", "Load a project before exporting DRC.");
    return;
  }
  const QString selected = QFileDialog::getSaveFileName(
      this, "Export DRC Report", qstr(current_path_.stem().string() + ".drc.json"),
      "JSON Files (*.json);;All files (*)");
  if (selected.isEmpty()) {
    return;
  }
  const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
  std::ostringstream out;
  out << "{\n  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < diagnostics.size(); ++i) {
    const ccad::Diagnostic& diagnostic = diagnostics.at(i);
    out << "    {\n";
    out << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\",\n";
    out << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n";
    out << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n";
    out << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\"\n";
    out << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << "\n";
  }
  out << "  ]\n}\n";
  try {
    writeFile(selected.toStdString(), out.str());
    statusBar()->showMessage("Exported DRC report");
  } catch (const std::exception& e) {
    criticalUser("Export failed", QString::fromStdString(e.what()));
  }
}

void ReviewWindow::showComponentWizard() {
  ComponentWizardDialog dialog(this);
  if (agent_panel_) {
      dialog.setAgentPanel(agent_panel_);
      agent_panel_->setComponentWizardCallback([&dialog](const QJsonObject& data) {
          QMetaObject::invokeMethod(&dialog, [&dialog, data]() {
              dialog.updatePins(data);
          }, Qt::QueuedConnection);
      });
  }
  
  if (dialog.exec() == QDialog::Accepted) {
    if (agent_panel_) {
        agent_panel_->setComponentWizardCallback(nullptr);
    }
    QString type = dialog.getComponentType();
    QString name = dialog.getComponentName();
    int pins = dialog.getPinCount();
    
    if (name.isEmpty()) {
      warnUser("Validation Error", "Component name cannot be empty.");
      return;
    }

    QString default_out = name + (type == "symbol" ? ".json" : ".json");
    QString filename = QFileDialog::getSaveFileName(
        this, "Save Component JSON", default_out, "JSON Files (*.json)");
        
    if (filename.isEmpty()) return;
    
    std::string json_data;
    if (type == "symbol") {
      ccad::SymbolParams params;
      params.name = name.toStdString();
      params.pin_count = pins;
      ccad::Symbol sym = ccad::generateParametricSymbol(params);
      json_data = ccad::dumpSymbolsJson({sym});
    } else {
      ccad::FootprintParams params;
      params.name = name.toStdString();
      params.pin_count = pins;
      params.package_type = dialog.getPackageType().toStdString();
      ccad::Footprint fp = ccad::generateParametricFootprint(params);
      json_data = ccad::dumpFootprintJson(fp);
    }
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&file);
      out << QString::fromStdString(json_data);
      QMessageBox::information(this, "Success", "Component saved successfully.");
    } else {
      criticalUser("Error", "Failed to save file.");
    }
  }
}

void ReviewWindow::cancelInteractionMode() {
  for (QGraphicsItem* item : interaction_ghost_items_) {
    if (item->scene() != nullptr) {
      item->scene()->removeItem(item);
    }
    delete item;
  }
  interaction_ghost_items_.clear();
  interaction_mode_ = InteractionMode::Default;
  interaction_component_id_.clear();
  interaction_board_text_.clear();
  interaction_layer_id_.clear();
  interaction_rotation_degrees_ = 0.0;
  interaction_has_anchor_ = false;
  canvas_view_->viewport()->unsetCursor();
  schematic_view_->viewport()->unsetCursor();
}

void ReviewWindow::enterPlaceFootprintMode(const std::string& component_id, const ccad::Footprint& footprint, const std::string& layer_id) {
  cancelInteractionMode();
  interaction_mode_ = InteractionMode::PlaceFootprint;
  interaction_component_id_ = component_id;
  interaction_footprint_ = footprint;
  interaction_layer_id_ = layer_id;

  try {
    addPadPreviewItems(*canvas_scene_, interaction_ghost_items_, footprint);
    interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
    moveGhostTo(*canvas_view_, interaction_ghost_items_, interaction_last_mouse_pos_);
    tool_status_->setText("Tool Place Footprint");
  } catch (const std::exception& e) {
    warnUser("Error", "Failed to load footprint for placement preview: " + QString(e.what()));
    cancelInteractionMode();
  }
}

void ReviewWindow::enterPlaceSymbolMode(const std::string& component_id, const ccad::Symbol& symbol, double rotation_degrees) {
  cancelInteractionMode();
  interaction_mode_ = InteractionMode::PlaceSymbol;
  interaction_component_id_ = component_id;
  interaction_symbol_ = symbol;
  interaction_rotation_degrees_ = rotation_degrees;

  try {
    addSymbolPreviewItems(*schematic_scene_, interaction_ghost_items_, symbol, QColor("#008484"));
    interaction_last_mouse_pos_ =
        schematic_view_->mapToScene(schematic_view_->mapFromGlobal(QCursor::pos()));
    moveGhostTo(*schematic_view_, interaction_ghost_items_, interaction_last_mouse_pos_);
    tool_status_->setText("Tool Place Symbol");
  } catch (const std::exception& e) {
    warnUser("Error", "Failed to load symbol for placement preview: " + QString(e.what()));
    cancelInteractionMode();
  }
}

void ReviewWindow::enterMoveFootprintMode(const std::string& component_id) {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) return;
  interaction_mode_ = InteractionMode::MoveFootprint;
  interaction_component_id_ = component_id;
  interaction_start_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
  interaction_last_mouse_pos_ = interaction_start_mouse_pos_;

  for (const auto& pad : project_cache_.boards[0].pads) {
    if (pad.component_id != component_id) {
      continue;
    }
    ccad::Size pad_size = ccad::Size{.width = ccad::millimeters(0), .height = ccad::millimeters(0)};
    std::string pad_shape = "rect";
    double roundrect_rratio = 0.0;
    double chamfer_ratio = 0.0;
    if (!pad.padstack.copper_props.empty()) {
        pad_size = pad.padstack.copper_props.begin()->second.shape.size;
        auto shape_enum = pad.padstack.copper_props.begin()->second.shape.shape;
        if (shape_enum == ccad::PadShape::Circle) pad_shape = "circle";
        else if (shape_enum == ccad::PadShape::Rectangle) pad_shape = "rect";
        else if (shape_enum == ccad::PadShape::RoundRect) pad_shape = "roundrect";
        else if (shape_enum == ccad::PadShape::Oval) pad_shape = "oval";
        else if (shape_enum == ccad::PadShape::ChamferedRect) pad_shape = "chamfered_rect";
        else if (shape_enum == ccad::PadShape::Trapezoid) pad_shape = "trapezoid";
        roundrect_rratio = pad.padstack.copper_props.begin()->second.shape.roundrect_rratio;
        chamfer_ratio = pad.padstack.copper_props.begin()->second.shape.chamfer_ratio;
    }
    const double w = pad_size.width.nanometers / 1e6;
    const double h = pad_size.height.nanometers / 1e6;
    const double x = pad.position.x.nanometers / 1e6;
    const double y = pad.position.y.nanometers / 1e6;
    const QPointF scene_center = boardPositionToScene(project_cache_.boards[0], x, y);
    auto* item = canvas_scene_->addPath(padPreviewPath(scene_center.x() / 10.0,
                                                       scene_center.y() / 10.0, w, h,
                                                       pad_shape, pad.rotation_degrees,
                                                       roundrect_rratio, chamfer_ratio),
                                        QPen(QColor(100, 180, 80), 1.0),
                                        QBrush(QColor(100, 255, 100, 150)));
    item->setZValue(1000);
    interaction_ghost_items_.push_back(item);
    if (pad.padstack.drill.size.width.nanometers > 0) {
      constexpr double scale = 10.0;
      const double drill = pad.padstack.drill.size.width.nanometers / 1e6 * scale;
      auto* drill_item = canvas_scene_->addEllipse(scene_center.x() - (drill / 2.0),
                                                   scene_center.y() - (drill / 2.0), drill, drill,
                                                   QPen(Qt::NoPen), QBrush(QColor("#07111f")));
      drill_item->setZValue(1001);
      interaction_ghost_items_.push_back(drill_item);
    }
  }
}

void ReviewWindow::enterAddViaMode() {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before placing vias.");
    return;
  }
  const std::string layer_id = activePcbLayerOrDefault();
  if (layer_id.empty()) {
    warnUser("No Copper Layer", "No copper layer is available for via placement.");
    return;
  }
  editor_tabs_->setCurrentWidget(canvas_view_);
  interaction_mode_ = InteractionMode::AddVia;
  interaction_layer_id_ = layer_id;
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));

  const CanvasRenderTheme theme;
  const QColor copper = colorForKiCadLayer(theme, layer_id);
  constexpr double scale = 10.0;
  const double diameter = defaultViaDiameter().nanometers / 1e6 * scale;
  auto* item = canvas_scene_->addEllipse(interaction_last_mouse_pos_.x() - (diameter / 2.0),
                                         interaction_last_mouse_pos_.y() - (diameter / 2.0),
                                         diameter, diameter,
                                         QPen(copper.lighter(135), 1.2),
                                         QBrush(QColor(copper.red(), copper.green(),
                                                       copper.blue(), 150)));
  item->setZValue(1000.0);
  interaction_ghost_items_.push_back(item);
  canvas_view_->viewport()->setCursor(Qt::CrossCursor);
  const std::string net_id = activePcbNetOrDefault();
  tool_status_->setText(net_id.empty() ? QString("Tool Add Via")
                                       : QString("Tool Add Via ") + qstr(net_id));
}

void ReviewWindow::enterRouteTrackMode() {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before routing tracks.");
    return;
  }
  const std::string layer_id = activePcbLayerOrDefault();
  if (layer_id.empty() && editor_tabs_->currentWidget() == canvas_view_) {
    warnUser("No Copper Layer", "No copper layer is available for track routing.");
    return;
  }
  
  QGraphicsView* active_view = (editor_tabs_->currentWidget() == schematic_view_) ? schematic_view_ : canvas_view_;
  editor_tabs_->setCurrentWidget(active_view);
  
  interaction_mode_ = InteractionMode::RouteTrack;
  interaction_layer_id_ = layer_id;
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = active_view->mapToScene(active_view->mapFromGlobal(QCursor::pos()));
  active_view->viewport()->setCursor(Qt::CrossCursor);
  const std::string net_id = activePcbNetOrDefault();
  tool_status_->setText(net_id.empty() ? QString("Tool Route Track")
                                       : QString("Tool Route Track ") + qstr(net_id));
}

void ReviewWindow::enterAddZoneMode() {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before drawing zones.");
    return;
  }
  const std::string layer_id = activePcbLayerOrDefault();
  if (layer_id.empty()) {
    warnUser("No Copper Layer", "No copper layer is available for zone drawing.");
    return;
  }
  editor_tabs_->setCurrentWidget(canvas_view_);
  interaction_mode_ = InteractionMode::AddZone;
  interaction_layer_id_ = layer_id;
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
  canvas_view_->viewport()->setCursor(Qt::CrossCursor);
  const std::string net_id = activePcbNetOrDefault();
  tool_status_->setText(net_id.empty() ? QString("Tool Add Zone ") + qstr(layer_id)
                                       : QString("Tool Add Zone ") + qstr(net_id) + " " +
                                             qstr(layer_id));
}

void ReviewWindow::enterAddKeepoutMode() {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before drawing keepouts.");
    return;
  }
  editor_tabs_->setCurrentWidget(canvas_view_);
  interaction_mode_ = InteractionMode::AddKeepout;
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
  canvas_view_->viewport()->setCursor(Qt::CrossCursor);
  tool_status_->setText("Tool Add Keepout");
}

void ReviewWindow::enterDrawGraphicMode() {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before drawing graphics.");
    return;
  }
  const std::string layer_id =
      defaultGraphicLayerId(project_cache_.boards[0], activePcbLayerOrDefault());
  if (layer_id.empty()) {
    warnUser("No Layer", "No board layer is available for graphic placement.");
    return;
  }
  editor_tabs_->setCurrentWidget(canvas_view_);
  interaction_mode_ = InteractionMode::DrawGraphic;
  interaction_layer_id_ = layer_id;
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
  canvas_view_->viewport()->setCursor(Qt::CrossCursor);
  tool_status_->setText(QString("Tool Draw Graphic ") + qstr(layer_id));
}

void ReviewWindow::enterPlaceTextMode(const QString& text) {
  cancelInteractionMode();
  if (!!project_cache_.boards.empty()) {
    warnUser("No Board", "Load a project with a board before placing text.");
    return;
  }
  const std::string layer_id =
      defaultBoardTextLayerId(project_cache_.boards[0], activePcbLayerOrDefault());
  if (layer_id.empty()) {
    warnUser("No Layer", "No board layer is available for text placement.");
    return;
  }
  editor_tabs_->setCurrentWidget(canvas_view_);
  interaction_mode_ = InteractionMode::PlaceText;
  interaction_layer_id_ = layer_id;
  interaction_board_text_ = text.trimmed().isEmpty() ? QString("TEXT") : text.trimmed();
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));

  const CanvasRenderTheme theme;
  const QColor layer_color = colorForKiCadLayer(theme, layer_id);
  QFont font;
  font.setPointSizeF(12.0);
  auto* item = canvas_scene_->addText(interaction_board_text_, font);
  item->setDefaultTextColor(layer_color.lighter(130));
  item->setPos(interaction_last_mouse_pos_);
  item->setZValue(1000.0);
  interaction_ghost_items_.push_back(item);
  canvas_view_->viewport()->setCursor(Qt::CrossCursor);
  tool_status_->setText(QString("Tool Place Text ") + qstr(layer_id));
}

void ReviewWindow::enterAddWireMode() {
  cancelInteractionMode();
  if (!!project_cache_.schematics.empty()) {
    warnUser("No Schematic", "Load a project with a schematic before adding wires.");
    return;
  }
  
  editor_tabs_->setCurrentWidget(schematic_view_);
  
  interaction_mode_ = InteractionMode::AddWire;
  interaction_layer_id_ = "";
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = schematic_view_->mapToScene(schematic_view_->mapFromGlobal(QCursor::pos()));
  schematic_view_->viewport()->setCursor(Qt::CrossCursor);
  tool_status_->setText("Tool Add Wire");
}

void ReviewWindow::enterAddLabelMode(const QString& text) {
  cancelInteractionMode();
  if (!!project_cache_.schematics.empty()) {
    warnUser("No Schematic", "Load a project with a schematic before adding labels.");
    return;
  }
  
  editor_tabs_->setCurrentWidget(schematic_view_);
  interaction_mode_ = InteractionMode::AddLabel;
  interaction_board_text_ = text.trimmed().isEmpty() ? QString("LABEL") : text.trimmed();
  interaction_has_anchor_ = false;
  interaction_last_mouse_pos_ = schematic_view_->mapToScene(schematic_view_->mapFromGlobal(QCursor::pos()));

  QFont font;
  font.setPointSizeF(12.0);
  auto* item = schematic_scene_->addText(interaction_board_text_, font);
  item->setDefaultTextColor(Qt::yellow);
  item->setPos(interaction_last_mouse_pos_);
  item->setZValue(1000.0);
  interaction_ghost_items_.push_back(item);
  schematic_view_->viewport()->setCursor(Qt::CrossCursor);
  tool_status_->setText("Tool Add Label");
}

void ReviewWindow::createRouteOrKeepoutGhost(const QPointF& scene_position) {
  if (canvas_scene_ == nullptr) {
    return;
  }
  if (!interaction_ghost_items_.empty()) {
    updateRouteOrKeepoutGhost(scene_position);
    return;
  }
  QGraphicsPathItem* item = nullptr;
  if (interaction_mode_ == InteractionMode::RouteTrack ||
      interaction_mode_ == InteractionMode::DrawGraphic) {
    const CanvasRenderTheme theme;
    const QColor color = colorForKiCadLayer(theme, interaction_layer_id_);
    QPen pen(color.lighter(130),
             interaction_mode_ == InteractionMode::DrawGraphic ? 1.6 : 2.0);
    pen.setCapStyle(Qt::RoundCap);
    item = canvas_scene_->addPath(sceneTrackPath(interaction_start_mouse_pos_, scene_position),
                                  pen, QBrush(Qt::NoBrush));
  } else if (interaction_mode_ == InteractionMode::AddZone ||
             interaction_mode_ == InteractionMode::AddKeepout) {
    const CanvasRenderTheme theme;
    const QColor color = interaction_mode_ == InteractionMode::AddZone
                             ? colorForKiCadLayer(theme, interaction_layer_id_)
                             : QColor("#f97316");
    QPen pen(color, 1.2, interaction_mode_ == InteractionMode::AddZone ? Qt::SolidLine
                                                                       : Qt::DashLine);
    item = canvas_scene_->addPath(
        sceneKeepoutPath(interaction_start_mouse_pos_, scene_position), pen,
        QBrush(QColor(color.red(), color.green(), color.blue(),
                      interaction_mode_ == InteractionMode::AddZone ? 70 : 48)));
  } else if (interaction_mode_ == InteractionMode::AddWire) {
    QPen pen(Qt::green, 2.0);
    pen.setCapStyle(Qt::RoundCap);
    item = schematic_scene_->addPath(sceneTrackPath(interaction_start_mouse_pos_, scene_position),
                                     pen, QBrush(Qt::NoBrush));
  }
  if (item != nullptr) {
    item->setZValue(1000.0);
    interaction_ghost_items_.push_back(item);
  }
}

void ReviewWindow::updateRouteOrKeepoutGhost(const QPointF& scene_position) {
  if (interaction_ghost_items_.empty()) {
    return;
  }
  auto* item = dynamic_cast<QGraphicsPathItem*>(interaction_ghost_items_.front());
  if (item == nullptr) {
    return;
  }
  if (interaction_mode_ == InteractionMode::RouteTrack ||
      interaction_mode_ == InteractionMode::DrawGraphic) {
    item->setPath(sceneTrackPath(interaction_start_mouse_pos_, scene_position));
  } else if (interaction_mode_ == InteractionMode::AddZone ||
             interaction_mode_ == InteractionMode::AddKeepout) {
    item->setPath(sceneKeepoutPath(interaction_start_mouse_pos_, scene_position));
  } else if (interaction_mode_ == InteractionMode::AddWire) {
    item->setPath(sceneTrackPath(interaction_start_mouse_pos_, scene_position));
  }
}

bool ReviewWindow::eventFilter(QObject* obj, QEvent* event) {
  if (interaction_mode_ != InteractionMode::Default) {
    QGraphicsView* active_view = (interaction_mode_ == InteractionMode::PlaceSymbol ||
                                  interaction_mode_ == InteractionMode::AddWire ||
                                  interaction_mode_ == InteractionMode::AddLabel) ? schematic_view_
                                                                                  : canvas_view_;
    if (obj == active_view->viewport()) {
      if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPointF scene_pos = active_view->mapToScene(me->pos());
        if ((interaction_mode_ == InteractionMode::RouteTrack ||
             interaction_mode_ == InteractionMode::DrawGraphic ||
             interaction_mode_ == InteractionMode::AddZone ||
             interaction_mode_ == InteractionMode::AddKeepout ||
             interaction_mode_ == InteractionMode::AddWire) &&
            interaction_has_anchor_) {
          updateRouteOrKeepoutGhost(scene_pos);
        } else {
          QPointF delta = scene_pos - interaction_last_mouse_pos_;
          for (QGraphicsItem* item : interaction_ghost_items_) {
            item->setPos(item->pos() + delta);
          }
        }
        interaction_last_mouse_pos_ = scene_pos;
        return true; // Consume event
      } else if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
          // Finalize placement
          QPointF scene_pos = active_view->mapToScene(me->pos());

          if (interaction_mode_ == InteractionMode::PlaceFootprint) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot place footprint without a board");
              }
              pushUndoSnapshot();
              ccad::placeFootprint(
                  project_cache_, interaction_footprint_, interaction_component_id_,
                  boardPointFromScene(project_cache_.boards[0], scene_pos),
                  0.0, interaction_layer_id_);
              // Save the project
              std::ofstream out(current_path_);
              if (out) {
                out << ccad::dumpProjectJson(project_cache_);
                out.close();
                if (out) {
                  cancelInteractionMode();
                  reloadProject();
                } else {
                  criticalUser("Save Error", "Failed to write project file.");
                }
              } else {
                criticalUser("Save Error", "Failed to write project file.");
              }
            } catch (const std::exception& e) {
              criticalUser("Placement Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::PlaceSymbol) {
            try {
              pushUndoSnapshot();
              ccad::placeComponent(project_cache_, interaction_symbol_, interaction_component_id_,
                                   schematicPointFromScene(scene_pos),
                                   interaction_rotation_degrees_);
              std::ofstream out(current_path_);
              if (out) {
                out << ccad::dumpProjectJson(project_cache_);
                out.close();
                if (out) {
                  cancelInteractionMode();
                  reloadProject();
                } else {
                  criticalUser("Save Error", "Failed to write project file.");
                }
              } else {
                criticalUser("Save Error", "Failed to write project file.");
              }
            } catch (const std::exception& e) {
              criticalUser("Placement Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::AddLabel) {
            try {
              if (!!project_cache_.schematics.empty()) {
                throw std::runtime_error("cannot place label without a schematic");
              }
              pushUndoSnapshot();
              ccad::Schematic& sch = project_cache_.schematics[0];
              sch.labels.push_back(ccad::SchLabel{.id = "label_" + std::to_string(sch.labels.size()),
                                                  .text = interaction_board_text_.toStdString(),
                                                  .net_id = interaction_board_text_.toStdString(),
                                                  .position = schematicPointFromScene(scene_pos),
                                                  .rotation_degrees = 0.0,
                                                  .type = ccad::LabelType::Local});
              cancelInteractionMode();
              saveProjectCacheAfterMutation(QString::fromStdString("Placed label " + sch.labels.back().id));
            } catch (const std::exception& e) {
              criticalUser("Label Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::PlaceText) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot place text without a board");
              }
              pushUndoSnapshot();
              ccad::Board& board = project_cache_.boards[0];
              board.texts.push_back(ccad::BoardText{.id = nextBoardTextId(board),
                                                    .layer_id = interaction_layer_id_,
                                                    .text = interaction_board_text_.toStdString(),
                                                    .position = boardPointFromScene(board, scene_pos),
                                                    .rotation_degrees = 0.0,
                                                    .size = defaultBoardTextSize()});
              const QString text_id = qstr(board.texts.back().id);
              cancelInteractionMode();
              saveProjectCacheAfterMutation("Placed text " + text_id);
              selectCanvasObjectById(*canvas_scene_, text_id);
            } catch (const std::exception& e) {
              criticalUser("Text Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::MoveFootprint) {
            try {
              QPointF delta = scene_pos - interaction_start_mouse_pos_;
              pushUndoSnapshot();
              ccad::moveFootprint(project_cache_, interaction_component_id_,
                                  boardDeltaFromSceneDelta(delta));
              std::ofstream out(current_path_);
              if (out) {
                out << ccad::dumpProjectJson(project_cache_);
                out.close();
                if (out) {
                  cancelInteractionMode();
                  reloadProject();
                } else {
                  criticalUser("Save Error", "Failed to write project file.");
                }
              }
            } catch (const std::exception& e) {
              criticalUser("Move Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::AddVia) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot place via without a board");
              }
              pushUndoSnapshot();
              ccad::Board& board = project_cache_.boards[0];
              board.vias.push_back(ccad::Via{.id = nextViaId(board),
                                             .net_id = activePcbNetOrDefault(),
                                             .position = boardPointFromScene(board, scene_pos),
                                             .diameter = defaultViaDiameter(),
                                             .drill = defaultViaDrill()});
              const QString via_id = qstr(board.vias.back().id);
              cancelInteractionMode();
              saveProjectCacheAfterMutation("Placed via " + via_id);
              selectCanvasObjectById(*canvas_scene_, via_id);
            } catch (const std::exception& e) {
              criticalUser("Via Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::RouteTrack) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot route track without a board");
              }
              if (!interaction_has_anchor_) {
                interaction_start_mouse_pos_ = scene_pos;
                interaction_last_mouse_pos_ = scene_pos;
                interaction_has_anchor_ = true;
                createRouteOrKeepoutGhost(scene_pos);
                tool_status_->setText("Tool Route Track Anchor");
                return true;
              }
              pushUndoSnapshot();
              ccad::Board& board = project_cache_.boards[0];
              board.tracks.push_back(
                  ccad::TrackSegment{.id = nextTrackId(board),
                                     .net_id = activePcbNetOrDefault(),
                                     .layer_id = interaction_layer_id_,
                                     .start = boardPointFromScene(board, interaction_start_mouse_pos_),
                                     .end = boardPointFromScene(board, scene_pos),
                                     .width = defaultTrackWidth(board),
                                     .source_route_request_id = ""});
              const QString track_id = qstr(board.tracks.back().id);
              cancelInteractionMode();
              saveProjectCacheAfterMutation("Routed track " + track_id);
              selectCanvasObjectById(*canvas_scene_, track_id);
            } catch (const std::exception& e) {
              criticalUser("Track Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::AddWire) {
            try {
              if (!!project_cache_.schematics.empty()) {
                throw std::runtime_error("cannot route wire without a schematic");
              }
              if (!interaction_has_anchor_) {
                interaction_start_mouse_pos_ = scene_pos;
                interaction_last_mouse_pos_ = scene_pos;
                interaction_has_anchor_ = true;
                createRouteOrKeepoutGhost(scene_pos);
                tool_status_->setText("Tool Add Wire Anchor");
                return true;
              }
              pushUndoSnapshot();
              ccad::Schematic& sch = project_cache_.schematics[0];
              sch.wires.push_back(
                  ccad::SchWire{.id = "wire_" + std::to_string(sch.wires.size()),
                                .start = schematicPointFromScene(interaction_start_mouse_pos_),
                                .end = schematicPointFromScene(scene_pos),
                                .net_id = ""});
              cancelInteractionMode();
              saveProjectCacheAfterMutation(QString::fromStdString("Added wire " + sch.wires.back().id));
            } catch (const std::exception& e) {
              criticalUser("Wire Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::DrawGraphic) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot draw graphic without a board");
              }
              if (!interaction_has_anchor_) {
                interaction_start_mouse_pos_ = scene_pos;
                interaction_last_mouse_pos_ = scene_pos;
                interaction_has_anchor_ = true;
                createRouteOrKeepoutGhost(scene_pos);
                tool_status_->setText("Tool Draw Graphic Anchor");
                return true;
              }
              const ccad::Point start =
                  boardPointFromScene(project_cache_.boards[0], interaction_start_mouse_pos_);
              const ccad::Point end = boardPointFromScene(project_cache_.boards[0], scene_pos);
              if (start.x.nanometers == end.x.nanometers &&
                  start.y.nanometers == end.y.nanometers) {
                throw std::runtime_error("graphic line requires distinct start and end points");
              }
              pushUndoSnapshot();
              ccad::Board& board = project_cache_.boards[0];
              board.graphics.push_back(ccad::BoardGraphic{.id = nextGraphicId(board),
                                                          .kind = "line",
                                                          .layer_id = interaction_layer_id_,
                                                          .start = start,
                                                          .end = end,
                                                          .width = defaultGraphicWidth()});
              const QString graphic_id = qstr(board.graphics.back().id);
              cancelInteractionMode();
              saveProjectCacheAfterMutation("Drew graphic " + graphic_id);
              selectCanvasObjectById(*canvas_scene_, graphic_id);
            } catch (const std::exception& e) {
              criticalUser("Graphic Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::AddZone) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot draw zone without a board");
              }
              if (!interaction_has_anchor_) {
                interaction_start_mouse_pos_ = scene_pos;
                interaction_last_mouse_pos_ = scene_pos;
                interaction_has_anchor_ = true;
                createRouteOrKeepoutGhost(scene_pos);
                tool_status_->setText("Tool Add Zone Anchor");
                return true;
              }
              const ccad::Point start = boardPointFromScene(project_cache_.boards[0],
                                                            interaction_start_mouse_pos_);
              const ccad::Point end = boardPointFromScene(project_cache_.boards[0], scene_pos);
              const std::int64_t min_x = std::min(start.x.nanometers, end.x.nanometers);
              const std::int64_t min_y = std::min(start.y.nanometers, end.y.nanometers);
              const std::int64_t max_x = std::max(start.x.nanometers, end.x.nanometers);
              const std::int64_t max_y = std::max(start.y.nanometers, end.y.nanometers);
              if (max_x == min_x || max_y == min_y) {
                throw std::runtime_error("zone requires a non-zero rectangle");
              }
              pushUndoSnapshot();
              ccad::Board& board = project_cache_.boards[0];
              const std::string zone_id = nextZoneId(board);
              board.zones.push_back(ccad::BoardZone{
                  .id = zone_id,
                  .name = "Zone " + zone_id,
                  .net_id = activePcbNetOrDefault(),
                  .layer_ids = {interaction_layer_id_},
                  .outline = {ccad::Point{.x = ccad::nanometers(min_x),
                                          .y = ccad::nanometers(min_y)},
                              ccad::Point{.x = ccad::nanometers(max_x),
                                          .y = ccad::nanometers(min_y)},
                              ccad::Point{.x = ccad::nanometers(max_x),
                                          .y = ccad::nanometers(max_y)},
                              ccad::Point{.x = ccad::nanometers(min_x),
                                          .y = ccad::nanometers(max_y)}},
                  .priority = 0,
                  .clearance = defaultZoneClearance(board),
                  .min_thickness = defaultZoneMinThickness(board),
                  .fill_enabled = true,
                  .pad_connection = "thermal"});
              const QString zone_qid = qstr(board.zones.back().id);
              cancelInteractionMode();
              saveProjectCacheAfterMutation("Added zone " + zone_qid);
              selectCanvasObjectById(*canvas_scene_, zone_qid);
            } catch (const std::exception& e) {
              criticalUser("Zone Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::AddKeepout) {
            try {
              if (!!project_cache_.boards.empty()) {
                throw std::runtime_error("cannot draw keepout without a board");
              }
              if (!interaction_has_anchor_) {
                interaction_start_mouse_pos_ = scene_pos;
                interaction_last_mouse_pos_ = scene_pos;
                interaction_has_anchor_ = true;
                createRouteOrKeepoutGhost(scene_pos);
                tool_status_->setText("Tool Add Keepout Anchor");
                return true;
              }
              const ccad::Point start = boardPointFromScene(project_cache_.boards[0],
                                                            interaction_start_mouse_pos_);
              const ccad::Point end = boardPointFromScene(project_cache_.boards[0], scene_pos);
              const std::int64_t min_x = std::min(start.x.nanometers, end.x.nanometers);
              const std::int64_t min_y = std::min(start.y.nanometers, end.y.nanometers);
              const std::int64_t width = std::llabs(end.x.nanometers - start.x.nanometers);
              const std::int64_t height = std::llabs(end.y.nanometers - start.y.nanometers);
              if (width == 0 || height == 0) {
                throw std::runtime_error("keepout requires a non-zero rectangle");
              }
              pushUndoSnapshot();
              ccad::Board& board = project_cache_.boards[0];
              board.keepouts.push_back(ccad::Keepout{
                  .id = nextKeepoutId(board),
                  .kind = "routing",
                  .area = ccad::Rect{.origin = ccad::Point{.x = ccad::nanometers(min_x),
                                                           .y = ccad::nanometers(min_y)},
                                     .size = ccad::Size{.width = ccad::nanometers(width),
                                                        .height = ccad::nanometers(height)}}});
              const QString keepout_id = qstr(board.keepouts.back().id);
              cancelInteractionMode();
              saveProjectCacheAfterMutation("Added keepout " + keepout_id);
              selectCanvasObjectById(*canvas_scene_, keepout_id);
            } catch (const std::exception& e) {
              criticalUser("Keepout Error", QString::fromUtf8(e.what()));
            }
          }
          if (interaction_mode_ != InteractionMode::Default) {
            cancelInteractionMode();
          }
          return true; // Consume event
        }
      }
    }
  } else {
    // If not in a specific interaction mode, handle general hotkeys if applicable
    if (event->type() == QEvent::KeyPress) {
      auto* ke = static_cast<QKeyEvent*>(event);
      if (ke->key() == Qt::Key_M) {
        auto selected = canvas_scene_->selectedItems();
        if (!selected.empty() && !project_cache_.boards.empty()) {
          QString object_id = selected.first()->data(0).toString();
          for (const auto& pad : project_cache_.boards[0].pads) {
            if (pad.id == object_id.toStdString()) {
              enterMoveFootprintMode(pad.component_id);
              return true;
            }
          }
        }
      }
    }
  }

  if (interaction_mode_ != InteractionMode::Default && event->type() == QEvent::KeyPress) {
    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->key() == Qt::Key_Escape) {
      cancelInteractionMode();
      return true; // Consume event
    }
  }
  return QMainWindow::eventFilter(obj, event);
}
