#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RebuttalEntry {
    int id; QString point; QString category; QString counter;
    qreal strength; int references; bool convincing; QColor color;
};
class PaperRebuttalBuilder : public QWidget {
    Q_OBJECT
public:
    explicit PaperRebuttalBuilder(QWidget* parent = nullptr);
    void addEntry(const RebuttalEntry& entry);
    QList<RebuttalEntry> entries() const;
    int convincingCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rebuttalBuilt(int id, qreal strength);
private slots:
    void onBuild();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRebuttalView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RebuttalEntry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
