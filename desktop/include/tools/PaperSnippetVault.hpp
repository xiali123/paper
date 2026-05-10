#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct SnippetEntry {
    int id;
    QString name;
    QString tag;
    QString category;
    QString language;
    int lines;
    int uses;
    bool favorite;
    QColor color;
};

class PaperSnippetVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperSnippetVault(QWidget* parent = nullptr);
    void addEntry(const SnippetEntry& entry);
    QList<SnippetEntry> entries() const;
    int favoriteCount() const;
    qreal avgLines() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void snippetStored(int id, const QString& name);

private slots:
    void onStore();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawSnippetList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<SnippetEntry> entries_;
    QSettings settings_;
    QPushButton* storeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
