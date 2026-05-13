#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SslCheckerEntry {
    int id; QString domain; QString category; QString protocol;
    qreal score; int days; bool valid; QColor color;
};
class PaperSslChecker : public QWidget {
    Q_OBJECT
public:
    explicit PaperSslChecker(QWidget* parent = nullptr);
    void addEntry(const SslCheckerEntry& entry);
    QList<SslCheckerEntry> entries() const;
    int validCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void certChecked(int id, qreal score);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCheckerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SslCheckerEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
