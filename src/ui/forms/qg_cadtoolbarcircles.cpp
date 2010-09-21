#include "qg_cadtoolbarcircles.h"

#include <qvariant.h>
#include "qg_cadtoolbar.h"
#include "qg_cadtoolbarcircles.ui.h"
/*
 *  Constructs a QG_CadToolBarCircles as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarCircles::QG_CadToolBarCircles(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarCircles::~QG_CadToolBarCircles()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarCircles::languageChange()
{
    retranslateUi(this);
}

