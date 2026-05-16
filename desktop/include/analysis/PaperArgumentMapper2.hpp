#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArgumentMapper2Entry {
    int id; QString argument; QString category; QString stance;
    qreal strength; int evidence; bool valid; QColor color;
};
class PaperArgumentMapper2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentMapper2(QWidget* parent = nullptr);
    void addEntry(const ArgumentMapper2Entry& entry);
    QList<ArgumentMapper2Entry> entries() const;
    int validCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void argumentMapped(int id, qreal strength);
private slots:
    void onMap();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMapView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArgumentMapper2Entry> entries_;
    QSettings settings_;
    QPushButton* mapBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
