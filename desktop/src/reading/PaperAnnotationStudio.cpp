#include "reading/PaperAnnotationStudio.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAnnotationStudio::PaperAnnotationStudio(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AnnotationStudio")
{
    setupUI();
    loadSettings();
}

void PaperAnnotationStudio::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperAnnotationStudio::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Highlight", "Note", "Tag", "Bookmark"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAnnotationStudio::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter annotation text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Create annotations");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperAnnotationStudio::addEntry(const AnnotationEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit annotationCreated(entry.id, entry.confidence);
    update();
}

QList<AnnotationEntry> PaperAnnotationStudio::entries() const { return entries_; }

int PaperAnnotationStudio::starredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.starred) c++;
    return c;
}

qreal PaperAnnotationStudio::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperAnnotationStudio::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAnnotationStudio::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"highlight", "note", "tag", "bookmark"};
    QStringList tags = {"important", "review", "question", "insight", "method"};
    QStringList pages = {"p.1", "p.2", "p.3", "p.5", "p.8", "p.12"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        AnnotationEntry e;
        e.id = entries_.size() + 1;
        e.text = text.left(10) + " ann" + QString::number(i);
        e.highlight = "hl-" + QString::number(QRandomGenerator::global()->bounded(100));
        e.tag = tags[QRandomGenerator::global()->bounded(tags.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.page = pages[QRandomGenerator::global()->bounded(pages.size())];
        e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.starred = QRandomGenerator::global()->bounded(4) == 0;
        e.color = e.starred ? QColor(245,158,11) : (e.category == "highlight" ? QColor(59,130,246) : QColor(16,185,129));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperAnnotationStudio::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Create annotations");
    update();
}

void PaperAnnotationStudio::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create annotations");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Annotation Studio");
    int w = width(), h = height();
    drawAnnotationList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAnnotationStudio::drawAnnotationList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.text.left(14) + (e.starred ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.tag + " | " + e.page + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.starred ? "starred" : "normal");
    }
}

void PaperAnnotationStudio::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"highlight", "note", "tag", "bookmark"};
    QString labels[] = {"Highlight", "Note", "Tag", "Bookmark"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperAnnotationStudio::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Annotations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Starred", QString::number(starredCount()), QColor(245,158,11)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(16,185,129)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperAnnotationStudio::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Create annotations"); return; }
    infoLabel_->setText(QString("%1 annotations | %2 starred | %3% conf")
        .arg(entries_.size()).arg(starredCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperAnnotationStudio::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AnnotationEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.highlight = settings_.value("highlight").toString();
        e.tag = settings_.value("tag").toString();
        e.category = settings_.value("category").toString();
        e.page = settings_.value("page").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.starred = settings_.value("starred").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAnnotationStudio::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("text", entries_[i].text);
        settings_.setValue("highlight", entries_[i].highlight);
        settings_.setValue("tag", entries_[i].tag);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("page", entries_[i].page);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("starred", entries_[i].starred);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
