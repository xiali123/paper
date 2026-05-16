#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QMap>
#include <QList>
#include <QSettings>
#include <QPainter>

struct SessionEvent {
    QString action;
    qint64 timestamp{0};
    QString detail;
};

class SessionStatisticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit SessionStatisticsWidget(QWidget* parent = nullptr);

    void recordEvent(const QString& action, const QString& detail = "");
    void recordSearch(const QString& keyword);
    void recordPaperView(int paperId, const QString& title);
    void recordExport(const QString& format);
    void recordSessionStart();

    int totalSearches() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    int totalViews() const;
    int totalExports() const;
    int sessionCount() const;
    QString mostSearchedKeyword() const;

signals:
    void statsUpdated();

private slots:
    void onPeriodChanged(int index);
    void onReset();
    void onExport();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshDashboard();
    void refreshTable();
    void drawActivityChart(QPainter& p, const QRect& rect);

    QComboBox* periodCombo_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QLabel* summaryLabel_{nullptr};
    QTableWidget* recentTable_{nullptr};
    QTableWidget* topTable_{nullptr};
    QWidget* chartCanvas_{nullptr};

    QList<SessionEvent> events_;
    int sessionCount_{0};
    qint64 sessionStart_{0};
};
