#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ExpEntry {
    int id; QString name; QString category; QString status;
    QString result; qreal confidence; QString date; bool success; QColor color;
};
class PaperExperimentLogger : public QWidget {
    Q_OBJECT
public:
    explicit PaperExperimentLogger(QWidget* parent = nullptr);
    void addEntry(const ExpEntry& entry);
    QList<ExpEntry> entries() const;
    int successCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void experimentLogged(int id, qreal confidence);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawExpList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ExpEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
