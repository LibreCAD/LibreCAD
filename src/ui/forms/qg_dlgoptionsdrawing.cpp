#include "qg_dlgoptionsdrawing.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include "rs_units.h"
#include "qg_dlgoptionsdrawing.ui.h"
/*
 *  Constructs a QG_DlgOptionsDrawing as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgOptionsDrawing::QG_DlgOptionsDrawing(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgOptionsDrawing::~QG_DlgOptionsDrawing()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsDrawing::languageChange()
{
    retranslateUi(this);
}

