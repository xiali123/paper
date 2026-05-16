#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QMap>
#include <QSettings>
#include <QColorDialog>

struct ColorRole {
    QString key;
    QString name;
    QColor defaultColor;
};

struct ColorScheme {
    QString name;
    QMap<QString, QColor> colors;
    bool builtin{true};
};

class ColorSchemeEditor : public QWidget {
    Q_OBJECT

public:
    explicit ColorSchemeEditor(QWidget* parent = nullptr);

    void applyScheme(const QString& name);
    void applyScheme(const ColorScheme& scheme);
    ColorScheme currentScheme() const;
    QList<ColorScheme> schemes() const;
    QString currentSchemeName() const;

signals:
    void schemeApplied(const QString& name);
    void schemeChanged(const ColorScheme& scheme);
    void schemeCreated(const QString& name);

private slots:
    void onSchemeSelected(int row);
    void onApply();
    void onReset();
    void onSave();
    void onDelete();
    void onColorClicked();

private:
    void setupUI();
    void loadDefaults();
    void loadSettings();
    void saveSettings();
    void refreshSchemeList();
    void refreshColorGrid();
    void updatePreview();

    QListWidget* schemeList_{nullptr};
    QWidget* colorGrid_{nullptr};
    QWidget* previewWidget_{nullptr};
    QLabel* schemeNameLabel_{nullptr};
    QPushButton* applyBtn_{nullptr};
    QPushButton* saveBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QLabel* statusLabel_{nullptr};

    QList<ColorScheme> schemes_;
    QList<ColorRole> roles_;
    int currentSchemeIndex_{0};
    QMap<QString, QColor> activeColors_;

    static QList<ColorRole> defaultRoles();
};
