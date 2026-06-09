#include <QtTest>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>

#include "ccad_gui/agent_settings_dialog.hpp"
#include "ccad_gui/agent_marketplace_dialog.hpp"
#include "ccad_gui/agent_panel.hpp"

class TestGuiAgentPanel : public QObject {
  Q_OBJECT

private slots:
  void testSettingsDialogInteractions() {
    AgentPanel panel;
    AgentSettingsDialog dialog(&panel);

    QTimer::singleShot(0, &dialog, &QWidget::show);
    QTest::qWait(100);

    auto* save_btn = dialog.findChild<QPushButton*>("action:primaryButton");
    QVERIFY(save_btn != nullptr);

    // 1. General Tab
    auto* provider_combo = dialog.findChild<QComboBox*>("control:providerCombo");
    QVERIFY(provider_combo != nullptr);
    QTest::keyClick(provider_combo, Qt::Key_Down);

    auto* model_input = dialog.findChild<QLineEdit*>("control:modelInput");
    QVERIFY(model_input != nullptr);
    QTest::keyClicks(model_input, "gpt-4o-test");

    auto* sandbox_cb = dialog.findChild<QCheckBox*>("control:sandboxCb");
    QVERIFY(sandbox_cb != nullptr);
    QTest::mouseClick(sandbox_cb, Qt::LeftButton);

    auto* approval_cb = dialog.findChild<QCheckBox*>("control:approvalCb");
    QVERIFY(approval_cb != nullptr);
    QTest::mouseClick(approval_cb, Qt::LeftButton);

    auto* project_name = dialog.findChild<QLineEdit*>("control:projectNameInput");
    QVERIFY(project_name != nullptr);
    QTest::keyClicks(project_name, "test_proj");

    auto* project_path = dialog.findChild<QLineEdit*>("control:projectPathInput");
    QVERIFY(project_path != nullptr);
    QTest::keyClicks(project_path, "/tmp/test");

    auto* trust_level = dialog.findChild<QComboBox*>("control:trustLevelCombo");
    QVERIFY(trust_level != nullptr);
    QTest::keyClick(trust_level, Qt::Key_Down);

    // 2. Capabilities
    auto* stm_cb = dialog.findChild<QCheckBox*>("control:stmCb");
    QVERIFY(stm_cb != nullptr);
    QTest::mouseClick(stm_cb, Qt::LeftButton);

    auto* ltm_cb = dialog.findChild<QCheckBox*>("control:ltmCb");
    QVERIFY(ltm_cb != nullptr);
    QTest::mouseClick(ltm_cb, Qt::LeftButton);

    auto* episodic_cb = dialog.findChild<QCheckBox*>("control:episodicCb");
    QVERIFY(episodic_cb != nullptr);
    QTest::mouseClick(episodic_cb, Qt::LeftButton);

    auto* hooks_combo = dialog.findChild<QComboBox*>("control:hooksCombo");
    QVERIFY(hooks_combo != nullptr);
    QTest::keyClick(hooks_combo, Qt::Key_Down);

    // 3. Personalisation
    auto* follow_up = dialog.findChild<QLineEdit*>("control:followUpInput");
    QVERIFY(follow_up != nullptr);
    QTest::keyClicks(follow_up, "Be concise.");

    auto* context_window = dialog.findChild<QCheckBox*>("control:contextWindowCb");
    QVERIFY(context_window != nullptr);
    QTest::mouseClick(context_window, Qt::LeftButton);

    auto* chat_mode = dialog.findChild<QComboBox*>("control:chatModeCombo");
    QVERIFY(chat_mode != nullptr);
    QTest::keyClick(chat_mode, Qt::Key_Down);

    auto* personality = dialog.findChild<QComboBox*>("control:agentPersonalityCombo");
    QVERIFY(personality != nullptr);
    QTest::keyClick(personality, Qt::Key_Down);

    // Skip custom instructions text edit as QTest::keyClicks can be flaky on QTextEdit

    // 4. API & Providers
    auto* test_export = dialog.findChild<QPushButton*>("action:testExportBtn");
    QVERIFY(test_export != nullptr);
    QTest::mouseClick(test_export, Qt::LeftButton);

    // Save
    QTest::mouseClick(save_btn, Qt::LeftButton);
    QVERIFY(dialog.isHidden());
  }

  void testMarketplaceInteractions() {
    AgentPanel panel;
    AgentMarketplaceDialog dialog(&panel, &panel);

    QTimer::singleShot(0, &dialog, &QWidget::show);
    QTest::qWait(100);

    QVERIFY(dialog.isVisible());
    dialog.accept();
  }
};

QTEST_MAIN(TestGuiAgentPanel)
#include "test_gui_agent_panel.moc"
