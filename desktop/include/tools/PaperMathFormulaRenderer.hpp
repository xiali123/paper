#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QSettings>

struct FormulaEntry {
    int id{-1};
    QString latex;
    QString label;
    QString category; // "algebra", "calculus", "linear-algebra", "probability", "physics"
    QColor color;
};

class PaperMathFormulaRenderer : public QWidget {
    Q_OBJECT

public:
    explicit PaperMathFormulaRenderer(QWidget* parent = nullptr);

    void addFormula(const FormulaEntry& formula);
    QList<FormulaEntry> formulas() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void formulaRendered(const QString& latex);
    void formulaClicked(int id);

private slots:
    void onAdd();
    void onRender();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawFormulaCards(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawPreview(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* renderBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<FormulaEntry> formulas_;
    int selectedFormula_{-1};
    QSettings settings_;
};
