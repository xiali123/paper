#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct OmegaEntry {
    int id; QString title; QString category; QString phase;
    qreal completion; int chapters; bool finished; QColor color;
};
class PaperReadingOmega : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingOmega(QWidget* parent = nullptr);
    void addEntry(const OmegaEntry& entry);
    QList<OmegaEntry> entries() const;
    int finishedCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void phaseCompleted(int id, qreal completion);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawOmegaView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<OmegaEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
