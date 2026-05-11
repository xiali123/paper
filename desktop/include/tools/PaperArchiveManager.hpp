#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArchiveEntry {
    int id; QString name; QString category; QString format;
    int size; QString date; bool compressed; bool restored; QColor color;
};
class PaperArchiveManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperArchiveManager(QWidget* parent = nullptr);
    void addEntry(const ArchiveEntry& entry);
    QList<ArchiveEntry> entries() const;
    int compressedCount() const;
    int totalSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void archiveCreated(int id, int size);
private slots:
    void onCreate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawArchiveList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArchiveEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
