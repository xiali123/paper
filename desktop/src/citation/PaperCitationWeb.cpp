#include "citation/PaperCitationWeb.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainter>
#include <cmath>

PaperCitationWeb::PaperCitationWeb(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationWeb")
{
    setupUI();
    loadSettings();
}

void PaperCitationWeb::setupUI()
{
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    buildBtn_ = new QPushButton("Build Web");
    buildBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(buildBtn_, &QPushButton::clicked, this, &PaperCitationWeb::onBuild);
    toolbar->addWidget(buildBtn_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("All Categories");
    categoryCombo_->addItem("Journal");
    categoryCombo_->addItem("Conference");
    categoryCombo_->addItem("Preprint");
    categoryCombo_->addItem("Book");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationWeb::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Build citation web");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationWeb::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Build citation web");
        return;
    }

    int w = width();
    int h = height();

    drawWebGraph(p, QRect(20, 50, w / 3 * 2, h - 80));
    drawCategoryChart(p, QRect(w / 3 * 2 + 10, 50, w / 3 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 3 * 2 + 10, h / 2 + 20, w / 3 - 30, h / 2 - 50));
}

void PaperCitationWeb::drawWebGraph(QPainter& p, const QRect& rect)
{
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.topLeft(), "Citation Web");

    for (int i = 0; i < entries_.size(); ++i) {
        for (int j = i + 1; j < entries_.size(); ++j) {
            if (entries_[i].category == entries_[j].category) {
                qreal alpha = (entries_[i].weight + entries_[j].weight) / 200.0;
                p.setPen(QPen(QColor(203, 213, 225, static_cast<int>(alpha * 255)), 1));
                int x1 = rect.x() + 50 + (i % 3) * (rect.width() / 3);
                int y1 = rect.y() + 30 + (i / 3) * 80;
                int x2 = rect.x() + 50 + (j % 3) * (rect.width() / 3);
                int y2 = rect.y() + 30 + (j / 3) * 80;
                p.drawLine(x1, y1, x2, y2);
            }
        }
    }

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int x = rect.x() + 50 + (i % 3) * (rect.width() / 3);
        int y = rect.y() + 30 + (i / 3) * 80;
        int radius = qBound(8, 6 + e.citations / 80, 22);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(QPointF(x, y), radius, radius);

        if (e.hub) {
            p.setPen(QPen(e.color, 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(x, y), radius + 4, radius + 4);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 30, y + radius + 10, 60, 12, Qt::AlignCenter, e.paper.left(10));
    }
}

void PaperCitationWeb::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;

    QList<QColor> barColors = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        if (it.value() > maxCount) maxCount = it.value();
    if (maxCount == 0) maxCount = 1;

    int y = rect.y() + 22;
    int barMaxWidth = rect.width() - 130;
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd() && y < rect.bottom() - 20; ++it) {
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(rect.x() + 5, y, 80, 22), Qt::AlignLeft | Qt::AlignVCenter, it.key());

        p.fillRect(rect.x() + 90, y + 3, barMaxWidth, 16, QColor(51, 65, 85));
        int barW = static_cast<int>(barMaxWidth * (static_cast<qreal>(it.value()) / maxCount));
        p.fillRect(rect.x() + 90, y + 3, barW, 16, barColors.at(colorIdx % barColors.size()));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(rect.x() + 90 + barMaxWidth + 5, y, 30, 22), Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += 30;
        ++colorIdx;
    }
}

void PaperCitationWeb::drawStats(QPainter& p, const QRect& rect)
{
    int totalCitations = 0;
    qreal totalWeight = 0.0;
    int hubCount = 0;
    for (const auto& e : entries_) {
        totalCitations += e.citations;
        totalWeight += e.weight;
        if (e.hub) ++hubCount;
    }
    qreal avgWeight = entries_.isEmpty() ? 0.0 : totalWeight / entries_.size();

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Citations", QString::number(totalCitations), QColor(22, 163, 74)},
        {"Hubs", QString::number(hubCount), QColor(217, 119, 6)},
        {"Avg Weight", QString::number(avgWeight, 'f', 2), QColor(124, 58, 237)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperCitationWeb::onBuild()
{
    entries_.clear();

    QStringList categories = {"Journal", "Conference", "Preprint", "Book", "Thesis"};
    QStringList relations = {"cites", "cited-by", "co-cite", "related"};
    QList<QColor> colors = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
    QStringList titles = {
        "Attention Is All You Need", "BERT", "GPT-3", "ResNet", "ViT",
        "DQN", "PPO", "GAN", "Diffusion", "Transformer-XL"};

    int count = 4 + QRandomGenerator::global()->bounded(5);

    for (int i = 0; i < count; ++i) {
        WebEntry entry;
        entry.id = i;
        entry.paper = titles[i % titles.size()];
        entry.category = categories[i % categories.size()];
        entry.relation = relations[QRandomGenerator::global()->bounded(relations.size())];
        entry.weight = 20.0 + QRandomGenerator::global()->bounded(80);
        entry.citations = 50 + QRandomGenerator::global()->bounded(950);
        entry.hub = QRandomGenerator::global()->bounded(3) == 0;
        entry.color = colors[i % colors.size()];
        entries_.append(entry);
    }

    saveSettings();
    updateInfo();
    update();
}

void PaperCitationWeb::onClear()
{
    entries_.clear();
    inputField_->clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperCitationWeb::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText("Build citation web");
        return;
    }
    int totalCitations = 0;
    int hubCount = 0;
    for (const auto& e : entries_) {
        totalCitations += e.citations;
        if (e.hub) ++hubCount;
    }
    infoLabel_->setText(QString("%1 entries | %2 citations | %3 hubs")
        .arg(entries_.size()).arg(totalCitations).arg(hubCount));
}

void PaperCitationWeb::loadSettings()
{
    settings_.beginGroup("CitationWeb");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        WebEntry entry;
        entry.id = settings_.value("id", i).toInt();
        entry.paper = settings_.value("paper").toString();
        entry.category = settings_.value("category").toString();
        entry.relation = settings_.value("relation").toString();
        entry.weight = settings_.value("weight", 0.0).toReal();
        entry.citations = settings_.value("citations", 0).toInt();
        entry.hub = settings_.value("hub", false).toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperCitationWeb::saveSettings()
{
    settings_.beginGroup("CitationWeb");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_.at(i);
        settings_.setValue("id", e.id);
        settings_.setValue("paper", e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("relation", e.relation);
        settings_.setValue("weight", e.weight);
        settings_.setValue("citations", e.citations);
        settings_.setValue("hub", e.hub);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
