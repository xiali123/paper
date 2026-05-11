#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct StackEntry {
    int id;
    QString label;
    QString group;
    QString category;
    qreal value1;
    qreal value2;
    qreal value3;
    qreal total;
    bool highlighted;
    QColor color;
};

class PaperStackedChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperStackedChart(QWidget* parent = nullptr);
    void addEntry(const StackEntry& entry);
    QList<StackEntry> entries() const;
    int highlightedCount() const;
    qreal maxTotal() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void chartGenerated(int id, qreal total);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawStackedView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<StackEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
