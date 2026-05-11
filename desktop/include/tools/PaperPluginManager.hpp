#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct PluginEntry {
    int id;
    QString name;
    QString version;
    QString category;
    QString author;
    int downloads;
    qreal rating;
    bool enabled;
    bool updated;
    QColor color;
};

class PaperPluginManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperPluginManager(QWidget* parent = nullptr);
    void addEntry(const PluginEntry& entry);
    QList<PluginEntry> entries() const;
    int enabledCount() const;
    qreal avgRating() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void pluginLoaded(int id, const QString& name);

private slots:
    void onLoad();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawPluginList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<PluginEntry> entries_;
    QSettings settings_;
    QPushButton* loadBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
