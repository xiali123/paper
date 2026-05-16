#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ImpactEntry {
    int id; QString paper; QString category; QString venue;
    qreal impact; qreal citations; qreal hIndex; bool highImpact; QColor color;
};
class PaperImpactPredictor : public QWidget {
    Q_OBJECT
public:
    explicit PaperImpactPredictor(QWidget* parent = nullptr);
    void addEntry(const ImpactEntry& entry);
    QList<ImpactEntry> entries() const;
    int highImpactCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void impactPredicted(int id, qreal impact);
private slots:
    void onPredict();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawImpactList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ImpactEntry> entries_;
    QSettings settings_;
    QPushButton* predictBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
