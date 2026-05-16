#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SchemaMigrationEntry {
    int id; QString migration; QString category; QString status;
    qreal progress; int steps; bool applied; QColor color;
};
class PaperSchemaMigrator : public QWidget {
    Q_OBJECT
public:
    explicit PaperSchemaMigrator(QWidget* parent = nullptr);
    void addEntry(const SchemaMigrationEntry& entry);
    QList<SchemaMigrationEntry> entries() const;
    int appliedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void migrationComplete(int id, qreal progress);
private slots:
    void onMigrate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMigrationList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SchemaMigrationEntry> entries_;
    QSettings settings_;
    QPushButton* migrateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
