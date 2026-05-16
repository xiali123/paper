#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct PluginEntry {
    int id;
    QString pluginName;
    QString version;
    QString author;
    QString category;
    int downloads;
    qreal rating;
    QString status;
    QString installedDate;
    bool enabled;
    QColor color;
};

class PaperPluginInstaller : public QWidget {
    Q_OBJECT
public:
    explicit PaperPluginInstaller(QWidget* parent = nullptr);
    void addEntry(const PluginEntry& entry);
    QList<PluginEntry> entries() const;
    int enabledCount() const;
    qreal avgRating() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void pluginInstalled(int id, QString status);

private slots:
    void onInstall();
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

    QSettings settings_;
    QVector<PluginEntry> entries_;
    QPushButton* installBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
