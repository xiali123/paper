#include "tools/PaperMathFormulaRenderer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperMathFormulaRenderer::PaperMathFormulaRenderer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MathFormula")
{
    setupUI();
    loadSettings();
}

void PaperMathFormulaRenderer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperMathFormulaRenderer::onAdd);
    toolbar->addWidget(addBtn_);

    renderBtn_ = new QPushButton("Render");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperMathFormulaRenderer::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMathFormulaRenderer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter LaTeX formula (e.g. \\frac{a}{b})...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Add and render math formulas");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 460);
}

void PaperMathFormulaRenderer::addFormula(const FormulaEntry& formula) {
    formulas_.append(formula);
    saveSettings();
    updateInfo();
    emit formulaRendered(formula.latex);
    update();
}

QList<FormulaEntry> PaperMathFormulaRenderer::formulas() const { return formulas_; }

QMap<QString, int> PaperMathFormulaRenderer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& f : formulas_) counts[f.category]++;
    return counts;
}

void PaperMathFormulaRenderer::onAdd() {
    QString latex = inputField_->text().trimmed();
    if (latex.isEmpty()) return;

    bool ok;
    QStringList cats = {"algebra", "calculus", "linear-algebra", "probability", "physics"};
    QString cat = QInputDialog::getItem(this, "Add Formula", "Category:", cats, 0, false, &ok);
    if (!ok) return;

    FormulaEntry f;
    f.id = formulas_.size() + 1;
    f.latex = latex;
    f.label = latex.left(20);
    f.category = cat;

    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    int catIdx = cats.indexOf(cat);
    f.color = catColors[qBound(0, catIdx, 4)];
    addFormula(f);
    inputField_->clear();
}

void PaperMathFormulaRenderer::onRender() {
    if (formulas_.isEmpty()) return;
    update();
}

void PaperMathFormulaRenderer::onClear() {
    formulas_.clear();
    selectedFormula_ = -1;
    saveSettings();
    infoLabel_->setText("Add and render math formulas");
    update();
}

void PaperMathFormulaRenderer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (formulas_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add and render math formulas");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Math Formula Renderer");

    int w = width(), h = height();
    drawFormulaCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawPreview(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMathFormulaRenderer::drawFormulaCards(QPainter& p, const QRect& rect) {
    int show = qMin(10, formulas_.size());
    int cardH = qMin(42, (rect.height() - 10) / show);

    for (int i = 0; i < show; ++i) {
        const auto& f = formulas_[i];
        int y = rect.y() + i * (cardH + 3);
        bool sel = (f.id == selectedFormula_);

        p.setPen(Qt::NoPen);
        p.setBrush(sel ? f.color.lighter(170) : QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(f.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 9));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 20, Qt::AlignVCenter,
                   f.latex.left(30));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 24, rect.width() - 20, 14, Qt::AlignVCenter,
                   f.category + " | #" + QString::number(f.id));
    }
}

void PaperMathFormulaRenderer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QList<QString> cats = counts.keys();
    if (cats.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / cats.size());
    for (int i = 0; i < cats.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[cats[i]]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        QColor c(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(counts[cats[i]]));
    }
}

void PaperMathFormulaRenderer::drawPreview(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Preview");

    if (formulas_.isEmpty()) return;
    const auto& f = formulas_.last();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect.x(), rect.y() + 18, rect.width(), rect.height() - 22, 6, 6);

    p.setPen(f.color);
    p.setFont(QFont("Courier", 11));
    p.drawText(QRect(rect.x() + 10, rect.y() + 25, rect.width() - 20, 40), Qt::AlignCenter | Qt::TextWordWrap,
               f.latex);

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + 10, rect.y() + 70, rect.width() - 20, 20, Qt::AlignCenter,
               f.category + " | " + f.label);
}

void PaperMathFormulaRenderer::updateInfo() {
    if (formulas_.isEmpty()) { infoLabel_->setText("Add and render math formulas"); return; }
    infoLabel_->setText(QString("%1 formulas | %2 categories")
        .arg(formulas_.size()).arg(categoryCounts().size()));
}

void PaperMathFormulaRenderer::loadSettings() {
    int size = settings_.beginReadArray("formulas");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FormulaEntry f;
        f.id = settings_.value("id").toInt();
        f.latex = settings_.value("latex").toString();
        f.label = settings_.value("label").toString();
        f.category = settings_.value("category").toString();
        f.color = QColor(settings_.value("color").toString());
        formulas_.append(f);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMathFormulaRenderer::saveSettings() {
    settings_.beginWriteArray("formulas");
    for (int i = 0; i < formulas_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", formulas_[i].id);
        settings_.setValue("latex", formulas_[i].latex);
        settings_.setValue("label", formulas_[i].label);
        settings_.setValue("category", formulas_[i].category);
        settings_.setValue("color", formulas_[i].color.name());
    }
    settings_.endArray();
}
