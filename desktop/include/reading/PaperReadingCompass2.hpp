#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CompassEntry {
    int id; QString direction; QString category; QString paper;
    qreal score; int pagesRead; bool focused; QColor color;
};
class PaperReadingCompass2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingCompass2(QWidget* parent = nullptr);
    void addEntry(const CompassEntry& entry);
    QList<CompassEntry> entries() const;
    int focusedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void directionSet(int id, qreal score);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCompassView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CompassEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
