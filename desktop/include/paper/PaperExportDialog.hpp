#pragma once

#include <QDialog>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QList>

struct Paper;

class PaperExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaperExportDialog(const QList<Paper>& papers, QWidget* parent = nullptr);

private slots:
    void onFormatChanged(int index);
    void onPreview();
    void onExport();

private:
    void setupUI();
    QString generateCsv() const;
    QString generateBibtex() const;
    QString generateMarkdown() const;
    QString generateJson() const;
    QString generateEndNote() const;
    QString generateRis() const;

    QList<Paper> papers_;

    QComboBox* formatCombo_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QLineEdit* filenameEdit_{nullptr};
    QCheckBox* includeAbstractCheck_{nullptr};
    QCheckBox* includeKeywordsCheck_{nullptr};
    QLabel* countLabel_{nullptr};
};
