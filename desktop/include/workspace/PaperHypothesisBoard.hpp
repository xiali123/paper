#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HypothesisEntry {
    int id; QString hypothesis; QString category; QString status;
    qreal confidence; qreal support; qreal contradiction; bool validated; QColor color;
};
class PaperHypothesisBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperHypothesisBoard(QWidget* parent = nullptr);
    void addEntry(const HypothesisEntry& entry);
    QList<HypothesisEntry> entries() const;
    int validatedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void hypothesisAdded(int id, qreal confidence);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHypothesisList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HypothesisEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
