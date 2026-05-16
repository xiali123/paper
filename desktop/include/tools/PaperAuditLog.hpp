#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AuditEntry {
    int id; QString action; QString category; QString user;
    QString resource; QString timestamp; bool success; bool critical; QColor color;
};
class PaperAuditLog : public QWidget {
    Q_OBJECT
public:
    explicit PaperAuditLog(QWidget* parent = nullptr);
    void addEntry(const AuditEntry& entry);
    QList<AuditEntry> entries() const;
    int criticalCount() const;
    int failureCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void auditLogged(int id, const QString& action);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAuditList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AuditEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
