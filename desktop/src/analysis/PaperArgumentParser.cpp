#include "analysis/PaperArgumentParser.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperArgumentParser::PaperArgumentParser(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentParser")
{
    setupUI();
    loadSettings();
}

void PaperArgumentParser::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperArgumentParser::onAdd);
    toolbar->addWidget(addBtn_);

    analyzeBtn_ = new QPushButton("Analyze");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperArgumentParser::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentParser::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument claim...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Parse paper arguments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperArgumentParser::addArgument(const ArgumentNode& arg) {
    arguments_.append(arg);
    saveSettings();
    updateInfo();
    emit argumentParsed(arg.id, arg.type);
    update();
}

QList<ArgumentNode> PaperArgumentParser::arguments() const { return arguments_; }

QMap<QString, int> PaperArgumentParser::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& a : arguments_) counts[a.type]++;
    return counts;
}

int PaperArgumentParser::strongArguments() const {
    int c = 0;
    for (const auto& a : arguments_) if (a.strength == "strong") c++;
    return c;
}

void PaperArgumentParser::onAdd() {
    QString claim = inputField_->text().trimmed();
    if (claim.isEmpty()) return;

    bool ok;
    QStringList types = {"premise", "evidence", "conclusion", "counter"};
    QString type = QInputDialog::getItem(this, "Add Argument", "Type:", types, 0, false, &ok);
    if (!ok) return;
    QStringList strengths = {"strong", "moderate", "weak"};
    QString strength = QInputDialog::getItem(this, "Add Argument", "Strength:", strengths, 1, false, &ok);
    if (!ok) return;

    ArgumentNode a;
    a.id = arguments_.size() + 1;
    a.claim = claim;
    a.type = type;
    a.strength = strength;

    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
    int tIdx = types.indexOf(type);
    a.color = typeColors[qBound(0, tIdx, 3)];
    addArgument(a);
    inputField_->clear();
}

void PaperArgumentParser::onAnalyze() {
    if (arguments_.isEmpty()) return;
    emit analysisComplete(arguments_.size());
    update();
}

void PaperArgumentParser::onClear() {
    arguments_.clear();
    selectedArg_ = -1;
    saveSettings();
    infoLabel_->setText("Parse paper arguments");
    update();
}

void PaperArgumentParser::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (arguments_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Parse paper arguments");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Parser");

    int w = width(), h = height();
    drawArgumentGraph(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStrengthChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentParser::drawArgumentGraph(QPainter& p, const QRect& rect) {
    int show = qMin(10, arguments_.size());
    int nodeH = qMin(40, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& a = arguments_[i];
        int y = rect.y() + i * (nodeH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), nodeH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color);
        p.drawRoundedRect(rect.x(), y, 4, nodeH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   a.claim.left(28));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() - 20, 14, Qt::AlignVCenter,
                   a.type + " | " + a.strength);
    }
}

void PaperArgumentParser::drawStrengthChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Strength Distribution");

    QStringList strengths = {"strong", "moderate", "weak"};
    QString labels[] = {"Strong", "Moderate", "Weak"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
    auto counts = typeCounts();

    int maxVal = 1;
    QMap<QString, int> strengthCounts;
    for (const auto& a : arguments_) {
        strengthCounts[a.strength]++;
        maxVal = qMax(maxVal, strengthCounts[a.strength]);
    }

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = strengthCounts.contains(strengths[i]) ? strengthCounts[strengths[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperArgumentParser::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(arguments_.size()), QColor(59,130,246)},
        {"Strong", QString::number(strongArguments()), QColor(16,185,129)},
        {"Types", QString::number(typeCounts().size()), QColor(245,158,11)},
        {"Conclusions", QString::number(typeCounts().value("conclusion", 0)), QColor(139,92,246)}
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

void PaperArgumentParser::updateInfo() {
    if (arguments_.isEmpty()) { infoLabel_->setText("Parse paper arguments"); return; }
    infoLabel_->setText(QString("%1 arguments | %2 strong | %3 types")
        .arg(arguments_.size()).arg(strongArguments()).arg(typeCounts().size()));
}

void PaperArgumentParser::loadSettings() {
    int size = settings_.beginReadArray("arguments");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentNode a;
        a.id = settings_.value("id").toInt();
        a.claim = settings_.value("claim").toString();
        a.type = settings_.value("type").toString();
        a.strength = settings_.value("strength").toString();
        a.color = QColor(settings_.value("color").toString());
        arguments_.append(a);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentParser::saveSettings() {
    settings_.beginWriteArray("arguments");
    for (int i = 0; i < arguments_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", arguments_[i].id);
        settings_.setValue("claim", arguments_[i].claim);
        settings_.setValue("type", arguments_[i].type);
        settings_.setValue("strength", arguments_[i].strength);
        settings_.setValue("color", arguments_[i].color.name());
    }
    settings_.endArray();
}
