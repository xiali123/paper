#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct FormulaEntry {
    int id;
    QString name;
    QString latex;
    QString category;
    qreal relevance;
    int usageCount;
    QString source;
    QString description;
    QColor color;
};

class PaperFormulaSearchEngine : public QWidget {
    Q_OBJECT
public:
    explicit PaperFormulaSearchEngine(QWidget* parent = nullptr);
    void addFormula(const FormulaEntry& entry);
    QList<FormulaEntry> formulas() const;
    QMap<QString, int> categoryCounts() const;
    qreal avgRelevance() const;
    int totalUsage() const;

signals:
    void formulaFound(int id, const QString& name);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onSearch();
    void onClear();
    void drawFormulaList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<FormulaEntry> formulas_;
    QPushButton* searchBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
};
