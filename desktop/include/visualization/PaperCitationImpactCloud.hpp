#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

struct ImpactEntry {
    int id;
    QString keyword;
    int citationCount;
    qreal impactScore;
    QString category;
    int papers;
    qreal trend;
    QColor color;
};

class PaperCitationImpactCloud : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationImpactCloud(QWidget* parent = nullptr);
    void addEntry(const ImpactEntry& entry);
    QList<ImpactEntry> entries() const;
    QMap<QString, int> categoryCounts() const;
    qreal avgImpact() const;
    int totalCitations() const;

signals:
    void impactUpdated(int count);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawCloudView(QPainter& p, const QRect& rect);
    void drawImpactChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<ImpactEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
};
