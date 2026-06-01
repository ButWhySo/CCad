#include "ccad_gui/library_browser_dialog.hpp"

#include "ccad_core/canvas.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/kicad_symbol_import.hpp"
#include "ccad_gui/board_canvas_renderer.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSplitter>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QStringList>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QFrame>
#include <QGraphicsTextItem>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace {

std::string readCacheFile(const QString& path) {
  std::ifstream input(path.toStdString());
  if (!input) {
    throw std::runtime_error("failed to open " + path.toStdString());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

ccad::Footprint loadFootprintFile(const QFileInfo& file) {
  const std::string content = readCacheFile(file.absoluteFilePath());
  return file.suffix().compare("kicad_mod", Qt::CaseInsensitive) == 0
             ? ccad::importKiCadFootprint(content)
             : ccad::loadFootprintJson(content);
}

ccad::Symbol loadSymbolFile(const QFileInfo& file) {
  if (file.suffix().compare("kicad_sym", Qt::CaseInsensitive) == 0) {
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(readCacheFile(file.absoluteFilePath()));
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
  return ccad::loadSymbolJsonFileWithLocalInheritance(file.absoluteFilePath().toStdString());
}

QString describeFootprintFile(const QFileInfo& file) {
  try {
    const ccad::Footprint footprint = loadFootprintFile(file);
    return QString::number(footprint.pads.size()) + " pads";
  } catch (...) {
    return "Footprint";
  }
}

QString describeSymbolFile(const QFileInfo& file) {
  try {
    const ccad::Symbol symbol = loadSymbolFile(file);
    return QString::number(symbol.pins.size()) + " pins";
  } catch (...) {
    return "Symbol";
  }
}

QString catalogueDescription(const QFileInfo& file, LibraryType type) {
  const QString suffix = file.suffix().toLower();
  if (type == LibraryType::Footprint) {
    if (suffix == "kicad_mod") {
      return "KiCad footprint";
    }
    if (suffix == "json") {
      return "CCad footprint JSON";
    }
    return "Footprint";
  }
  if (suffix == "kicad_sym") {
    return "KiCad symbol library";
  }
  if (suffix == "json") {
    return "CCad symbol JSON";
  }
  return "Symbol";
}

QString selectedMetadata(const QFileInfo& file, LibraryType type) {
  try {
    return type == LibraryType::Footprint ? describeFootprintFile(file) : describeSymbolFile(file);
  } catch (const std::exception& error) {
    return "Metadata unavailable: " + QString::fromStdString(error.what());
  }
}

QStringList catalogueNameFilters(LibraryType type) {
  return type == LibraryType::Footprint ? QStringList{"*.json", "*.kicad_mod"}
                                        : QStringList{"*.json", "*.kicad_sym"};
}

QPainterPath padPreviewPath(double x, double y, double w, double h, const std::string& shape,
                            double rotation_degrees, std::optional<double> roundrect_rratio,
                            std::optional<double> chamfer_ratio) {
  QRectF rect(x - (w / 2.0), y - (h / 2.0), w, h);
  const QPointF center = rect.center();
  QPainterPath path;
  if (shape == "rect") {
    path.addRect(rect);
  } else if (shape == "roundrect") {
    const double radius = std::min(w, h) * roundrect_rratio.value_or(0.25);
    path.addRoundedRect(rect, radius, radius);
  } else if (shape == "circle") {
    const double diameter = std::min(w, h);
    path.addEllipse(QRectF(center.x() - (diameter / 2.0), center.y() - (diameter / 2.0), diameter, diameter));
  } else if (shape == "oval") {
    path.addEllipse(rect);
  } else if (shape == "trapezoid") {
    QPolygonF polygon;
    polygon << QPointF(rect.left() + (w * 0.15), rect.top()) << rect.topRight()
            << QPointF(rect.right() - (w * 0.15), rect.bottom()) << rect.bottomLeft();
    path.addPolygon(polygon);
    path.closeSubpath();
  } else if (shape == "chamfered_rect") {
    const double cut = std::min(w, h) * chamfer_ratio.value_or(0.18);
    QPolygonF polygon;
    polygon << QPointF(rect.left() + cut, rect.top()) << rect.topRight()
            << rect.bottomRight() << rect.bottomLeft() << QPointF(rect.left(), rect.top() + cut);
    path.addPolygon(polygon);
    path.closeSubpath();
  } else {
    path.addRect(rect);
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

QPainterPath linePath(double sx, double sy, double ex, double ey) {
  QPainterPath path;
  path.moveTo(sx, sy);
  path.lineTo(ex, ey);
  return path;
}

void fitPreview(QGraphicsView& view, QGraphicsScene& scene) {
  const QRectF bounds = scene.itemsBoundingRect().adjusted(-2.0, -2.0, 2.0, 2.0);
  if (!bounds.isNull()) {
    scene.setSceneRect(bounds);
    view.fitInView(bounds, Qt::KeepAspectRatio);
  }
}

void renderFootprintPreview(QGraphicsScene& scene, const ccad::Footprint& footprint) {
  constexpr double scale = 10.0;
  const CanvasRenderTheme theme;
  QPen copper_pen(colorForKiCadLayer(theme, "F.Cu"), 0.12 * scale);
  copper_pen.setCapStyle(Qt::RoundCap);
  copper_pen.setJoinStyle(Qt::RoundJoin);

  for (const auto& line : footprint.lines) {
    QPen graphic_pen(colorForKiCadLayer(theme, line.layer), 0.08 * scale);
    scene.addPath(linePath(line.start.x.nanometers / 1e6 * scale, line.start.y.nanometers / 1e6 * scale,
                           line.end.x.nanometers / 1e6 * scale, line.end.y.nanometers / 1e6 * scale),
                  graphic_pen, QBrush(Qt::NoBrush));
  }
  for (const auto& circle : footprint.circles) {
    QPen graphic_pen(colorForKiCadLayer(theme, circle.layer), 0.08 * scale);
    const double cx = circle.center.x.nanometers / 1e6 * scale;
    const double cy = circle.center.y.nanometers / 1e6 * scale;
    const double ex = circle.end.x.nanometers / 1e6 * scale;
    const double ey = circle.end.y.nanometers / 1e6 * scale;
    const double radius = std::hypot(ex - cx, ey - cy);
    scene.addEllipse(cx - radius, cy - radius, radius * 2.0, radius * 2.0, graphic_pen, QBrush(Qt::NoBrush));
  }
  for (const auto& polyline : footprint.polylines) {
    QPen graphic_pen(colorForKiCadLayer(theme, polyline.layer), 0.08 * scale);
    QPainterPath path;
    bool started = false;
    for (const auto& point : polyline.points) {
      const double x = point.x.nanometers / 1e6 * scale;
      const double y = point.y.nanometers / 1e6 * scale;
      if (!started) {
        path.moveTo(x, y);
        started = true;
      } else {
        path.lineTo(x, y);
      }
    }
    scene.addPath(path, graphic_pen, QBrush(Qt::NoBrush));
  }
  for (const auto& pad : footprint.pads) {
    std::string primary_layer = "F.Cu";
    for (const std::string& layer : pad.layers) {
      if (layer.ends_with(".Cu") || layer == "*.Cu") {
        primary_layer = layer == "*.Cu" ? "F.Cu" : layer;
        break;
      }
    }
    const QColor pad_color = colorForKiCadLayer(theme, primary_layer);
    copper_pen.setColor(pad_color.lighter(125));
    const QBrush copper_brush(QColor(pad_color.red(), pad_color.green(), pad_color.blue(), 185));
    const double x = pad.position.x.nanometers / 1e6 * scale;
    const double y = pad.position.y.nanometers / 1e6 * scale;
    const double w = pad.size.width.nanometers / 1e6 * scale;
    const double h = pad.size.height.nanometers / 1e6 * scale;
    scene.addPath(padPreviewPath(x, y, w, h, pad.shape, pad.rotation_degrees,
                                 pad.roundrect_rratio, pad.chamfer_ratio),
                  copper_pen, copper_brush);
    if (pad.drill.has_value()) {
      const double drill = pad.drill->nanometers / 1e6 * scale;
      scene.addEllipse(x - (drill / 2.0), y - (drill / 2.0), drill, drill,
                       QPen(Qt::NoPen), QBrush(QColor("#07111f")));
    }
  }
  auto* label = scene.addText(QString::fromStdString(footprint.name));
  label->setDefaultTextColor(colorForKiCadLayer(theme, "F.Fab"));
  label->setPos(scene.itemsBoundingRect().left(), scene.itemsBoundingRect().top() - 18.0);
}

void renderSymbolPreview(QGraphicsScene& scene, const ccad::Symbol& symbol) {
  constexpr double scale = 10.0;
  const ccad::CanvasScene symbol_scene = ccad::buildCanvasScene(symbol);
  QPen pen(QColor("#00806b"), 0.12 * scale);
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  QBrush fill(QColor(0, 128, 107, 38));
  QPen pin_pen(QColor("#b32318"), 0.08 * scale);

  for (const auto& line : symbol_scene.lines) {
    scene.addPath(linePath(line.start_x_units * scale, line.start_y_units * scale,
                           line.end_x_units * scale, line.end_y_units * scale),
                  pen, QBrush(Qt::NoBrush));
  }
  for (const auto& circle : symbol_scene.circles) {
    const double radius = circle.radius_units * scale;
    scene.addEllipse((circle.center_x_units * scale) - radius, (circle.center_y_units * scale) - radius,
                     radius * 2.0, radius * 2.0, pen,
                     circle.fill_type == "solid" ? fill : QBrush(Qt::NoBrush));
  }
  for (const auto& polygon : symbol_scene.polygons) {
    QPolygonF qpolygon;
    for (std::size_t i = 0; i < polygon.pts_x_units.size() && i < polygon.pts_y_units.size(); ++i) {
      qpolygon << QPointF(polygon.pts_x_units.at(i) * scale, polygon.pts_y_units.at(i) * scale);
    }
    QPainterPath path;
    path.addPolygon(qpolygon);
    scene.addPath(path, pen, polygon.fill_type == "solid" ? fill : QBrush(Qt::NoBrush));
  }
  for (const auto& pin : symbol.pins) {
    const double x = pin.position.x.nanometers / 1e6 * scale;
    const double y = pin.position.y.nanometers / 1e6 * scale;
    scene.addEllipse(x - 2.5, y - 2.5, 5.0, 5.0, pin_pen, QBrush(QColor(179, 35, 24, 130)));
  }
  auto* label = scene.addText(QString::fromStdString(symbol.name));
  label->setDefaultTextColor(QColor("#00806b"));
  label->setPos(scene.itemsBoundingRect().left(), scene.itemsBoundingRect().top() - 18.0);
}

}  // namespace

LibraryBrowserDialog::LibraryBrowserDialog(LibraryType type, QWidget* parent)
    : LibraryBrowserDialog(type, QStringLiteral("library-cache"), parent) {}

LibraryBrowserDialog::LibraryBrowserDialog(LibraryType type, const QString& cache_root, QWidget* parent)
    : QDialog(parent), type_(type), cache_root_(cache_root) {
  setWindowTitle(type_ == LibraryType::Footprint ? "Choose Footprint" : "Choose Symbol");
  setMinimumSize(920, 640);

  auto* layout = new QVBoxLayout(this);

  search_edit_ = new QLineEdit(this);
  search_edit_->setPlaceholderText(type_ == LibraryType::Footprint
                                       ? "Filter footprints by library, name, or file..."
                                       : "Filter symbols by library, name, description, or file...");
  layout->addWidget(search_edit_);

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  component_list_ = new QTreeWidget(this);
  component_list_->setAlternatingRowColors(true);
  component_list_->setColumnCount(3);
  component_list_->setHeaderLabels({"Item", "Description", "Library"});
  component_list_->setRootIsDecorated(false);
  component_list_->setSelectionMode(QAbstractItemView::SingleSelection);
  component_list_->setSelectionBehavior(QAbstractItemView::SelectRows);
  component_list_->header()->setStretchLastSection(false);
  component_list_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  component_list_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  component_list_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  splitter->addWidget(component_list_);

  auto* details = new QWidget(splitter);
  auto* details_layout = new QVBoxLayout(details);
  details_layout->setContentsMargins(8, 8, 8, 8);
  detail_label_ = new QLabel("Select an item to view library details.", details);
  detail_label_->setWordWrap(true);
  preview_scene_ = new QGraphicsScene(details);
  preview_view_ = new QGraphicsView(preview_scene_, details);
  preview_view_->setMinimumSize(320, 260);
  preview_view_->setRenderHint(QPainter::Antialiasing, true);
  preview_view_->setBackgroundBrush(QColor("#07111f"));
  preview_view_->setFrameShape(QFrame::StyledPanel);
  preview_status_label_ = new QLabel("Select an item to render a preview.", details);
  preview_status_label_->setWordWrap(true);
  preview_status_label_->setMinimumWidth(260);
  details_layout->addWidget(detail_label_);
  details_layout->addWidget(preview_view_, 1);
  details_layout->addWidget(preview_status_label_);
  splitter->addWidget(details);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  layout->addWidget(splitter, 1);

  auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  ok_button_ = button_box->button(QDialogButtonBox::Ok);
  ok_button_->setEnabled(false);
  layout->addWidget(button_box);

  connect(search_edit_, &QLineEdit::textChanged, this, &LibraryBrowserDialog::filterComponents);
  connect(component_list_, &QTreeWidget::itemSelectionChanged, this, [this]() {
    ok_button_->setEnabled(!component_list_->selectedItems().isEmpty());
    updateDetails();
  });
  connect(component_list_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*, int) {
    onAccept();
  });
  connect(button_box, &QDialogButtonBox::accepted, this, &LibraryBrowserDialog::onAccept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  loadComponents();
}

void LibraryBrowserDialog::loadComponents() {
  const QString dir_path = QDir(cache_root_).filePath(type_ == LibraryType::Footprint ? "footprints" : "symbols");
  QDir dir(dir_path);
  
  if (!dir.exists()) {
    QMessageBox::warning(this, "Library Cache", "Library cache directory not found: " + dir_path);
    return;
  }

  QDirIterator it(dir.absolutePath(), catalogueNameFilters(type_), QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QFileInfo file(it.next());
    const QString relative_library_path = QDir::fromNativeSeparators(dir.relativeFilePath(file.absolutePath()));
    const QString library = relative_library_path.section('/', 0, 0);
    const QString library_name = library == "." ? dir.dirName() : library;
    const QString description = catalogueDescription(file, type_);
    auto* item = new QTreeWidgetItem(component_list_);
    item->setText(0, file.completeBaseName());
    item->setText(1, description);
    item->setText(2, library_name);
    item->setData(0, Qt::UserRole, file.absoluteFilePath());
    item->setData(0, Qt::UserRole + 1, library_name);
    item->setData(0, Qt::UserRole + 2, file.fileName());
  }
}

void LibraryBrowserDialog::filterComponents(const QString& text) {
  for (int i = 0; i < component_list_->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = component_list_->topLevelItem(i);
    const QString haystack = item->text(0) + " " + item->text(1) + " " + item->text(2) + " " +
                             item->data(0, Qt::UserRole + 2).toString();
    item->setHidden(!haystack.contains(text, Qt::CaseInsensitive));
  }
}

void LibraryBrowserDialog::onAccept() {
  auto selected = component_list_->selectedItems();
  if (!selected.isEmpty()) {
    result_ = selected.first()->data(0, Qt::UserRole).toString().toStdString();
    accept();
  }
}

void LibraryBrowserDialog::updateDetails() {
  auto selected = component_list_->selectedItems();
  if (selected.isEmpty()) {
    detail_label_->setText("Select an item to view library details.");
    clearPreview("Select an item to render a preview.");
    return;
  }
  const QTreeWidgetItem* item = selected.first();
  const QFileInfo file(item->data(0, Qt::UserRole).toString());
  const QString library = item->data(0, Qt::UserRole + 1).toString();
  const QString metadata = selectedMetadata(file, type_);
  detail_label_->setText("<b>Library</b>: " + library + "<br><b>Name</b>: " +
                         file.completeBaseName() + "<br><b>File</b>: " + file.fileName() +
                         "<br><b>Metadata</b>: " + metadata + "<br><b>Path</b>: " +
                         file.absoluteFilePath());
  renderSelectedPreview(file.absoluteFilePath());
}

std::optional<std::string> LibraryBrowserDialog::result() const {
  return result_;
}

bool LibraryBrowserDialog::selectFirstVisibleItemForTest() {
  for (int i = 0; i < component_list_->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = component_list_->topLevelItem(i);
    if (!item->isHidden()) {
      component_list_->setCurrentItem(item);
      item->setSelected(true);
      updateDetails();
      return true;
    }
  }
  return false;
}

void LibraryBrowserDialog::clearPreview(const QString& message) {
  if (preview_scene_ != nullptr) {
    preview_scene_->clear();
  }
  if (preview_status_label_ != nullptr) {
    preview_status_label_->setText(message);
  }
}

void LibraryBrowserDialog::renderSelectedPreview(const QString& path) {
  if (preview_scene_ == nullptr || preview_view_ == nullptr) {
    return;
  }
  preview_scene_->clear();
  const QFileInfo file(path);
  try {
    if (type_ == LibraryType::Footprint) {
      renderFootprintPreview(*preview_scene_, loadFootprintFile(file));
      preview_status_label_->setText(
          "Footprint preview. This selected row is parsed lazily; final placement imports the accepted item.");
    } else {
      renderSymbolPreview(*preview_scene_, loadSymbolFile(file));
      preview_status_label_->setText(
          "Symbol preview. This selected row is parsed lazily; final placement resolves inherited pins.");
    }
    fitPreview(*preview_view_, *preview_scene_);
  } catch (const std::exception& error) {
    clearPreview("Preview unavailable: " + QString::fromStdString(error.what()));
  } catch (...) {
    clearPreview("Preview unavailable for this library item.");
  }
}
