#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogicEntry {
    int id; QString premise; QString category; QString conclusion;
    qreal validity; int steps; bool sound; QColor color;
};
class PaperLogicChain : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogicChain(QWidget* parent = nullptr);
    void addEntry(const LogicEntry& entry);
    QList<LogicEntry> entries() const;
    int soundCount() const;
    qreal avgValidity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void chainValidated(int id, qreal validity);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawChainDiagram(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogicEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
