#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QSettings>

class ApiManager;
class AuthManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(ApiManager* apiManager, AuthManager* authManager,
                           QWidget* parent = nullptr);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    // General settings
    QLineEdit* serverUrlEdit_;
    QComboBox* themeCombo_;
    QSpinBox* pageSizeSpin_;
    QCheckBox* autoRefreshCheck_;
    QSpinBox* refreshIntervalSpin_;

    // Search settings
    QCheckBox* saveHistoryCheck_;
    QSpinBox* maxHistorySpin_;
    QCheckBox* autoSearchCheck_;

    ApiManager* apiManager_;
    AuthManager* authManager_;
};
