#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>

struct ReportPaper {
    int id{-1};
    QString title;
    QStringList authors;
    int year{0};
    QString journal;
    QString abstractText;
    QString doi;
    int rating{0};
};

class PaperReportGenerator : public QWidget {
    Q_OBJECT

public:
    explicit PaperReportGenerator(QWidget* parent = nullptr);

    void addPaper(const ReportPaper& paper);
    void addPapers(const QList<ReportPaper>& papers);
    void clearPapers();
    QString generateReport() const;
    int paperCount() const;

signals:
    void reportGenerated(const QString& format, int count);
    void reportExported(const QString& path);

private slots:
    void onGenerate();
    void onCopy();
    void onExport();
    void onFormatChanged(int index);
    void onRemovePaper();

private:
    void setupUI();
    void refreshTable();

    QString formatHTML() const;
    QString formatMarkdown() const;
    QString formatPlainText() const;
    QString formatLatex() const;

    QTableWidget* paperTable_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QComboBox* formatCombo_{nullptr};
    QPushButton* generateBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QComboBox* sectionCombo_{nullptr};

    QList<ReportPaper> papers_;
    int currentFormat_{0};
};
