#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FlagEntry {
    int id; QString name; QString category; QString environment;
    bool enabled; qreal rollout; QString description; bool stable; QColor color;
};
class PaperFeatureFlag : public QWidget {
    Q_OBJECT
public:
    explicit PaperFeatureFlag(QWidget* parent = nullptr);
    void addEntry(const FlagEntry& entry);
    QList<FlagEntry> entries() const;
    int enabledCount() const;
    qreal avgRollout() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void flagToggled(int id, qreal rollout);
private slots:
    void onToggle();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFlagList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FlagEntry> entries_;
    QSettings settings_;
    QPushButton* toggleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
