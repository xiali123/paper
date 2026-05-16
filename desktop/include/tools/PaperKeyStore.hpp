#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct KeyEntry {
    int id; QString name; QString category; QString type;
    QString created; QString expires; bool active; bool expired; QColor color;
};
class PaperKeyStore : public QWidget {
    Q_OBJECT
public:
    explicit PaperKeyStore(QWidget* parent = nullptr);
    void addEntry(const KeyEntry& entry);
    QList<KeyEntry> entries() const;
    int activeCount() const;
    int expiredCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void keyStored(int id, const QString& type);
private slots:
    void onStore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawKeyList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<KeyEntry> entries_;
    QSettings settings_;
    QPushButton* storeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
