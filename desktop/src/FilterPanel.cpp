#include "FilterPanel.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>

FilterPanel::FilterPanel(QWidget* parent) : QGroupBox("Filters", parent) {
    setupUI();
}

void FilterPanel::setupUI() {
    setStyleSheet(
        "QGroupBox { font-weight: bold; color: palette(text); border: 1px solid palette(mid); "
        "border-radius: 8px; margin-top: 12px; padding-top: 20px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; }"
        "QCheckBox { padding: 4px 0; color: palette(text); }"
        "QCheckBox::indicator { width: 16px; height: 16px; }"
        "QComboBox { padding: 6px; border: 1px solid palette(mid); border-radius: 6px; }"
    );

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);

    // Level filters
    auto* levelLabel = new QLabel("CCF Level:");
    levelLabel->setStyleSheet("font-weight: 600; color: palette(mid); font-size: 11px;");
    layout->addWidget(levelLabel);

    checkAll_ = new QCheckBox("All Levels", this);
    checkA_ = new QCheckBox("CCF-A", this);
    checkB_ = new QCheckBox("CCF-B", this);
    checkC_ = new QCheckBox("CCF-C", this);

    checkAll_->setChecked(true);
    checkA_->setChecked(true);
    checkB_->setChecked(true);
    checkC_->setChecked(true);

    layout->addWidget(checkAll_);
    layout->addWidget(checkA_);
    layout->addWidget(checkB_);
    layout->addWidget(checkC_);

    connect(checkAll_, &QCheckBox::toggled, this, [this](bool checked) {
        checkA_->setChecked(checked);
        checkB_->setChecked(checked);
        checkC_->setChecked(checked);
        emit filterChanged(getLevel(), getYear(), getType());
    });
    connect(checkA_, &QCheckBox::toggled, this, &FilterPanel::onLevelToggled);
    connect(checkB_, &QCheckBox::toggled, this, &FilterPanel::onLevelToggled);
    connect(checkC_, &QCheckBox::toggled, this, &FilterPanel::onLevelToggled);

    layout->addSpacing(12);

    // Year filter
    auto* yearLabel = new QLabel("Year:");
    yearLabel->setStyleSheet("font-weight: 600; color: palette(mid); font-size: 11px;");
    layout->addWidget(yearLabel);

    yearCombo_ = new QComboBox(this);
    yearCombo_->addItem("All Years", "");
    int currentYear = QDateTime::currentDateTime().date().year();
    for (int y = currentYear; y >= 2010; --y) {
        yearCombo_->addItem(QString::number(y), QString::number(y));
    }
    layout->addWidget(yearCombo_);

    connect(yearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FilterPanel::onYearChanged);

    layout->addSpacing(12);

    // Paper type filter
    auto* typeLabel = new QLabel("Type:");
    typeLabel->setStyleSheet("font-weight: 600; color: palette(mid); font-size: 11px;");
    layout->addWidget(typeLabel);

    typeCombo_ = new QComboBox(this);
    typeCombo_->addItem("All Types", "");
    typeCombo_->addItem("Journal", "journal");
    typeCombo_->addItem("Conference", "conference");
    typeCombo_->addItem("Preprint", "preprint");
    typeCombo_->addItem("Book", "book");
    layout->addWidget(typeCombo_);

    connect(typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emit filterChanged(getLevel(), getYear(), getType()); });

    layout->addStretch();
}

void FilterPanel::onLevelToggled(bool) {
    updateLevelCheckboxes();
    emit filterChanged(getLevel(), getYear(), getType());
}

void FilterPanel::onYearChanged(int) {
    emit filterChanged(getLevel(), getYear(), getType());
}

void FilterPanel::updateLevelCheckboxes() {
    bool allChecked = checkA_->isChecked() && checkB_->isChecked() && checkC_->isChecked();
    checkAll_->blockSignals(true);
    checkAll_->setChecked(allChecked);
    checkAll_->blockSignals(false);
}

QString FilterPanel::getLevel() const {
    QStringList levels;
    if (checkA_->isChecked()) levels << "A";
    if (checkB_->isChecked()) levels << "B";
    if (checkC_->isChecked()) levels << "C";

    if (levels.size() == 3 || levels.isEmpty()) return "";
    return levels.join(",");
}

QString FilterPanel::getYear() const {
    return yearCombo_ ? yearCombo_->currentData().toString() : "";
}

QString FilterPanel::getType() const {
    return typeCombo_ ? typeCombo_->currentData().toString() : "";
}
