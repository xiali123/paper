#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SecretWarden2Entry {
    int id; QString secret; QString category; QString vault;
    qreal rotation; int accesses; bool expired; QColor color;
};
class PaperSecretWarden2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperSecretWarden2(QWidget* parent = nullptr);
    void addEntry(const SecretWarden2Entry& entry);
    QList<SecretWarden2Entry> entries() const;
    int expiredCount() const;
    qreal avgRotation() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void secretRotated(int id, qreal rotation);
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
    QList<SecretWarden2Entry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
