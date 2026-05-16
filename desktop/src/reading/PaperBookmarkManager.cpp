#include "reading/PaperBookmarkManager.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperBookmarkManager::PaperBookmarkManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BookmarkManager")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList categories = {"Introduction", "Method", "Result", "Discussion", "Conclusion"};
        QStringList papers = {
            "Attention Is All You Need", "Deep Residual Learning",
            "BERT: Pre-training of Transformers", "GPT-4 Technical Report",
            "Diffusion Models Beat GANs", "Vision Transformer",
            "Contrastive Learning Survey", "Graph Neural Networks"
        };
        QStringList locations = {
            "Page 1", "Page 3", "Page 5", "Page 7",
            "Page 10", "Page 12", "Page 15", "Page 18"
        };
        QColor palette[] = {
            QColor(59, 130, 246),  QColor(22, 163, 74),
            QColor(217, 119, 6),   QColor(220, 38, 38),
            QColor(124, 58, 237)
        };
        for (int i = 0; i < 8; ++i) {
            BookmarkManagerEntry e;
            e.id = i + 1;
            e.paper = papers[i];
            e.category = categories[i % categories.size()];
            e.location = locations[i];
            e.progress = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
            e.annotations = QRandomGenerator::global()->bounded(12);
            e.starred = QRandomGenerator::global()->bounded(2) == 0;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperBookmarkManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Introduction", "Method", "Result", "Discussion", "Conclusion"});
    toolbar->addWidget(categoryCombo_);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Bookmark location...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperBookmarkManager::onAdd);
    toolbar->addWidget(addBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBookmarkManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Manage paper bookmarks");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    layout->addStretch();
    setMinimumSize(580, 480);
}

void PaperBookmarkManager::addEntry(const BookmarkManagerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bookmarkAdded(entry.id, entry.progress);
    update();
}

QList<BookmarkManagerEntry> PaperBookmarkManager::entries() const { return entries_; }

int PaperBookmarkManager::starredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.starred) c++;
    return c;
}

qreal PaperBookmarkManager::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperBookmarkManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBookmarkManager::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Introduction", "Method", "Result", "Discussion", "Conclusion"};
    QStringList papers = {
        "Attention Is All You Need", "Deep Residual Learning",
        "BERT: Pre-training of Transformers", "GPT-4 Technical Report",
        "Diffusion Models Beat GANs", "Vision Transformer",
        "Contrastive Learning Survey", "Graph Neural Networks"
    };
    QColor palette[] = {
        QColor(59, 130, 246),  QColor(22, 163, 74),
        QColor(217, 119, 6),   QColor(220, 38, 38),
        QColor(124, 58, 237)
    };
    int cIdx = categoryCombo_->currentIndex();
    BookmarkManagerEntry e;
    e.id = entries_.size() + 1;
    e.paper = papers[QRandomGenerator::global()->bounded(papers.size())];
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                            : categories[cIdx - 1];
    e.location = text;
    e.progress = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.annotations = QRandomGenerator::global()->bounded(12);
    e.starred = QRandomGenerator::global()->bounded(2) == 0;
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
    inputField_->clear();
}

void PaperBookmarkManager::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperBookmarkManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add paper bookmarks");
        return;
    }
    int w = width(), h = height();
    int col1W = w / 3;
    int col2W = w / 3;
    int col3W = w - col1W - col2W;
    drawBookmarkView(p, QRect(20, 10, col1W - 30, h - 20));
    drawCategoryChart(p, QRect(col1W + 10, 10, col2W - 20, h - 20));
    drawStats(p, QRect(col1W + col2W + 10, 10, col3W - 30, h - 20));
}

void PaperBookmarkManager::drawBookmarkView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Bookmarks");
    int show = qMin(10, entries_.size());
    int itemH = qMin(38, (rect.height() - 40) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 32 + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        int textX = rect.x() + 10;
        if (e.starred) {
            p.setPen(QColor(217, 119, 6));
            p.setFont(QFont("Arial", 10));
            p.drawText(textX, y, 16, itemH, Qt::AlignVCenter, QString::fromUtf8("★"));
            textX += 16;
        }
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(textX, y + 2, rect.width() - textX + rect.x() - 10, 16,
                   Qt::AlignVCenter, e.location.left(20));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX, y + 18, rect.width() - textX + rect.x() - 10, 14,
                   Qt::AlignVCenter, e.paper.left(22) + " | " + e.category);
        int barW = static_cast<int>(e.progress * 50);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() - 60, y + (itemH - 6) / 2, barW, 6, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 60 + barW + 3, y + (itemH - 6) / 2 + 8,
                   QString::number(e.progress, 'f', 2));
    }
}

void PaperBookmarkManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() + 20, "Categories");
    auto counts = categoryCounts();
    QStringList catKeys = {"Introduction", "Method", "Result", "Discussion", "Conclusion"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6),  QColor(220, 38, 38),
        QColor(124, 58, 237)
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 50) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 36 + i * (barH + 6);
        int count = counts.contains(catKeys[i]) ? counts[catKeys[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, catKeys[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperBookmarkManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Bookmarks", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Starred",         QString::number(starredCount()), QColor(22, 163, 74)},
        {"Avg Progress",    QString::number(avgProgress(), 'f', 2), QColor(217, 119, 6)}
    };
    int boxH = qMin(48, (rect.height() - 20) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperBookmarkManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage paper bookmarks"); return; }
    infoLabel_->setText(QString("Bookmarks: %1 | Starred: %2 | Avg Progress: %3")
        .arg(entries_.size()).arg(starredCount()).arg(avgProgress(), 0, 'f', 2));
}

void PaperBookmarkManager::loadSettings() {
    settings_.beginGroup("BookmarkManager");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BookmarkManagerEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.location = settings_.value("location").toString();
        e.progress = settings_.value("progress").toDouble();
        e.annotations = settings_.value("annotations").toInt();
        e.starred = settings_.value("starred").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperBookmarkManager::saveSettings() {
    settings_.beginGroup("BookmarkManager");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("location", entries_[i].location);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("annotations", entries_[i].annotations);
        settings_.setValue("starred", entries_[i].starred);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
