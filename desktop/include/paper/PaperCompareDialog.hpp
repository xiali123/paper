#pragma once

#include <QDialog>
#include <QLabel>
#include <QTextEdit>
#include <QTableWidget>
#include <QList>

struct Paper;

class PaperCompareDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaperCompareDialog(const Paper& left, const Paper& right, QWidget* parent = nullptr);

private:
    void setupUI(const Paper& left, const Paper& right);
    void addCompareRow(QTableWidget* table, const QString& field,
                       const QString& leftVal, const QString& rightVal);
    QString diffHighlight(const QString& left, const QString& right) const;
};
