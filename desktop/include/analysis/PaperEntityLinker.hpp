#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EntityLinkEntry {
    int id; QString entity; QString category; QString linked;
    qreal confidence; qreal frequency; int cooccurrences; bool confirmed; QColor color;
};
class PaperEntityLinker : public QWidget {
    Q_OBJECT
public:
    explicit PaperEntityLinker(QWidget* parent = nullptr);
    void addEntry(const EntityLinkEntry& entry);
    QList<EntityLinkEntry> entries() const;
    int confirmedCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void entityLinked(int id, qreal confidence);
private slots:
    void onLink();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEntityList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EntityLinkEntry> entries_;
    QSettings settings_;
    QPushButton* linkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
