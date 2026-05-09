#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct EntityEntry {
    int id{-1};
    QString text;
    QString type; // "person", "org", "location", "date", "concept"
    qreal confidence{0};
    int occurrences{0};
    QString context;
    QColor color;
};

class PaperEntityExtractor : public QWidget {
    Q_OBJECT

public:
    explicit PaperEntityExtractor(QWidget* parent = nullptr);

    void addEntity(const EntityEntry& entity);
    QList<EntityEntry> entities() const;
    QMap<QString, int> typeCounts() const;
    qreal avgConfidence() const;
    int uniqueEntities() const;

signals:
    void entityExtracted(int id, const QString& type);
    void extractionComplete(int total);

private slots:
    void onExtract();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawEntityList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* extractBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<EntityEntry> entities_;
    QSettings settings_;
};
