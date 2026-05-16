#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct AreaEntry {
    int id;
    QString label;
    QString category;
    qreal value;
    qreal baseline;
    qreal delta;
    int order;
    bool positive;
    QColor color;
};

class PaperAreaChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperAreaChart(QWidget* parent = nullptr);
    void addEntry(const AreaEntry& entry);
    QList<AreaEntry> entries() const;
    int positiveCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void chartGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawAreaView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<AreaEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
