#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogicProverEntry {
    int id; QString axiom; QString category; QString proof;
    qreal confidence; int steps; bool valid; QColor color;
};
class PaperLogicProver : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogicProver(QWidget* parent = nullptr);
    void addEntry(const LogicProverEntry& entry);
    QList<LogicProverEntry> entries() const;
    int validCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void proofCompleted(int id, qreal confidence);
private slots:
    void onProve();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawProverView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogicProverEntry> entries_;
    QSettings settings_;
    QPushButton* proveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
