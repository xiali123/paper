#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingSummit2Entry {
    int id; QString peak; QString category; QString approach;
    qreal altitude; int stages; bool summited; QColor color;
};
class PaperReadingSummit2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSummit2(QWidget* parent = nullptr);
    void addEntry(const ReadingSummit2Entry& entry);
    QList<ReadingSummit2Entry> entries() const;
    int summitedCount() const;
    qreal avgAltitude() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void peakReached(int id, qreal altitude);
private slots:
    void onClimb();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSummitView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingSummit2Entry> entries_;
    QSettings settings_;
    QPushButton* climbBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
