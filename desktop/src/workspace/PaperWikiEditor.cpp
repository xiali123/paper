#include "workspace/PaperWikiEditor.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperWikiEditor::PaperWikiEditor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WikiEditor")
{
    setupUI();
    loadSettings();
}

void PaperWikiEditor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Methods", "Results", "Discussion", "References"});
    toolbar->addWidget(categoryCombo_);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Page title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    saveBtn_ = new QPushButton("Save");
    saveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(saveBtn_, &QPushButton::clicked, this, &PaperWikiEditor::onSave);
    toolbar->addWidget(saveBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWikiEditor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Wiki Editor");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    layout->addStretch(1);
    setMinimumSize(620, 500);
}

void PaperWikiEditor::addEntry(const WikiEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pageSaved(entry.id, entry.completeness);
    update();
}

QList<WikiEntry> PaperWikiEditor::entries() const { return entries_; }

int PaperWikiEditor::reviewedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.reviewed) c++;
    return c;
}

qreal PaperWikiEditor::avgCompleteness() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.completeness;
    return sum / entries_.size();
}

QMap<QString, int> PaperWikiEditor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperWikiEditor::onSave() {
    QString title = inputField_->text().trimmed();
    if (title.isEmpty()) return;
    QStringList categories = {"Research", "Methods", "Results", "Discussion", "References"};
    QStringList sections = {"Introduction", "Background", "Analysis", "Conclusion", "Appendix"};
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };
    int cIdx = categoryCombo_->currentIndex();
    WikiEntry e;
    e.id = entries_.size() + 1;
    e.title = title;
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                            : categories[cIdx - 1];
    e.completeness = QRandomGenerator::global()->bounded(100) / 100.0;
    e.edits = 1 + QRandomGenerator::global()->bounded(100);
    e.reviewed = QRandomGenerator::global()->bounded(2) == 1;
    e.section = sections[QRandomGenerator::global()->bounded(sections.size())];
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
    inputField_->clear();
}

void PaperWikiEditor::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperWikiEditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No wiki pages yet");
        return;
    }
    int w = width(), h = height();
    int colW = w / 3;
    drawPageList(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryChart(p, QRect(colW + 5, 10, colW - 10, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperWikiEditor::drawPageList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Wiki Pages");
    int show = qMin(12, entries_.size());
    int itemH = qMin(38, (rect.height() - 30) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 28 + i * (itemH + 3);
        // background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // color accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // title
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - 70, 16, Qt::AlignVCenter,
                   e.title.left(18));
        // completeness bar
        int barY = y + 20;
        int barW = rect.width() - 80;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + 10, barY, barW, 6, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 10, barY, static_cast<int>(barW * e.completeness), 6, 3, 3);
        // completeness text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14 + barW, barY + 6,
                   QString::number(e.completeness * 100, 'f', 0) + "%");
        // reviewed badge
        if (e.reviewed) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawRoundedRect(rect.x() + rect.width() - 52, y + 4, 44, 14, 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 52, y + 4, 44, 14,
                       Qt::AlignCenter, "Reviewed");
        }
    }
}

void PaperWikiEditor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Research", "Methods", "Results", "Discussion", "References"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };
    int maxVal = 1;
    for (const auto& cat : categories) {
        if (counts.contains(cat)) maxVal = qMax(maxVal, counts[cat]);
    }
    int barH = qMin(26, (rect.height() - 40) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 30 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        // label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 4, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);
        // bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);
        // count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 79 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperWikiEditor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Pages", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Reviewed", QString::number(reviewedCount()), QColor(22, 163, 74)},
        {"Avg Complete", QString::number(avgCompleteness() * 100, 'f', 1) + "%", QColor(217, 119, 6)}
    };
    int boxH = qMin(50, (rect.height() - 30) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);
        // background card
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        // value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 26, Qt::AlignVCenter,
                   stats[i].value);
        // label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperWikiEditor::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Wiki Editor");
        return;
    }
    infoLabel_->setText(QString("Pages: %1 | Reviewed: %2 | Avg Complete: %3%")
        .arg(entries_.size())
        .arg(reviewedCount())
        .arg(avgCompleteness() * 100, 0, 'f', 1));
}

void PaperWikiEditor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WikiEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.section = settings_.value("section").toString();
        e.completeness = settings_.value("completeness").toDouble();
        e.edits = settings_.value("edits").toInt();
        e.reviewed = settings_.value("reviewed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWikiEditor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("section", entries_[i].section);
        settings_.setValue("completeness", entries_[i].completeness);
        settings_.setValue("edits", entries_[i].edits);
        settings_.setValue("reviewed", entries_[i].reviewed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
