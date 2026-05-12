#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContractVaultEntry {
    int id; QString contract; QString category; QString party;
    qreal value; int daysRemaining; bool active; QColor color;
};
class PaperContractVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperContractVault(QWidget* parent = nullptr);
    void addEntry(const ContractVaultEntry& entry);
    QList<ContractVaultEntry> entries() const;
    int activeCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contractSigned(int id, qreal value);
private slots:
    void onSign();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVaultView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContractVaultEntry> entries_;
    QSettings settings_;
    QPushButton* signBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
