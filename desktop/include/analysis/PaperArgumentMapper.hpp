#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ArgumentEntry {
    int id;
    QString argument;
    QString argType;
    qreal strength;
    QString premise;
    int supportCount;
    qreal coherence;
    QString conclusion;
    bool valid;
    QColor color;
};

class PaperArgumentMapper : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentMapper(QWidget* parent = nullptr);
    void addEntry(const ArgumentEntry& entry);
    QList<ArgumentEntry> entries() const;
    qreal avgStrength() const;
    int validCount() const;
    QMap<QString, int> argTypeCounts() const;
signals:
    void argumentMapped(int id, qreal strength);
private slots:
    void onMap();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawArgumentList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArgumentEntry> entries_;
    QSettings settings_;
    QPushButton* mapBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
