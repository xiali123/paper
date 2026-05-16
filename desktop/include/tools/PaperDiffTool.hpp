#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DiffFileEntry {
    int id; QString filename; QString category; QString change;
    int added; int removed; qreal similarity; QString date; bool conflict; QColor color;
};
class PaperDiffTool : public QWidget {
    Q_OBJECT
public:
    explicit PaperDiffTool(QWidget* parent = nullptr);
    void addEntry(const DiffFileEntry& entry);
    QList<DiffFileEntry> entries() const;
    int conflictCount() const;
    qreal avgSimilarity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void diffComputed(int id, qreal similarity);
private slots:
    void onCompute();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDiffList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DiffFileEntry> entries_;
    QSettings settings_;
    QPushButton* computeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
