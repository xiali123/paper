#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include "PaperTypes.hpp"

class ExportManager : public QObject {
    Q_OBJECT

public:
    explicit ExportManager(QObject* parent = nullptr);
    ~ExportManager() = default;

    bool exportToCSV(const QString& fileName, const QList<Paper>& papers);
    bool exportToBibTeX(const QString& fileName, const QList<Paper>& papers);
    bool exportToJSON(const QString& fileName, const QList<Paper>& papers);
    bool exportToPDF(const QString& fileName, const QList<Paper>& papers);

    QString showSaveDialog(QWidget* parent, ExportFormat format);
    QString getExtension(ExportFormat format) const;
    QString getFilter(ExportFormat format) const;

signals:
    void exportProgress(int current, int total);
    void exportCompleted(const QString& fileName, int count);
    void exportFailed(const QString& error);

private:
    QString sanitizeForCSV(const QString& text) const;
    QString sanitizeForBibTeX(const QString& text) const;
    QString generateBibKey(const Paper& paper) const;
};
