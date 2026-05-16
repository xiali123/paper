#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ContractEntry {
    int id;
    QString contractId;
    QString vendor;
    QString type;
    qreal value;
    QString startDate;
    QString endDate;
    QString status;
    int papersCovered;
    bool active;
    QColor color;
};

class PaperContractManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperContractManager(QWidget* parent = nullptr);
    void addEntry(const ContractEntry& entry);
    QList<ContractEntry> entries() const;
    qreal totalValue() const;
    int activeCount() const;
    QMap<QString, int> typeCounts() const;
signals:
    void contractAdded(int id, qreal value);
private slots:
    void onAdd();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawContractList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContractEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
