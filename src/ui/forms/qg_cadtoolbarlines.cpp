#include "qg_cadtoolbarlines.h"

#include <qvariant.h>
#include "qg_cadtoolbar.h"
#include "qg_cadtoolbarlines.ui.h"
/*
 *  Constructs a QG_CadToolBarLines as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarLines::QG_CadToolBarLines(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarLines::~QG_CadToolBarLines()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarLines::languageChange()
{
    retranslateUi(this);
}

