#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DepthEntry {
    int id; QString paper; QString category; QString level;
    qreal depth; qreal coverage; qreal mastery; bool expert; QColor color;
};
class PaperReadingDepth : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingDepth(QWidget* parent = nullptr);
    void addEntry(const DepthEntry& entry);
    QList<DepthEntry> entries() const;
    int expertCount() const;
    qreal avgDepth() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void depthMeasured(int id, qreal depth);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDepthView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DepthEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
