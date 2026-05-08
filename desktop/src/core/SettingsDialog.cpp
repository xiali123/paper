#include "core/SettingsDialog.hpp"
#include "core/ApiManager.hpp"
#include "core/AuthManager.hpp"
#include "core/ThemeManager.hpp"
#include <QSettings>

SettingsDialog::SettingsDialog(ApiManager* apiManager, AuthManager* authManager,
                               QWidget* parent)
    : QDialog(parent)
    , apiManager_(apiManager)
    , authManager_(authManager)
{
    setWindowTitle("Settings");
    setMinimumSize(500, 480);
    setModal(true);
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QString groupStyle =
        "QGroupBox { font-weight: bold; color: palette(text); border: 1px solid palette(mid); "
        "border-radius: 8px; margin-top: 12px; padding-top: 20px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; }";

    // --- Connection group ---
    auto* connectionGroup = new QGroupBox("Connection");
    connectionGroup->setStyleSheet(groupStyle);
    auto* connLayout = new QVBoxLayout(connectionGroup);

    auto* urlRow = new QHBoxLayout();
    urlRow->addWidget(new QLabel("Server URL:"));
    serverUrlEdit_ = new QLineEdit();
    serverUrlEdit_->setPlaceholderText("http://localhost:8080");
    serverUrlEdit_->setStyleSheet(
        "QLineEdit { padding: 8px; border: 1px solid palette(mid); border-radius: 6px; }"
    );
    urlRow->addWidget(serverUrlEdit_);

    auto* testBtn = new QPushButton("Test");
    testBtn->setStyleSheet(
        "QPushButton { background: #4f46e5; color: white; border: none; "
        "border-radius: 6px; padding: 8px 16px; font-weight: bold; }"
    );
    connect(testBtn, &QPushButton::clicked, this, [this]() {
        if (apiManager_) {
            apiManager_->setBaseUrl(serverUrlEdit_->text());
            apiManager_->checkHealth();
            QMessageBox::information(this, "Test", "Health check sent to " + serverUrlEdit_->text());
        }
    });
    urlRow->addWidget(testBtn);
    connLayout->addLayout(urlRow);

    mainLayout->addWidget(connectionGroup);

    // --- Display group ---
    auto* displayGroup = new QGroupBox("Display");
    displayGroup->setStyleSheet(groupStyle);
    auto* displayLayout = new QVBoxLayout(displayGroup);

    auto* themeRow = new QHBoxLayout();
    themeRow->addWidget(new QLabel("Theme:"));
    themeCombo_ = new QComboBox();
    themeCombo_->addItems({"System Default", "Light", "Dark"});
    themeCombo_->setStyleSheet("QComboBox { padding: 6px; border: 1px solid palette(mid); border-radius: 6px; }");
    themeRow->addWidget(themeCombo_);
    displayLayout->addLayout(themeRow);

    auto* pageRow = new QHBoxLayout();
    pageRow->addWidget(new QLabel("Papers per page:"));
    pageSizeSpin_ = new QSpinBox();
    pageSizeSpin_->setRange(10, 100);
    pageSizeSpin_->setSingleStep(10);
    pageSizeSpin_->setStyleSheet("QSpinBox { padding: 6px; border: 1px solid palette(mid); border-radius: 6px; }");
    pageRow->addWidget(pageSizeSpin_);
    pageRow->addStretch();
    displayLayout->addLayout(pageRow);

    mainLayout->addWidget(displayGroup);

    // --- Search group ---
    auto* searchGroup = new QGroupBox("Search");
    searchGroup->setStyleSheet(groupStyle);
    auto* searchLayout = new QVBoxLayout(searchGroup);

    saveHistoryCheck_ = new QCheckBox("Save search history");
    searchLayout->addWidget(saveHistoryCheck_);

    auto* historyRow = new QHBoxLayout();
    historyRow->addWidget(new QLabel("Max history items:"));
    maxHistorySpin_ = new QSpinBox();
    maxHistorySpin_->setRange(10, 500);
    maxHistorySpin_->setSingleStep(10);
    maxHistorySpin_->setStyleSheet("QSpinBox { padding: 6px; border: 1px solid palette(mid); border-radius: 6px; }");
    historyRow->addWidget(maxHistorySpin_);
    historyRow->addStretch();
    searchLayout->addLayout(historyRow);

    autoSearchCheck_ = new QCheckBox("Auto-search on type (debounced)");
    searchLayout->addWidget(autoSearchCheck_);

    mainLayout->addWidget(searchGroup);

    // --- Refresh group ---
    auto* refreshGroup = new QGroupBox("Data Refresh");
    refreshGroup->setStyleSheet(groupStyle);
    auto* refreshLayout = new QVBoxLayout(refreshGroup);

    autoRefreshCheck_ = new QCheckBox("Auto-refresh health status");
    refreshLayout->addWidget(autoRefreshCheck_);

    auto* refreshRow = new QHBoxLayout();
    refreshRow->addWidget(new QLabel("Refresh interval (seconds):"));
    refreshIntervalSpin_ = new QSpinBox();
    refreshIntervalSpin_->setRange(10, 300);
    refreshIntervalSpin_->setSingleStep(10);
    refreshIntervalSpin_->setStyleSheet("QSpinBox { padding: 6px; border: 1px solid palette(mid); border-radius: 6px; }");
    refreshRow->addWidget(refreshIntervalSpin_);
    refreshRow->addStretch();
    refreshLayout->addLayout(refreshRow);

    mainLayout->addWidget(refreshGroup);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* cancelButton = new QPushButton("Cancel");
    cancelButton->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: palette(light); }"
    );
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);

    auto* saveButton = new QPushButton("Save");
    saveButton->setStyleSheet(
        "QPushButton { background: #4f46e5; color: white; border: none; "
        "border-radius: 6px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: #4338ca; }"
    );
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        saveSettings();
        accept();
    });
    buttonLayout->addWidget(saveButton);

    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::loadSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    serverUrlEdit_->setText(settings.value("server/url", "http://localhost:8080").toString());
    themeCombo_->setCurrentIndex(settings.value("display/theme", 0).toInt());
    pageSizeSpin_->setValue(settings.value("display/pageSize", 20).toInt());
    saveHistoryCheck_->setChecked(settings.value("search/saveHistory", true).toBool());
    maxHistorySpin_->setValue(settings.value("search/maxHistory", 100).toInt());
    autoSearchCheck_->setChecked(settings.value("search/autoSearch", false).toBool());
    autoRefreshCheck_->setChecked(settings.value("refresh/autoRefresh", true).toBool());
    refreshIntervalSpin_->setValue(settings.value("refresh/interval", 30).toInt());
}

void SettingsDialog::saveSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    settings.setValue("server/url", serverUrlEdit_->text());
    settings.setValue("display/theme", themeCombo_->currentIndex());
    settings.setValue("display/pageSize", pageSizeSpin_->value());
    settings.setValue("search/saveHistory", saveHistoryCheck_->isChecked());
    settings.setValue("search/maxHistory", maxHistorySpin_->value());
    settings.setValue("search/autoSearch", autoSearchCheck_->isChecked());
    settings.setValue("refresh/autoRefresh", autoRefreshCheck_->isChecked());
    settings.setValue("refresh/interval", refreshIntervalSpin_->value());

    // Apply server URL immediately
    if (apiManager_) {
        apiManager_->setBaseUrl(serverUrlEdit_->text());
    }
}
