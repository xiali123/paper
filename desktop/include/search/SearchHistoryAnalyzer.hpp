#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QList>
#include <QMap>
#include <QDate>
#include <QSettings>

struct SearchRecord {
    QString query;
    int results{0};
    QDate date;
    int clickedResults{0};
};

class SearchHistoryAnalyzer : public QWidget {
    Q_OBJECT

public:
    explicit SearchHistoryAnalyzer(QWidget* parent = nullptr);

    void addSearch(const QString& query, int results = 0);
    void clear();
    QList<SearchRecord> history() const;
    QStringList topQueries(int limit = 10) const;
    QMap<QString, int> queryFrequency() const;

signals:
    void querySelected(const QString& query);

private slots:
    void onClear();
    void onPeriodChanged(int index);
    void onQueryClicked();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawTopQueries(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void refreshList();
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* periodCombo_{nullptr};
    QListWidget* queryList_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<SearchRecord> history_;
    QSettings settings_;
};
