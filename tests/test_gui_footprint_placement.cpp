#include "ccad_gui/footprint_placement_dialog.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QDir>
#include <QFile>

#include <fstream>
#include <iostream>

#include "ccad_gui/library_browser_dialog.hpp"
#include "ccad_gui/board_canvas_renderer.hpp"

class TestGuiFootprintPlacement : public QObject {
  Q_OBJECT

 private slots:
  void testPopulateLayers() {
    ccad::Board board;
    board.layers.push_back({"F.Cu", "Front Copper", "copper", true});
    board.layers.push_back({"F.SilkS", "Front Silkscreen", "user", true});

    FootprintPlacementDialog dialog(board);
    auto* layer_combo = dialog.findChild<QComboBox*>();
    QVERIFY(layer_combo != nullptr);
    QCOMPARE(layer_combo->count(), 1);
    QCOMPARE(layer_combo->currentData().toString(), QString("F.Cu"));
  }

  void testAcceptsValidInput() {
    // Write a dummy footprint file
    std::string test_fp_path = "test_fp.json";
    std::ofstream out(test_fp_path);
    out << R"({
      "name": "TestFP",
      "pads": [
        {
          "number": "1",
          "type": "smd",
          "shape": "rect",
          "x_nm": 0, "y_nm": 0,
          "rotation_degrees": 0,
          "width_nm": 1000000,
          "height_nm": 1000000,
          "drill_nm": 0,
          "layers": ["F.Cu"]
        }
      ]
    })";
    out.close();

    ccad::Board board;
    board.layers.push_back({"F.Cu", "Front Copper", "copper", true});

    FootprintPlacementDialog dialog(board);

    // Find widgets
    auto* path_edit = dialog.findChild<QLineEdit*>();
    auto* layer_combo = dialog.findChild<QComboBox*>();
    auto* button_box = dialog.findChild<QDialogButtonBox*>();

    QVERIFY(path_edit != nullptr);
    QVERIFY(layer_combo != nullptr);
    QVERIFY(button_box != nullptr);

    // Since footprint_path_edit_ is read-only and updated by the browse button,
    // we bypass the file dialog by setting the text directly for this test.
    // In a real Qt test we could mock the QFileDialog, but here we just set the line edit.
    path_edit->setReadOnly(false); 
    path_edit->setText("test_fp.json");
    
    // Trigger accept
    QPushButton* okButton = button_box->button(QDialogButtonBox::Ok);
    QVERIFY(okButton != nullptr);

    QTimer::singleShot(100, [&]() {
      okButton->click();
    });
    
    dialog.exec();

    QVERIFY(dialog.result().has_value());
    auto result = dialog.result().value();
    QCOMPARE(QString::fromStdString(result.component_id), QString("U1"));
    QCOMPARE(QString::fromStdString(result.layer_id), QString("F.Cu"));
    QCOMPARE(QString::fromStdString(result.footprint_path), QString("test_fp.json"));
  }

  void testLibraryChooserDoesNotParseFootprintsDuringConstruction() {
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QDir root(temp_dir.path());
    QVERIFY(root.mkpath("footprints/Bad.pretty"));

    QFile footprint(root.filePath("footprints/Bad.pretty/OnePad.json"));
    QVERIFY(footprint.open(QIODevice::WriteOnly | QIODevice::Text));
    footprint.write(R"({
      "name": "OnePad",
      "pads": [
        {
          "number": "1",
          "type": "smd",
          "shape": "rect",
          "x_nm": 0, "y_nm": 0,
          "rotation_degrees": 0,
          "width_nm": 1000000,
          "height_nm": 1000000,
          "drill_nm": 0,
          "layers": ["F.Cu"]
        }
      ]
    })");
    footprint.close();

    LibraryBrowserDialog dialog(LibraryType::Footprint, temp_dir.path());
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 1);
    QCOMPARE(tree->topLevelItem(0)->text(0), QString("OnePad"));
    QVERIFY(!tree->topLevelItem(0)->text(1).contains("pads"));

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
    QCOMPARE(QString::fromStdString(selection->item_name), QString("OnePad"));
    QCOMPARE(QString::fromStdString(selection->library_name), QString("Bad.pretty"));
    QCOMPARE(QString::fromStdString(selection->source_kind), QString("CCad footprint JSON"));
    QCOMPARE(QString::fromStdString(selection->path), footprint.fileName());
  }

  void testLibraryChooserPreviewUsesDistinctMaskAndPasteColors() {
    QTemporaryDir temp_dir;
    QVERIFY(temp_dir.isValid());
    QDir root(temp_dir.path());
    QVERIFY(root.mkpath("footprints/Layered.pretty"));

    QFile footprint(root.filePath("footprints/Layered.pretty/LayeredPad.json"));
    QVERIFY(footprint.open(QIODevice::WriteOnly | QIODevice::Text));
    footprint.write(R"({
      "name": "LayeredPad",
      "pads": [
        {
          "number": "1",
          "type": "smd",
          "shape": "roundrect",
          "x_nm": 0, "y_nm": 0,
          "rotation_degrees": 0,
          "width_nm": 1800000,
          "height_nm": 1200000,
          "roundrect_rratio": 0.25,
          "layers": ["F.Cu", "F.Mask", "F.Paste"]
        }
      ]
    })");
    footprint.close();

    LibraryBrowserDialog dialog(LibraryType::Footprint, temp_dir.path());
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(tree != nullptr);
    QCOMPARE(tree->topLevelItemCount(), 1);
    tree->setCurrentItem(tree->topLevelItem(0));
    QApplication::processEvents();

    auto* preview = dialog.findChild<QGraphicsView*>();
    QVERIFY(preview != nullptr);
    QVERIFY(preview->scene() != nullptr);

    const CanvasRenderTheme theme;
    bool saw_copper = false;
    bool saw_mask = false;
    bool saw_paste = false;
    for (QGraphicsItem* item : preview->scene()->items()) {
      auto* path = dynamic_cast<QGraphicsPathItem*>(item);
      if (path == nullptr) {
        continue;
      }
      const QColor color = path->pen().color();
      saw_copper = saw_copper || color == colorForKiCadLayer(theme, "F.Cu").lighter(125);
      saw_mask = saw_mask || color == colorForKiCadLayer(theme, "F.Mask");
      saw_paste = saw_paste || color == colorForKiCadLayer(theme, "F.Paste");
    }
    QVERIFY(saw_copper);
    QVERIFY(saw_mask);
    QVERIFY(saw_paste);
  }
};

QTEST_MAIN(TestGuiFootprintPlacement)
#include "test_gui_footprint_placement.moc"
