#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct LogEntry {
    int id;
    QString message;
    QString level;
    QString source;
    QString category;
    QString timestamp;
    bool error;
    QColor color;
};

class PaperLogInspector : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogInspector(QWidget* parent = nullptr);
    void addEntry(const LogEntry& entry);
    QList<LogEntry> entries() const;
    int errorCount() const;
    QMap<QString, int> levelCounts() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void logCaptured(int id, const QString& level);

private slots:
    void onCapture();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawLogList(QPainter& p, const QRect& rect);
    void drawLevelChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<LogEntry> entries_;
    QSettings settings_;
    QPushButton* captureBtn_;
    QPushButton* clearBtn_;
    QComboBox* levelCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
