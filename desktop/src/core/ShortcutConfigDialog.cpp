#include "core/ShortcutConfigDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QSettings>

ShortcutConfigDialog::ShortcutConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Keyboard Shortcuts");
    resize(500, 400);
    setupUI();
    loadDefaults();
}

void ShortcutConfigDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    table_ = new QTableWidget();
    table_->setColumnCount(2);
    table_->setHorizontalHeaderLabels({"Action", "Shortcut"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &ShortcutConfigDialog::onItemDoubleClicked);
    layout->addWidget(table_, 1);

    auto* btnRow = new QHBoxLayout();

    resetBtn_ = new QPushButton("Reset Defaults");
    connect(resetBtn_, &QPushButton::clicked, this, &ShortcutConfigDialog::onResetDefaults);
    btnRow->addWidget(resetBtn_);

    btnRow->addStretch();

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        emit shortcutsChanged(currentShortcuts_);
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    btnRow->addWidget(buttonBox);

    layout->addLayout(btnRow);
}

void ShortcutConfigDialog::loadDefaults() {
    currentShortcuts_ = {
        {"Search", QKeySequence("Ctrl+S")},
        {"Advanced Search", QKeySequence("Ctrl+Shift+F")},
        {"Compile LaTeX", QKeySequence("Ctrl+Return")},
        {"Save LaTeX", QKeySequence("Ctrl+S")},
        {"Find/Replace", QKeySequence("Ctrl+F")},
        {"Replace", QKeySequence("Ctrl+H")},
        {"Toggle Comment", QKeySequence("Ctrl+/")},
        {"Toggle Theme", QKeySequence("Ctrl+T")},
        {"Preferences", QKeySequence("Ctrl+P")},
        {"Tab 1 - Search", QKeySequence("Ctrl+1")},
        {"Tab 2 - Favorites", QKeySequence("Ctrl+2")},
        {"Tab 3 - AI Chat", QKeySequence("Ctrl+3")},
        {"Tab 4 - Crawler", QKeySequence("Ctrl+4")},
        {"Tab 5 - Recommendations", QKeySequence("Ctrl+5")},
        {"Tab 6 - Admin", QKeySequence("Ctrl+6")},
        {"Tab 7 - Statistics", QKeySequence("Ctrl+7")},
        {"Tab 8 - LaTeX", QKeySequence("Ctrl+8")},
        {"Recent History", QKeySequence("Ctrl+H")},
    };

    table_->setRowCount(currentShortcuts_.size());
    int row = 0;
    for (auto it = currentShortcuts_.constBegin(); it != currentShortcuts_.constEnd(); ++it) {
        table_->setItem(row, 0, new QTableWidgetItem(it.key()));
        table_->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
        row++;
    }
}

QMap<QString, QKeySequence> ShortcutConfigDialog::shortcuts() const {
    return currentShortcuts_;
}

void ShortcutConfigDialog::setShortcuts(const QMap<QString, QKeySequence>& shortcuts) {
    currentShortcuts_ = shortcuts;
    table_->setRowCount(shortcuts.size());
    int row = 0;
    for (auto it = shortcuts.constBegin(); it != shortcuts.constEnd(); ++it) {
        table_->setItem(row, 0, new QTableWidgetItem(it.key()));
        table_->setItem(row, 1, new QTableWidgetItem(it.value().toString()));
        row++;
    }
}

void ShortcutConfigDialog::onResetDefaults() {
    loadDefaults();
}

void ShortcutConfigDialog::onItemDoubleClicked(int row, int col) {
    if (col != 1) return;

    QString action = table_->item(row, 0)->text();
    QKeySequence current = currentShortcuts_.value(action);

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Edit Shortcut: %1").arg(action));
    auto* layout = new QVBoxLayout(&dlg);

    auto* edit = new QKeySequenceEdit(current);
    layout->addWidget(edit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        QKeySequence newSeq = edit->keySequence();
        currentShortcuts_[action] = newSeq;
        table_->setItem(row, 1, new QTableWidgetItem(newSeq.toString()));
    }
}
