#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief Progress display widget
 */
class ProgressView : public QWidget {
    Q_OBJECT

public:
    explicit ProgressView(QWidget* parent = nullptr);

public slots:
    void updateProgress(int current, int total, const QString& message);
    void complete();
    void error(const QString& errorMsg);

private:
    void setupUI();

    QProgressBar* progressBar_{nullptr};
    QLabel* statusLabel_{nullptr};
};
