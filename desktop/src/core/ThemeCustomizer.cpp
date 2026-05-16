#include "core/ThemeCustomizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QColorDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QRegularExpression>

ThemeCustomizer::ThemeCustomizer(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Theme Customizer");
    resize(500, 600);
    currentColors_ = defaultLightTheme();
    setupUI();
}

void ThemeCustomizer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Preset
    auto* presetRow = new QHBoxLayout();
    presetRow->addWidget(new QLabel("Preset:"));
    presetCombo_ = new QComboBox();
    presetCombo_->addItems({"Light (Default)", "Dark (Default)", "Nord", "Solarized", "Custom"});
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ThemeCustomizer::onPresetChanged);
    presetRow->addWidget(presetCombo_, 1);
    layout->addLayout(presetRow);

    // Color slots
    auto* colorGroup = new QGroupBox("Colors");
    auto* colorLayout = new QFormLayout(colorGroup);

    QStringList keys = {"primary", "secondary", "accent", "background", "surface",
                        "text", "textSecondary", "border", "success", "warning", "error"};
    QStringList labels = {"Primary", "Secondary", "Accent", "Background", "Surface",
                         "Text", "Text Secondary", "Border", "Success", "Warning", "Error"};

    for (int i = 0; i < keys.size(); ++i) {
        auto* row = new QHBoxLayout();

        auto* btn = new QPushButton();
        btn->setFixedSize(40, 28);
        btn->setStyleSheet(QString("background: %1; border: 1px solid #999; border-radius: 4px;")
            .arg(currentColors_.value(keys[i], QColor("#999")).name()));

        auto* hexLabel = new QLabel(currentColors_.value(keys[i], QColor("#999")).name());
        hexLabel->setStyleSheet("font-family: Consolas; font-size: 11px;");

        connect(btn, &QPushButton::clicked, this, [this, key = keys[i]]() {
            onPickColor(key);
        });

        row->addWidget(btn);
        row->addWidget(hexLabel);
        row->addStretch();
        colorLayout->addRow(labels[i] + ":", row);

        ColorSlot slot;
        slot.key = keys[i];
        slot.label = labels[i];
        slot.button = btn;
        slot.hexLabel = hexLabel;
        colorSlots_.append(slot);
    }

    layout->addWidget(colorGroup, 1);

    // Preview
    previewLabel_ = new QLabel("Preview text with custom theme colors");
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setMinimumHeight(60);
    refreshPreview();
    layout->addWidget(previewLabel_);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    auto* resetBtn = new QPushButton("Reset");
    connect(resetBtn, &QPushButton::clicked, this, &ThemeCustomizer::onReset);
    btnRow->addWidget(resetBtn);
    btnRow->addStretch();

    auto* applyBtn = new QPushButton("Apply");
    applyBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 20px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(applyBtn, &QPushButton::clicked, this, &ThemeCustomizer::onApply);
    btnRow->addWidget(applyBtn);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(closeBtn);

    layout->addLayout(btnRow);
}

QMap<QString, QColor> ThemeCustomizer::defaultLightTheme() {
    return {
        {"primary", "#3b82f6"}, {"secondary", "#6366f1"}, {"accent", "#f59e0b"},
        {"background", "#ffffff"}, {"surface", "#f8fafc"}, {"text", "#1e293b"},
        {"textSecondary", "#64748b"}, {"border", "#e2e8f0"},
        {"success", "#059669"}, {"warning", "#d97706"}, {"error", "#dc2626"}
    };
}

QMap<QString, QColor> ThemeCustomizer::defaultDarkTheme() {
    return {
        {"primary", "#3b82f6"}, {"secondary", "#818cf8"}, {"accent", "#fbbf24"},
        {"background", "#0f172a"}, {"surface", "#1e293b"}, {"text", "#f1f5f9"},
        {"textSecondary", "#94a3b8"}, {"border", "#334155"},
        {"success", "#34d399"}, {"warning", "#fbbf24"}, {"error", "#f87171"}
    };
}

QMap<QString, QColor> ThemeCustomizer::nordTheme() {
    return {
        {"primary", "#88c0d0"}, {"secondary", "#81a1c1"}, {"accent", "#ebcb8b"},
        {"background", "#2e3440"}, {"surface", "#3b4252"}, {"text", "#eceff4"},
        {"textSecondary", "#d8dee9"}, {"border", "#4c566a"},
        {"success", "#a3be8c"}, {"warning", "#ebcb8b"}, {"error", "#bf616a"}
    };
}

QMap<QString, QColor> ThemeCustomizer::solarizedTheme() {
    return {
        {"primary", "#268bd2"}, {"secondary", "#6c71c4"}, {"accent", "#b58900"},
        {"background", "#fdf6e3"}, {"surface", "#eee8d5"}, {"text", "#073642"},
        {"textSecondary", "#586e75"}, {"border", "#93a1a1"},
        {"success", "#859900"}, {"warning", "#b58900"}, {"error", "#dc322f"}
    };
}

void ThemeCustomizer::onPickColor(const QString& key) {
    QColor current = currentColors_.value(key, QColor("#999"));
    QColor chosen = QColorDialog::getColor(current, this, "Pick Color");
    if (chosen.isValid()) {
        currentColors_[key] = chosen;
        refreshPreview();
    }
}

void ThemeCustomizer::onPresetChanged(int index) {
    switch (index) {
        case 0: currentColors_ = defaultLightTheme(); break;
        case 1: currentColors_ = defaultDarkTheme(); break;
        case 2: currentColors_ = nordTheme(); break;
        case 3: currentColors_ = solarizedTheme(); break;
        default: break;
    }
    refreshPreview();
}

void ThemeCustomizer::onApply() {
    emit themeChanged(currentColors_);
    accept();
}

void ThemeCustomizer::onReset() {
    currentColors_ = defaultLightTheme();
    presetCombo_->setCurrentIndex(0);
    refreshPreview();
    emit resetToDefault();
}

QMap<QString, QColor> ThemeCustomizer::customColors() const {
    return currentColors_;
}

void ThemeCustomizer::setCustomColors(const QMap<QString, QColor>& colors) {
    currentColors_ = colors;
    presetCombo_->setCurrentIndex(4);
    refreshPreview();
}

void ThemeCustomizer::refreshPreview() {
    for (auto& slot : colorSlots_) {
        QColor c = currentColors_.value(slot.key, QColor("#999"));
        slot.button->setStyleSheet(QString("background: %1; border: 1px solid #999; border-radius: 4px;")
            .arg(c.name()));
        slot.hexLabel->setText(c.name());
    }

    QString bg = currentColors_.value("background", "#ffffff").name();
    QString fg = currentColors_.value("text", "#000000").name();
    QString primary = currentColors_.value("primary", "#3b82f6").name();
    QString border = currentColors_.value("border", "#ccc").name();

    previewLabel_->setStyleSheet(QString(
        "background: %1; color: %2; border: 2px solid %3; "
        "border-radius: 8px; padding: 12px; font-size: 14px;"
    ).arg(bg, fg, border));
}
