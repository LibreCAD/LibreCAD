#include "qg_widgetpen.h"

#include <qvariant.h>
#include "qg_colorbox.h"
#include "qg_widthbox.h"
#include "qg_linetypebox.h"
#include "qg_widgetpen.ui.h"
/*
 *  Constructs a QG_WidgetPen as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_WidgetPen::QG_WidgetPen(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_WidgetPen::~QG_WidgetPen()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_WidgetPen::languageChange()
{
    retranslateUi(this);
}

