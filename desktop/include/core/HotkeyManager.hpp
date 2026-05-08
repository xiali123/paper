#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QKeySequence>
#include <QShortcut>

struct HotkeyAction {
    QString id;
    QString name;
    QString category;
    QKeySequence defaultKey;
    QKeySequence currentKey;
    QString description;
};

class HotkeyManager : public QWidget {
    Q_OBJECT

public:
    explicit HotkeyManager(QWidget* parent = nullptr);

    void registerAction(const QString& id, const QString& name,
                        const QString& category, const QKeySequence& defaultKey,
                        const QString& description = "");
    void removeAction(const QString& id);
    void resetAll();
    void resetAction(const QString& id);

    QKeySequence keyForAction(const QString& id) const;
    QList<HotkeyAction> actions() const;

    void loadSettings();
    void saveSettings();

    void applyShortcuts(QWidget* target);

signals:
    void shortcutChanged(const QString& id, const QKeySequence& newKey);
    void shortcutsReset();

private slots:
    void onEditShortcut();
    void onResetSelected();
    void onResetAll();

private:
    void setupUI();
    void refreshList();
    bool isKeyUsed(const QKeySequence& key, const QString& excludeId) const;

    QListWidget* listWidget_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* editBtn_{nullptr};

    QMap<QString, HotkeyAction> actions_;
    QMap<QString, QShortcut*> shortcuts_;
};
