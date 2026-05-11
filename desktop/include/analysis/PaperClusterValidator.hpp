#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClusterEntry {
    int id; QString cluster; QString category; QString method;
    int members; qreal silhouette; qreal cohesion; bool valid; QColor color;
};
class PaperClusterValidator : public QWidget {
    Q_OBJECT
public:
    explicit PaperClusterValidator(QWidget* parent = nullptr);
    void addEntry(const ClusterEntry& entry);
    QList<ClusterEntry> entries() const;
    int validCount() const;
    qreal avgSilhouette() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void clusterValidated(int id, qreal silhouette);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawClusterList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ClusterEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
