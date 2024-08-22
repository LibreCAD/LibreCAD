#ifndef LC_WIDGETOPTIONSDIALOG_H
#define LC_WIDGETOPTIONSDIALOG_H

#include "ui_lc_widgetoptionsdialog.h"


/**
 * a generic dialog for native Qt widget options
 */
class LC_WidgetOptionsDialog
        : public QDialog
        , public Ui::LC_WidgetOptionsDialog
{
    Q_OBJECT

public:
    explicit LC_WidgetOptionsDialog(QWidget* parent = 0);

public slots:
    void chooseStyleSheet();
};

#endif // LC_WIDGETOPTIONSDIALOG_H
