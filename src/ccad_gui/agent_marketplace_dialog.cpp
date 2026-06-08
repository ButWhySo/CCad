#include "agent_marketplace_dialog.hpp"

#include <QVBoxLayout>
#include <QLabel>

AgentMarketplaceDialog::AgentMarketplaceDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Agent Marketplace");
  resize(400, 600);
  
  auto* layout = new QVBoxLayout(this);
  auto* label = new QLabel("Agent Marketplace\n(Coming Soon)", this);
  label->setAlignment(Qt::AlignCenter);
  layout->addWidget(label);
}
