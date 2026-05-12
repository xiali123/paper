#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EnduranceEntry {
    int id; QString session; QString category; QString material;
    qreal stamina; int pages; bool sustained; QColor color;
};
class PaperReadingEndurance : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingEndurance(QWidget* parent = nullptr);
    void addEntry(const EnduranceEntry& entry);
    QList<EnduranceEntry> entries() const;
    int sustainedCount() const;
    qreal avgStamina() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void enduranceTracked(int id, qreal stamina);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEnduranceView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EnduranceEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
