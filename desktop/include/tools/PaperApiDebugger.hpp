#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ApiDebuggerEntry {
    int id; QString endpoint; QString category; QString method;
    qreal latency; int requests; bool failing; QColor color;
};
class PaperApiDebugger : public QWidget {
    Q_OBJECT
public:
    explicit PaperApiDebugger(QWidget* parent = nullptr);
    void addEntry(const ApiDebuggerEntry& entry);
    QList<ApiDebuggerEntry> entries() const;
    int failingCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void endpointTested(int id, qreal latency);
private slots:
    void onDebug();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDebuggerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ApiDebuggerEntry> entries_;
    QSettings settings_;
    QPushButton* debugBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
