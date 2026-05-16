#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct EvidenceEntry {
    int id;
    QString claim;
    QString evidenceType;
    qreal strength;
    QString source;
    int citations;
    QString quality;
    QString domain;
    bool reproducible;
    QColor color;
};

class PaperEvidenceStrength : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceStrength(QWidget* parent = nullptr);
    void addEntry(const EvidenceEntry& entry);
    QList<EvidenceEntry> entries() const;
    qreal avgStrength() const;
    int strongEvidence() const;
    QMap<QString, int> typeCounts() const;

signals:
    void evidenceEvaluated(int id, qreal strength);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onEvaluate();
    void onClear();
    void drawEvidenceList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* evaluateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<EvidenceEntry> entries_;
    QSettings settings_;
};
