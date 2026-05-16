#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct ComplianceEntry {
    int id;
    QString ruleName;
    QString category;
    qreal score;
    QString status;
    QString framework;
    int checksTotal;
    int checksPassed;
    QString lastAudit;
    bool compliant;
    QColor color;
};

class PaperComplianceMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperComplianceMonitor(QWidget* parent = nullptr);
    void addEntry(const ComplianceEntry& entry);
    QList<ComplianceEntry> entries() const;
    qreal avgScore() const;
    int compliantCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void complianceChecked(int id, qreal score);

private slots:
    void onCheck();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawComplianceList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<ComplianceEntry> entries_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
