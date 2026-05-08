#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QGridLayout>
#include <QList>

struct SymbolItem {
    QString latex;      // e.g. \alpha
    QString display;    // e.g. α
    QString tooltip;    // e.g. "Greek lowercase alpha"
};

class LatexSymbolPalette : public QWidget {
    Q_OBJECT

public:
    explicit LatexSymbolPalette(QWidget* parent = nullptr);

signals:
    void symbolInsert(const QString& latex);

private:
    void setupUI();
    QWidget* createSymbolGrid(const QList<SymbolItem>& symbols);
    QPushButton* createSymbolButton(const SymbolItem& sym);

    QTabWidget* tabWidget_{nullptr};
};
