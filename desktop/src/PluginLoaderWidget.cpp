#include "PluginLoaderWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QSettings>
#include <QDir>

PluginLoaderWidget::PluginLoaderWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PluginLoaderWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel("Plugin Manager");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(header);

    statsLabel_ = new QLabel("No plugins loaded");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    pluginTable_ = new QTableWidget();
    pluginTable_->setColumnCount(5);
    pluginTable_->setHorizontalHeaderLabels({"Name", "Version", "Author", "Status", "Path"});
    pluginTable_->horizontalHeader()->setStretchLastSection(true);
    pluginTable_->setColumnWidth(0, 120);
    pluginTable_->setColumnWidth(1, 60);
    pluginTable_->setColumnWidth(2, 100);
    pluginTable_->setColumnWidth(3, 60);
    pluginTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(pluginTable_, 1);

    auto* btnRow = new QHBoxLayout();

    scanBtn_ = new QPushButton("Scan Directory");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(scanBtn_, &QPushButton::clicked, this, &PluginLoaderWidget::onScan);
    btnRow->addWidget(scanBtn_);

    loadBtn_ = new QPushButton("Load");
    connect(loadBtn_, &QPushButton::clicked, this, &PluginLoaderWidget::onLoad);
    btnRow->addWidget(loadBtn_);

    unloadBtn_ = new QPushButton("Unload");
    connect(unloadBtn_, &QPushButton::clicked, this, &PluginLoaderWidget::onUnload);
    btnRow->addWidget(unloadBtn_);

    toggleBtn_ = new QPushButton("Enable/Disable");
    connect(toggleBtn_, &QPushButton::clicked, this, &PluginLoaderWidget::onToggle);
    btnRow->addWidget(toggleBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void PluginLoaderWidget::scanPlugins(const QString& pluginDir) {
    QDir dir(pluginDir);
    if (!dir.exists()) return;

    // Scan for shared libraries
    QStringList filters;
    #ifdef Q_OS_LINUX
    filters << "*.so";
    #elif defined(Q_OS_MAC)
    filters << "*.dylib";
    #elif defined(Q_OS_WIN)
    filters << "*.dll";
    #endif

    for (const auto& entry : dir.entryList(filters, QDir::Files)) {
        QString name = QFileInfo(entry).baseName();
        name.remove(0, 3); // remove "lib" prefix on Linux
        if (plugins_.contains(name)) continue;

        PluginInfo info;
        info.name = name;
        info.path = dir.absoluteFilePath(entry);
        info.enabled = true;
        info.loaded = false;
        info.status = "disabled";
        plugins_[name] = info;
    }

    refreshTable();
    emit pluginsScanned(plugins_.size());
}

void PluginLoaderWidget::loadPlugin(const QString& name) {
    if (!plugins_.contains(name)) return;
    auto& p = plugins_[name];
    if (!p.enabled) return;

    // Placeholder: in real implementation use QPluginLoader
    p.loaded = true;
    p.status = "loaded";
    refreshTable();
    emit pluginLoaded(name);
}

void PluginLoaderWidget::unloadPlugin(const QString& name) {
    if (!plugins_.contains(name)) return;
    auto& p = plugins_[name];
    p.loaded = false;
    p.status = "disabled";
    refreshTable();
    emit pluginUnloaded(name);
}

void PluginLoaderWidget::enablePlugin(const QString& name) {
    if (!plugins_.contains(name)) return;
    plugins_[name].enabled = true;
    refreshTable();
}

void PluginLoaderWidget::disablePlugin(const QString& name) {
    if (!plugins_.contains(name)) return;
    auto& p = plugins_[name];
    p.enabled = false;
    if (p.loaded) {
        p.loaded = false;
        p.status = "disabled";
    }
    refreshTable();
}

QList<PluginInfo> PluginLoaderWidget::plugins() const {
    return plugins_.values();
}

QStringList PluginLoaderWidget::loadedPluginNames() const {
    QStringList names;
    for (const auto& p : plugins_) {
        if (p.loaded) names << p.name;
    }
    return names;
}

void PluginLoaderWidget::onScan() {
    QString dir = QFileDialog::getExistingDirectory(this, "Plugin Directory",
        defaultPluginDir());
    if (dir.isEmpty()) return;
    scanPlugins(dir);
}

void PluginLoaderWidget::onLoad() {
    int row = pluginTable_->currentRow();
    if (row < 0) return;
    QString name = pluginTable_->item(row, 0)->text();
    loadPlugin(name);
}

void PluginLoaderWidget::onUnload() {
    int row = pluginTable_->currentRow();
    if (row < 0) return;
    QString name = pluginTable_->item(row, 0)->text();
    unloadPlugin(name);
}

void PluginLoaderWidget::onToggle() {
    int row = pluginTable_->currentRow();
    if (row < 0) return;
    QString name = pluginTable_->item(row, 0)->text();
    if (plugins_[name].enabled) disablePlugin(name);
    else enablePlugin(name);
}

void PluginLoaderWidget::refreshTable() {
    pluginTable_->setRowCount(plugins_.size());
    int row = 0;
    for (const auto& p : plugins_) {
        pluginTable_->setItem(row, 0, new QTableWidgetItem(p.name));
        pluginTable_->setItem(row, 1, new QTableWidgetItem(p.version));
        pluginTable_->setItem(row, 2, new QTableWidgetItem(p.author));

        auto* statusItem = new QTableWidgetItem(p.status);
        if (p.loaded) statusItem->setForeground(QColor("#059669"));
        else if (!p.enabled) statusItem->setForeground(QColor("#94a3b8"));
        else statusItem->setForeground(QColor("#d97706"));
        pluginTable_->setItem(row, 3, statusItem);

        pluginTable_->setItem(row, 4, new QTableWidgetItem(p.path));
        row++;
    }

    int loaded = 0;
    for (const auto& p : plugins_) if (p.loaded) loaded++;
    statsLabel_->setText(QString("%1 plugins (%2 loaded, %3 disabled)")
        .arg(plugins_.size()).arg(loaded).arg(plugins_.size() - loaded));
}

QString PluginLoaderWidget::defaultPluginDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/plugins";
}
