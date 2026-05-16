#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SemanticEntry {
    int id; QString concept; QString category; QString relation;
    qreal similarity; qreal distance; int neighbors; bool core; QColor color;
};
class PaperSemanticMapper : public QWidget {
    Q_OBJECT
public:
    explicit PaperSemanticMapper(QWidget* parent = nullptr);
    void addEntry(const SemanticEntry& entry);
    QList<SemanticEntry> entries() const;
    int coreCount() const;
    qreal avgSimilarity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void conceptMapped(int id, qreal similarity);
private slots:
    void onMap();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSemanticList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SemanticEntry> entries_;
    QSettings settings_;
    QPushButton* mapBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
