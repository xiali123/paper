#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QList>

struct QueryClause {
    int id{-1};
    QString field;
    QString operator_;
    QString value;
    QString logic{"AND"};
};

class SearchQueryBuilder : public QWidget {
    Q_OBJECT

public:
    explicit SearchQueryBuilder(QWidget* parent = nullptr);

    void addClause(const QString& field, const QString& op, const QString& value, const QString& logic = "AND");
    void clearClauses();
    QList<QueryClause> clauses() const;
    QString buildQuery() const;
    QString buildElasticsearchQuery() const;
    void setFields(const QStringList& fields);

signals:
    void queryBuilt(const QString& queryString);
    void searchRequested(const QString& queryString);

private slots:
    void onAdd();
    void onRemove();
    void onClear();
    void onSearch();
    void onClauseChanged();

private:
    void setupUI();
    void refreshList();
    void updatePreview();

    QListWidget* clauseList_{nullptr};
    QLineEdit* valueEdit_{nullptr};
    QComboBox* fieldCombo_{nullptr};
    QComboBox* opCombo_{nullptr};
    QComboBox* logicCombo_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QPushButton* searchBtn_{nullptr};
    QLabel* countLabel_{nullptr};

    QList<QueryClause> clauses_;
    int nextId_{1};
};
