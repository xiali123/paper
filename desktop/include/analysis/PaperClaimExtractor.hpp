#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClaimExtractorEntry {
    int id; QString claim; QString category; QString source;
    qreal confidence; int references; bool verified; QColor color;
};
class PaperClaimExtractor : public QWidget {
    Q_OBJECT
public:
    explicit PaperClaimExtractor(QWidget* parent = nullptr);
    void addEntry(const ClaimExtractorEntry& entry);
    QList<ClaimExtractorEntry> entries() const;
    int verifiedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void claimExtracted(int id, qreal confidence);
private slots:
    void onExtract();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawClaimView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ClaimExtractorEntry> entries_;
    QSettings settings_;
    QPushButton* extractBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
