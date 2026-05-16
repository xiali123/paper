#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct EntityEntry {
    int id;
    QString text;
    QString entityType;
    qreal confidence;
    int startPos;
    int endPos;
    QString context;
    QString category;
    bool verified;
    QColor color;
};

class PaperEntityRecognizer : public QWidget {
    Q_OBJECT
public:
    explicit PaperEntityRecognizer(QWidget* parent = nullptr);
    void addEntry(const EntityEntry& entry);
    QList<EntityEntry> entries() const;
    qreal avgConfidence() const;
    int verifiedCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void entityRecognized(int id, qreal confidence);

private slots:
    void onRecognize();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawEntityList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<EntityEntry> entries_;
    QPushButton* recognizeBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
