#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RetroEntry {
    int id; QString topic; QString category; QString action;
    qreal impact; qreal effort; QString owner; bool completed; QColor color;
};
class PaperRetrospective : public QWidget {
    Q_OBJECT
public:
    explicit PaperRetrospective(QWidget* parent = nullptr);
    void addEntry(const RetroEntry& entry);
    QList<RetroEntry> entries() const;
    int completedCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void retroAdded(int id, qreal impact);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRetroList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RetroEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
