#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConceptDrift2Entry {
    int id; QString concept; QString category; QString direction;
    qreal magnitude; int periods; bool significant; QColor color;
};
class PaperConceptDrift2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperConceptDrift2(QWidget* parent = nullptr);
    void addEntry(const ConceptDrift2Entry& entry);
    QList<ConceptDrift2Entry> entries() const;
    int significantCount() const;
    qreal avgMagnitude() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void driftDetected(int id, qreal magnitude);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDriftView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConceptDrift2Entry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
