#include "qg_cadtoolbarmain.h"

#include <qvariant.h>
#include "qg_cadtoolbar.h"
#include "qg_cadtoolbarmain.ui.h"
/*
 *  Constructs a QG_CadToolBarMain as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CadToolBarMain::QG_CadToolBarMain(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CadToolBarMain::~QG_CadToolBarMain()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CadToolBarMain::languageChange()
{
    retranslateUi(this);
}

