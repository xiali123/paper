#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CookieAuditorEntry {
    int id; QString domain; QString category; QString cookie;
    qreal expiry; int count; bool secure; QColor color;
};
class PaperCookieAuditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperCookieAuditor(QWidget* parent = nullptr);
    void addEntry(const CookieAuditorEntry& entry);
    QList<CookieAuditorEntry> entries() const;
    int secureCount() const;
    qreal avgExpiry() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void cookieAudited(int id, qreal expiry);
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
    QList<CookieAuditorEntry> entries_;
    QSettings settings_;
    QPushButton* auditBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
