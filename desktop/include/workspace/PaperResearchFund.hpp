#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FundEntry {
    int id; QString grant; QString category; QString agency;
    qreal amount; int years; bool active; QColor color;
};
class PaperResearchFund : public QWidget {
    Q_OBJECT
public:
    explicit PaperResearchFund(QWidget* parent = nullptr);
    void addEntry(const FundEntry& entry);
    QList<FundEntry> entries() const;
    int activeCount() const;
    qreal totalAmount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void fundAwarded(int id, qreal amount);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFundList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FundEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
