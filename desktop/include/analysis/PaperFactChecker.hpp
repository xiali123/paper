#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FactCheckerEntry {
    int id; QString statement; QString category; QString source;
    qreal accuracy; int verifications; bool confirmed; QColor color;
};
class PaperFactChecker : public QWidget {
    Q_OBJECT
public:
    explicit PaperFactChecker(QWidget* parent = nullptr);
    void addEntry(const FactCheckerEntry& entry);
    QList<FactCheckerEntry> entries() const;
    int confirmedCount() const;
    qreal avgAccuracy() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void factChecked(int id, qreal accuracy);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFactView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FactCheckerEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
