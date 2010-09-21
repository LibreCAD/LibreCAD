#include "qg_mousewidget.h"

#include <qvariant.h>
#include "rs_settings.h"
#include "qg_mousewidget.ui.h"
/*
 *  Constructs a QG_MouseWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_MouseWidget::QG_MouseWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_MouseWidget::~QG_MouseWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_MouseWidget::languageChange()
{
    retranslateUi(this);
}

