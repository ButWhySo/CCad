#include <QTest>
#include <QApplication>
#include <QTimer>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QDir>
#include <QFile>
#include <QGraphicsView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTreeWidget>

#include "ccad_core/model.hpp"
#include "ccad_core/symbol.hpp"
#include "ccad_gui/library_browser_dialog.hpp"
#include "ccad_gui/symbol_placement_dialog.hpp"

class TestGuiSymbolPlacement : public QObject {
  Q_OBJECT
private slots:
  void testBasicPlacement() {
    ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.schematics.push_back(ccad::Schematic{});
    project.id = "test-proj";
    
    // We mock a symbol file
    SymbolPlacementDialog dialog(project);
    
    // We cannot easily test file dialogs in QTest because QFileDialog::getOpenFileName is static and blocks.
    // However, we can bypass the button and set the line edit directly.
    auto* path_edit = dialog.findChild<QLineEdit*>();
    QVERIFY(path_edit != nullptr);
    path_edit->setText("test_symbol.json"); // Note: we need a valid symbol json here for it to succeed.

    // Let's just verify the spins exist
    auto spins = dialog.findChildren<QDoubleSpinBox*>();
    QVERIFY(spins.size() >= 3);
    
    // Since we don't have a real file, onAccept will show a QMessageBox, which blocks. 
    // Testing blocking dialogs with QTest requires a QTimer to click them.
    // For now, this test just verifies the dialog constructs and exposes the right fields.
  }

  void testLibraryChooserDoesNotParseSymbolsDuringConstruction() {
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QDir root(temp_dir.path());
    QVERIFY(root.mkpath("symbols/Bad.kicad_sym"));

    QFile symbol(root.filePath("symbols/Bad.kicad_sym/OnePin.json"));
    QVERIFY(symbol.open(QIODevice::WriteOnly | QIODevice::Text));
    symbol.write(R"({
      "name": "OnePin",
      "extends": "",
      "pins": [
        {"name":"A", "number":"1", "electrical_type":"passive", "x_nm":0, "y_nm":0, "rotation_degrees":0}
      ],
      "properties": [],
      "rectangles": [],
      "lines": [],
      "arcs": [],
      "circles": [],
      "polylines": [],
      "texts": []
    })");
    symbol.close();

    LibraryBrowserDialog dialog(LibraryType::Symbol, temp_dir.path());
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 1);
    QCOMPARE(tree->topLevelItem(0)->text(0), QString("OnePin"));
    QVERIFY(!tree->topLevelItem(0)->text(1).contains("pins"));

    tree->setCurrentItem(tree->topLevelItem(0));
    QApplication::processEvents();
    auto* preview = dialog.findChild<QGraphicsView*>();
    QVERIFY(preview != nullptr);
    QVERIFY(preview->scene() != nullptr);
    QVERIFY(preview->scene()->items().size() > 0);

    auto* button_box = dialog.findChild<QDialogButtonBox*>();
    QVERIFY(button_box != nullptr);
    QPushButton* ok_button = button_box->button(QDialogButtonBox::Ok);
    QVERIFY(ok_button != nullptr);
    QTimer::singleShot(0, [&]() {
      ok_button->click();
    });
    dialog.exec();

    const auto selection = dialog.selection();
    QVERIFY(selection.has_value());
    QCOMPARE(QString::fromStdString(selection->item_name), QString("OnePin"));
    QCOMPARE(QString::fromStdString(selection->library_name), QString("Bad.kicad_sym"));
    QCOMPARE(QString::fromStdString(selection->source_kind), QString("CCad symbol JSON"));
    QCOMPARE(QString::fromStdString(selection->path), symbol.fileName());
  }

  void testKicadSymbolLibraryExpandsTopLevelSymbols() {
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QDir root(temp_dir.path());
    QVERIFY(root.mkpath("symbols"));

    QFile symbol_lib(root.filePath("symbols/Device.kicad_sym"));
    QVERIFY(symbol_lib.open(QIODevice::WriteOnly | QIODevice::Text));
    symbol_lib.write(R"(
(kicad_symbol_lib
  (version 20240101)
  (generator ccad-test)
  (symbol "Parent"
    (property "Reference" "U" (id 0) (at 0 0 0))
    (symbol "Parent_1_1"
      (rectangle (start -1 -1) (end 1 1) (stroke (width 0.1)) (fill (type none)))
      (pin passive line (at 0 0 0) (length 2.54) (name "A") (number "1"))
    )
  )
  (symbol "Derived"
    (extends "Parent")
    (property "Reference" "U" (id 0) (at 0 0 0))
  )
)
)");
    symbol_lib.close();

    LibraryBrowserDialog dialog(LibraryType::Symbol, temp_dir.path());
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 2);
    QCOMPARE(tree->topLevelItem(0)->text(0), QString("Parent"));
    QCOMPARE(tree->topLevelItem(1)->text(0), QString("Derived"));
    QCOMPARE(tree->topLevelItem(1)->text(2), QString("Device"));
    QVERIFY(tree->topLevelItem(1)->text(1).contains("extends Parent"));

    tree->setCurrentItem(tree->topLevelItem(1));
    QApplication::processEvents();
    auto* preview = dialog.findChild<QGraphicsView*>();
    QVERIFY(preview != nullptr);
    QVERIFY(preview->scene() != nullptr);
    QVERIFY(preview->scene()->items().size() > 0);

    auto* button_box = dialog.findChild<QDialogButtonBox*>();
    QVERIFY(button_box != nullptr);
    QPushButton* ok_button = button_box->button(QDialogButtonBox::Ok);
    QVERIFY(ok_button != nullptr);
    QTimer::singleShot(0, [&]() {
      ok_button->click();
    });
    dialog.exec();

    const auto selection = dialog.selection();
    QVERIFY(selection.has_value());
    QCOMPARE(QString::fromStdString(selection->item_name), QString("Derived"));
    QCOMPARE(QString::fromStdString(selection->library_name), QString("Device"));
    QCOMPARE(QString::fromStdString(selection->source_kind), QString("KiCad symbol library"));
    QCOMPARE(QString::fromStdString(selection->extends), QString("Parent"));
    QCOMPARE(QString::fromStdString(selection->path), symbol_lib.fileName());
  }
};

QTEST_MAIN(TestGuiSymbolPlacement)
#include "test_gui_symbol_placement.moc"
