#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QMap>
#include <QList>

struct LanguageResult {
    QString language;
    QString code;
    double confidence{0.0};
    int charCount{0};
    double percentage{0.0};
};

class LanguageDetectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit LanguageDetectorWidget(QWidget* parent = nullptr);

    LanguageResult detect(const QString& text) const;
    QList<LanguageResult> detectMulti(const QString& text) const;
    QMap<QString, int> batchDetect(const QList<QString>& texts) const;
    void setText(const QString& text);
    void setBatchTexts(const QList<QString>& texts);

signals:
    void detectionCompleted(const LanguageResult& result);
    void batchCompleted(const QMap<QString, int>& distribution);
    void languageClicked(const QString& language);

private slots:
    void onDetect();
    void onBatchDetect();
    void onClear();

private:
    void setupUI();
    void refreshResults(const QList<LanguageResult>& results);
    void refreshDistribution(const QMap<QString, int>& dist);

    QTextEdit* inputEdit_{nullptr};
    QTableWidget* resultTable_{nullptr};
    QTableWidget* distTable_{nullptr};
    QLabel* statusLabel_{nullptr};
    QPushButton* detectBtn_{nullptr};
    QPushButton* batchBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
};
