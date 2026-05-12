#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VortexEntry {
    int id; QString paper; QString category; QString direction;
    qreal velocity; int rotations; bool diverging; QColor color;
};
class PaperReadingVortex : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingVortex(QWidget* parent = nullptr);
    void addEntry(const VortexEntry& entry);
    QList<VortexEntry> entries() const;
    int divergingCount() const;
    qreal avgVelocity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void vortexSpin(int id, qreal velocity);
private slots:
    void onSpin();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVortexDiagram(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VortexEntry> entries_;
    QSettings settings_;
    QPushButton* spinBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
