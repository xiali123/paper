#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TornadoEntry {
    int id; QString factor; QString category; QString side;
    qreal positive; qreal negative; qreal net; bool significant; QColor color;
};
class PaperTornadoChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperTornadoChart(QWidget* parent = nullptr);
    void addEntry(const TornadoEntry& entry);
    QList<TornadoEntry> entries() const;
    int significantCount() const;
    qreal maxNet() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void tornadoRendered(int id, qreal net);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTornadoView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TornadoEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
