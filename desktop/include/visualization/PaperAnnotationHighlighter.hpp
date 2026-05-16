#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct Annotation {
    int id{-1};
    int paperId{-1};
    QString text;
    QString note;
    QString color;
    int page{-1};
    int startPos{-1};
    int endPos{-1};
    qint64 timestamp{0};
};

class PaperAnnotationHighlighter : public QWidget {
    Q_OBJECT

public:
    explicit PaperAnnotationHighlighter(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addAnnotation(const Annotation& ann);
    void removeAnnotation(int annId);
    QList<Annotation> annotations() const;
    QList<Annotation> annotationsByColor(const QString& color) const;
    void exportAnnotations(const QString& format);

signals:
    void annotationAdded(int paperId, int annId);
    void annotationRemoved(int annId);
    void annotationClicked(int annId);

private slots:
    void onAdd();
    void onDelete();
    void onAnnotationSelected();
    void onColorChanged(int index);
    void onFilterChanged(int index);
    void onExport();

private:
    void setupUI();
    void refreshList();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QListWidget* annList_{nullptr};
    QTextEdit* noteEdit_{nullptr};
    QTextEdit* textEdit_{nullptr};
    QComboBox* colorCombo_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<Annotation> annotations_;
    int nextId_{1};
    int currentPaperId_{-1};
    int selectedId_{-1};
};
