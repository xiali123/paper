#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Grant2Entry {
    int id; QString title; QString category; QString funder;
    qreal amount; int duration; bool awarded; QColor color;
};
class PaperResearchGrant2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperResearchGrant2(QWidget* parent = nullptr);
    void addEntry(const Grant2Entry& entry);
    QList<Grant2Entry> entries() const;
    int awardedCount() const;
    qreal totalAmount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void grantSubmitted(int id, qreal amount);
private slots:
    void onSubmit();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGrantList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Grant2Entry> entries_;
    QSettings settings_;
    QPushButton* submitBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
