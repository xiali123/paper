#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QMap>
#include <QKeySequence>

class ShortcutConfigDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutConfigDialog(QWidget* parent = nullptr);

    QMap<QString, QKeySequence> shortcuts() const;
    void setShortcuts(const QMap<QString, QKeySequence>& shortcuts);

signals:
    void shortcutsChanged(const QMap<QString, QKeySequence>& shortcuts);

private slots:
    void onResetDefaults();
    void onItemDoubleClicked(int row, int col);

private:
    void setupUI();
    void loadDefaults();

    QTableWidget* table_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QMap<QString, QKeySequence> currentShortcuts_;
};
