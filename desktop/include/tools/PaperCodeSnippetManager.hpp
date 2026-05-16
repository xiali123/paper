#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct CodeSnippet {
    int id{-1};
    QString title;
    QString language;
    QString code;
    QString category; // "algorithm", "visualization", "data-processing", "utility"
    QString description;
    int usageCount{0};
    QColor color;
};

class PaperCodeSnippetManager : public QWidget {
    Q_OBJECT

public:
    explicit PaperCodeSnippetManager(QWidget* parent = nullptr);

    void addSnippet(const CodeSnippet& snippet);
    QList<CodeSnippet> snippets() const;
    QMap<QString, int> languageCounts() const;
    QMap<QString, int> categoryCounts() const;
    int totalUsage() const;

signals:
    void snippetAdded(int id);
    void snippetExecuted(const QString& code);

private slots:
    void onAdd();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawSnippetList(QPainter& p, const QRect& rect);
    void drawLanguageChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CodeSnippet> snippets_;
    QSettings settings_;
};
