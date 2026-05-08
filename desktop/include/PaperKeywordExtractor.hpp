#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct KeywordResult {
    QString keyword;
    qreal score{0};
    int frequency{0};
    QString category;
    QColor color;
};

class PaperKeywordExtractor : public QWidget {
    Q_OBJECT

public:
    explicit PaperKeywordExtractor(QWidget* parent = nullptr);

    void setPaperText(const QString& text);
    void extract(int maxKeywords = 20);
    QList<KeywordResult> keywords() const;
    QStringList topKeywords(int limit = 10) const;
    QMap<QString, int> categoryCounts() const;

signals:
    void keywordsExtracted(int count);
    void keywordClicked(const QString& keyword);

private slots:
    void onExtract();
    void onMaxChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawKeywordCloud(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawScoreBars(QPainter& p, const QRect& rect);
    void updateInfo();

    QComboBox* maxCombo_{nullptr};
    QPushButton* extractBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<KeywordResult> keywords_;
    QString sourceText_;
    QSettings settings_;
};
