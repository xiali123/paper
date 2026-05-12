#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct NexusEntry {
    int id; QString node; QString category; QString link;
    qreal centrality; int connections; bool hub; QColor color;
};
class PaperReadingNexus : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingNexus(QWidget* parent = nullptr);
    void addEntry(const NexusEntry& entry);
    QList<NexusEntry> entries() const;
    int hubCount() const;
    qreal avgCentrality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void nodeLinked(int id, qreal centrality);
private slots:
    void onConnect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawNexusGraph(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NexusEntry> entries_;
    QSettings settings_;
    QPushButton* connectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
