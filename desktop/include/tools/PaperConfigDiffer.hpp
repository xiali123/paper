#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConfigEntry {
    int id; QString key; QString category; QString oldValue;
    QString newValue; int line; bool changed; bool breaking; QColor color;
};
class PaperConfigDiffer : public QWidget {
    Q_OBJECT
public:
    explicit PaperConfigDiffer(QWidget* parent = nullptr);
    void addEntry(const ConfigEntry& entry);
    QList<ConfigEntry> entries() const;
    int changedCount() const;
    int breakingCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void configDiffed(int id, int line);
private slots:
    void onDiff();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDiffList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConfigEntry> entries_;
    QSettings settings_;
    QPushButton* diffBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
