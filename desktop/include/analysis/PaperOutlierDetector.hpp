#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct OutlierEntry {
    int id;
    QString point;
    QString category;
    QString method;
    qreal value;
    qreal zScore;
    qreal threshold;
    bool outlier;
    QColor color;
};

class PaperOutlierDetector : public QWidget {
    Q_OBJECT
public:
    explicit PaperOutlierDetector(QWidget* parent = nullptr);
    void addEntry(const OutlierEntry& entry);
    QList<OutlierEntry> entries() const;
    int outlierCount() const;
    qreal avgZScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void outlierDetected(int id, qreal zScore);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawOutlierList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<OutlierEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
