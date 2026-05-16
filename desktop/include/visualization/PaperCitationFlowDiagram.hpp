#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

struct FlowEntry {
    int id;
    QString source;
    QString target;
    qreal weight;
    QString flowType;
    int year;
    qreal strength;
    QString label;
    QColor color;
};

class PaperCitationFlowDiagram : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationFlowDiagram(QWidget* parent = nullptr);
    void addEntry(const FlowEntry& entry);
    QList<FlowEntry> entries() const;
    QMap<QString, int> typeCounts() const;
    qreal totalWeight() const;
    qreal avgStrength() const;

signals:
    void flowUpdated(int count);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawFlowView(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<FlowEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
};
