#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QImage>

class PdfThumbnailWidget : public QWidget {
    Q_OBJECT

public:
    explicit PdfThumbnailWidget(QWidget* parent = nullptr);

    void loadFromPdfPath(const QString& path);
    void setPageCount(int count);
    void setCurrentPage(int page);
    int currentPage() const { return currentPage_; }
    int pageCount() const { return pageCount_; }

    void setThumbnailSize(int width);
    int thumbnailSize() const { return thumbWidth_; }

signals:
    void pageClicked(int page);
    void pageRendered(int page);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onPrevPage();
    void onNextPage();

private:
    void setupUI();
    void renderThumbnails();
    void updateCurrentHighlight();

    QListWidget* thumbList_{nullptr};
    QLabel* pageLabel_{nullptr};
    QPushButton* prevBtn_{nullptr};
    QPushButton* nextBtn_{nullptr};

    int currentPage_{1};
    int pageCount_{0};
    int thumbWidth_{120};
    QString pdfPath_;
};
