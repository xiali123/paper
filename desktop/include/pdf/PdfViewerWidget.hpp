#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QComboBox>
#include <QStackedWidget>

class PdfViewerWidget : public QWidget {
    Q_OBJECT

public:
    explicit PdfViewerWidget(QWidget* parent = nullptr);

    void loadFile(const QString& path);
    void setPageCount(int count);
    void setCurrentPage(int page);

    int currentPage() const { return currentPage_; }
    int pageCount() const { return pageCount_; }
    QString filePath() const { return filePath_; }

    void setZoom(int percent);
    int zoom() const { return zoomPercent_; }

    void setFitMode(const QString& mode); // "width", "page", "custom"

signals:
    void pageChanged(int page);
    void zoomChanged(int percent);
    void fileLoaded(const QString& path);
    void annotationRequested(int page);

private slots:
    void onPrevPage();
    void onNextPage();
    void onFirstPage();
    void onLastPage();
    void onZoomIn();
    void onZoomOut();
    void onFitModeChanged(int index);
    void onSliderMoved(int value);

private:
    void setupUI();
    void renderPage();
    void updatePageInfo();
    void updateNavButtons();

    QScrollArea* scrollArea_{nullptr};
    QLabel* pageLabel_{nullptr};
    QLabel* pageInfo_{nullptr};
    QSlider* pageSlider_{nullptr};
    QComboBox* fitCombo_{nullptr};
    QPushButton* prevBtn_{nullptr};
    QPushButton* nextBtn_{nullptr};
    QPushButton* zoomInBtn_{nullptr};
    QPushButton* zoomOutBtn_{nullptr};

    int currentPage_{1};
    int pageCount_{0};
    int zoomPercent_{100};
    QString filePath_;
    QString fitMode_{"width"};
};
