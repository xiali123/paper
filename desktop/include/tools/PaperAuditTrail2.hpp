#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AuditTrail2Entry {
    int id; QString action; QString category; QString user;
    qreal impact; int events; bool flagged; QColor color;
};
class PaperAuditTrail2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperAuditTrail2(QWidget* parent = nullptr);
    void addEntry(const AuditTrail2Entry& entry);
    QList<AuditTrail2Entry> entries() const;
    int flaggedCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void eventFlagged(int id, qreal impact);
private slots:
    void onAudit();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrailView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AuditTrail2Entry> entries_;
    QSettings settings_;
    QPushButton* auditBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
