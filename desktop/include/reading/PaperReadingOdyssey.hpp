#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct OdysseyEntry {
    int id; QString chapter; QString category; QString challenge;
    qreal progress; int pages; bool conquered; QColor color;
};
class PaperReadingOdyssey : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingOdyssey(QWidget* parent = nullptr);
    void addEntry(const OdysseyEntry& entry);
    QList<OdysseyEntry> entries() const;
    int conqueredCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void chapterDone(int id, qreal progress);
private slots:
    void onVoyage();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawOdysseyMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<OdysseyEntry> entries_;
    QSettings settings_;
    QPushButton* voyageBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
