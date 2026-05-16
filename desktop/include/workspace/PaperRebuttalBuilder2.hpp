#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RebuttalBuilder2Entry {
    int id; QString point; QString category; QString response;
    qreal strength; int evidence; bool convincing; QColor color;
};
class PaperRebuttalBuilder2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperRebuttalBuilder2(QWidget* parent = nullptr);
    void addEntry(const RebuttalBuilder2Entry& entry);
    QList<RebuttalBuilder2Entry> entries() const;
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
    void drawBuilderView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RebuttalBuilder2Entry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
