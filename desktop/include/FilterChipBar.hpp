#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>

struct FilterChip {
    QString key;
    QString label;
    QString value;
    bool active{false};
};

class FilterChipBar : public QFrame {
    Q_OBJECT

public:
    explicit FilterChipBar(QWidget* parent = nullptr);

    void addChip(const QString& key, const QString& label, const QString& value);
    void removeChip(const QString& key, const QString& value);
    void clearChips();
    void setAvailableFilters(const QMap<QString, QStringList>& filters);

    QStringList activeFilters(const QString& key) const;
    QMap<QString, QStringList> allActiveFilters() const;

signals:
    void filterChanged(const QMap<QString, QStringList>& active);
    void chipAdded(const QString& key, const QString& value);
    void chipRemoved(const QString& key, const QString& value);

private:
    void rebuildChips();
    void onChipToggle(const QString& key, const QString& value, bool active);

    QHBoxLayout* chipLayout_{nullptr};
    QList<FilterChip> chips_;
    QMap<QString, QStringList> availableFilters_;
};
