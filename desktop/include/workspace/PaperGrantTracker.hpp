#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct GrantEntry {
    int id{-1};
    QString title;
    QString agency;
    QString status; // "draft", "submitted", "reviewed", "awarded", "rejected"
    qreal amount{0};
    QDate deadline;
    QDate startDate;
    qreal progress{0};
    QString pi;
    QColor color;
};

class PaperGrantTracker : public QWidget {
    Q_OBJECT

public:
    explicit PaperGrantTracker(QWidget* parent = nullptr);

    void addGrant(const GrantEntry& grant);
    QList<GrantEntry> grants() const;
    QMap<QString, int> statusCounts() const;
    qreal totalFunding() const;
    qreal awardedFunding() const;

signals:
    void grantSubmitted(int id);
    void grantAwarded(int id, qreal amount);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawGrantCards(QPainter& p, const QRect& rect);
    void drawFundingChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<GrantEntry> grants_;
    QSettings settings_;
};
