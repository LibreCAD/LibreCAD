#include "qg_cadtoolbarellipses.h"

#include <qvariant.h>
#include "qg_cadtoolbar.h"
#include "qg_cadtoolbarellipses.ui.h"
/*
 *  Constructs a QG_CadToolBarEllipses as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarEllipses::QG_CadToolBarEllipses(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarEllipses::~QG_CadToolBarEllipses()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarEllipses::languageChange()
{
    retranslateUi(this);
}

