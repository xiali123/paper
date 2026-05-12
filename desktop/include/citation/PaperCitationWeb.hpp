#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WebEntry {
    int id; QString paper; QString category; QString relation;
    qreal weight; int citations; bool hub; QColor color;
};
class PaperCitationWeb : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationWeb(QWidget* parent = nullptr);
    void addEntry(const WebEntry& entry);
    QList<WebEntry> entries() const;
    int hubCount() const;
    qreal avgWeight() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void nodeSelected(int id, qreal weight);
private slots:
    void onBuild();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWebGraph(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WebEntry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
