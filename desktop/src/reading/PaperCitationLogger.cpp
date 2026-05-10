#include "reading/PaperCitationLogger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCitationLogger::PaperCitationLogger(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationLogger")
{
    setupUI();
    loadSettings();
}

void PaperCitationLogger::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperCitationLogger::onLog);
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(new QLabel("Format:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"All", "APA", "MLA", "Chicago", "BibTeX"});
    toolbar->addWidget(formatCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationLogger::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter citation source...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Log citations");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCitationLogger::addEntry(const CitationLogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit citationLogged(entry.id, entry.source);
    update();
}

QList<CitationLogEntry> PaperCitationLogger::entries() const { return entries_; }

int PaperCitationLogger::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) c++;
    return c;
}

QMap<QString, int> PaperCitationLogger::formatCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.format]++;
    return counts;
}

QMap<QString, int> PaperCitationLogger::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCitationLogger::onLog() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList formats = {"apa", "mla", "chicago", "bibtex"};
    QStringList categories = {"journal", "conference", "book", "preprint"};
    QStringList contexts = {"inline", "footnote", "bibliography", "appendix"};
    int fIdx = formatCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        CitationLogEntry e;
        e.id = entries_.size() + 1;
        e.source = text.left(8) + " src" + QString::number(i);
        e.target = "paper-" + QString::number(QRandomGenerator::global()->bounded(100));
        e.context = contexts[QRandomGenerator::global()->bounded(contexts.size())];
        e.format = fIdx == 0 ? formats[QRandomGenerator::global()->bounded(formats.size())] : formats[fIdx - 1];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.verified = QRandomGenerator::global()->bounded(4) != 0;
        e.color = e.verified ? QColor(16,185,129) : (e.format == "bibtex" ? QColor(139,92,246) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCitationLogger::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Log citations");
    update();
}

void PaperCitationLogger::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log citations");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Logger");
    int w = width(), h = height();
    drawCitationList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFormatChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationLogger::drawCitationList(QPainter& p, const QRect& rect) {
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
                   e.source.left(14) + (e.verified ? " [V]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.target + " | " + e.context + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.format.toUpper());
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.verified ? "verified" : "pending");
    }
}

void PaperCitationLogger::drawFormatChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Formats");
    auto counts = formatCounts();
    QStringList formats = {"apa", "mla", "chicago", "bibtex"};
    QString labels[] = {"APA", "MLA", "Chicago", "BibTeX"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(formats[i]) ? counts[formats[i]] : 0;
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

void PaperCitationLogger::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Citations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Verified", QString::number(verifiedCount()), QColor(16,185,129)},
        {"Formats", QString::number(formatCounts().size()), QColor(245,158,11)},
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

void PaperCitationLogger::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Log citations"); return; }
    infoLabel_->setText(QString("%1 citations | %2 verified | %3 formats")
        .arg(entries_.size()).arg(verifiedCount()).arg(formatCounts().size()));
}

void PaperCitationLogger::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationLogEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.context = settings_.value("context").toString();
        e.format = settings_.value("format").toString();
        e.category = settings_.value("category").toString();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationLogger::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("context", entries_[i].context);
        settings_.setValue("format", entries_[i].format);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
