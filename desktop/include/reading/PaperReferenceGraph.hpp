#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RefEntry {
    int id; QString paper; QString category; QString relation;
    int citations; qreal relevance; int year; bool seminal; QColor color;
};
class PaperReferenceGraph : public QWidget {
    Q_OBJECT
public:
    explicit PaperReferenceGraph(QWidget* parent = nullptr);
    void addEntry(const RefEntry& entry);
    QList<RefEntry> entries() const;
    int seminalCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void referenceAdded(int id, qreal relevance);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRefList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RefEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
