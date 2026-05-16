#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ExperimentLog2Entry {
    int id; QString experiment; QString category; QString result;
    qreal confidence; int trials; bool significant; QColor color;
};
class PaperExperimentLog2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperExperimentLog2(QWidget* parent = nullptr);
    void addEntry(const ExperimentLog2Entry& entry);
    QList<ExperimentLog2Entry> entries() const;
    int significantCount() const;
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
    void drawLogView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ExperimentLog2Entry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
