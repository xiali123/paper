#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Marimekko3Entry {
    int id; QString segment; QString category; QString axis;
    qreal width; qreal height; bool highlight; QColor color;
};
class PaperMarimekko3 : public QWidget {
    Q_OBJECT
public:
    explicit PaperMarimekko3(QWidget* parent = nullptr);
    void addEntry(const Marimekko3Entry& entry);
    QList<Marimekko3Entry> entries() const;
    int highlightCount() const;
    qreal totalArea() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void segmentSelected(int id, qreal area);
private slots:
    void onLayout();
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
    QList<Marimekko3Entry> entries_;
    QSettings settings_;
    QPushButton* layoutBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
