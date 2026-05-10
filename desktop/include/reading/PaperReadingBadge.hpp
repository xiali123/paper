#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct BadgeEntry {
    int id;
    QString badgeName;
    QString badgeType;
    int papersRequired;
    int papersRead;
    qreal progress;
    QString tier;
    QString icon;
    bool earned;
    QColor color;
};

class PaperReadingBadge : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingBadge(QWidget* parent = nullptr);
    void addEntry(const BadgeEntry& entry);
    QList<BadgeEntry> entries() const;
    int earnedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> tierCounts() const;
signals:
    void badgeEarned(int id, qreal progress);
private slots:
    void onEarn();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawBadgeList(QPainter& p, const QRect& rect);
    void drawTierChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BadgeEntry> entries_;
    QSettings settings_;
    QPushButton* earnBtn_;
    QPushButton* clearBtn_;
    QComboBox* tierCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
