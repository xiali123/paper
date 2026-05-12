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
    qreal timestamp; int changes; bool critical; QColor color;
};
class PaperAuditTrail : public QWidget {
    Q_OBJECT
public:
    explicit PaperAuditTrail(QWidget* parent = nullptr);
    void addEntry(const AuditEntry& entry);
    QList<AuditEntry> entries() const;
    int criticalCount() const;
    qreal totalChanges() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void auditRecorded(int id, qreal timestamp);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAuditView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AuditEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
