#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct CitationEntry {
    int id{-1};
    QString title;
    QString author;
    QString year;
    QString journal;
    QString volume;
    QString pages;
    QString doi;
};

struct StyledCitation {
    int entryId{-1};
    QString style;
    QString formatted;
};

class CitationStyleGenerator : public QWidget {
    Q_OBJECT

public:
    explicit CitationStyleGenerator(QWidget* parent = nullptr);

    void addEntry(const CitationEntry& entry);
    QList<CitationEntry> entries() const;
    StyledCitation generate(int entryId, const QString& style) const;
    QList<StyledCitation> generateAll(const QString& style) const;
    QStringList availableStyles() const;

signals:
    void citationGenerated(const QString& style, int count);

private slots:
    void onStyleChanged(int index);
    void onAdd();
    void onCopy();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawPreviewList(QPainter& p, const QRect& rect);
    void drawStyleComparison(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* styleCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CitationEntry> entries_;
    QSettings settings_;
};
