#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ClaimEntry {
    int id;
    QString claimText;
    QString source;
    QString status;
    qreal confidence;
    int evidence;
    QString category;
    bool verified;
    QColor color;
};

class PaperClaimTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperClaimTracker(QWidget* parent = nullptr);
    void addEntry(const ClaimEntry& entry);
    QList<ClaimEntry> entries() const;
    int verifiedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> statusCounts() const;

signals:
    void claimTracked(int id, qreal confidence);

private slots:
    void onTrack();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawClaimList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* statusCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ClaimEntry> entries_;
};
