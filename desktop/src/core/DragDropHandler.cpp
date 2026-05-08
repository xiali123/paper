#include "core/DragDropHandler.hpp"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QWidget>
#include <QFileInfo>

DragDropHandler::DragDropHandler(QObject* parent)
    : QObject(parent)
{
}

void DragDropHandler::enableFor(QWidget* widget) {
    widget->setAcceptDrops(true);
    widget->installEventFilter(this);

    // Override via event filter
    class DropFilter : public QObject {
        DragDropHandler* handler_;
    public:
        DropFilter(DragDropHandler* h, QObject* parent) : QObject(parent), handler_(h) {}

        bool eventFilter(QObject* watched, QEvent* event) override {
            if (event->type() == QEvent::DragEnter) {
                auto* de = static_cast<QDragEnterEvent*>(event);
                if (de->mimeData()->hasUrls() || de->mimeData()->hasText()) {
                    de->acceptProposedAction();
                    return true;
                }
            } else if (event->type() == QEvent::Drop) {
                auto* de = static_cast<QDropEvent*>(event);
                auto result = DragDropHandler::parseMimeData(de->mimeData());

                switch (result.type) {
                    case DropResult::PdfFile:
                        for (const auto& f : result.files) {
                            if (f.endsWith(".pdf", Qt::CaseInsensitive)) {
                                emit handler_->pdfDropped(f);
                            }
                        }
                        break;
                    case DropResult::BibFile:
                        for (const auto& f : result.files) {
                            if (f.endsWith(".bib", Qt::CaseInsensitive)) {
                                emit handler_->bibFileDropped(f);
                            }
                        }
                        break;
                    case DropResult::Url:
                        for (const auto& url : result.urls) {
                            emit handler_->urlDropped(url);
                        }
                        break;
                    case DropResult::Text:
                        emit handler_->textDropped(result.content);
                        emit handler_->searchRequested(result.content.trimmed());
                        break;
                    default:
                        break;
                }
                de->acceptProposedAction();
                return true;
            }
            return QObject::eventFilter(watched, event);
        }
    };

    auto* filter = new DropFilter(this, widget);
    widget->installEventFilter(filter);
}

DragDropHandler::DropResult DragDropHandler::parseMimeData(const QMimeData* mime) {
    DropResult result;

    if (mime->hasUrls()) {
        QList<QUrl> urls = mime->urls();
        for (const auto& url : urls) {
            if (url.isLocalFile()) {
                QString path = url.toLocalFile();
                QFileInfo fi(path);
                if (fi.suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
                    result.type = DropResult::PdfFile;
                    result.files.append(path);
                } else if (fi.suffix().compare("bib", Qt::CaseInsensitive) == 0) {
                    result.type = DropResult::BibFile;
                    result.files.append(path);
                } else if (fi.suffix().compare("txt", Qt::CaseInsensitive) == 0) {
                    result.type = DropResult::Text;
                    result.files.append(path);
                }
            } else {
                result.type = DropResult::Url;
                result.urls.append(url);
            }
        }
    }

    if (result.type == DropResult::Unknown && mime->hasText()) {
        result.type = DropResult::Text;
        result.content = mime->text();
    }

    return result;
}
