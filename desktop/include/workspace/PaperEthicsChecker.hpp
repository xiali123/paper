#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct EthicsEntry {
    int id;
    QString paperTitle;
    QString category;
    QString severity;
    qreal riskScore;
    QString description;
    QString recommendation;
    bool resolved;
    QColor color;
};

class PaperEthicsChecker : public QWidget {
    Q_OBJECT
public:
    explicit PaperEthicsChecker(QWidget* parent = nullptr);
    void addEntry(const EthicsEntry& entry);
    QList<EthicsEntry> entries() const;
    QMap<QString, int> categoryCounts() const;
    qreal avgRisk() const;
    int highRiskCount() const;

signals:
    void ethicsChecked(int id, qreal riskScore);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onCheck();
    void onClear();
    void drawIssueList(QPainter& p, const QRect& rect);
    void drawSeverityChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<EthicsEntry> entries_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
