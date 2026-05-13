#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationAuditEntry {
    int id; QString reference; QString category; QString status;
    qreal accuracy; int checks; bool verified; QColor color;
};
class PaperCitationAudit : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationAudit(QWidget* parent = nullptr);
    void addEntry(const CitationAuditEntry& entry);
    QList<CitationAuditEntry> entries() const;
    int verifiedCount() const;
    qreal avgAccuracy() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void citationAudited(int id, qreal accuracy);
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
    QList<CitationAuditEntry> entries_;
    QSettings settings_;
    QPushButton* auditBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
