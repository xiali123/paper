#include "analysis/PaperArgumentParser2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentParser2::PaperArgumentParser2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentParser2")
{
    setupUI();
    loadSettings();
}

void PaperArgumentParser2::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* left = new QHBoxLayout();
    left->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Deductive", "Inductive", "Abductive", "Analogical", "Causal"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter premise...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(inputField_);

    parseBtn_ = new QPushButton("Parse");
    parseBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(parseBtn_, &QPushButton::clicked, this, &PaperArgumentParser2::onParse);
    left->addWidget(parseBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentParser2::onClear);
    left->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Arguments: 0 | Sound: 0 | Avg Validity: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    left->addWidget(infoLabel_);

    layout->addLayout(left);
    layout->addStretch();

    setMinimumSize(640, 520);
}

void PaperArgumentParser2::addEntry(const ArgumentParseEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ArgumentParseEntry> PaperArgumentParser2::entries() const {
    return entries_;
}

int PaperArgumentParser2::soundCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.sound) ++c;
    return c;
}

qreal PaperArgumentParser2::avgValidity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.validity;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentParser2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperArgumentParser2::onParse() {
    QString premise = inputField_->text().trimmed();
    if (premise.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    qreal validity = QRandomGenerator::global()->generateDouble();
    int steps = QRandomGenerator::global()->bounded(1, 11);
    bool sound = validity > 0.8;

    QStringList conclusions = {
        "Therefore the hypothesis holds",
        "The evidence supports the claim",
        "Further analysis is warranted",
        "The premise leads to a valid conclusion",
        "This argument requires more evidence",
        "The data suggests a strong correlation",
        "No significant contradiction found",
        "The result confirms the theory"
    };
    QString conclusion = conclusions[QRandomGenerator::global()->bounded(conclusions.size())];

    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };
    QColor color = colors[QRandomGenerator::global()->bounded(5)];

    ArgumentParseEntry entry;
    entry.id = entries_.size() + 1;
    entry.premise = premise;
    entry.category = category;
    entry.conclusion = conclusion;
    entry.validity = validity;
    entry.steps = steps;
    entry.sound = sound;
    entry.color = color;

    addEntry(entry);
    emit argumentParsed(entry.id, entry.validity);
    inputField_->clear();
}

void PaperArgumentParser2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperArgumentParser2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Parse paper arguments");
        return;
    }

    int w = width(), h = height();
    int colW = (w - 50) / 3;
    drawArgumentTree(p, QRect(10, 10, colW, h - 20));
    drawCategoryChart(p, QRect(20 + colW, 10, colW, h - 20));
    drawStats(p, QRect(30 + colW * 2, 10, colW, h - 20));
}

void PaperArgumentParser2::drawArgumentTree(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Argument Parser");

    int show = qMin(10, entries_.size());
    int nodeH = qMin(44, (rect.height() - 40) / qMax(show, 1));
    int startY = rect.y() + 35;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = startY + i * (nodeH + 6);

        // Connecting line from previous node
        if (i > 0) {
            p.setPen(QColor(203, 213, 225));
            p.drawLine(rect.x() + rect.width() / 2, y - 6,
                       rect.x() + rect.width() / 2, y);
        }

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(e.validity >= 0.5
            ? QColor(16, 163, 74, 25)
            : QColor(220, 38, 38, 25));
        p.drawRoundedRect(rect.x(), y, rect.width(), nodeH, 6, 6);

        // Left color bar - green for valid, red for invalid
        p.setBrush(e.validity >= 0.5 ? QColor(16, 163, 74) : QColor(220, 38, 38));
        p.drawRoundedRect(rect.x(), y, 4, nodeH, 2, 2);

        // Premise label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 24, 16, Qt::AlignVCenter,
                   e.premise.left(26));

        // Conclusion with arrow
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString conclusionText = "-> " + e.conclusion.left(30);
        p.drawText(rect.x() + 12, y + 20, rect.width() - 24, 14, Qt::AlignVCenter,
                   conclusionText);

        // Sound checkmark
        if (e.sound) {
            p.setPen(QColor(16, 163, 74));
            p.setFont(QFont("Arial", 11, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 20, y + 4, 16, 20, Qt::AlignVCenter,
                       QChar(0x2713));
        }
    }
}

void PaperArgumentParser2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    QStringList categories = counts.keys();
    int maxVal = 1;
    for (int v : counts.values())
        maxVal = qMax(maxVal, v);

    QColor barColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int barH = qMin(28, (rect.height() - 40) / qMax(categories.size(), 1));
    int startY = rect.y() + 38;

    for (int i = 0; i < categories.size(); ++i) {
        int y = startY + i * (barH + 4);
        int count = counts[categories[i]];
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(barColors[i % 5]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperArgumentParser2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Arguments", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Sound Count", QString::number(soundCount()), QColor(22, 163, 74)},
        {"Avg Validity", QString::number(avgValidity(), 'f', 2), QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)},
        {"Unsound", QString::number(entries_.size() - soundCount()), QColor(220, 38, 38)}
    };

    int boxH = qMin(48, (rect.height() - 20) / qMax(stats.size(), 1));
    int startY = rect.y() + 10;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 24, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperArgumentParser2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Arguments: 0 | Sound: 0 | Avg Validity: 0.00");
        return;
    }
    infoLabel_->setText(
        QString("Arguments: %1 | Sound: %2 | Avg Validity: %3")
            .arg(entries_.size())
            .arg(soundCount())
            .arg(avgValidity(), 0, 'f', 2));
}

void PaperArgumentParser2::loadSettings() {
    settings_.beginGroup("ArgumentParser2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentParseEntry e;
        e.id = settings_.value("id").toInt();
        e.premise = settings_.value("premise").toString();
        e.category = settings_.value("category").toString();
        e.conclusion = settings_.value("conclusion").toString();
        e.validity = settings_.value("validity").toDouble();
        e.steps = settings_.value("steps").toInt();
        e.sound = settings_.value("sound").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperArgumentParser2::saveSettings() {
    settings_.beginGroup("ArgumentParser2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("premise", entries_[i].premise);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("conclusion", entries_[i].conclusion);
        settings_.setValue("validity", entries_[i].validity);
        settings_.setValue("steps", entries_[i].steps);
        settings_.setValue("sound", entries_[i].sound);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
