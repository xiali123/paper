#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QTextEdit>
#include <QMap>
#include <QList>
#include <QSettings>

struct GradeDimension {
    QString name;
    int weight{1};
    int score{0};
    int maxScore{10};
    QString comment;
};

struct PaperGrade {
    int paperId{-1};
    QString paperTitle;
    QList<GradeDimension> dimensions;
    QString overallComment;
    qint64 gradedAt{0};
    double weightedScore() const;
    double maxPossible() const;
};

class PaperGraderWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperGraderWidget(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void setDimensions(const QList<GradeDimension>& dims);
    void loadGrade(int paperId);
    QList<PaperGrade> allGrades() const;
    void exportGrades(const QString& filePath);

signals:
    void gradeSaved(int paperId, double score);
    void gradeDeleted(int paperId);
    void gradeExported(int count);

private slots:
    void onSave();
    void onDelete();
    void onExport();
    void onDimensionChanged();
    void onPaperSelected(int row);

private:
    void setupUI();
    void loadDefaults();
    void loadSettings();
    void saveSettings();
    void refreshGradeList();
    void refreshSliders();
    void updateScoreDisplay();

    QTableWidget* gradeList_{nullptr};
    QWidget* sliderPanel_{nullptr};
    QTableWidget* dimensionTable_{nullptr};
    QTextEdit* commentEdit_{nullptr};
    QLabel* scoreLabel_{nullptr};
    QLabel* paperLabel_{nullptr};
    QPushButton* saveBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};

    QMap<int, PaperGrade> grades_;
    QList<GradeDimension> currentDims_;
    int currentPaperId_{-1};
};
