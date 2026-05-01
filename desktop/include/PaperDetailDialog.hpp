#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include "PaperTypes.hpp"

class PaperDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaperDetailDialog(const Paper& paper, QWidget* parent = nullptr);

private:
    void setupUI(const Paper& paper);
    QWidget* createInfoRow(const QString& label, const QString& value);
    QWidget* createBadge(const QString& text, const QString& color);

    Paper paper_;
};
