#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct Insight {
    int id{-1};
    int paperId{-1};
    QString type;
    QString content;
    QString context;
    qreal confidence{0.0};
    qint64 timestamp{0};
};

class PaperInsightExtractor : public QWidget {
    Q_OBJECT

public:
    explicit PaperInsightExtractor(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void extractFromText(const QString& text);
    void addInsight(const Insight& insight);
    void removeInsight(int id);
    QList<Insight> insights() const;
    QList<Insight> insightsByType(const QString& type) const;

signals:
    void insightExtracted(int paperId, int insightId, const QString& type);
    void insightRemoved(int id);
    void extractionComplete(int paperId, int count);

private slots:
    void onExtract();
    void onDelete();
    void onFilterChanged(int index);
    void onInsightSelected();
    void onExport();

private:
    void setupUI();
    void refreshList();
    void updateStats();
    void loadSettings();
    void saveSettings();
    QList<Insight> doExtraction(const QString& text);

    QTextEdit* inputEdit_{nullptr};
    QListWidget* insightList_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* extractBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<Insight> insights_;
    int nextId_{1};
    int currentPaperId_{-1};
};
