#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EvidenceEntry {
    int id; QString claim; QString category; QString source;
    qreal weight; qreal reliability; qreal relevance; bool strong; QColor color;
};
class PaperEvidenceWeigher : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceWeigher(QWidget* parent = nullptr);
    void addEntry(const EvidenceEntry& entry);
    QList<EvidenceEntry> entries() const;
    int strongCount() const;
    qreal avgWeight() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void evidenceWeighed(int id, qreal weight);
private slots:
    void onWeigh();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEvidenceList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EvidenceEntry> entries_;
    QSettings settings_;
    QPushButton* weighBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
