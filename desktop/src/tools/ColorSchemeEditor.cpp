#include "tools/ColorSchemeEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QPainter>

ColorSchemeEditor::ColorSchemeEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadDefaults();
    loadSettings();
}

void ColorSchemeEditor::setupUI() {
    auto* layout = new QHBoxLayout(this);
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: scheme list
    auto* leftPanel = new QVBoxLayout();
    schemeNameLabel_ = new QLabel("Color Schemes");
    schemeNameLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    leftPanel->addWidget(schemeNameLabel_);

    schemeList_ = new QListWidget();
    schemeList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(schemeList_, &QListWidget::currentRowChanged, this, &ColorSchemeEditor::onSchemeSelected);
    leftPanel->addWidget(schemeList_, 1);

    auto* btnRow = new QHBoxLayout();
    saveBtn_ = new QPushButton("Save As...");
    saveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(saveBtn_, &QPushButton::clicked, this, &ColorSchemeEditor::onSave);
    btnRow->addWidget(saveBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ColorSchemeEditor::onDelete);
    btnRow->addWidget(deleteBtn_);

    leftPanel->addLayout(btnRow);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Right: color grid + preview
    auto* rightPanel = new QVBoxLayout();

    colorGrid_ = new QWidget();
    rightPanel->addWidget(colorGrid_, 1);

    rightPanel->addWidget(new QLabel("Preview:"));
    previewWidget_ = new QWidget();
    previewWidget_->setMinimumHeight(80);
    previewWidget_->setMaximumHeight(100);
    rightPanel->addWidget(previewWidget_);

    auto* actionRow = new QHBoxLayout();
    applyBtn_ = new QPushButton("Apply Scheme");
    applyBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(applyBtn_, &QPushButton::clicked, this, &ColorSchemeEditor::onApply);
    actionRow->addWidget(applyBtn_);

    resetBtn_ = new QPushButton("Reset to Default");
    connect(resetBtn_, &QPushButton::clicked, this, &ColorSchemeEditor::onReset);
    actionRow->addWidget(resetBtn_);

    rightPanel->addLayout(actionRow);

    statusLabel_ = new QLabel("Select a scheme");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    rightPanel->addWidget(statusLabel_);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter);
}

QList<ColorRole> ColorSchemeEditor::defaultRoles() {
    return {
        {"primary", "Primary", QColor(59, 130, 246)},
        {"primaryHover", "Primary Hover", QColor(37, 99, 235)},
        {"secondary", "Secondary", QColor(139, 92, 246)},
        {"success", "Success", QColor(5, 150, 105)},
        {"warning", "Warning", QColor(245, 158, 11)},
        {"danger", "Danger", QColor(220, 38, 38)},
        {"background", "Background", QColor(255, 255, 255)},
        {"surface", "Surface", QColor(248, 250, 252)},
        {"textPrimary", "Text Primary", QColor(30, 41, 59)},
        {"textSecondary", "Text Secondary", QColor(100, 116, 139)},
        {"border", "Border", QColor(226, 232, 240)},
        {"accent", "Accent", QColor(236, 72, 153)},
    };
}

void ColorSchemeEditor::loadDefaults() {
    roles_ = defaultRoles();

    // Light scheme
    ColorScheme light;
    light.name = "Light (Default)";
    light.builtin = true;
    for (const auto& role : roles_) light.colors[role.key] = role.defaultColor;
    schemes_.append(light);

    // Dark scheme
    ColorScheme dark;
    dark.name = "Dark";
    dark.builtin = true;
    dark.colors["primary"] = QColor(96, 165, 250);
    dark.colors["primaryHover"] = QColor(59, 130, 246);
    dark.colors["secondary"] = QColor(167, 139, 250);
    dark.colors["success"] = QColor(52, 211, 153);
    dark.colors["warning"] = QColor(251, 191, 36);
    dark.colors["danger"] = QColor(248, 113, 113);
    dark.colors["background"] = QColor(15, 23, 42);
    dark.colors["surface"] = QColor(30, 41, 59);
    dark.colors["textPrimary"] = QColor(226, 232, 240);
    dark.colors["textSecondary"] = QColor(148, 163, 184);
    dark.colors["border"] = QColor(51, 65, 85);
    dark.colors["accent"] = QColor(244, 114, 182);
    schemes_.append(dark);

    // Blue scheme
    ColorScheme blue;
    blue.name = "Ocean Blue";
    blue.builtin = true;
    blue.colors = light.colors;
    blue.colors["primary"] = QColor(14, 165, 233);
    blue.colors["secondary"] = QColor(6, 182, 212);
    blue.colors["accent"] = QColor(56, 189, 248);
    schemes_.append(blue);

    activeColors_ = schemes_[0].colors;
    refreshSchemeList();
    refreshColorGrid();
}

void ColorSchemeEditor::applyScheme(const QString& name) {
    for (int i = 0; i < schemes_.size(); ++i) {
        if (schemes_[i].name == name) {
            currentSchemeIndex_ = i;
            activeColors_ = schemes_[i].colors;
            schemeList_->setCurrentRow(i);
            refreshColorGrid();
            updatePreview();
            emit schemeApplied(name);
            emit schemeChanged(schemes_[i]);
            break;
        }
    }
}

void ColorSchemeEditor::applyScheme(const ColorScheme& scheme) {
    activeColors_ = scheme.colors;
    refreshColorGrid();
    updatePreview();
    emit schemeChanged(scheme);
}

ColorScheme ColorSchemeEditor::currentScheme() const {
    if (currentSchemeIndex_ >= 0 && currentSchemeIndex_ < schemes_.size())
        return schemes_[currentSchemeIndex_];
    return ColorScheme();
}

QList<ColorScheme> ColorSchemeEditor::schemes() const { return schemes_; }
QString ColorSchemeEditor::currentSchemeName() const {
    if (currentSchemeIndex_ >= 0 && currentSchemeIndex_ < schemes_.size())
        return schemes_[currentSchemeIndex_].name;
    return "";
}

void ColorSchemeEditor::onSchemeSelected(int row) {
    if (row < 0 || row >= schemes_.size()) return;
    currentSchemeIndex_ = row;
    activeColors_ = schemes_[row].colors;
    refreshColorGrid();
    updatePreview();
    statusLabel_->setText(QString("Selected: %1").arg(schemes_[row].name));
}

void ColorSchemeEditor::onApply() {
    if (currentSchemeIndex_ < 0) return;
    applyScheme(schemes_[currentSchemeIndex_].name);
    statusLabel_->setText("Applied: " + schemes_[currentSchemeIndex_].name);
}

void ColorSchemeEditor::onReset() {
    if (currentSchemeIndex_ >= 0 && currentSchemeIndex_ < schemes_.size() && schemes_[currentSchemeIndex_].builtin) {
        activeColors_ = schemes_[currentSchemeIndex_].colors;
        refreshColorGrid();
        updatePreview();
        statusLabel_->setText("Reset to default");
    }
}

void ColorSchemeEditor::onSave() {
    QString name = QInputDialog::getText(this, "Save Scheme", "Scheme name:");
    if (name.trimmed().isEmpty()) return;

    ColorScheme custom;
    custom.name = name;
    custom.builtin = false;
    custom.colors = activeColors_;
    schemes_.append(custom);
    saveSettings();
    refreshSchemeList();
    schemeList_->setCurrentRow(schemes_.size() - 1);
    statusLabel_->setText("Saved: " + name);
    emit schemeCreated(name);
}

void ColorSchemeEditor::onDelete() {
    if (currentSchemeIndex_ < 0 || currentSchemeIndex_ >= schemes_.size()) return;
    if (schemes_[currentSchemeIndex_].builtin) {
        statusLabel_->setText("Cannot delete built-in scheme");
        return;
    }
    schemes_.removeAt(currentSchemeIndex_);
    saveSettings();
    refreshSchemeList();
    currentSchemeIndex_ = 0;
    schemeList_->setCurrentRow(0);
}

void ColorSchemeEditor::onColorClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString key = btn->property("colorKey").toString();
    QColor current = activeColors_.value(key, QColor(128, 128, 128));
    QColor color = QColorDialog::getColor(current, this, "Choose color for " + key);
    if (color.isValid()) {
        activeColors_[key] = color;
        btn->setStyleSheet(QString("QPushButton { background: %1; border: 2px solid #d1d5db; border-radius: 6px; min-height: 32px; }")
            .arg(color.name()));
        updatePreview();
    }
}

void ColorSchemeEditor::refreshSchemeList() {
    schemeList_->clear();
    for (const auto& scheme : schemes_) {
        QString label = scheme.name + (scheme.builtin ? " (built-in)" : "");
        auto* item = new QListWidgetItem(label);
        if (scheme.builtin) item->setForeground(QColor(100, 116, 139));
        schemeList_->addItem(item);
    }
}

void ColorSchemeEditor::refreshColorGrid() {
    auto* oldGrid = colorGrid_->layout();
    if (oldGrid) {
        QLayoutItem* item;
        while ((item = oldGrid->takeAt(0)) != nullptr) { delete item->widget(); delete item; }
        delete oldGrid;
    }

    auto* grid = new QGridLayout(colorGrid_);
    grid->setSpacing(8);

    int col = 0, row = 0;
    const int maxCols = 3;
    for (const auto& role : roles_) {
        auto* label = new QLabel(role.name);
        label->setStyleSheet("font-size: 11px;");
        grid->addWidget(label, row * 2, col);

        QColor color = activeColors_.value(role.key, role.defaultColor);
        auto* btn = new QPushButton();
        btn->setProperty("colorKey", role.key);
        btn->setStyleSheet(QString("QPushButton { background: %1; border: 2px solid #d1d5db; border-radius: 6px; min-height: 32px; }")
            .arg(color.name()));
        btn->setToolTip(role.key + ": " + color.name());
        connect(btn, &QPushButton::clicked, this, &ColorSchemeEditor::onColorClicked);
        grid->addWidget(btn, row * 2 + 1, col);

        col++;
        if (col >= maxCols) { col = 0; row++; }
    }
}

void ColorSchemeEditor::updatePreview() {
    if (!previewWidget_) return;
    QPainter p(previewWidget_);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = previewWidget_->rect();
    QColor bg = activeColors_.value("background", QColor(255, 255, 255));
    QColor surface = activeColors_.value("surface", QColor(248, 250, 252));
    QColor primary = activeColors_.value("primary", QColor(59, 130, 246));
    QColor text = activeColors_.value("textPrimary", QColor(30, 41, 59));
    QColor textSec = activeColors_.value("textSecondary", QColor(100, 116, 139));
    QColor border = activeColors_.value("border", QColor(226, 232, 240));
    QColor success = activeColors_.value("success", QColor(5, 150, 105));

    p.fillRect(r, bg);

    // Simulated card
    QRect card(r.x() + 20, r.y() + 10, r.width() - 40, r.height() - 20);
    p.setBrush(surface);
    p.setPen(QPen(border, 1));
    p.drawRoundedRect(card, 8, 8);

    // Title bar
    p.setBrush(primary);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRect(card.x(), card.y(), card.width(), 20), 8, 8);
    p.drawRect(QRect(card.x(), card.y() + 10, card.width(), 10));

    // Text lines
    p.setPen(text);
    QFont font = p.font();
    font.setPixelSize(10);
    p.setFont(font);
    p.drawText(card.x() + 10, card.y() + 42, "Sample text preview");

    p.setPen(textSec);
    p.drawText(card.x() + 10, card.y() + 58, "Secondary text color");

    // Status dot
    p.setBrush(success);
    p.setPen(Qt::NoPen);
    p.drawEllipse(card.right() - 20, card.y() + 6, 8, 8);
}

void ColorSchemeEditor::loadSettings() {
    QSettings settings("PaperCrawler", "ColorSchemes");
    QByteArray data = settings.value("custom_schemes").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ColorScheme scheme;
        scheme.name = obj["name"].toString();
        scheme.builtin = false;
        QJsonObject colors = obj["colors"].toObject();
        for (auto it = colors.begin(); it != colors.end(); ++it) {
            scheme.colors[it.key()] = QColor(it.value().toString());
        }
        schemes_.append(scheme);
    }
    refreshSchemeList();
}

void ColorSchemeEditor::saveSettings() {
    QJsonArray arr;
    for (const auto& scheme : schemes_) {
        if (scheme.builtin) continue;
        QJsonObject obj;
        obj["name"] = scheme.name;
        QJsonObject colors;
        for (auto it = scheme.colors.begin(); it != scheme.colors.end(); ++it)
            colors[it.key()] = it.value().name();
        obj["colors"] = colors;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ColorSchemes");
    settings.setValue("custom_schemes", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
