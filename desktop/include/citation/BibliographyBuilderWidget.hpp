#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>
#include <QSettings>

struct BibPaper {
    int id{-1};
    QString title;
    QStringList authors;
    int year{0};
    QString journal;
    QString doi;
    QString volume;
    QString pages;
    QString publisher;
    QString url;
    QString abstractText;
};

class BibliographyBuilderWidget : public QWidget {
    Q_OBJECT

public:
    explicit BibliographyBuilderWidget(QWidget* parent = nullptr);

    void addPaper(const BibPaper& paper);
    void addPapers(const QList<BibPaper>& papers);
    void removePaper(int paperId);
    void clearPapers();
    void setFormat(const QString& format);
    QString generateBibliography() const;
    QList<BibPaper> papers() const;

signals:
    void bibliographyGenerated(const QString& format, int count);
    void paperAdded(int paperId);
    void paperRemoved(int paperId);

private slots:
    void onAdd();
    void onRemove();
    void onMoveUp();
    void onMoveDown();
    void onGenerate();
    void onCopy();
    void onExport();
    void onFormatChanged(int index);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshList();
    void updateCount();

    QString formatAPA(const BibPaper& p) const;
    QString formatMLA(const BibPaper& p) const;
    QString formatChicago(const BibPaper& p) const;
    QString formatIEEE(const BibPaper& p) const;
    QString formatBibTeX(const BibPaper& p) const;
    QString formatVancouver(const BibPaper& p) const;

    QListWidget* paperList_{nullptr};
    QTextEdit* outputEdit_{nullptr};
    QComboBox* formatCombo_{nullptr};
    QPushButton* generateBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* upBtn_{nullptr};
    QPushButton* downBtn_{nullptr};
    QLabel* countLabel_{nullptr};
    QLabel* statusLabel_{nullptr};

    QList<BibPaper> papers_;
    int currentFormat_{0};
};
