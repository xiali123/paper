#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct EvidenceEntry {
    int id;
    QString source;
    QString claim;
    QString type;
    qreal strength;
    qreal reliability;
    bool verified;
    QColor color;
};

class PaperEvidenceEvaluator : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceEvaluator(QWidget* parent = nullptr);
    void addEntry(const EvidenceEntry& entry);
    QList<EvidenceEntry> entries() const;
    int verifiedCount() const;
    qreal avgStrength() const;
    QMap<QString, int> typeCounts() const;

signals:
    void evidenceEvaluated(int id, qreal strength);

private slots:
    void onEvaluate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawEvidenceList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<EvidenceEntry> entries_;
    QSettings settings_;
    QPushButton* evalBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
