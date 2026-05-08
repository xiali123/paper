#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QSpinBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct ReviewCriterion {
    QString name;
    qreal score{0};
    qreal maxScore{5};
    QString comment;
};

struct PeerReview {
    int id{-1};
    int paperId{-1};
    QString paperTitle;
    QString reviewer;
    QString verdict{"pending"};
    QList<ReviewCriterion> criteria;
    QString summary;
    QString strengths;
    QString weaknesses;
    QString suggestions;
    qint64 timestamp{0};
};

class PaperPeerReviewer : public QWidget {
    Q_OBJECT

public:
    explicit PaperPeerReviewer(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addReview(const PeerReview& review);
    void removeReview(int reviewId);
    QList<PeerReview> reviews() const;
    QList<PeerReview> reviewsForPaper(int paperId) const;
    qreal averageScore(int reviewId) const;

signals:
    void reviewAdded(int paperId, int reviewId, const QString& verdict);
    void reviewRemoved(int reviewId);

private slots:
    void onSubmit();
    void onDelete();
    void onReviewSelected();
    void onFilterChanged(int index);

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QTreeWidget* reviewTree_{nullptr};
    QComboBox* verdictCombo_{nullptr};
    QLineEdit* reviewerEdit_{nullptr};
    QTextEdit* summaryEdit_{nullptr};
    QTextEdit* strengthsEdit_{nullptr};
    QTextEdit* weaknessesEdit_{nullptr};
    QTextEdit* suggestionsEdit_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* submitBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<PeerReview> reviews_;
    int nextId_{1};
    int currentPaperId_{-1};
    int selectedId_{-1};
};
