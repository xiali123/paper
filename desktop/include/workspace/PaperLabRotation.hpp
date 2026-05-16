#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RotationEntry {
    int id; QString project; QString category; QString phase;
    qreal duration; int experiments; bool complete; QColor color;
};
class PaperLabRotation : public QWidget {
    Q_OBJECT
public:
    explicit PaperLabRotation(QWidget* parent = nullptr);
    void addEntry(const RotationEntry& entry);
    QList<RotationEntry> entries() const;
    int completeCount() const;
    qreal totalDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rotationDone(int id, qreal duration);
private slots:
    void onRotate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRotationWheel(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RotationEntry> entries_;
    QSettings settings_;
    QPushButton* rotateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
