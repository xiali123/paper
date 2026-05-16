#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct LicenseEntry {
    int id;
    QString licenseName;
    QString type;
    QString holder;
    int papersCovered;
    qreal complianceScore;
    QString status;
    QString expiry;
    bool active;
    QString category;
    QColor color;
};

class PaperLicenseTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperLicenseTracker(QWidget* parent = nullptr);
    void addEntry(const LicenseEntry& entry);
    QList<LicenseEntry> entries() const;
    qreal avgCompliance() const;
    int activeCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void licenseTracked(int id, qreal complianceScore);

private slots:
    void onTrack();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawLicenseList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<LicenseEntry> entries_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
