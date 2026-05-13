#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContradictionHunter2Entry {
    int id; QString claim; QString category; QString conflict;
    qreal severity; int contradictions; bool resolved; QColor color;
};
class PaperContradictionHunter2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperContradictionHunter2(QWidget* parent = nullptr);
    void addEntry(const ContradictionHunter2Entry& entry);
    QList<ContradictionHunter2Entry> entries() const;
    int unresolvedCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contradictionFound(int id, qreal severity);
private slots:
    void onHunt();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHunterView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContradictionHunter2Entry> entries_;
    QSettings settings_;
    QPushButton* huntBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
