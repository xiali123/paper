#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct BulletEntry {
    int id;
    QString label;
    QString category;
    qreal actual;
    qreal target;
    qreal rangeMax;
    bool onTrack;
    QColor color;
};

class PaperBulletChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperBulletChart(QWidget* parent = nullptr);
    void addEntry(const BulletEntry& entry);
    QList<BulletEntry> entries() const;
    int onTrackCount() const;
    qreal avgActual() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void chartGenerated(int id, qreal actual);
private slots:
    void onGenerate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBulletView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BulletEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
