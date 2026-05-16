#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QMap>
#include <QList>

struct ExtractedInfo {
    QString objective;
    QString method;
    QString result;
    QString conclusion;
    QStringList keywords;
    QString domain;
};

class AbstractSummaryWidget : public QWidget {
    Q_OBJECT

public:
    explicit AbstractSummaryWidget(QWidget* parent = nullptr);

    void setAbstract(const QString& text);
    ExtractedInfo extract(const QString& text) const;
    QList<ExtractedInfo> batchExtract(const QList<QString>& abstracts) const;
    QString generateStructuredSummary(const ExtractedInfo& info) const;

signals:
    void extractionCompleted(const ExtractedInfo& info);
    void batchCompleted(int count);

private slots:
    void onExtract();
    void onBatch();
    void onCopy();
    void onClear();
    void onFormatChanged(int index);

private:
    void setupUI();
    void displayResult(const ExtractedInfo& info);

    QTextEdit* inputEdit_{nullptr};
    QTextEdit* outputEdit_{nullptr};
    QListWidget* keywordList_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* extractBtn_{nullptr};
    QPushButton* batchBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QComboBox* formatCombo_{nullptr};

    ExtractedInfo lastResult_;
};
