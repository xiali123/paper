#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ClaimEntry {
    int id;
    QString claim;
    QString evidence;
    qreal supportScore;
    QString status;
    QString source;
    int references;
    qreal consistency;
    QString category;
    bool verified;
    QColor color;
};

class PaperClaimVerifier : public QWidget {
    Q_OBJECT
public:
    explicit PaperClaimVerifier(QWidget* parent = nullptr);
    void addEntry(const ClaimEntry& entry);
    QList<ClaimEntry> entries() const;
    qreal avgSupport() const;
    int verifiedCount() const;
    QMap<QString, int> statusCounts() const;
signals:
    void claimVerified(int id, qreal score);
private slots:
    void onVerify();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawClaimList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ClaimEntry> entries_;
    QSettings settings_;
    QPushButton* verifyBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
