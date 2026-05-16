#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DeprecationEntry {
    int id; QString api; QString category; QString replacement;
    QString deadline; qreal usage; bool critical; QColor color;
};
class PaperDeprecationTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperDeprecationTracker(QWidget* parent = nullptr);
    void addEntry(const DeprecationEntry& entry);
    QList<DeprecationEntry> entries() const;
    int criticalCount() const;
    qreal avgUsage() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void deprecationTracked(int id, qreal usage);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDeprecationList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DeprecationEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
