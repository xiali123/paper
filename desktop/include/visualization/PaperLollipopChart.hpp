#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct LollipopEntry {
    int id;
    QString label;
    QString category;
    qreal value;
    qreal target;
    int rank;
    bool aboveTarget;
    QColor color;
};

class PaperLollipopChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperLollipopChart(QWidget* parent = nullptr);
    void addEntry(const LollipopEntry& entry);
    QList<LollipopEntry> entries() const;
    int aboveTargetCount() const;
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
    void drawLollipopView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LollipopEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
