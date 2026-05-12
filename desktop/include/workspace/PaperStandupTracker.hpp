#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StandupEntry {
    int id; QString member; QString category; QString status;
    QString yesterday; QString today; QString blocker; bool blocked; QColor color;
};
class PaperStandupTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperStandupTracker(QWidget* parent = nullptr);
    void addEntry(const StandupEntry& entry);
    QList<StandupEntry> entries() const;
    int blockedCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void standupLogged(int id, const QString& member);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStandupList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StandupEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
