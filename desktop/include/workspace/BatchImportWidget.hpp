#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>
#include <QList>

struct ImportEntry {
    QString rawInput;       // DOI, URL, title, or file path
    QString type;           // "doi", "url", "file", "title"
    QString status;         // "pending", "fetching", "success", "failed"
    QString title;
    QString error;
};

class BatchImportWidget : public QWidget {
    Q_OBJECT

public:
    explicit BatchImportWidget(QWidget* parent = nullptr);

    void setApiManager(class ApiManager* api) { apiManager_ = api; }

signals:
    void importCompleted(int success, int failed);
    void paperImported(const QJsonObject& paperData);

private slots:
    void onAddEntries();
    void onAddFile();
    void onStartImport();
    void onStopImport();
    void onClearAll();

private:
    void setupUI();
    void parseEntries(const QString& text);
    void importNext();
    void updateStats();
    void detectType(ImportEntry& entry);

    QTableWidget* table_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* importBtn_{nullptr};
    QPushButton* stopBtn_{nullptr};
    QComboBox* sourceCombo_{nullptr};

    QList<ImportEntry> entries_;
    int currentIndex_{0};
    int successCount_{0};
    int failCount_{0};
    bool importing_{false};
    class ApiManager* apiManager_{nullptr};

    static constexpr int MAX_ENTRIES = 500;
};
