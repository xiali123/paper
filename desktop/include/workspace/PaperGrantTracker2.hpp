#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GrantTracker2Entry {
    int id; QString grant; QString category; QString agency;
    qreal amount; int milestones; bool funded; QColor color;
};
class PaperGrantTracker2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperGrantTracker2(QWidget* parent = nullptr);
    void addEntry(const GrantTracker2Entry& entry);
    QList<GrantTracker2Entry> entries() const;
    int fundedCount() const;
    qreal totalAmount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void grantUpdated(int id, qreal amount);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrackerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GrantTracker2Entry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
