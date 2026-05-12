#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StaminaEntry {
    int id; QString session; QString category; QString period;
    qreal stamina; int pagesRead; bool sustained; QColor color;
};
class PaperReadingStaminaTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingStaminaTracker(QWidget* parent = nullptr);
    void addEntry(const StaminaEntry& entry);
    QList<StaminaEntry> entries() const;
    int sustainedCount() const;
    qreal avgStamina() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void staminaTracked(int id, qreal stamina);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStaminaChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StaminaEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
