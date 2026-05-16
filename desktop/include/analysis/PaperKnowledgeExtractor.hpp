#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct KnowEntry {
    int id; QString entity; QString category; QString relation;
    qreal confidence; qreal frequency; int sources; bool verified; QColor color;
};
class PaperKnowledgeExtractor : public QWidget {
    Q_OBJECT
public:
    explicit PaperKnowledgeExtractor(QWidget* parent = nullptr);
    void addEntry(const KnowEntry& entry);
    QList<KnowEntry> entries() const;
    int verifiedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void knowledgeExtracted(int id, qreal confidence);
private slots:
    void onExtract();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawKnowledgeList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<KnowEntry> entries_;
    QSettings settings_;
    QPushButton* extractBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
