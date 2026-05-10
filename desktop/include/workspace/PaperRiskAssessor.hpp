#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct RiskEntry {
    int id;
    QString riskName;
    QString category;
    qreal probability;
    qreal impact;
    qreal score;
    QString mitigation;
    QString status;
    QString owner;
    bool critical;
    QColor color;
};

class PaperRiskAssessor : public QWidget {
    Q_OBJECT
public:
    explicit PaperRiskAssessor(QWidget* parent = nullptr);
    void addEntry(const RiskEntry& entry);
    QList<RiskEntry> entries() const;
    qreal avgScore() const;
    int criticalCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void riskAssessed(int id, qreal score);

private slots:
    void onAssess();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawRiskList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<RiskEntry> entries_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
