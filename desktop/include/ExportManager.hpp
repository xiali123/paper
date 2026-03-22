#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>

/**
 * @brief Paper data structure for export
 */
struct Paper {
    int id;
    QString title;
    QString journal;
    QString year;
    QString level;
    QString authors;
    QString doiUrl;
};

/**
 * @brief Export formats
 */
enum class ExportFormat {
    CSV,
    BibTeX,
    JSON,
    PDF  // TODO: Implement PDF export
};

/**
 * @brief Export manager for papers
 *
 * Handles exporting papers to various formats:
 * - CSV (spreadsheet compatible)
 * - BibTeX (LaTeX compatible)
 * - JSON (data exchange)
 * - PDF (formatted document - TODO)
 */
class ExportManager : public QObject {
    Q_OBJECT

public:
    explicit ExportManager(QObject* parent = nullptr);
    ~ExportManager() = default;

    // Export functions
    bool exportToCSV(const QString& fileName, const QList<Paper>& papers);
    bool exportToBibTeX(const QString& fileName, const QList<Paper>& papers);
    bool exportToJSON(const QString& fileName, const QList<Paper>& papers);
    bool exportToPDF(const QString& fileName, const QList<Paper>& papers);  // TODO

    // Helper function to show save dialog
    QString showSaveDialog(QWidget* parent, ExportFormat format);

    // Get default extension for format
    QString getExtension(ExportFormat format) const;
    QString getFilter(ExportFormat format) const;

signals:
    void exportProgress(int current, int total);
    void exportCompleted(const QString& fileName, int count);
    void exportFailed(const QString& error);

private:
    // Helper functions
    QString sanitizeForCSV(const QString& text) const;
    QString sanitizeForBibTeX(const QString& text) const;
    QString generateBibKey(const Paper& paper) const;
};
