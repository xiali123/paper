#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct AutoTagResult {
    int paperId{-1};
    QString paperTitle;
    QStringList suggestedTags;
    QStringList confirmedTags;
    qreal confidence{0};
    QColor color;
};

class PaperAutoTagger : public QWidget {
    Q_OBJECT

public:
    explicit PaperAutoTagger(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void autoTag();
    QList<AutoTagResult> results() const;
    QMap<QString, int> tagFrequency() const;
    int totalTagsGenerated() const;

signals:
    void taggingComplete(int papers, int tags);
    void tagsAccepted(int paperId, const QStringList& tags);

private slots:
    void onAutoTag();
    void onConfidenceChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTagCloud(QPainter& p, const QRect& rect);
    void drawConfidenceChart(QPainter& p, const QRect& rect);
    void drawPaperList(QPainter& p, const QRect& rect);
    void updateInfo();

    QComboBox* confidenceCombo_{nullptr};
    QPushButton* autoTagBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<AutoTagResult> results_;
};
