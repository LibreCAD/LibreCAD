#include "qg_dlgoptionsvariables.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include "rs_units.h"
#include "rs_filterdxf.h"
#include "qg_dlgoptionsvariables.ui.h"
/*
 *  Constructs a QG_DlgOptionsVariables as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgOptionsVariables::QG_DlgOptionsVariables(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgOptionsVariables::~QG_DlgOptionsVariables()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsVariables::languageChange()
{
    retranslateUi(this);
}

