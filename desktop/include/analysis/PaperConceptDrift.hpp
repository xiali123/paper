#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConceptDriftEntry {
    int id; QString concept; QString category; QString period;
    qreal driftRate; int samples; bool drifting; QColor color;
};
class PaperConceptDrift : public QWidget {
    Q_OBJECT
public:
    explicit PaperConceptDrift(QWidget* parent = nullptr);
    void addEntry(const ConceptDriftEntry& entry);
    QList<ConceptDriftEntry> entries() const;
    int driftingCount() const;
    qreal avgDriftRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void driftDetected(int id, qreal rate);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDriftChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConceptDriftEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
