#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct LogEntry {
    int id;
    QString message;
    QString level;
    QString source;
    QString timestamp;
    QString category;
    int line;
    bool isError;
    QColor color;
};

class PaperLogViewer : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogViewer(QWidget* parent = nullptr);
    void addEntry(const LogEntry& entry);
    QList<LogEntry> entries() const;
    int errorCount() const;
    int warningCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void logAdded(int id, QString level);
private slots:
    void onFetch();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawLogList(QPainter& p, const QRect& rect);
    void drawLevelChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogEntry> entries_;
    QSettings settings_;
    QPushButton* fetchBtn_;
    QPushButton* clearBtn_;
    QComboBox* levelCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
