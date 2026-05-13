#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingHorizon2Entry {
    int id; QString horizon; QString category; QString direction;
    qreal distance; int waypoints; bool reached; QColor color;
};
class PaperReadingHorizon2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingHorizon2(QWidget* parent = nullptr);
    void addEntry(const ReadingHorizon2Entry& entry);
    QList<ReadingHorizon2Entry> entries() const;
    int reachedCount() const;
    qreal avgDistance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void horizonReached(int id, qreal distance);
private slots:
    void onNavigate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHorizonView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingHorizon2Entry> entries_;
    QSettings settings_;
    QPushButton* navigateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
