#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PortScanner2Entry {
    int id; QString host; QString category; QString port;
    qreal response; int services; bool open; QColor color;
};
class PaperPortScanner2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperPortScanner2(QWidget* parent = nullptr);
    void addEntry(const PortScanner2Entry& entry);
    QList<PortScanner2Entry> entries() const;
    int openCount() const;
    qreal avgResponse() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void portScanned(int id, qreal response);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScanView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PortScanner2Entry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
