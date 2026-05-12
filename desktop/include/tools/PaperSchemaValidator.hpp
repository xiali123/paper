#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SchemaEntry {
    int id; QString schema; QString category; QString version;
    qreal coverage; int fields; bool valid; QColor color;
};
class PaperSchemaValidator : public QWidget {
    Q_OBJECT
public:
    explicit PaperSchemaValidator(QWidget* parent = nullptr);
    void addEntry(const SchemaEntry& entry);
    QList<SchemaEntry> entries() const;
    int validCount() const;
    qreal avgCoverage() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void schemaValidated(int id, qreal coverage);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSchemaView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SchemaEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
