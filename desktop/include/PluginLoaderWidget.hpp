#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>

struct PluginInfo {
    QString name;
    QString version;
    QString author;
    QString description;
    QString path;
    bool enabled{true};
    bool loaded{false};
    QString status; // "loaded", "error", "disabled"
};

class PluginLoaderWidget : public QWidget {
    Q_OBJECT

public:
    explicit PluginLoaderWidget(QWidget* parent = nullptr);

    void scanPlugins(const QString& pluginDir);
    void loadPlugin(const QString& name);
    void unloadPlugin(const QString& name);
    void enablePlugin(const QString& name);
    void disablePlugin(const QString& name);

    QList<PluginInfo> plugins() const;
    QStringList loadedPluginNames() const;

signals:
    void pluginLoaded(const QString& name);
    void pluginUnloaded(const QString& name);
    void pluginError(const QString& name, const QString& error);
    void pluginsScanned(int count);

private slots:
    void onScan();
    void onLoad();
    void onUnload();
    void onToggle();

private:
    void setupUI();
    void refreshTable();
    QString defaultPluginDir() const;

    QTableWidget* pluginTable_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* scanBtn_{nullptr};
    QPushButton* loadBtn_{nullptr};
    QPushButton* unloadBtn_{nullptr};
    QPushButton* toggleBtn_{nullptr};

    QMap<QString, PluginInfo> plugins_;
};
