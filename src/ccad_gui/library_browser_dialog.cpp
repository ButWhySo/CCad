#include "ccad_gui/library_browser_dialog.hpp"

#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/kicad_symbol_import.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSplitter>
#include <QHeaderView>
#include <QAbstractItemView>

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

QString describeFootprintFile(const QFileInfo& file) {
  try {
    const std::string content = readCacheFile(file.absoluteFilePath());
    const ccad::Footprint footprint = file.suffix().compare("kicad_mod", Qt::CaseInsensitive) == 0
                                           ? ccad::importKiCadFootprint(content)
                                           : ccad::loadFootprintJson(content);
    return QString::number(footprint.pads.size()) + " pads";
  } catch (...) {
    return "Footprint";
  }
}

QString describeSymbolFile(const QFileInfo& file) {
  try {
    ccad::Symbol symbol;
    if (file.suffix().compare("kicad_sym", Qt::CaseInsensitive) == 0) {
      const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(readCacheFile(file.absoluteFilePath()));
      if (!symbols.empty()) {
        symbol = symbols.front();
      }
    } else {
      symbol = ccad::loadSymbolJsonFileWithLocalInheritance(file.absoluteFilePath().toStdString());
    }
    return QString::number(symbol.pins.size()) + " pins";
  } catch (...) {
    return "Symbol";
  }
}

}  // namespace

LibraryBrowserDialog::LibraryBrowserDialog(LibraryType type, QWidget* parent)
    : QDialog(parent), type_(type) {
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
  preview_label_ = new QLabel("Preview metadata will appear here.", details);
  preview_label_->setWordWrap(true);
  preview_label_->setMinimumWidth(260);
  details_layout->addWidget(detail_label_);
  details_layout->addWidget(preview_label_, 1);
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
  QString dir_path = type_ == LibraryType::Footprint ? "library-cache/footprints" : "library-cache/symbols";
  QDir dir(dir_path);
  
  if (!dir.exists()) {
    QMessageBox::warning(this, "Library Cache", "Library cache directory not found: " + dir_path);
    return;
  }

  QDirIterator it(dir.absolutePath(), {"*.json", "*.kicad_mod", "*.kicad_sym"},
                  QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QFileInfo file(it.next());
    const QString library = dir.relativeFilePath(file.absolutePath()).section('/', 0, 0);
    const QString library_name = library == "." ? dir.dirName() : library;
    const QString description = type_ == LibraryType::Footprint ? describeFootprintFile(file)
                                                                : describeSymbolFile(file);
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
    preview_label_->setText("Preview metadata will appear here.");
    return;
  }
  const QTreeWidgetItem* item = selected.first();
  const QFileInfo file(item->data(0, Qt::UserRole).toString());
  const QString library = item->data(0, Qt::UserRole + 1).toString();
  detail_label_->setText("<b>Library</b>: " + library + "<br><b>Name</b>: " +
                         file.completeBaseName() + "<br><b>File</b>: " + file.fileName() +
                         "<br><b>Path</b>: " + file.absoluteFilePath());
  preview_label_->setText(type_ == LibraryType::Footprint
                              ? "Footprint chooser selection. Placement will use the core footprint importer and preserve pad shape metadata."
                              : "Symbol chooser selection. Placement will resolve local inherited symbol pins before adding the component.");
}

std::optional<std::string> LibraryBrowserDialog::result() const {
  return result_;
}
