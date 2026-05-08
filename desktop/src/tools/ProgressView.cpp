#include "tools/ProgressView.hpp"
#include <QTimer>

ProgressView::ProgressView(QWidget* parent) : QWidget(parent) {
    setupUI();
    setVisible(false);
}

void ProgressView::setupUI() {
    auto* layout = new QVBoxLayout(this);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);

    statusLabel_ = new QLabel(this);

    layout->addWidget(statusLabel_);
    layout->addWidget(progressBar_);
}

void ProgressView::updateProgress(int current, int total, const QString& message) {
    setVisible(true);
    statusLabel_->setText(message);

    if (total > 0) {
        int percentage = (current * 100) / total;
        progressBar_->setValue(percentage);
    } else {
        progressBar_->setRange(0, 0); // Indeterminate
    }
}

void ProgressView::complete() {
    progressBar_->setValue(100);
    statusLabel_->setText("Completed!");

    // Hide after 2 seconds
    QTimer::singleShot(2000, this, [this]() {
        setVisible(false);
    });
}

void ProgressView::error(const QString& errorMsg) {
    setVisible(true);
    statusLabel_->setText("Error: " + errorMsg);
    progressBar_->setValue(0);
}
