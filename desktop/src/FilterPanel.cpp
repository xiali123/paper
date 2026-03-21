#include "FilterPanel.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>

FilterPanel::FilterPanel(QWidget* parent) : QGroupBox("Filters", parent) {
    setupUI();
}

void FilterPanel::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Level filters
    checkAll_ = new QCheckBox("All Levels", this);
    checkA_ = new QCheckBox("CCF-A", this);
    checkB_ = new QCheckBox("CCF-B", this);
    checkC_ = new QCheckBox("CCF-C", this);

    checkAll_->setChecked(true);

    layout->addWidget(checkAll_);
    layout->addWidget(checkA_);
    layout->addWidget(checkB_);
    layout->addWidget(checkC_);

    // Year filter
    yearCombo_ = new QComboBox(this);
    yearCombo_->addItem("All Years");
    yearCombo_->addItem("2024");
    yearCombo_->addItem("2023");
    yearCombo_->addItem("2022");

    layout->addWidget(new QLabel("Year:", this));
    layout->addWidget(yearCombo_);

    layout->addStretch();
}
