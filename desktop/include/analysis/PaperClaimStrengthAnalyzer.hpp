#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct ClaimStrengthEntry {
    int id;
    QString claim;
    QString evidence;
    qreal strength;
    QString type;
    qreal confidence;
    QString source;
    int supportCount;
    bool verified;
    QColor color;
};

class PaperClaimStrengthAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperClaimStrengthAnalyzer(QWidget* parent = nullptr);
    void addEntry(const ClaimStrengthEntry& entry);
    QList<ClaimStrengthEntry> entries() const;
    qreal avgStrength() const;
    int weakClaims() const;
    QMap<QString, int> typeCounts() const;

signals:
    void claimAnalyzed(int id, qreal strength);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAnalyze();
    void onClear();
    void drawClaimList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<ClaimStrengthEntry> entries_;
    QSettings settings_;
};
