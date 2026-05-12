#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SemanticsEntry {
    int id; QString concept; QString category; QString relation;
    qreal similarity; qreal distance; qreal weight; bool core; QColor color;
};
class PaperSemanticsMapper : public QWidget {
    Q_OBJECT
public:
    explicit PaperSemanticsMapper(QWidget* parent = nullptr);
    void addEntry(const SemanticsEntry& entry);
    QList<SemanticsEntry> entries() const;
    int coreCount() const;
    qreal avgSimilarity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void semanticsMapped(int id, qreal similarity);
private slots:
    void onMap();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSemanticsMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SemanticsEntry> entries_;
    QSettings settings_;
    QPushButton* mapBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
