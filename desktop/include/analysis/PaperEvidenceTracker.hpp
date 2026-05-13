#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EvidenceTrackerEntry {
    int id; QString claim; QString category; QString evidence;
    qreal strength; int sources; bool verified; QColor color;
};
class PaperEvidenceTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceTracker(QWidget* parent = nullptr);
    void addEntry(const EvidenceTrackerEntry& entry);
    QList<EvidenceTrackerEntry> entries() const;
    int verifiedCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void evidenceTracked(int id, qreal strength);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrackerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EvidenceTrackerEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
