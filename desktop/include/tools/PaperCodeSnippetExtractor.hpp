#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct SnippetEntry {
    int id;
    QString paperTitle;
    QString language;
    QString snippet;
    int lineCount;
    qreal confidence;
    QString category;
    QString algorithm;
    bool runnable;
    int figureNum;
    QColor color;
};

class PaperCodeSnippetExtractor : public QWidget {
    Q_OBJECT
public:
    explicit PaperCodeSnippetExtractor(QWidget* parent = nullptr);
    void addEntry(const SnippetEntry& entry);
    QList<SnippetEntry> entries() const;
    int runnableCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> languageCounts() const;

signals:
    void snippetExtracted(int id, const QString& language);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onExtract();
    void onClear();
    void drawSnippetList(QPainter& p, const QRect& rect);
    void drawLanguageChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* langCombo_;
    QPushButton* extractBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<SnippetEntry> entries_;
    QSettings settings_;
};
