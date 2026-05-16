#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct RiskEntry {
    int id;
    QString risk;
    QString category;
    QString likelihood;
    QString impact;
    int score;
    QString mitigation;
    bool critical;
    QColor color;
};

class PaperRiskMatrix : public QWidget {
    Q_OBJECT
public:
    explicit PaperRiskMatrix(QWidget* parent = nullptr);
    void addEntry(const RiskEntry& entry);
    QList<RiskEntry> entries() const;
    int criticalCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void riskIdentified(int id, int score);

private slots:
    void onAssess();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawRiskList(QPainter& p, const QRect& rect);
    void drawMatrixView(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<RiskEntry> entries_;
    QSettings settings_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
