#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SecretEntry {
    int id; QString file; QString category; QString severity;
    qreal confidence; int occurrences; bool critical; QColor color;
};
class PaperSecretScanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperSecretScanner(QWidget* parent = nullptr);
    void addEntry(const SecretEntry& entry);
    QList<SecretEntry> entries() const;
    int criticalCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void secretFound(int id, qreal confidence);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSecretList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SecretEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
