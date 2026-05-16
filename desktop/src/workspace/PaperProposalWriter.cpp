#include "workspace/PaperProposalWriter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <numeric>

PaperProposalWriter::PaperProposalWriter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ProposalWriter")
{
    setupUI();
    loadSettings();
}

void PaperProposalWriter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "NSF", "NIH", "DARPA", "ERC", "Industry"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search proposals...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperProposalWriter::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; padding: 4px 12px; border-radius: 4px; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProposalWriter::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Write research proposals");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch();
    setMinimumSize(680, 520);
}

void PaperProposalWriter::addEntry(const ProposalWriterEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit proposalSubmitted(entry.id, entry.completeness);
    update();
}

QList<ProposalWriterEntry> PaperProposalWriter::entries() const { return entries_; }

int PaperProposalWriter::submittedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.submitted) c++;
    return c;
}

qreal PaperProposalWriter::avgCompleteness() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completeness;
    return sum / entries_.size();
}

QMap<QString, int> PaperProposalWriter::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperProposalWriter::onAdd() {
    QStringList titles = {
        "Quantum Computing Research", "Climate Modeling Study",
        "Neural Interface Design", "Genomic Analysis Platform",
        "Autonomous Systems Framework", "Materials Discovery Lab",
        "Cybersecurity Protocol Suite", "Renewable Energy Grid"
    };
    QStringList categories = {"NSF", "NIH", "DARPA", "ERC", "Industry"};
    QStringList statuses = {"Draft", "Review", "Submitted", "Accepted", "Rejected"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int cIdx = categoryCombo_->currentIndex();
    ProposalWriterEntry e;
    e.id = entries_.size() + 1;
    e.title = titles[QRandomGenerator::global()->bounded(titles.size())];
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                           : categories[cIdx - 1];
    e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    e.completeness = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
    e.sections = 1 + QRandomGenerator::global()->bounded(12);
    e.submitted = (e.status == "Submitted" || e.status == "Accepted");
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperProposalWriter::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperProposalWriter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Write research proposals");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Proposal Writer");

    int w = width(), h = height();
    int leftW = static_cast<int>(w * 0.6);
    int topH = static_cast<int>(h * 0.75);

    drawProposalView(p, QRect(20, 50, leftW - 30, topH - 50));
    drawCategoryChart(p, QRect(leftW + 10, 50, w - leftW - 30, topH - 50));
    drawStats(p, QRect(20, topH + 10, w - 40, h - topH - 20));
}

void PaperProposalWriter::drawProposalView(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Proposals");

    int maxShow = qMin(8, entries_.size());
    int itemH = qMin(48, (area.height() - 20) / qMax(maxShow, 1));

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = area.y() + 20 + i * (itemH + 4);

        // card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(area.x(), y, area.width(), itemH, 6, 6);

        // left accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(area.x(), y, 5, itemH, 2, 2);

        int textX = area.x() + 14;
        int textW = area.width() - 24;

        // title
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(textX, y + 2, textW * 0.5, 18, Qt::AlignVCenter, e.title.left(22));

        // category badge
        QRect catRect(textX + textW * 0.5, y + 3, 50, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(150));
        p.drawRoundedRect(catRect, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(catRect, Qt::AlignCenter, e.category);

        // status indicator dot
        QColor statusColor = e.status == "Accepted" ? QColor(0x16, 0xa3, 0x4a) :
                             e.status == "Rejected" ? QColor(0xdc, 0x26, 0x26) :
                             e.status == "Submitted" ? QColor(0x3b, 0x82, 0xf6) :
                             e.status == "Review" ? QColor(0xd9, 0x77, 0x06) :
                                                    QColor(0x94, 0xa3, 0xb8);
        p.setPen(Qt::NoPen);
        p.setBrush(statusColor);
        p.drawEllipse(textX + textW - 60, y + 6, 10, 10);

        // status text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX + textW - 46, y + 2, 46, 18, Qt::AlignVCenter, e.status);

        // completeness progress bar
        int barY = y + 22;
        int barW = textW * 0.6;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(textX, barY, barW, 8, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(textX, barY, static_cast<int>(barW * e.completeness), 8, 4, 4);

        // completeness percentage
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX + barW + 4, barY + 8,
                   QString::number(static_cast<int>(e.completeness * 100)) + "%");

        // section count
        p.drawText(textX + textW * 0.65, barY + 8,
                   QString::number(e.sections) + " sections");

        // submitted checkmark
        if (e.submitted) {
            p.setPen(QColor(0x16, 0xa3, 0x4a));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(textX + textW - 16, barY + 9, QChar(0x2713));
        }
    }
}

void PaperProposalWriter::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Status Distribution");

    QMap<QString, int> statusMap;
    for (const auto& e : entries_) statusMap[e.status]++;

    QStringList statuses = {"Draft", "Review", "Submitted", "Accepted", "Rejected"};
    QColor statusColors[] = {
        QColor(0x94, 0xa3, 0xb8), QColor(0xd9, 0x77, 0x06),
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xdc, 0x26, 0x26)
    };

    int maxVal = 1;
    for (const auto& v : statusMap) maxVal = qMax(maxVal, v);

    int barH = qMin(26, (area.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = area.y() + 22 + i * (barH + 4);
        int count = statusMap.contains(statuses[i]) ? statusMap[statuses[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (area.width() - 100));

        // label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x(), y, 60, barH, Qt::AlignRight | Qt::AlignVCenter, statuses[i]);

        // bar
        p.setPen(Qt::NoPen);
        p.setBrush(statusColors[i]);
        p.drawRoundedRect(area.x() + 65, y + 3, barW, barH - 6, 3, 3);

        // count
        p.setPen(QColor(100, 116, 139));
        p.drawText(area.x() + 68 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperProposalWriter::drawStats(QPainter& p, const QRect& area) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Proposals", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Submitted Count", QString::number(submittedCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Avg Completeness", QString::number(static_cast<int>(avgCompleteness() * 100)) + "%",
         QColor(0xd9, 0x77, 0x06)},
        {"Total Sections", QString::number(std::accumulate(entries_.constBegin(),
         entries_.constEnd(), 0, [](int s, const ProposalWriterEntry& e) { return s + e.sections; })),
         QColor(0x7c, 0x3a, 0xed)}
    };

    int boxW = (area.width() - 30) / 4;
    int boxH = qMin(56, area.height() - 10);

    for (int i = 0; i < stats.size(); ++i) {
        int x = area.x() + i * (boxW + 10);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, area.y(), boxW, boxH, 8, 8);

        // accent bar at top
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, area.y(), boxW, 4, 2, 2);

        // value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 10, area.y() + 8, boxW - 20, 28, Qt::AlignVCenter, stats[i].value);

        // label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 10, area.y() + 36, boxW - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperProposalWriter::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Write research proposals");
        return;
    }
    infoLabel_->setText(QString("%1 proposals | %2 submitted | avg %3% complete")
        .arg(entries_.size())
        .arg(submittedCount())
        .arg(static_cast<int>(avgCompleteness() * 100)));
}

void PaperProposalWriter::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ProposalWriterEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.completeness = settings_.value("completeness").toDouble();
        e.sections = settings_.value("sections").toInt();
        e.submitted = settings_.value("submitted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    if (entries_.isEmpty()) {
        QStringList titles = {
            "Quantum Computing Research", "Climate Modeling Study",
            "Neural Interface Design", "Genomic Analysis Platform",
            "Autonomous Systems Framework", "Materials Discovery Lab",
            "Cybersecurity Protocol Suite", "Renewable Energy Grid"
        };
        QStringList categories = {"NSF", "NIH", "DARPA", "ERC", "Industry"};
        QStringList statuses = {"Draft", "Review", "Submitted", "Accepted", "Rejected"};
        QColor palette[] = {
            QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };

        for (int i = 0; i < 8; ++i) {
            ProposalWriterEntry e;
            e.id = i + 1;
            e.title = titles[i];
            e.category = categories[i % 5];
            e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
            e.completeness = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
            e.sections = 1 + QRandomGenerator::global()->bounded(12);
            e.submitted = (e.status == "Submitted" || e.status == "Accepted");
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperProposalWriter::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("completeness", entries_[i].completeness);
        settings_.setValue("sections", entries_[i].sections);
        settings_.setValue("submitted", entries_[i].submitted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
