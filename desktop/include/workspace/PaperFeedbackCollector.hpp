#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct FeedbackEntry {
    int id{-1};
    int paperId{-1};
    QString paperTitle;
    QString reviewer;
    QString category; // "quality", "clarity", "novelty", "methodology", "relevance"
    int rating{0}; // 1-5
    QString comment;
    QDate date;
};

class PaperFeedbackCollector : public QWidget {
    Q_OBJECT

public:
    explicit PaperFeedbackCollector(QWidget* parent = nullptr);

    void addFeedback(const FeedbackEntry& entry);
    QList<FeedbackEntry> feedbacks() const;
    qreal averageRating() const;
    QMap<QString, qreal> ratingsByCategory() const;
    int feedbackCount() const;

signals:
    void feedbackAdded(int paperId, int rating);
    void feedbackSummary(int count, qreal avg);

private slots:
    void onAdd();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawRatingChart(QPainter& p, const QRect& rect);
    void drawCategoryRadar(QPainter& p, const QRect& rect);
    void drawRecentList(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<FeedbackEntry> feedbacks_;
    QSettings settings_;
};
