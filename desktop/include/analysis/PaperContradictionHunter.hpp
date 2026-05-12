#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContradictionEntry {
    int id; QString claim1; QString category; QString claim2;
    qreal conflict; int sources; bool resolved; QColor color;
};
class PaperContradictionHunter : public QWidget {
    Q_OBJECT
public:
    explicit PaperContradictionHunter(QWidget* parent = nullptr);
    void addEntry(const ContradictionEntry& entry);
    QList<ContradictionEntry> entries() const;
    int resolvedCount() const;
    qreal avgConflict() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void conflictFound(int id, qreal conflict);
private slots:
    void onHunt();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawConflictMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContradictionEntry> entries_;
    QSettings settings_;
    QPushButton* huntBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
