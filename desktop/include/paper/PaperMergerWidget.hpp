#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct MergePaper {
    int id{-1};
    QString title;
    QString authors;
    int year{0};
    QString journal;
    QString doi;
    QString abstractText;
    QString keywords;
    QString pdfUrl;
    int rating{0};
};

class PaperMergerWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperMergerWidget(QWidget* parent = nullptr);

    void setPapers(const MergePaper& paperA, const MergePaper& paperB);
    void autoMerge(int keepId);
    MergePaper mergedResult() const;

signals:
    void mergeCompleted(int keepId, int removeId);
    void mergeCancelled();

private slots:
    void onMerge();
    void onCancel();
    void onAutoSelectA();
    void onAutoSelectB();

private:
    void setupUI();
    void refreshTable();
    void updatePreview();

    QTableWidget* mergeTable_{nullptr};
    QLabel* previewLabel_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* mergeBtn_{nullptr};
    QPushButton* cancelBtn_{nullptr};
    QPushButton* autoABtn_{nullptr};
    QPushButton* autoBBtn_{nullptr};

    MergePaper paperA_;
    MergePaper paperB_;
    QMap<QString, int> fieldChoices_;
    MergePaper result_;

    static QStringList fieldNames();
};
