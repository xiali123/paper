#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsEffect>

/**
 * @brief Feature cards widget matching web frontend
 *
 * Displays 4 feature cards in a grid layout:
 * - 论文搜索 (Paper Search)
 * - 统计分析 (Statistics)
 * - 数据导出 (Data Export)
 * - 高性能 (High Performance)
 */
class FeatureCards : public QWidget {
    Q_OBJECT

public:
    explicit FeatureCards(QWidget* parent = nullptr);

private:
    void setupUI();
    void setupStyles();
    QWidget* createFeatureCard(const QString& icon, const QString& title,
                               const QString& description);

    QGridLayout* gridLayout_{nullptr};
};
