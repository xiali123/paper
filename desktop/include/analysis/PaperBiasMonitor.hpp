#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BiasEntry {
    int id; QString source; QString category; QString biasType;
    qreal score; qreal severity; int occurrences; bool flagged; QColor color;
};
class PaperBiasMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperBiasMonitor(QWidget* parent = nullptr);
    void addEntry(const BiasEntry& entry);
    QList<BiasEntry> entries() const;
    int flaggedCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void biasDetected(int id, qreal severity);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBiasList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BiasEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
