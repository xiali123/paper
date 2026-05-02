#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QDateTime>

struct LatexDocument {
    int id{0};
    QString title;
    QString content;
    QString ownerId;
    bool isCollaborative{false};
    int version{1};
    bool isCompiled{false};
    QString pdfPath;
    QDateTime createdAt;
    QDateTime updatedAt;

    static LatexDocument fromJson(const QJsonObject& json) {
        LatexDocument doc;
        doc.id = json["id"].toInt();
        doc.title = json["title"].toString(json["name"].toString());
        doc.content = json["content"].toString();
        doc.ownerId = json["ownerId"].toString(json["owner_id"].toString());
        doc.isCollaborative = json["isCollaborative"].toBool(json["is_collaborative"].toBool());
        doc.version = json["version"].toInt(1);
        doc.isCompiled = json["isCompiled"].toBool(json["is_compiled"].toBool());
        doc.pdfPath = json["pdfPath"].toString(json["pdf_path"].toString());
        return doc;
    }

    QJsonObject toJson() const {
        QJsonObject json;
        json["id"] = id;
        json["title"] = title;
        json["content"] = content;
        json["ownerId"] = ownerId;
        return json;
    }
};

struct LatexTemplate {
    int id{0};
    QString name;
    QString description;
    QString category;
    QString content;
    QString icon;
    bool isBuiltIn{true};

    static LatexTemplate fromJson(const QJsonObject& json) {
        LatexTemplate t;
        t.id = json["id"].toInt();
        t.name = json["name"].toString();
        t.description = json["description"].toString();
        t.category = json["category"].toString();
        t.content = json["content"].toString(json["template"].toString());
        t.icon = json["icon"].toString();
        t.isBuiltIn = json["isBuiltIn"].toBool(json["is_built_in"].toBool(true));
        return t;
    }
};

struct LatexCompilationResult {
    bool success{false};
    QString pdfPath;
    QString log;
    QString errorMessage;
    int compileTimeMs{0};

    static LatexCompilationResult fromJson(const QJsonObject& json) {
        LatexCompilationResult r;
        r.success = json["success"].toBool();
        r.pdfPath = json["pdfPath"].toString(json["pdf_path"].toString());
        r.log = json["log"].toString(json["output"].toString());
        r.errorMessage = json["error"].toString(json["errorMessage"].toString());
        r.compileTimeMs = json["compileTimeMs"].toInt(json["compile_time_ms"].toInt(json["duration"].toInt()));
        return r;
    }
};

struct LatexProject {
    int id{0};
    QString name;
    QString ownerId;
    QString mainFile;
    QString description;
    bool isPublic{false};
    int version{1};

    static LatexProject fromJson(const QJsonObject& json) {
        LatexProject p;
        p.id = json["id"].toInt();
        p.name = json["name"].toString();
        p.ownerId = json["ownerId"].toString(json["owner_id"].toString());
        p.mainFile = json["mainFile"].toString(json["main_file"].toString());
        p.description = json["description"].toString();
        p.isPublic = json["isPublic"].toBool(json["is_public"].toBool());
        p.version = json["version"].toInt(1);
        return p;
    }
};

struct LatexProjectFile {
    int id{0};
    int projectId{0};
    QString name;
    QString path;
    QString content;
    QString type;

    static LatexProjectFile fromJson(const QJsonObject& json) {
        LatexProjectFile f;
        f.id = json["id"].toInt();
        f.projectId = json["projectId"].toInt(json["project_id"].toInt());
        f.name = json["name"].toString();
        f.path = json["path"].toString();
        f.content = json["content"].toString();
        f.type = json["type"].toString("other");
        return f;
    }
};
