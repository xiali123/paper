#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SecretEntry {
    int id; QString name; QString category; QString type;
    QString created; QString lastUsed; bool rotated; bool expired; QColor color;
};
class PaperSecretVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperSecretVault(QWidget* parent = nullptr);
    void addEntry(const SecretEntry& entry);
    QList<SecretEntry> entries() const;
    int rotatedCount() const;
    int expiredCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void secretStored(int id, const QString& type);
private slots:
    void onStore();
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
    QPushButton* storeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
