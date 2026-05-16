#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Bias2Entry {
    int id; QString claim; QString category; QString biasType;
    qreal severity; int instances; bool confirmed; QColor color;
};
class PaperBiasDetector2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBiasDetector2(QWidget* parent = nullptr);
    void addEntry(const Bias2Entry& entry);
    QList<Bias2Entry> entries() const;
    int confirmedCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void biasFound(int id, qreal severity);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBiasMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Bias2Entry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
