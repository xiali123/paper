#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DependencyEntry {
    int id; QString module; QString category; QString depends;
    int depth; qreal criticality; QString version; bool cyclic; QColor color;
};
class PaperDependencyGraph : public QWidget {
    Q_OBJECT
public:
    explicit PaperDependencyGraph(QWidget* parent = nullptr);
    void addEntry(const DependencyEntry& entry);
    QList<DependencyEntry> entries() const;
    int cyclicCount() const;
    qreal avgCriticality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dependencyAdded(int id, qreal criticality);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDependencyList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DependencyEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
