#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContractEntry {
    int id; QString vendor; QString category; QString status;
    qreal value; int daysLeft; bool active; QColor color;
};
class PaperContractManager2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperContractManager2(QWidget* parent = nullptr);
    void addEntry(const ContractEntry& entry);
    QList<ContractEntry> entries() const;
    int activeCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contractAdded(int id, qreal value);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawContractList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContractEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
