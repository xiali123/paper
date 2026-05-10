#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ExpenseEntry {
    int id;
    QString description;
    QString category;
    qreal amount;
    QString date;
    QString project;
    int papersRelated;
    bool reimbursable;
    QString receipt;
    QColor color;
};

class PaperExpenseLogger : public QWidget {
    Q_OBJECT
public:
    explicit PaperExpenseLogger(QWidget* parent = nullptr);
    void addEntry(const ExpenseEntry& entry);
    QList<ExpenseEntry> entries() const;
    qreal totalAmount() const;
    int reimbursableCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void expenseLogged(int id, qreal amount);
private slots:
    void onAdd();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawExpenseList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ExpenseEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
