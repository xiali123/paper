#include "workspace/ExportManager.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>

ExportManager::ExportManager(QObject* parent)
    : QObject(parent) {
}

bool ExportManager::exportToCSV(const QString& fileName, const QList<Paper>& papers) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFailed("无法打开文件: " + fileName);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // CSV Header with BOM for Excel compatibility
    out << "\uFEFF";  // UTF-8 BOM
    out << "ID,Title,Journal,Year,Level,Authors,DOI URL\n";

    // Write papers
    for (int i = 0; i < papers.size(); ++i) {
        const Paper& paper = papers.at(i);

        out << paper.id << ","
            << sanitizeForCSV(paper.title) << ","
            << sanitizeForCSV(paper.journal) << ","
            << paper.year << ","
            << paper.level << ","
            << sanitizeForCSV(paper.authors) << ","
            << sanitizeForCSV(paper.doiUrl) << "\n";

        emit exportProgress(i + 1, papers.size());
    }

    file.close();
    emit exportCompleted(fileName, papers.size());
    return true;
}

bool ExportManager::exportToBibTeX(const QString& fileName, const QList<Paper>& papers) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFailed("无法打开文件: " + fileName);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Write each paper as BibTeX entry
    for (int i = 0; i < papers.size(); ++i) {
        const Paper& paper = papers.at(i);
        QString bibKey = generateBibKey(paper);

        out << "@" << (paper.journal.contains("conference") || paper.journal.contains("CVPR") ||
                       paper.journal.contains("ICCV") || paper.journal.contains("NeurIPS") ? "inproceedings" : "article")
            << "{" << bibKey << ",\n";

        out << "  title = {" << sanitizeForBibTeX(paper.title) << "},\n";
        out << "  author = {" << sanitizeForBibTeX(paper.authors) << "},\n";
        out << "  journal = {" << sanitizeForBibTeX(paper.journal) << "},\n";
        out << "  year = {" << paper.year << "},\n";

        if (!paper.doiUrl.isEmpty()) {
            out << "  doi = {" << paper.doiUrl << "},\n";
            out << "  url = {" << paper.doiUrl << "},\n";
        }

        out << "  keywords = {" << paper.level << " CCF Level}\n";
        out << "}\n\n";

        emit exportProgress(i + 1, papers.size());
    }

    file.close();
    emit exportCompleted(fileName, papers.size());
    return true;
}

bool ExportManager::exportToJSON(const QString& fileName, const QList<Paper>& papers) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit exportFailed("无法打开文件: " + fileName);
        return false;
    }

    QJsonArray papersArray;
    for (int i = 0; i < papers.size(); ++i) {
        const Paper& paper = papers.at(i);

        QJsonObject paperObj;
        paperObj["id"] = paper.id;
        paperObj["title"] = paper.title;
        paperObj["journal"] = paper.journal;
        paperObj["year"] = paper.year;
        paperObj["level"] = paper.level;
        paperObj["authors"] = paper.authors;
        paperObj["doiUrl"] = paper.doiUrl;

        papersArray.append(paperObj);
        emit exportProgress(i + 1, papers.size());
    }

    QJsonObject root;
    root["papers"] = papersArray;
    root["total"] = papers.size();
    root["exportDate"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));

    file.close();
    emit exportCompleted(fileName, papers.size());
    return true;
}

bool ExportManager::exportToPDF(const QString& fileName, const QList<Paper>& papers) {
    // TODO: Implement PDF export using QtPdf or QPrinter
    emit exportFailed("PDF导出功能尚未实现");
    return false;
}

QString ExportManager::showSaveDialog(QWidget* parent, ExportFormat format) {
    QString defaultFileName = QString("papers_export_%1")
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    return QFileDialog::getSaveFileName(
        parent,
        "导出论文",
        defaultFileName + getExtension(format),
        getFilter(format)
    );
}

QString ExportManager::getExtension(ExportFormat format) const {
    switch (format) {
        case ExportFormat::CSV: return ".csv";
        case ExportFormat::BibTeX: return ".bib";
        case ExportFormat::JSON: return ".json";
        case ExportFormat::PDF: return ".pdf";
        default: return "";
    }
}

QString ExportManager::getFilter(ExportFormat format) const {
    switch (format) {
        case ExportFormat::CSV:
            return "CSV Files (*.csv);;All Files (*)";
        case ExportFormat::BibTeX:
            return "BibTeX Files (*.bib);;All Files (*)";
        case ExportFormat::JSON:
            return "JSON Files (*.json);;All Files (*)";
        case ExportFormat::PDF:
            return "PDF Files (*.pdf);;All Files (*)";
        default:
            return "All Files (*)";
    }
}

QString ExportManager::sanitizeForCSV(const QString& text) const {
    QString sanitized = text;

    // Escape quotes
    sanitized.replace("\"", "\"\"");

    // Wrap in quotes if contains special characters
    if (sanitized.contains(',') || sanitized.contains('"') || sanitized.contains('\n')) {
        sanitized = "\"" + sanitized + "\"";
    }

    return sanitized;
}

QString ExportManager::sanitizeForBibTeX(const QString& text) const {
    QString sanitized = text;

    // Basic sanitization for BibTeX
    sanitized.replace("{", "\\{");
    sanitized.replace("}", "\\}");
    sanitized.replace("$", "\\$");
    sanitized.replace("%", "\\%");
    sanitized.replace("#", "\\#");
    sanitized.replace("&", "\\&");

    return sanitized;
}

QString ExportManager::generateBibKey(const Paper& paper) const {
    // Generate BibTeX key: FirstAuthorLastName_Year_FirstWord
    QString firstAuthor = paper.authors.split(',').first().trimmed();

    // Extract last name
    QStringList nameParts = firstAuthor.split(' ');
    QString lastName = nameParts.isEmpty() ? "Unknown" : nameParts.last();

    // Clean last name
    lastName.remove(QRegularExpression("[^A-Za-z]"));

    // Get first word of title
    QString firstWord = paper.title.split(' ').first().toLower();
    firstWord.remove(QRegularExpression("[^A-Za-z]"));

    QString year = paper.year.trimmed();

    return QString("%1%2_%3").arg(lastName).arg(year).arg(firstWord);
}
