#include "tools/PaperBookmarkOrganizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBookmarkOrganizer::PaperBookmarkOrganizer(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperBookmarkOrganizer::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Paper", "Article", "Blog", "Video", "Tool"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Bookmark title...");
    addBtn_ = new QPushButton("Add", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Bookmarks: 0 | Favorites: 0 | Avg Relevance: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(addBtn_, &QPushButton::clicked, this, &PaperBookmarkOrganizer::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBookmarkOrganizer::onClear);
}

void PaperBookmarkOrganizer::addEntry(const BookmarkEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<BookmarkEntry> PaperBookmarkOrganizer::entries() const { return entries_; }

int PaperBookmarkOrganizer::favoriteCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.favorite) c++;
    return c;
}

qreal PaperBookmarkOrganizer::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperBookmarkOrganizer::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperBookmarkOrganizer::onAdd() {
    BookmarkEntry e;
    e.id = entries_.size() + 1;
    e.title = inputField_->text().trimmed();
    if (e.title.isEmpty()) e.title = QString("Bookmark_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.url = QString("https://example.com/%1").arg(e.id);
    e.visits = QRandomGenerator::global()->bounded(0, 100);
    e.relevance = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.favorite = e.relevance > 0.8;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit bookmarkAdded(e.id, e.relevance);
    update();
}

void PaperBookmarkOrganizer::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperBookmarkOrganizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawBookmarkList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperBookmarkOrganizer::drawBookmarkList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Bookmarks:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        QColor dotColor = e.favorite ? QColor(0xd97706) : e.color;
        p.setPen(dotColor);
        p.setBrush(dotColor);
        // Star shape for favorites
        if (e.favorite) {
            p.drawEllipse(rect.left(), y, 8, 8);
        } else {
            p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        }
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Visits: %3 | Rel: %4 | %5")
            .arg(e.title.left(15), e.category)
            .arg(e.visits)
            .arg(QString::number(e.relevance, 'f', 2))
            .arg(e.date);
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperBookmarkOrganizer::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperBookmarkOrganizer::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Favorites: %1").arg(favoriteCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Relevance: %1").arg(QString::number(avgRelevance(), 'f', 3)));
}

void PaperBookmarkOrganizer::updateInfo() {
    infoLabel_->setText(QString("Bookmarks: %1 | Favorites: %2 | Avg Relevance: %3")
        .arg(entries_.size()).arg(favoriteCount())
        .arg(QString::number(avgRelevance(), 'f', 2)));
}

void PaperBookmarkOrganizer::loadSettings() {
    settings_.beginGroup("BookmarkOrganizer");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        BookmarkEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.title = settings_.value(QString("title_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.url = settings_.value(QString("url_%1").arg(i)).toString();
        e.visits = settings_.value(QString("visits_%1").arg(i)).toInt();
        e.relevance = settings_.value(QString("relevance_%1").arg(i)).toDouble();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.favorite = settings_.value(QString("favorite_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperBookmarkOrganizer::saveSettings() {
    settings_.beginGroup("BookmarkOrganizer");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("title_%1").arg(i), e.title);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("url_%1").arg(i), e.url);
        settings_.setValue(QString("visits_%1").arg(i), e.visits);
        settings_.setValue(QString("relevance_%1").arg(i), e.relevance);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("favorite_%1").arg(i), e.favorite);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
