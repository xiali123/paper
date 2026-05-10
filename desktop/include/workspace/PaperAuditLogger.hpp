#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct AuditEntry {
    int id;
    QString action;
    QString actor;
    QString resource;
    QString timestamp;
    QString category;
    QString severity;
    QString details;
    bool suspicious;
    QColor color;
};

class PaperAuditLogger : public QWidget {
    Q_OBJECT
public:
    explicit PaperAuditLogger(QWidget* parent = nullptr);
    void addEntry(const AuditEntry& entry);
    QList<AuditEntry> entries() const;
    int suspiciousCount() const;
    QMap<QString, int> categoryCounts() const;
    QMap<QString, int> severityCounts() const;

signals:
    void auditLogged(int id, QString severity);

private slots:
    void onLog();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawAuditList(QPainter& p, const QRect& rect);
    void drawSeverityChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<AuditEntry> entries_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
