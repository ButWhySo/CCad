#include "selection_inspector_panel.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>

namespace {

QString formatLength(ccad::Length length) {
  double mm = static_cast<double>(length.nanometers) / 1000000.0;
  double mil = static_cast<double>(length.nanometers) / 25400.0;
  return QString::number(mm, 'f', 2) + " mm (" + QString::number(mil, 'f', 2) + " mil)";
}

double toMm(ccad::Length length) {
  return static_cast<double>(length.nanometers) / 1000000.0;
}

QString displayType(const QString& type) {
  if (type.isEmpty()) {
    return "--";
  }
  QString result = type;
  result[0] = result[0].toUpper();
  return result;
}

}  // namespace

SelectionInspectorPanel::SelectionInspectorPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  title_ = new QLabel(this);
  title_->setObjectName("inspectorTitle");

  detail_ = new QLabel(this);
  detail_->setObjectName("inspectorDetail");
  detail_->setWordWrap(true);

  form_ = new QFormLayout();
  form_->setContentsMargins(0, 0, 0, 0);
  form_->setSpacing(6);

  layout->addWidget(title_);
  layout->addWidget(detail_);
  layout->addLayout(form_);
  layout->addStretch(1);

  setRow("Type", "--");
  setRow("ID", "--");
  clearSelection();
}

void SelectionInspectorPanel::clearSelection() {
  title_->setText("No selection");
  detail_->setText("Select a board object to inspect its stable identity.");
  clearExtraRows();
  setRow("Type", "--");
  setRow("ID", "--");
}

void SelectionInspectorPanel::renderSelection(const QString& type, const QString& id) {
  title_->setText(displayType(type) + " " + id);
  detail_->setText("Selected canvas object identity.");
  clearExtraRows();
  setRow("Type", type);
  setRow("ID", id);
}

void SelectionInspectorPanel::renderSelection(const std::optional<ccad::Board>& board, const QString& type, const QString& id) {
  if (!board.has_value()) {
    renderSelection(type, id);
    return;
  }

  const QString lowerType = type.toLower();

  if (lowerType == "pad") {
    for (const auto& pad : board->pads) {
      if (QString::fromStdString(pad.id) == id) {
        title_->setText("Pad " + id);
        detail_->setText("Edit the physical properties of this pad.");
        clearExtraRows();
        setRow("Type", "pad");
        setRow("ID", id);
        setRow("Component ID", QString::fromStdString(pad.component_id));
        setRow("Pin Name", QString::fromStdString(pad.pin_name));
        setRow("Net", pad.net_id.empty() ? "--" : QString::fromStdString(pad.net_id));
        setRow("Layer", pad.layers.empty() ? "--" : QString::fromStdString(pad.layers.front()));
        setRow("Position X", formatLength(pad.position.x));
        setRow("Position Y", formatLength(pad.position.y));

        auto* width_input = new QLineEdit(this);
        width_input->setObjectName("padWidthInput");
        width_input->setText(QString::number(toMm(pad.size.width), 'f', 4));
        form_->addRow("Width (mm):", width_input);

        auto* height_input = new QLineEdit(this);
        height_input->setObjectName("padHeightInput");
        height_input->setText(QString::number(toMm(pad.size.height), 'f', 4));
        form_->addRow("Height (mm):", height_input);

        auto* rotation_input = new QLineEdit(this);
        rotation_input->setObjectName("padRotationInput");
        rotation_input->setText(QString::number(pad.rotation_degrees, 'f', 1));
        form_->addRow("Rotation (deg):", rotation_input);

        auto* update_button = new QPushButton("Update Pad", this);
        update_button->setObjectName("updatePadButton");
        form_->addRow("", update_button);

        connect(update_button, &QPushButton::clicked, this, [this, id, width_input, height_input, rotation_input]() {
          bool ok1, ok2, ok3;
          double width = width_input->text().toDouble(&ok1);
          double height = height_input->text().toDouble(&ok2);
          double rotation = rotation_input->text().toDouble(&ok3);
          if (ok1 && ok2 && ok3 && width > 0 && height > 0) {
            if (pad_changed_callback_) {
              pad_changed_callback_(id, width, height, rotation);
            }
          } else {
            QMessageBox::warning(this, "Invalid Input", "Width and height must be positive numbers, and rotation must be a valid number.");
          }
        });
        return;
      }
    }
  } else if (lowerType == "via") {
    for (const auto& via : board->vias) {
      if (QString::fromStdString(via.id) == id) {
        title_->setText("Via " + id);
        detail_->setText("Edit the physical properties of this via.");
        clearExtraRows();
        setRow("Type", "via");
        setRow("ID", id);
        setRow("Net", via.net_id.empty() ? "--" : QString::fromStdString(via.net_id));
        setRow("Position X", formatLength(via.position.x));
        setRow("Position Y", formatLength(via.position.y));

        auto* diameter_input = new QLineEdit(this);
        diameter_input->setObjectName("viaDiameterInput");
        diameter_input->setText(QString::number(toMm(via.diameter), 'f', 4));
        form_->addRow("Diameter (mm):", diameter_input);

        auto* drill_input = new QLineEdit(this);
        drill_input->setObjectName("viaDrillInput");
        drill_input->setText(QString::number(toMm(via.drill), 'f', 4));
        form_->addRow("Drill (mm):", drill_input);

        auto* update_button = new QPushButton("Update Via", this);
        update_button->setObjectName("updateViaButton");
        form_->addRow("", update_button);

        connect(update_button, &QPushButton::clicked, this, [this, id, diameter_input, drill_input]() {
          bool ok1, ok2;
          double diameter = diameter_input->text().toDouble(&ok1);
          double drill = drill_input->text().toDouble(&ok2);
          if (ok1 && ok2 && diameter > 0 && drill > 0 && drill <= diameter) {
            if (via_changed_callback_) {
              via_changed_callback_(id, diameter, drill);
            }
          } else if (ok1 && ok2 && drill > diameter) {
            QMessageBox::warning(this, "Invalid Input", "Drill size must be less than or equal to diameter.");
          } else {
            QMessageBox::warning(this, "Invalid Input", "Diameter and drill must be positive numbers.");
          }
        });
        return;
      }
    }
  } else if (lowerType == "track") {
    for (const auto& track : board->tracks) {
      if (QString::fromStdString(track.id) == id) {
        title_->setText("Track " + id);
        detail_->setText("Edit the physical properties of this track segment.");
        clearExtraRows();
        setRow("Type", "track");
        setRow("ID", id);
        setRow("Net", track.net_id.empty() ? "--" : QString::fromStdString(track.net_id));
        setRow("Layer", QString::fromStdString(track.layer_id));
        setRow("Start X", formatLength(track.start.x));
        setRow("Start Y", formatLength(track.start.y));
        setRow("End X", formatLength(track.end.x));
        setRow("End Y", formatLength(track.end.y));

        auto* width_input = new QLineEdit(this);
        width_input->setObjectName("trackWidthInput");
        width_input->setText(QString::number(toMm(track.width), 'f', 4));
        form_->addRow("Width (mm):", width_input);

        double dx = static_cast<double>(track.end.x.nanometers - track.start.x.nanometers);
        double dy = static_cast<double>(track.end.y.nanometers - track.start.y.nanometers);
        double length_val = std::hypot(dx, dy);
        setRow("Length", formatLength(ccad::nanometers(static_cast<int64_t>(length_val))));
        setRow("Source Route Request", track.source_route_request_id.empty() ? "--" : QString::fromStdString(track.source_route_request_id));

        auto* update_button = new QPushButton("Update Track", this);
        update_button->setObjectName("updateTrackButton");
        form_->addRow("", update_button);

        connect(update_button, &QPushButton::clicked, this, [this, id, width_input]() {
          bool ok;
          double width = width_input->text().toDouble(&ok);
          if (ok && width > 0) {
            if (track_changed_callback_) {
              track_changed_callback_(id, width);
            }
          } else {
            QMessageBox::warning(this, "Invalid Input", "Track width must be a positive number.");
          }
        });
        return;
      }
    }
  } else if (lowerType == "keepout") {
    for (const auto& keepout : board->keepouts) {
      if (QString::fromStdString(keepout.id) == id) {
        title_->setText("Keepout " + id);
        detail_->setText("Edit the physical properties of this keepout area.");
        clearExtraRows();
        setRow("Type", "keepout");
        setRow("ID", id);
        setRow("Origin X", formatLength(keepout.area.origin.x));
        setRow("Origin Y", formatLength(keepout.area.origin.y));
        setRow("Kind", QString::fromStdString(keepout.kind));

        auto* width_input = new QLineEdit(this);
        width_input->setObjectName("keepoutWidthInput");
        width_input->setText(QString::number(toMm(keepout.area.size.width), 'f', 4));
        form_->addRow("Width (mm):", width_input);

        auto* height_input = new QLineEdit(this);
        height_input->setObjectName("keepoutHeightInput");
        height_input->setText(QString::number(toMm(keepout.area.size.height), 'f', 4));
        form_->addRow("Height (mm):", height_input);

        auto* update_button = new QPushButton("Update Keepout", this);
        update_button->setObjectName("updateKeepoutButton");
        form_->addRow("", update_button);

        connect(update_button, &QPushButton::clicked, this, [this, id, width_input, height_input]() {
          bool ok1, ok2;
          double width = width_input->text().toDouble(&ok1);
          double height = height_input->text().toDouble(&ok2);
          if (ok1 && ok2 && width > 0 && height > 0) {
            if (keepout_changed_callback_) {
              keepout_changed_callback_(id, width, height);
            }
          } else {
            QMessageBox::warning(this, "Invalid Input", "Width and height must be positive numbers.");
          }
        });
        return;
      }
    }
  } else if (lowerType == "placement_region" || lowerType == "placement-region") {
    for (const auto& pr : board->placement_regions) {
      if (QString::fromStdString(pr.id) == id) {
        title_->setText("Placement_region " + id);
        detail_->setText("Edit the physical properties of this placement region.");
        clearExtraRows();
        setRow("Type", "placement_region");
        setRow("ID", id);
        setRow("Origin X", formatLength(pr.area.origin.x));
        setRow("Origin Y", formatLength(pr.area.origin.y));
        setRow("Kind", QString::fromStdString(pr.kind));

        auto* width_input = new QLineEdit(this);
        width_input->setObjectName("regionWidthInput");
        width_input->setText(QString::number(toMm(pr.area.size.width), 'f', 4));
        form_->addRow("Width (mm):", width_input);

        auto* height_input = new QLineEdit(this);
        height_input->setObjectName("regionHeightInput");
        height_input->setText(QString::number(toMm(pr.area.size.height), 'f', 4));
        form_->addRow("Height (mm):", height_input);

        auto* update_button = new QPushButton("Update Region", this);
        update_button->setObjectName("updateRegionButton");
        form_->addRow("", update_button);

        connect(update_button, &QPushButton::clicked, this, [this, id, width_input, height_input]() {
          bool ok1, ok2;
          double width = width_input->text().toDouble(&ok1);
          double height = height_input->text().toDouble(&ok2);
          if (ok1 && ok2 && width > 0 && height > 0) {
            if (region_changed_callback_) {
              region_changed_callback_(id, width, height);
            }
          } else {
            QMessageBox::warning(this, "Invalid Input", "Width and height must be positive numbers.");
          }
        });
        return;
      }
    }
  }

  // Fallback if type not matched or object not found
  renderSelection(type, id);
}

void SelectionInspectorPanel::renderBoardRules(const std::optional<ccad::Board>& board) {
  title_->setText("Board Design Rules");
  detail_->setText("Edit the board-level physical DRC constraints.");
  clearExtraRows();

  if (!board.has_value()) {
    setRow("Type", "--");
    setRow("ID", "--");
    return;
  }

  auto* clearance_input = new QLineEdit(this);
  clearance_input->setObjectName("clearanceInput");
  clearance_input->setText(QString::number(toMm(board->design_rules.copper_clearance), 'f', 4));

  auto* track_width_input = new QLineEdit(this);
  track_width_input->setObjectName("trackWidthInput");
  track_width_input->setText(QString::number(toMm(board->design_rules.min_track_width), 'f', 4));

  auto* annular_ring_input = new QLineEdit(this);
  annular_ring_input->setObjectName("annularRingInput");
  annular_ring_input->setText(QString::number(toMm(board->design_rules.min_via_annular_ring), 'f', 4));

  auto* apply_button = new QPushButton("Apply Rules", this);
  apply_button->setObjectName("applyRulesButton");

  form_->addRow("Copper Clearance (mm):", clearance_input);
  form_->addRow("Min Track Width (mm):", track_width_input);
  form_->addRow("Via Annular Ring (mm):", annular_ring_input);
  form_->addRow("", apply_button);

  connect(apply_button, &QPushButton::clicked, this, [this, clearance_input, track_width_input, annular_ring_input]() {
    bool ok1, ok2, ok3;
    double clearance = clearance_input->text().toDouble(&ok1);
    double track_width = track_width_input->text().toDouble(&ok2);
    double annular_ring = annular_ring_input->text().toDouble(&ok3);
    if (ok1 && ok2 && ok3 && clearance > 0 && track_width > 0 && annular_ring > 0) {
      if (design_rules_changed_callback_) {
        ccad::DesignRules rules{
            .copper_clearance = ccad::millimeters(clearance),
            .min_track_width = ccad::millimeters(track_width),
            .min_via_annular_ring = ccad::millimeters(annular_ring)};
        design_rules_changed_callback_(rules);
      }
    } else {
      QMessageBox::warning(this, "Invalid Input", "All rules must be positive numbers.");
    }
  });
}

void SelectionInspectorPanel::renderCanvasItem() {
  title_->setText("Canvas item");
  detail_->setText("Selected item has no stable CCad object identity.");
  clearExtraRows();
  setRow("Type", "--");
  setRow("ID", "--");
}

QString SelectionInspectorPanel::titleText() const {
  return title_->text();
}

QString SelectionInspectorPanel::detailText() const {
  return detail_->text();
}

QString SelectionInspectorPanel::rowText(const QString& label) const {
  const auto it = rows_.find(label);
  if (it != rows_.end()) {
    return (*it)->text();
  }

  if (form_ != nullptr) {
    for (int i = 0; i < form_->rowCount(); ++i) {
      QLayoutItem* labelItem = form_->itemAt(i, QFormLayout::LabelRole);
      if (labelItem != nullptr) {
        auto* labelWidget = qobject_cast<QLabel*>(labelItem->widget());
        if (labelWidget != nullptr) {
          QString text = labelWidget->text();
          if (text.endsWith(':')) {
            text.chop(1);
          }
          if (text == label || text == label + " (mm)" || text == label + " (deg)") {
            QLayoutItem* fieldItem = form_->itemAt(i, QFormLayout::FieldRole);
            if (fieldItem != nullptr) {
              auto* lineEdit = qobject_cast<QLineEdit*>(fieldItem->widget());
              if (lineEdit != nullptr) {
                return lineEdit->text();
              }
            }
          }
        }
      }
    }
  }

  return {};
}

void SelectionInspectorPanel::setRow(const QString& label, const QString& value) {
  auto it = rows_.find(label);
  if (it == rows_.end()) {
    auto* value_label = new QLabel(this);
    value_label->setObjectName("inspectorValue");
    form_->addRow(label + ":", value_label);
    it = rows_.insert(label, value_label);
  }
  (*it)->setText(value);
}

void SelectionInspectorPanel::clearExtraRows() {
  if (form_ != nullptr) {
    const auto delete_item = [](QLayoutItem* item) {
      if (item == nullptr) {
        return;
      }
      if (QWidget* widget = item->widget()) {
        widget->deleteLater();
      }
      delete item;
    };
    while (form_->rowCount() > 0) {
      const QFormLayout::TakeRowResult row = form_->takeRow(0);
      delete_item(row.labelItem);
      delete_item(row.fieldItem);
    }
  }
  rows_.clear();
}
