#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AnnotationSync2Entry {
    int id; QString annotation; QString category; QString device;
    qreal syncProgress; int edits; bool synced; QColor color;
};
class PaperAnnotationSync2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperAnnotationSync2(QWidget* parent = nullptr);
    void addEntry(const AnnotationSync2Entry& entry);
    QList<AnnotationSync2Entry> entries() const;
    int syncedCount() const;
    qreal avgSyncProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void annotationSynced(int id, qreal syncProgress);
private slots:
    void onSync();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSyncView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AnnotationSync2Entry> entries_;
    QSettings settings_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
