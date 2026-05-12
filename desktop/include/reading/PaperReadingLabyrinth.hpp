#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LabyrinthEntry {
    int id; QString section; QString category; QString path;
    qreal complexity; int turns; bool solved; QColor color;
};
class PaperReadingLabyrinth : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingLabyrinth(QWidget* parent = nullptr);
    void addEntry(const LabyrinthEntry& entry);
    QList<LabyrinthEntry> entries() const;
    int solvedCount() const;
    qreal avgComplexity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void mazeSolved(int id, qreal complexity);
private slots:
    void onNavigate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLabyrinthMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LabyrinthEntry> entries_;
    QSettings settings_;
    QPushButton* navigateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
