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

#include <fstream>
#include <iostream>

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
};

QTEST_MAIN(TestGuiFootprintPlacement)
#include "test_gui_footprint_placement.moc"
