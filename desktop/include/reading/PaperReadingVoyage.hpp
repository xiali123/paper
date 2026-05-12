#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VoyageEntry {
    int id; QString destination; QString category; QString vessel;
    qreal distance; int ports; bool arrived; QColor color;
};
class PaperReadingVoyage : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingVoyage(QWidget* parent = nullptr);
    void addEntry(const VoyageEntry& entry);
    QList<VoyageEntry> entries() const;
    int arrivedCount() const;
    qreal avgDistance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void voyageComplete(int id, qreal distance);
private slots:
    void onSail();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVoyageMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VoyageEntry> entries_;
    QSettings settings_;
    QPushButton* sailBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
