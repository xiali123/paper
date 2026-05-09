#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct ReviewEntry {
    int id{-1};
    QString paperTitle;
    QString reviewer;
    QString recommendation; // "accept", "minor-revision", "major-revision", "reject"
    qreal quality{0};
    qreal clarity{0};
    qreal novelty{0};
    qreal significance{0};
    QString comments;
    QColor color;
};

class PaperPeerReviewTracker : public QWidget {
    Q_OBJECT

public:
    explicit PaperPeerReviewTracker(QWidget* parent = nullptr);

    void addReview(const ReviewEntry& review);
    QList<ReviewEntry> reviews() const;
    QMap<QString, int> recommendationCounts() const;
    qreal avgQuality() const;
    int acceptCount() const;

signals:
    void reviewAdded(int id, const QString& recommendation);
    void reviewsComplete(int total);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawReviewCards(QPainter& p, const QRect& rect);
    void drawScoreRadar(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ReviewEntry> reviews_;
    QSettings settings_;
};
