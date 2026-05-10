#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct FallacyEntry {
    int id;
    QString text;
    QString fallacyType;
    qreal severity;
    QString context;
    int occurrence;
    QString suggestion;
    QString category;
    bool critical;
    QColor color;
};

class PaperFallacyDetector : public QWidget {
    Q_OBJECT
public:
    explicit PaperFallacyDetector(QWidget* parent = nullptr);
    void addEntry(const FallacyEntry& entry);
    QList<FallacyEntry> entries() const;
    qreal avgSeverity() const;
    int criticalCount() const;
    QMap<QString, int> fallacyTypeCounts() const;
signals:
    void fallacyDetected(int id, qreal severity);
private slots:
    void onDetect();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawFallacyList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FallacyEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
