#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ApiTestEntry {
    int id;
    QString endpoint;
    QString method;
    int statusCode;
    qreal responseTime;
    QString status;
    int retries;
    QString category;
    int papersAffected;
    qreal successRate;
    bool passing;
    QColor color;
};

class PaperApiTester : public QWidget {
    Q_OBJECT
public:
    explicit PaperApiTester(QWidget* parent = nullptr);
    void addEntry(const ApiTestEntry& entry);
    QList<ApiTestEntry> entries() const;
    int passingCount() const;
    qreal avgResponseTime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void testCompleted(int id, qreal responseTime);
private slots:
    void onTest();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawTestList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ApiTestEntry> entries_;
    QSettings settings_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QComboBox* methodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
