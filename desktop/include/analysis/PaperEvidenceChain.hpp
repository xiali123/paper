#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct EvidenceChainEntry {
    int id;
    QString claim;
    QString evidence;
    QString chainType;
    qreal strength;
    QString source;
    int links;
    qreal reliability;
    QString category;
    bool complete;
    QColor color;
};

class PaperEvidenceChain : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceChain(QWidget* parent = nullptr);
    void addEntry(const EvidenceChainEntry& entry);
    QList<EvidenceChainEntry> entries() const;
    qreal avgStrength() const;
    int completeCount() const;
    QMap<QString, int> chainTypeCounts() const;
signals:
    void chainBuilt(int id, qreal strength);
private slots:
    void onBuild();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawChainList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EvidenceChainEntry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
