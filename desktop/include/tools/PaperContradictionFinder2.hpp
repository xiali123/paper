#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContradictionFinder2Entry {
    int id; QString source1; QString category; QString source2;
    qreal conflict; int overlaps; bool resolved; QColor color;
};
class PaperContradictionFinder2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperContradictionFinder2(QWidget* parent = nullptr);
    void addEntry(const ContradictionFinder2Entry& entry);
    QList<ContradictionFinder2Entry> entries() const;
    int unresolvedCount() const;
    qreal avgConflict() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contradictionFound(int id, qreal conflict);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFinderView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContradictionFinder2Entry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
