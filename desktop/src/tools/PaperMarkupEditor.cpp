#include "tools/PaperMarkupEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMarkupEditor::PaperMarkupEditor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MarkupEditor")
{
    setupUI();
    loadSettings();
}

void PaperMarkupEditor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperMarkupEditor::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Heading", "List", "Code", "Table"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMarkupEditor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter markup title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Edit markup content");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperMarkupEditor::addEntry(const MarkupEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit markupCreated(entry.id, entry.format);
    update();
}

QList<MarkupEntry> PaperMarkupEditor::entries() const { return entries_; }

int PaperMarkupEditor::renderedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.rendered) c++;
    return c;
}

qreal PaperMarkupEditor::avgChars() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.chars;
    return sum / entries_.size();
}

QMap<QString, int> PaperMarkupEditor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperMarkupEditor::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"heading", "list", "code", "table"};
    QStringList formats = {"markdown", "html", "latex", "rst"};
    QStringList elements = {"h1", "ul", "pre", "table", "blockquote", "em"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        MarkupEntry e;
        e.id = entries_.size() + 1;
        e.title = text.left(8) + " mkup" + QString::number(i);
        e.format = formats[QRandomGenerator::global()->bounded(formats.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.element = elements[QRandomGenerator::global()->bounded(elements.size())];
        e.lines = 2 + QRandomGenerator::global()->bounded(30);
        e.chars = 20 + QRandomGenerator::global()->bounded(2000);
        e.rendered = QRandomGenerator::global()->bounded(3) != 0;
        e.color = e.rendered ? QColor(16,185,129) : (e.format == "latex" ? QColor(139,92,246) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperMarkupEditor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Edit markup content");
    update();
}

void PaperMarkupEditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Edit markup content");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Markup Editor");
    int w = width(), h = height();
    drawMarkupList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMarkupEditor::drawMarkupList(QPainter& p, const QRect& rect) {
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
                   e.title.left(14) + (e.rendered ? " [R]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.format + " | <" + e.element + "> | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.lines) + " lines");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.chars) + " chars");
    }
}

void PaperMarkupEditor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"heading", "list", "code", "table"};
    QString labels[] = {"Heading", "List", "Code", "Table"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperMarkupEditor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Elements", QString::number(entries_.size()), QColor(59,130,246)},
        {"Rendered", QString::number(renderedCount()), QColor(16,185,129)},
        {"Avg Chars", QString::number(avgChars(), 'f', 0), QColor(245,158,11)},
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

void PaperMarkupEditor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Edit markup content"); return; }
    infoLabel_->setText(QString("%1 elements | %2 rendered | %3 avg chars")
        .arg(entries_.size()).arg(renderedCount()).arg(avgChars(), 0, 'f', 0));
}

void PaperMarkupEditor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MarkupEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.format = settings_.value("format").toString();
        e.category = settings_.value("category").toString();
        e.element = settings_.value("element").toString();
        e.lines = settings_.value("lines").toInt();
        e.chars = settings_.value("chars").toInt();
        e.rendered = settings_.value("rendered").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMarkupEditor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("format", entries_[i].format);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("element", entries_[i].element);
        settings_.setValue("lines", entries_[i].lines);
        settings_.setValue("chars", entries_[i].chars);
        settings_.setValue("rendered", entries_[i].rendered);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
