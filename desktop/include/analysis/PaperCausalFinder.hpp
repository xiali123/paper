#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CausalEntry {
    int id; QString cause; QString category; QString effect;
    qreal strength; qreal confidence; qreal lag; bool significant; QColor color;
};
class PaperCausalFinder : public QWidget {
    Q_OBJECT
public:
    explicit PaperCausalFinder(QWidget* parent = nullptr);
    void addEntry(const CausalEntry& entry);
    QList<CausalEntry> entries() const;
    int significantCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void causalFound(int id, qreal strength);
private slots:
    void onFind();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCausalList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CausalEntry> entries_;
    QSettings settings_;
    QPushButton* findBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
