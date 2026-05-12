#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingVoyage2Entry {
    int id; QString destination; QString category; QString vessel;
    qreal distance; int ports; bool arrived; QColor color;
};
class PaperReadingVoyage2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingVoyage2(QWidget* parent = nullptr);
    void addEntry(const ReadingVoyage2Entry& entry);
    QList<ReadingVoyage2Entry> entries() const;
    int arrivedCount() const;
    qreal avgDistance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void portReached(int id, qreal distance);
private slots:
    void onSail();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVoyageView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingVoyage2Entry> entries_;
    QSettings settings_;
    QPushButton* sailBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
