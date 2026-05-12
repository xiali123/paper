#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VelocityEntry {
    int id; QString period; QString category; QString trend;
    qreal papers; qreal pages; qreal hours; qreal rate; bool accelerating; QColor color;
};
class PaperReadingVelocity : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingVelocity(QWidget* parent = nullptr);
    void addEntry(const VelocityEntry& entry);
    QList<VelocityEntry> entries() const;
    int acceleratingCount() const;
    qreal avgRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void velocityMeasured(int id, qreal rate);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVelocityView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VelocityEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
