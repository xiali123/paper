#include "tools/PaperSnippetVault.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSnippetVault::PaperSnippetVault(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SnippetVault")
{
    setupUI();
    loadSettings();
}

void PaperSnippetVault::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    storeBtn_ = new QPushButton("Store");
    storeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(storeBtn_, &QPushButton::clicked, this, &PaperSnippetVault::onStore);
    toolbar->addWidget(storeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "LaTeX", "Python", "Bash", "Config"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSnippetVault::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter snippet name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Store code snippets");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperSnippetVault::addEntry(const SnippetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit snippetStored(entry.id, entry.name);
    update();
}

QList<SnippetEntry> PaperSnippetVault::entries() const { return entries_; }

int PaperSnippetVault::favoriteCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.favorite) c++;
    return c;
}

qreal PaperSnippetVault::avgLines() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.lines;
    return sum / entries_.size();
}

QMap<QString, int> PaperSnippetVault::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSnippetVault::onStore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"latex", "python", "bash", "config"};
    QStringList languages = {"LaTeX", "Python", "Bash", "YAML", "JSON"};
    QStringList tags = {"template", "utility", "example", "snippet", "macro"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SnippetEntry e;
        e.id = entries_.size() + 1;
        e.name = text.left(8) + " snip" + QString::number(i);
        e.tag = tags[QRandomGenerator::global()->bounded(tags.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.language = languages[QRandomGenerator::global()->bounded(languages.size())];
        e.lines = 3 + QRandomGenerator::global()->bounded(50);
        e.uses = QRandomGenerator::global()->bounded(30);
        e.favorite = QRandomGenerator::global()->bounded(4) == 0;
        e.color = e.favorite ? QColor(245,158,11) : (e.uses >= 10 ? QColor(16,185,129) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperSnippetVault::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Store code snippets");
    update();
}

void PaperSnippetVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Store code snippets");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Snippet Vault");
    int w = width(), h = height();
    drawSnippetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSnippetVault::drawSnippetList(QPainter& p, const QRect& rect) {
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
                   e.name.left(14) + (e.favorite ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.tag + " | " + e.language + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.lines) + " lines");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.uses) + " uses");
    }
}

void PaperSnippetVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"latex", "python", "bash", "config"};
    QString labels[] = {"LaTeX", "Python", "Bash", "Config"};
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

void PaperSnippetVault::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Snippets", QString::number(entries_.size()), QColor(59,130,246)},
        {"Favorites", QString::number(favoriteCount()), QColor(245,158,11)},
        {"Avg Lines", QString::number(avgLines(), 'f', 0), QColor(16,185,129)},
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

void PaperSnippetVault::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Store code snippets"); return; }
    infoLabel_->setText(QString("%1 snippets | %2 fav | %3 avg lines")
        .arg(entries_.size()).arg(favoriteCount()).arg(avgLines(), 0, 'f', 0));
}

void PaperSnippetVault::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SnippetEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.tag = settings_.value("tag").toString();
        e.category = settings_.value("category").toString();
        e.language = settings_.value("language").toString();
        e.lines = settings_.value("lines").toInt();
        e.uses = settings_.value("uses").toInt();
        e.favorite = settings_.value("favorite").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSnippetVault::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("tag", entries_[i].tag);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("language", entries_[i].language);
        settings_.setValue("lines", entries_[i].lines);
        settings_.setValue("uses", entries_[i].uses);
        settings_.setValue("favorite", entries_[i].favorite);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
