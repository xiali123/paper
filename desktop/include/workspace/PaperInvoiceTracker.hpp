#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct InvoiceEntry {
    int id;
    QString invoiceId;
    QString vendor;
    qreal amount;
    qreal paid;
    qreal balance;
    QString status;
    QString category;
    QString dueDate;
    int papersRelated;
    bool overdue;
    QColor color;
};

class PaperInvoiceTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperInvoiceTracker(QWidget* parent = nullptr);
    void addEntry(const InvoiceEntry& entry);
    QList<InvoiceEntry> entries() const;
    qreal totalAmount() const;
    int overdueCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void invoiceTracked(int id, qreal balance);
private slots:
    void onAdd();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawInvoiceList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<InvoiceEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
