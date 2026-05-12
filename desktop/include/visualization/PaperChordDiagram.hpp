#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ChordEntry {
    int id; QString source; QString target; QString category;
    qreal value; qreal strength; qreal flow; bool bidirectional; QColor color;
};
class PaperChordDiagram : public QWidget {
    Q_OBJECT
public:
    explicit PaperChordDiagram(QWidget* parent = nullptr);
    void addEntry(const ChordEntry& entry);
    QList<ChordEntry> entries() const;
    int bidirectionalCount() const;
    qreal totalFlow() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void chordRendered(int id, qreal flow);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawChordView(QPainter& p, const QRect& rect);
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
