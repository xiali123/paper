#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QList>

class CommandPalette : public QWidget {
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);

    void addAction(const QString& name, const QString& shortcut, const QString& category,
                   const std::function<void()>& callback);
    void showPalette();
    void hidePalette();

signals:
    void commandExecuted(const QString& name);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSearchChanged(const QString& text);
    void onItemActivated(QListWidgetItem* item);

private:
    void setupUI();
    void refreshList(const QString& filter);

    QLineEdit* searchEdit_{nullptr};
    QListWidget* listWidget_{nullptr};
    QLabel* hintLabel_{nullptr};

    struct Command {
        QString name;
        QString shortcut;
        QString category;
        std::function<void()> callback;
    };
    QList<Command> commands_;
};
