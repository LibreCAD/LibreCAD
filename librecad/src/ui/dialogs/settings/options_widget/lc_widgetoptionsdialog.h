#ifndef LC_WIDGETOPTIONSDIALOG_H
#define LC_WIDGETOPTIONSDIALOG_H

#include "ui_lc_widgetoptionsdialog.h"
#include "lc_dialog.h"
#include "lc_iconcolorsoptions.h"

class LC_WidgetOptionsDialog: public LC_Dialog, public Ui::LC_WidgetOptionsDialog{
    Q_OBJECT
public:
    QString selectFolder(QString title);
    void updateUIByOptions();
    explicit LC_WidgetOptionsDialog(QWidget* parent = nullptr);
    void reject() override;
public slots:
    void chooseStyleSheet();
    void accept() override;
    void applyIconColors();
    void onpbMainClicked();
    void onpbAccentClicked();
    void onpbBackClicked();
    void onMainIconColorChanged(const QString &);
    void onAccentIconColorChanged(const QString &);
    void onBackIconColorChanged(const QString &);
    void showAdvancedSetup();
    void setIconsOverrideFoler();
    void onSaveStylePressed();
    void onRemoveStylePressed();
    void onStyleChanged(const QString& val);
protected:
    QString m_currentIconsStyleName;
    LC_IconColorsOptions m_iconColorsOptions;
    QString set_color(QComboBox *combo);

    bool setupStylesCombobox();
    void updateStylesCombobox(QStringList options);
 };

#endif // LC_WIDGETOPTIONSDIALOG_H
