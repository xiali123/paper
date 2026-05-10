#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ConfigEntry {
    int id;
    QString key;
    QString value;
    QString type;
    QString category;
    QString description;
    bool modified;
    QString defaultValue;
    QColor color;
};

class PaperConfigEditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperConfigEditor(QWidget* parent = nullptr);
    void addEntry(const ConfigEntry& entry);
    QList<ConfigEntry> entries() const;
    int modifiedCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void configChanged(int id, QString key);
private slots:
    void onLoad();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawConfigList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConfigEntry> entries_;
    QSettings settings_;
    QPushButton* loadBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
