#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SecretWardenEntry {
    int id; QString secret; QString category; QString severity;
    qreal risk; int occurrences; bool exposed; QColor color;
};
class PaperSecretWarden : public QWidget {
    Q_OBJECT
public:
    explicit PaperSecretWarden(QWidget* parent = nullptr);
    void addEntry(const SecretWardenEntry& entry);
    QList<SecretWardenEntry> entries() const;
    int exposedCount() const;
    qreal avgRisk() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void secretFound(int id, qreal risk);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWardenView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SecretWardenEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
