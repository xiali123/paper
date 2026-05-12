#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ApiVersionEntry {
    int id; QString endpoint; QString category; QString version;
    qreal latency; int calls; bool deprecated; QColor color;
};
class PaperApiVersion : public QWidget {
    Q_OBJECT
public:
    explicit PaperApiVersion(QWidget* parent = nullptr);
    void addEntry(const ApiVersionEntry& entry);
    QList<ApiVersionEntry> entries() const;
    int deprecatedCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void versionChecked(int id, qreal latency);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVersionView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ApiVersionEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
