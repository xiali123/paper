#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Marimekko2Entry {
    int id; QString segment; QString category; QString axis;
    qreal width; qreal height; int area; bool highlighted; QColor color;
};
class PaperMarimekko2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperMarimekko2(QWidget* parent = nullptr);
    void addEntry(const Marimekko2Entry& entry);
    QList<Marimekko2Entry> entries() const;
    int highlightedCount() const;
    qreal totalArea() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void segmentClicked(int id, qreal area);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMarimekko(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Marimekko2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
