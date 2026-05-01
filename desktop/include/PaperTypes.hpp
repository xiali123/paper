#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>

// Unified paper data model for desktop client
struct Paper {
    int id;
    QString title;
    QString journal;
    QString journalFull;
    QString journalShort;
    QString year;
    QString level;
    QString authors;
    QString doiUrl;
    QString journalUrl;
    QString type;
    QString abstract;

    Paper() : id(0) {}

    static Paper fromJson(const QJsonObject& json) {
        Paper p;
        p.id = json["id"].toInt();
        p.title = json["title"].toString();
        p.journal = json["journal"].toString();
        p.journalFull = json["journalFull"].toString();
        p.journalShort = json["journalShort"].toString();
        p.year = json["year"].toString();
        p.level = json["level"].toString();
        p.authors = json["authors"].toString();
        p.doiUrl = json["doiUrl"].toString();
        p.journalUrl = json["journalUrl"].toString();
        p.type = json["type"].toString();
        p.abstract = json["abstract"].toString();
        return p;
    }
};

// Search result structure
struct SearchResult {
    QList<Paper> papers;
    int total;
    int offset;
    int limit;
    qreal durationMs;
    QString query;
};

// Export format enum (single definition)
enum class ExportFormat {
    CSV,
    BibTeX,
    JSON,
    PDF
};
