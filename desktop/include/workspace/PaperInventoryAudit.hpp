#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct InventoryAuditEntry {
    int id; QString item; QString category; QString status;
    qreal quantity; int discrepancies; bool flagged; QColor color;
};
class PaperInventoryAudit : public QWidget {
    Q_OBJECT
public:
    explicit PaperInventoryAudit(QWidget* parent = nullptr);
    void addEntry(const InventoryAuditEntry& entry);
    QList<InventoryAuditEntry> entries() const;
    int flaggedCount() const;
    qreal totalQuantity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void auditFlagged(int id, int discrepancies);
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
    QList<InventoryAuditEntry> entries_;
    QSettings settings_;
    QPushButton* auditBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
