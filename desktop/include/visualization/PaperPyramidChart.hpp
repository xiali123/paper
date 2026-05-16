#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct PyramidEntry {
    int id;
    QString label;
    QString category;
    qreal value;
    qreal percentage;
    int level;
    bool top;
    QColor color;
};

class PaperPyramidChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperPyramidChart(QWidget* parent = nullptr);
    void addEntry(const PyramidEntry& entry);
    QList<PyramidEntry> entries() const;
    int topCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void pyramidGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawPyramidView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<PyramidEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
