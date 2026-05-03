#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QComboBox>
#include <QList>

class WidgetGallery : public QWidget {
    Q_OBJECT

public:
    explicit WidgetGallery(QWidget* parent = nullptr);

    void addCategory(const QString& name);
    void addWidget(const QString& category, const QString& name, QWidget* widget);
    void addWidget(const QString& category, const QString& name, const QString& description);

    void showCategory(const QString& name);
    void showAll();

signals:
    void widgetSelected(const QString& category, const QString& name);

private slots:
    void onCategoryChanged(int index);

private:
    void setupUI();
    void rebuildGrid();

    QScrollArea* scrollArea_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QLabel* countLabel_{nullptr};

    struct GalleryEntry {
        QString category;
        QString name;
        QString description;
        QWidget* widget{nullptr};
    };

    QList<GalleryEntry> entries_;
    QStringList categories_;
    QString currentCategory_;
};
