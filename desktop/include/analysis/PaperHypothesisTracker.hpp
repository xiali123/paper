#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct HypothesisEntry {
    int id;
    QString hypothesis;
    QString status;
    qreal confidence;
    QString evidence;
    int experiments;
    qreal probability;
    QString category;
    bool confirmed;
    QColor color;
};

class PaperHypothesisTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperHypothesisTracker(QWidget* parent = nullptr);
    void addEntry(const HypothesisEntry& entry);
    QList<HypothesisEntry> entries() const;
    qreal avgConfidence() const;
    int confirmedCount() const;
    QMap<QString, int> statusCounts() const;
signals:
    void hypothesisTracked(int id, qreal confidence);
private slots:
    void onTrack();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawHypothesisList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HypothesisEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
