#include "qg_selectionwidget.h"

#include <qvariant.h>
#include "rs_settings.h"
#include "qg_selectionwidget.ui.h"
/*
 *  Constructs a QG_SelectionWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_SelectionWidget::QG_SelectionWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_SelectionWidget::~QG_SelectionWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_SelectionWidget::languageChange()
{
    retranslateUi(this);
}

