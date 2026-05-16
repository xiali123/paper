#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClaimAuditEntry {
    int id; QString claim; QString category; QString evidence;
    qreal score; int sources; bool substantiated; QColor color;
};
class PaperClaimAuditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperClaimAuditor(QWidget* parent = nullptr);
    void addEntry(const ClaimAuditEntry& entry);
    QList<ClaimAuditEntry> entries() const;
    int substantiatedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void claimAudited(int id, qreal score);
private slots:
    void onAudit();
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
    QList<ClaimAuditEntry> entries_;
    QSettings settings_;
    QPushButton* auditBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
