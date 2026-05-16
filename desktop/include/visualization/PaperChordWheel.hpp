#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ChordEntry {
    int id; QString source; QString target; QString category; QString relation;
    qreal weight; int connections; bool bidirectional; QColor color;
};
class PaperChordWheel : public QWidget {
    Q_OBJECT
public:
    explicit PaperChordWheel(QWidget* parent = nullptr);
    void addEntry(const ChordEntry& entry);
    QList<ChordEntry> entries() const;
    int bidirectionalCount() const;
    qreal totalWeight() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void chordSelected(int id, qreal weight);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawChordWheel(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ChordEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
