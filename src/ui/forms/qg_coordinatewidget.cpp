#include "qg_coordinatewidget.h"

#include <qvariant.h>
#include "rs_settings.h"
#include "qg_coordinatewidget.ui.h"
/*
 *  Constructs a QG_CoordinateWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CoordinateWidget::QG_CoordinateWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CoordinateWidget::~QG_CoordinateWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CoordinateWidget::languageChange()
{
    retranslateUi(this);
}

