#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QColor>

struct Annotation {
    int id{-1};
    int paperId{-1};
    int page{0};
    QString text;
    QString highlightText;
    QString category;  // "important", "question", "insight", "method", "result"
    QColor color;
    qint64 createdAt{0};
};

class AnnotationWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationWidget(QWidget* parent = nullptr);

    void setPaperId(int paperId);
    void setAnnotations(const QList<Annotation>& annotations);
    QList<Annotation> annotations() const;

    void addAnnotation(const Annotation& ann);
    void removeAnnotation(int annId);
    void updateAnnotation(int annId, const Annotation& ann);

    QList<Annotation> filterByCategory(const QString& category) const;
    QList<Annotation> filterByPage(int page) const;

signals:
    void annotationAdded(const Annotation& ann);
    void annotationRemoved(int annId);
    void annotationUpdated(int annId);
    void annotationClicked(int annId);

private slots:
    void onAddAnnotation();
    void onDeleteAnnotation();
    void onEditAnnotation();
    void onCategoryFilterChanged(int index);
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void refreshList();
    QColor categoryColor(const QString& category) const;
    QString categoryIcon(const QString& category) const;

    QListWidget* listWidget_{nullptr};
    QPlainTextEdit* noteEdit_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QLabel* countLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};

    QList<Annotation> annotations_;
    int paperId_{-1};
    int selectedAnnId_{-1};
    int nextId_{1};
    QString currentFilter_;
};
