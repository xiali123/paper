#include "tools/PaperCodeSnippetManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCodeSnippetManager::PaperCodeSnippetManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CodeSnippetManager")
{
    setupUI();
    loadSettings();
}

void PaperCodeSnippetManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Snippet");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCodeSnippetManager::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Algorithm", "Visualization", "Data Processing", "Utility"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCodeSnippetManager::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCodeSnippetManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Manage code snippets");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperCodeSnippetManager::addSnippet(const CodeSnippet& snippet) {
    snippets_.append(snippet);
    saveSettings();
    updateInfo();
    emit snippetAdded(snippet.id);
    update();
}

QList<CodeSnippet> PaperCodeSnippetManager::snippets() const { return snippets_; }

QMap<QString, int> PaperCodeSnippetManager::languageCounts() const {
    QMap<QString, int> counts;
    for (const auto& s : snippets_) counts[s.language]++;
    return counts;
}

QMap<QString, int> PaperCodeSnippetManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& s : snippets_) counts[s.category]++;
    return counts;
}

int PaperCodeSnippetManager::totalUsage() const {
    int t = 0;
    for (const auto& s : snippets_) t += s.usageCount;
    return t;
}

void PaperCodeSnippetManager::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Snippet", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QStringList langs = {"Python", "R", "Julia", "MATLAB", "C++"};
    QString lang = QInputDialog::getItem(this, "Add Snippet", "Language:", langs, 0, false, &ok);
    if (!ok) return;
    QStringList cats = {"algorithm", "visualization", "data-processing", "utility"};
    QString cat = QInputDialog::getItem(this, "Add Snippet", "Category:", cats, 0, false, &ok);
    if (!ok) return;

    CodeSnippet s;
    s.id = snippets_.size() + 1;
    s.title = title;
    s.language = lang;
    s.code = "# " + title;
    s.category = cat;
    s.description = title;
    s.usageCount = QRandomGenerator::global()->bounded(20);

    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int cIdx = cats.indexOf(cat);
    s.color = catColors[qBound(0, cIdx, 3)];
    addSnippet(s);
}

void PaperCodeSnippetManager::onFilterChanged(int) { update(); }

void PaperCodeSnippetManager::onClear() {
    snippets_.clear();
    saveSettings();
    infoLabel_->setText("Manage code snippets");
    update();
}

void PaperCodeSnippetManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (snippets_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage code snippets");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Code Snippet Manager");

    int w = width(), h = height();
    drawSnippetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLanguageChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCodeSnippetManager::drawSnippetList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(38, (rect.height() - 10) / maxShow);

    for (int i = snippets_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& s = snippets_[i];
        if (filterIdx == 1 && s.category != "algorithm") continue;
        if (filterIdx == 2 && s.category != "visualization") continue;
        if (filterIdx == 3 && s.category != "data-processing") continue;
        if (filterIdx == 4 && s.category != "utility") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   s.title.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   s.language + " | " + s.category);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, "used: " + QString::number(s.usageCount));
        show++;
    }
}

void PaperCodeSnippetManager::drawLanguageChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Language");

    auto counts = languageCounts();
    QList<QString> langs = counts.keys();
    if (langs.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / langs.size());
    for (int i = 0; i < langs.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[langs[i]]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, langs[i]);

        QColor c(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(counts[langs[i]]));
    }
}

void PaperCodeSnippetManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Snippets", QString::number(snippets_.size()), QColor(59,130,246)},
        {"Languages", QString::number(languageCounts().size()), QColor(16,185,129)},
        {"Categories", QString::number(categoryCounts().size()), QColor(245,158,11)},
        {"Total Usage", QString::number(totalUsage()), QColor(139,92,246)}
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

void PaperCodeSnippetManager::updateInfo() {
    if (snippets_.isEmpty()) { infoLabel_->setText("Manage code snippets"); return; }
    infoLabel_->setText(QString("%1 snippets | %2 languages | used %3 times")
        .arg(snippets_.size()).arg(languageCounts().size()).arg(totalUsage()));
}

void PaperCodeSnippetManager::loadSettings() {
    int size = settings_.beginReadArray("snippets");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CodeSnippet s;
        s.id = settings_.value("id").toInt();
        s.title = settings_.value("title").toString();
        s.language = settings_.value("language").toString();
        s.code = settings_.value("code").toString();
        s.category = settings_.value("category").toString();
        s.description = settings_.value("description").toString();
        s.usageCount = settings_.value("usageCount").toInt();
        s.color = QColor(settings_.value("color").toString());
        snippets_.append(s);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCodeSnippetManager::saveSettings() {
    settings_.beginWriteArray("snippets");
    for (int i = 0; i < snippets_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", snippets_[i].id);
        settings_.setValue("title", snippets_[i].title);
        settings_.setValue("language", snippets_[i].language);
        settings_.setValue("code", snippets_[i].code);
        settings_.setValue("category", snippets_[i].category);
        settings_.setValue("description", snippets_[i].description);
        settings_.setValue("usageCount", snippets_[i].usageCount);
        settings_.setValue("color", snippets_[i].color.name());
    }
    settings_.endArray();
}
