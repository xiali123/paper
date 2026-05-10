#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct FundingEntry {
    int id;
    QString grantName;
    QString agency;
    QString category;
    qreal amount;
    qreal spent;
    QString deadline;
    QString status;
    bool active;
    QColor color;
};

class PaperFundingMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperFundingMonitor(QWidget* parent = nullptr);
    void addEntry(const FundingEntry& entry);
    QList<FundingEntry> entries() const;
    qreal totalFunding() const;
    qreal totalSpent() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void fundingUpdated(int id, qreal amount);

private slots:
    void onTrack();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawFundingList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<FundingEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
