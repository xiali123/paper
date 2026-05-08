#pragma once

#include <QObject>
#include <QUrl>
#include <QStringList>

class QMimeData;
class QWidget;

class DragDropHandler : public QObject {
    Q_OBJECT

public:
    explicit DragDropHandler(QObject* parent = nullptr);

    void enableFor(QWidget* widget);

    struct DropResult {
        enum Type { PdfFile, Url, Text, BibFile, Unknown };
        Type type{Unknown};
        QString content;
        QStringList files;
        QList<QUrl> urls;
    };

    static DropResult parseMimeData(const QMimeData* mimeData);

signals:
    void pdfDropped(const QString& filePath);
    void urlDropped(const QUrl& url);
    void textDropped(const QString& text);
    void bibFileDropped(const QString& filePath);
    void searchRequested(const QString& query);
};
