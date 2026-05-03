#pragma once

#include <QDialog>
#include <QMap>
#include <QColor>
#include <QList>

class QPushButton;
class QComboBox;
class QLabel;

class ThemeCustomizer : public QDialog {
    Q_OBJECT

public:
    explicit ThemeCustomizer(QWidget* parent = nullptr);

    QMap<QString, QColor> customColors() const;
    void setCustomColors(const QMap<QString, QColor>& colors);

signals:
    void themeChanged(const QMap<QString, QColor>& colors);
    void resetToDefault();

private slots:
    void onPickColor(const QString& key);
    void onPresetChanged(int index);
    void onApply();
    void onReset();

private:
    void setupUI();
    void refreshPreview();

    struct ColorSlot {
        QString key;
        QString label;
        QPushButton* button{nullptr};
        QLabel* hexLabel{nullptr};
    };

    QList<ColorSlot> colorSlots_;
    QComboBox* presetCombo_{nullptr};
    QLabel* previewLabel_{nullptr};

    QMap<QString, QColor> currentColors_;

    static QMap<QString, QColor> defaultLightTheme();
    static QMap<QString, QColor> defaultDarkTheme();
    static QMap<QString, QColor> nordTheme();
    static QMap<QString, QColor> solarizedTheme();
};
