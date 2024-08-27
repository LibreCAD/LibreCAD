#ifndef LC_WIDGETOPTIONSDIALOG_H
#define LC_WIDGETOPTIONSDIALOG_H

#include "ui_lc_widgetoptionsdialog.h"
#include "lc_dialog.h"


/**
 * a generic dialog for native Qt widget options
 */
class LC_WidgetOptionsDialog
        : public LC_Dialog
        , public Ui::LC_WidgetOptionsDialog{
    Q_OBJECT

public:
    explicit LC_WidgetOptionsDialog(QWidget* parent = nullptr);

public slots:
    void chooseStyleSheet();
};

#endif // LC_WIDGETOPTIONSDIALOG_H
