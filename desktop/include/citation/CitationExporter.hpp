#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include "core/PaperTypes.hpp"

class CitationExporter : public QWidget {
    Q_OBJECT

public:
    explicit CitationExporter(QWidget* parent = nullptr);

    void setPapers(const QList<Paper>& papers);
    void setFormat(const QString& format);

    QStringList supportedFormats() const;
    QString generateCitation(const Paper& paper, const QString& format) const;
    QString generateAll(const QString& format) const;

signals:
    void copiedToClipboard(const QString& text);
    void exportedToFile(const QString& path, int count);

private slots:
    void onFormatChanged(int index);
    void onCopyAll();
    void onCopySelected();
    void onExportFile();
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void refreshList();

    QListWidget* paperList_{nullptr};
    QListWidget* citationList_{nullptr};
    QComboBox* formatCombo_{nullptr};
    QLabel* countLabel_{nullptr};
    QLabel* previewLabel_{nullptr};

    QList<Paper> papers_;
    QString currentFormat_{"APA"};
};
